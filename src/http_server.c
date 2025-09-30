/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <wifi_conn_mgr.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/data/json.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(http_service, LOG_LEVEL_DBG);

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint8_t main_js_gz[] = {
#include "main.js.gz.inc"
};

struct wifi_creds {
	const uint8_t* ssid;
	const uint8_t* psk;
};

static const struct json_obj_descr wifi_creds_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct wifi_creds, ssid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct wifi_creds, psk, JSON_TOK_STRING),
};

static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static struct http_resource_detail_static main_js_gz_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/javascript",
		},
	.static_data = main_js_gz,
	.static_data_len = sizeof(main_js_gz),
};

static int wifi_creds_parser(struct wifi_creds *creds, uint8_t *buf, size_t len)
{
	int ret, ret_json;

	if (!creds && !buf) {
		LOG_ERR("Invalid arguments");
		return -EINVAL;
	}

	memset(creds, 0, sizeof(struct wifi_creds));

	ret = json_obj_parse(buf, len, wifi_creds_descr, ARRAY_SIZE(wifi_creds_descr), creds);
	ret_json = (1 << ARRAY_SIZE(wifi_creds_descr)) - 1;

	if (ret < 0) {
		LOG_ERR("JSON Parse Error: %d", ret);
		return ret;
	} 
	
	if (ret != ret_json) {
		LOG_ERR("Not all values decoded; Expected return code %d but got %d", ret_json,
			ret);
		return -EINVAL;
	}

	return ret;
}

static int connect_handler(struct http_client_ctx *client, enum http_data_status status,
			   const struct http_request_ctx *request_ctx,
			   struct http_response_ctx *response_ctx, void *user_data)
{
	int ret;
	struct wifi_creds creds;

	if (status == HTTP_SERVER_DATA_ABORTED) {
		LOG_INF("Transaction aborted");
		return 0;
	} else if (status == HTTP_SERVER_DATA_MORE) {
		LOG_INF("More data is received");
		return 0;
	}

	ret = wifi_creds_parser(&creds, request_ctx->data, request_ctx->data_len);
	if (ret < 0) {
		return 0;
	}

	if(IS_ENABLED(CONFIG_WIFI)){
		struct net_if *iface = net_if_get_wifi_sta();
		if (!iface) {
			LOG_ERR("Wifi interface not found");
			return 0;
		}

		ret = wifi_connect(iface, creds.ssid, creds.psk);
		if (ret < 0) {
			LOG_ERR("Connect request failed: (%d)", ret);
			return 0;
		}
	}

	LOG_INF("SSID: %s, PSK: %s", creds.ssid, creds.psk);

	return 0;
}

static struct http_resource_detail_dynamic connect_resource_detail = {
	.common =
		{
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		},
	.cb = connect_handler,
	.user_data = NULL,
};

static uint16_t test_http_service_port = 80;
HTTP_SERVICE_DEFINE(test_http_service, NULL, &test_http_service_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

HTTP_RESOURCE_DEFINE(main_js_gz_resource, test_http_service, "/main.js",
		     &main_js_gz_resource_detail);

HTTP_RESOURCE_DEFINE(connect_resource, test_http_service, "/connect", &connect_resource_detail);
