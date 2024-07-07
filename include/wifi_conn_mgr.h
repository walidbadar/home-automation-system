/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_INCLUDE_WIFI_CONN_MGR_H_
#define APP_INCLUDE_WIFI_CONN_MGR_H_

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/wifi_mgmt.h>

int wifi_ap_enable(struct net_if *iface);
int wifi_ap_disable(struct net_if *iface);
int wifi_connect(struct net_if *iface, uint8_t *ssid, uint8_t *psk);
int wifi_disconnect(struct net_if *iface, uint8_t *ssid, uint8_t *psk);

#endif /* APP_INCLUDE_WIFI_CONN_MGR_H_ */
