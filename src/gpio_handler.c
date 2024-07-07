/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio_handler.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_config, LOG_LEVEL_DBG);

static struct gpio_callback gpio_cb[CONFIG_MAX_APPLIANCES];

const struct gpio_dt_spec buttons[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0})
};

const struct gpio_dt_spec appliances[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(appliance1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(appliance2), gpios, {0})
};

void button_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    for (int i = 0; i < CONFIG_MAX_APPLIANCES; i++) {
        if (pins & BIT(buttons[i].pin)) {
            bool state = gpio_pin_get_dt(&buttons[i]);
            gpio_pin_set_dt(&appliances[i], state);
            LOG_INF("Button %d pressed, Appliance %d set to %d", i + 1, i + 1, state);
        }
    }
}

static int pin_mode(const struct gpio_dt_spec *user_gpio, uint32_t dir) {
    int ret;

    if (!gpio_is_ready_dt(user_gpio)) {
        LOG_ERR("Error: GPIO device %s is not ready", user_gpio->port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(user_gpio, dir);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d", ret, user_gpio->port->name, user_gpio->pin);
        return ret;
    }

    if (dir == GPIO_INPUT) {
        ret = gpio_pin_interrupt_configure_dt(user_gpio, GPIO_INT_EDGE_BOTH);
        if (ret != 0) {
            LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret, user_gpio->port->name, user_gpio->pin);
            return ret;
        }

        gpio_init_callback(&gpio_cb[user_gpio->pin], button_cb, BIT(user_gpio->pin));
        gpio_add_callback(user_gpio->port, &gpio_cb[user_gpio->pin]);
    }

    return ret; 
}

static int gpio_init(void)
{
    bool state;

    for (int i = 0; i < CONFIG_MAX_APPLIANCES; i++) {
        pin_mode(&buttons[i], GPIO_INPUT);
        pin_mode(&appliances[i], GPIO_OUTPUT);

        state = gpio_pin_get_dt(&buttons[i]);
        gpio_pin_set_dt(&appliances[i], state);
    }
	return 0;
}

SYS_INIT(gpio_init, APPLICATION, 95);
