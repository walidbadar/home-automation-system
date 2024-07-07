/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_INCLUDE_GPIO_HANDLER_H_
#define APP_INCLUDE_GPIO_HANDLER_H_

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

extern const struct gpio_dt_spec buttons[CONFIG_MAX_APPLIANCES];
extern const struct gpio_dt_spec appliances[CONFIG_MAX_APPLIANCES];

#endif /* APP_INCLUDE_GPIO_HANDLER_H_ */

