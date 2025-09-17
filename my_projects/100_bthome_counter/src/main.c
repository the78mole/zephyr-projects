/*
 * Copyright (c) 2025 BTHome Counter Example with Library
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <bthome.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bthome_counter, LOG_LEVEL_INF);

/* LED for visual feedback */
#define LED1_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* BTHome device instance */
static struct bthome_device bthome_dev;

/* Counter state */
static uint16_t counter_value = 0;

/* Bluetooth ready callback */
static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");
}

/* Work handler for periodic counter updates */
static void counter_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(counter_work, counter_work_handler);

static void counter_work_handler(struct k_work *work)
{
    int err;

    /* LED on - advertisement cycle starts */
    gpio_pin_set_dt(&led1, 1);

    /* Reset measurements for new packet */
    bthome_reset_measurements(&bthome_dev);

    /* Increment counter */
    counter_value++;

    /* Add counter measurement (16-bit) */
    err = bthome_add_sensor(&bthome_dev, BTHOME_ID_COUNT2, counter_value);
    if (err) {
        LOG_ERR("Failed to add counter: %d", err);
        goto cleanup;
    }

    /* Send advertisement */
    err = bthome_advertise(&bthome_dev, 1500);  /* Advertise for 1.5 seconds */
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        goto cleanup;
    }

    LOG_INF("BTHome advertisement sent: Counter = %u", counter_value);

cleanup:
    /* LED off after short delay */
    k_sleep(K_MSEC(100));
    gpio_pin_set_dt(&led1, 0);

    /* Schedule next advertisement in 5 seconds */
    k_work_schedule(&counter_work, K_SECONDS(5));
}

int main(void)
{
    int err;
    struct bthome_config config = {
        .device_name = "BTHome Counter",
        .encryption = false,
        .trigger_based = false,
    };

    LOG_INF("BTHome Counter Example for nRF52840-DK (with Library)");
    LOG_INF("Board: %s", CONFIG_BOARD_TARGET);

    /* Initialize LED */
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        return -1;
    }
    
    err = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (err < 0) {
        LOG_ERR("Failed to configure LED1: %d", err);
        return -1;
    }

    LOG_INF("LED1 initialized successfully");

    /* Set fixed MAC address manually */
    err = bthome_set_fixed_mac();
    if (err) {
        LOG_WRN("Failed to set fixed MAC: %d", err);
    }

    /* Initialize BTHome device */
    err = bthome_init(&bthome_dev, &config);
    if (err) {
        LOG_ERR("Failed to initialize BTHome device: %d", err);
        return -1;
    }

    /* Initialize Bluetooth */
    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Wait for Bluetooth to be ready */
    k_sleep(K_SECONDS(2));

    /* Start periodic counter updates */
    k_work_schedule(&counter_work, K_SECONDS(3));

    LOG_INF("BTHome Counter is running...");
    LOG_INF("Sending counter values every 5 seconds");
    LOG_INF("Use nRF Connect or Home Assistant to receive BTHome data");

    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(10));
        LOG_INF("System running, current counter: %u", counter_value);
    }

    return 0;
}