/*
 * Copyright (c) 2025 BTHome Ultra Low-Power Counter Example
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <bthome.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/gpio.h>

/* Use static variables for pseudo-retained data (resets on reboot) */
static struct retained_data {
    uint16_t counter_value;
    uint32_t boot_count;
    uint8_t initialized;
} retained = {0};

/* LED for minimal visual feedback (can be completely disabled) */
#define LED1_NODE DT_ALIAS(led0)
#if DT_NODE_EXISTS(LED1_NODE)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#define HAS_LED 1
#else
#define HAS_LED 0
#endif

/* BTHome device instance */
static struct bthome_device bthome_dev;

/* Ultra low power configuration */
#define ADV_INTERVAL_SEC 30  /* 30 seconds for ultra low power */
#define ADV_DURATION_MS 1000 /* Only 1 second advertisement */

/* Power management state */
static bool bluetooth_ready = false;

/* Bluetooth ready callback - minimal */
static void bt_ready(int err)
{
    bluetooth_ready = (err == 0);
}

/* Work handler for ultra-efficient counter updates */
static void counter_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(counter_work, counter_work_handler);

static void enter_ultra_deep_sleep(void)
{
#if HAS_LED
    /* Turn off LED completely */
    gpio_pin_set_dt(&led1, 0);
#endif

    /* Ultra long sleep for maximum power savings */
    /* On nRF52840, k_sleep automatically uses lowest power mode */
    k_sleep(K_SECONDS(ADV_INTERVAL_SEC - 2));
}

static void counter_work_handler(struct k_work *work)
{
    int err;

    if (!bluetooth_ready) {
        goto schedule_next;
    }

#if HAS_LED
    /* Longer LED pulse to make it visible */
    gpio_pin_set_dt(&led1, 1);
    k_sleep(K_MSEC(50));  /* 50ms pulse - clearly visible */
    gpio_pin_set_dt(&led1, 0);
#endif

    /* Reset measurements for new packet */
    bthome_reset_measurements(&bthome_dev);

    /* Increment retained counter */
    retained.counter_value++;

    /* Add counter measurement (16-bit) */
    err = bthome_add_sensor(&bthome_dev, BTHOME_ID_COUNT2, retained.counter_value);
    if (err) {
        goto cleanup;
    }

    /* Ultra short advertisement */
    err = bthome_advertise(&bthome_dev, ADV_DURATION_MS);
    if (err) {
        goto cleanup;
    }

cleanup:
    /* Enter ultra deep sleep immediately */
    enter_ultra_deep_sleep();

schedule_next:
    /* Schedule next advertisement */
    k_work_schedule(&counter_work, K_SECONDS(2));
}

int main(void)
{
    int err;
    struct bthome_config config = {
        .device_name = "BTHome Ultra",
        .encryption = false,
        .trigger_based = false,
    };

    /* Check if this is a cold boot or wake from retention */
    if (retained.initialized != 0xA5) {
        /* Cold boot - initialize retained data */
        retained.counter_value = 0;
        retained.boot_count = 0;
        retained.initialized = 0xA5;
    }
    
    retained.boot_count++;

#if HAS_LED
    /* Ultra minimal LED initialization */
    if (gpio_is_ready_dt(&led1)) {
        gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
        /* Boot indicator - 3 quick flashes */
        for (int i = 0; i < 3; i++) {
            gpio_pin_set_dt(&led1, 1);
            k_sleep(K_MSEC(100));
            gpio_pin_set_dt(&led1, 0);
            k_sleep(K_MSEC(100));
        }
    }
#endif

    /* Set fixed MAC address */
    err = bthome_set_fixed_mac();
    if (err) {
        /* No logging in ultra low power mode - just continue */
    }

    /* Initialize BTHome device */
    err = bthome_init(&bthome_dev, &config);
    if (err) {
        return -1;
    }

    /* Initialize Bluetooth with minimal delay */
    err = bt_enable(bt_ready);
    if (err) {
        return -1;
    }

    /* Minimal wait for Bluetooth */
    k_sleep(K_MSEC(500));

    /* Start ultra low power counter updates */
    k_work_schedule(&counter_work, K_SECONDS(1));

    /* Ultra minimal main loop */
    while (1) {
        /* Sleep for very long periods */
        k_sleep(K_SECONDS(60));
        
        /* No logging - just continue working */
    }

    return 0;
}