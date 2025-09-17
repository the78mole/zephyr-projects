/*
 * Copyright (c) 2025 BTHome Low-Power Counter Example
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <bthome.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bthome_lowpower, LOG_LEVEL_WRN);  /* Minimal logging for power savings */

/* LED for visual feedback (optional, can be disabled for power savings) */
#define LED1_NODE DT_ALIAS(led0)
#if DT_NODE_EXISTS(LED1_NODE)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#define HAS_LED 1
#else
#define HAS_LED 0
#endif

/* BTHome device instance */
static struct bthome_device bthome_dev;

/* Counter state - stored in retention RAM */
static uint16_t counter_value = 0;

/* Advertisement interval in seconds (configurable for power optimization) */
#define ADV_INTERVAL_SEC 10  /* Send every 10 seconds for balanced power/responsiveness */
#define ADV_DURATION_MS 2000 /* Advertise for 2 seconds */

/* Power management state */
static bool bluetooth_ready = false;

/* Bluetooth ready callback */
static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    bluetooth_ready = true;
    LOG_WRN("Bluetooth initialized");
}

/* Work handler for periodic counter updates */
static void counter_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(counter_work, counter_work_handler);

static void enter_deep_sleep(void)
{
    LOG_WRN("Entering deep sleep for %d seconds", ADV_INTERVAL_SEC);
    
#if HAS_LED
    /* Turn off LED before sleep */
    gpio_pin_set_dt(&led1, 0);
#endif

    /* Use basic k_sleep for long sleep periods */
    /* On nRF52840, this will automatically enter low power modes */
    k_sleep(K_SECONDS(ADV_INTERVAL_SEC - 3));  /* Sleep most of the time, leave 3s for processing */
}

static void counter_work_handler(struct k_work *work)
{
    int err;

    if (!bluetooth_ready) {
        LOG_ERR("Bluetooth not ready, skipping advertisement");
        goto schedule_next;
    }

#if HAS_LED
    /* LED on - advertisement cycle starts */
    gpio_pin_set_dt(&led1, 1);
#endif

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
    err = bthome_advertise(&bthome_dev, ADV_DURATION_MS);
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        goto cleanup;
    }

    LOG_WRN("BTHome advertisement sent: Counter = %u", counter_value);

cleanup:
#if HAS_LED
    /* LED off after short delay */
    k_sleep(K_MSEC(100));
    gpio_pin_set_dt(&led1, 0);
#endif

    /* Enter deep sleep mode for power savings */
    enter_deep_sleep();

schedule_next:
    /* Schedule next advertisement */
    k_work_schedule(&counter_work, K_SECONDS(3));  /* Small delay before next cycle */
}

int main(void)
{
    int err;
    struct bthome_config config = {
        .device_name = "BTHome LowPower",
        .encryption = false,
        .trigger_based = false,
    };

    LOG_WRN("BTHome Low-Power Counter for nRF52840-DK");
    LOG_WRN("Board: %s", CONFIG_BOARD_TARGET);
    LOG_WRN("Advertisement interval: %d seconds", ADV_INTERVAL_SEC);

#if HAS_LED
    /* Initialize LED (optional) */
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        /* Don't fail - LED is optional for low power */
    } else {
        err = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
        if (err < 0) {
            LOG_ERR("Failed to configure LED1: %d", err);
        } else {
            LOG_WRN("LED1 initialized");
        }
    }
#else
    LOG_WRN("LED disabled for power savings");
#endif

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

    LOG_WRN("BTHome Low-Power Counter is running...");
    LOG_WRN("Sending counter values every %d seconds", ADV_INTERVAL_SEC);

    /* Main loop - just sleep and let work handler do everything */
    while (1) {
        /* Sleep for a moderate time, wake up only for work items */
        k_sleep(K_SECONDS(ADV_INTERVAL_SEC + 5));  /* Sleep a bit longer than one cycle */
        
        /* Moderate heartbeat logging */
        LOG_WRN("System heartbeat, counter: %u", counter_value);
    }

    return 0;
}