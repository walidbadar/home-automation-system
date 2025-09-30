/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <wifi_conn_mgr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_conn_mgr);

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

static bool net_dhcpv4_server_status = false;

static struct net_mgmt_event_callback wifi_cb;
static void wifi_conn_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(wifi_conn_dwork, wifi_conn_handler);

static const struct led_dt_spec wifi_status_led = LED_DT_SPEC_GET(DT_ALIAS(led0));

static int wifi_status(struct net_if *iface, struct wifi_iface_status *status)
{
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, status,
		       sizeof(struct wifi_iface_status));
	if (ret < 0) {
		LOG_WRN("Status request failed: %d", ret);
	}

	LOG_DBG("WiFi status: %s", wifi_state_txt(status->state));

	return ret;
}

static void wifi_config(struct net_if *iface, struct wifi_connect_req_params *config)
{
	ARG_UNUSED(iface);
	config->ssid_length = strlen(config->ssid);
	config->psk_length = strlen(config->psk);
	config->channel = WIFI_CHANNEL_ANY;
	config->band = WIFI_FREQ_BAND_2_4_GHZ;

	if (strlen(config->psk) == 0) {
		config->security = WIFI_SECURITY_TYPE_NONE;
	} else {
		config->security = WIFI_SECURITY_TYPE_PSK;
	}
}

#if defined(CONFIG_WIFI_USAGE_MODE_STA_AP)

static int wifi_ap_dhcpv4_server_start(struct net_if *iface)
{
	struct in_addr ip_addr;
	struct in_addr netmask;
	int ret;

	ret = net_addr_pton(AF_INET, CONFIG_WIFI_AP_STATIC_IP_ADDR, &ip_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IP address: %s", CONFIG_WIFI_AP_STATIC_IP_ADDR);
		return ret;
	}

	ret = net_addr_pton(AF_INET, CONFIG_WIFI_AP_STATIC_NETMASK, &netmask);
	if (ret < 0) {
		LOG_ERR("Invalid NETMASK: %s", CONFIG_WIFI_AP_STATIC_NETMASK);
		return ret;
	}

	net_if_ipv4_set_gw(iface, &ip_addr);

	if (net_if_ipv4_addr_add(iface, &ip_addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("unable to set IP address for AP interface");
		return -EINVAL;
	}

	if (!net_if_ipv4_set_netmask_by_addr(iface, &ip_addr, &netmask)) {
		LOG_ERR("Unable to set netmask for AP interface");
		return -EINVAL;
	}

	ip_addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */

	ret = net_dhcpv4_server_start(iface, &ip_addr);
	if (ret < 0) {
		LOG_ERR("DHCP server is not started for desired IP");
	} else {
		net_dhcpv4_server_status = true;
	}

	return ret;
}

int wifi_ap_enable(struct net_if *iface)
{
	struct wifi_connect_req_params config = {0};
	int ret;

	if (net_dhcpv4_server_status) {
		LOG_DBG("DHCPv4 Server is already started");
		return 0;
	}

	config.ssid = CONFIG_WIFI_AP_STATIC_SSID;
	config.psk = CONFIG_WIFI_AP_STATIC_PSK;
	wifi_config(iface, &config);

	ret = wifi_ap_dhcpv4_server_start(iface);
	if (ret < 0) {
		LOG_ERR("DHCPv4 Server failed, err: %d", ret);
		return ret;
	}

	LOG_INF("Enabling Access-Point Mode");
	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &config,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("Wifi AP enable request failed, err: %d", ret);
	}

	return ret;
}

int wifi_ap_disable(struct net_if *iface)
{
	struct wifi_connect_req_params config = {0};
	int ret;

	if (!net_dhcpv4_server_status) {
		LOG_DBG("DHCPv4 Server is already stopped");
		return 0;
	}

	config.ssid = CONFIG_WIFI_AP_STATIC_SSID;
	config.psk = CONFIG_WIFI_AP_STATIC_PSK;
	wifi_config(iface, &config);

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, &config,
		       sizeof(struct wifi_connect_req_params));
	if (ret < 0) {
		LOG_ERR("Wifi AP disable request failed, err: %d", ret);
		return ret;
	}

	ret = net_dhcpv4_server_stop(iface);
	if (ret < 0) {
		LOG_ERR("DHCP server stop failed, err: %d", ret);
	} else {
		net_dhcpv4_server_status = false;
	}

	return ret;
}
#endif /* CONFIG_WIFI_USAGE_MODE_STA_AP */

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	ARG_UNUSED(cb);

	struct net_if *iface_sap = net_if_get_wifi_sap();
	if (IS_ENABLED(CONFIG_WIFI_USAGE_MODE_STA_AP) && !iface_sap) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_DBG("Connected to Access-Point");
		led_on_dt(&wifi_status_led);

		if (IS_ENABLED(CONFIG_WIFI_USAGE_MODE_STA_AP)) {
			wifi_ap_disable(iface_sap);
		}
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		LOG_DBG("Disconnected from Access-Point");
		led_off_dt(&wifi_status_led);

		k_work_schedule(&wifi_conn_dwork, K_SECONDS(CONFIG_WIFI_CONN_RETRY_DELAY));
		if (IS_ENABLED(CONFIG_WIFI_USAGE_MODE_STA_AP)) {
			wifi_ap_enable(iface_sap);
		}
		break;
	}
	default:
		break;
	}
}

int wifi_connect(struct net_if *iface, const uint8_t *ssid, const uint8_t *psk)
{
	struct wifi_connect_req_params config = {0};
	struct wifi_iface_status status = {0};
	int ret = 0;

	net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&wifi_cb);

	wifi_status(iface, &status);

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		LOG_DBG("Connected to SSID: %s", ssid);
		return status.state;
	}

	config.ssid = ssid;
	config.psk = psk;
	wifi_config(iface, &config);

	if (status.state == WIFI_STATE_DISCONNECTED || status.state == WIFI_STATE_INACTIVE) {
		LOG_INF("Connecting to Access-Point, SSID: %s", config.ssid);
		ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &config, sizeof(config));
		if (ret < 0) {
			LOG_ERR("Connect request failed: (%d)", ret);
		}
	}

	return ret;
}

int wifi_disconnect(struct net_if *iface, const uint8_t *ssid, const uint8_t *psk)
{
	struct wifi_connect_req_params config = {0};
	int ret;

	config.ssid = ssid;
	config.psk = psk;
	wifi_config(iface, &config);

	ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, &config,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("WiFi disconnect request failed, err: %d", ret);
	}

	return ret;
}

static void wifi_conn_handler(struct k_work *work)
{
	struct net_if *iface = net_if_get_wifi_sta();
	if (!iface) {
		LOG_ERR("Wifi interface not found");
		return;
	}

	wifi_connect(iface, CONFIG_WIFI_STATIC_SSID, CONFIG_WIFI_STATIC_PSK);
}

static int wifi_conn_mgr(void)
{
	k_work_schedule(&wifi_conn_dwork, K_NO_WAIT);
	return 0;
}

SYS_INIT(wifi_conn_mgr, APPLICATION, CONFIG_WIFI_INIT_PRIORITY);
