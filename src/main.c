/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/server.h>
#include <mqtt_handler.h>
#include <gpio_handler.h>

int main(void)
{
    if(IS_ENABLED(CONFIG_HTTP_SERVER))
		http_server_start();

    while (1) {
        pub_sub();
        k_msleep(CONFIG_MQTT_SLEEP_MS);
    }
}

