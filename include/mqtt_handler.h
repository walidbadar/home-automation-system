/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_INCLUDE_MQTT_HANDLER_H_
#define APP_INCLUDE_MQTT_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>

/* The mqtt client connections status */
extern bool connected;

int pub_switch_state(const struct gpio_dt_spec *button, int index, uint8_t *pub_topics);
int sub_relay_state(const struct gpio_dt_spec *relay, uint8_t *payload, uint8_t *pub_topics);
int pub_sub(void);

#endif /* APP_INCLUDE_MQTT_HANDLER_H_ */

