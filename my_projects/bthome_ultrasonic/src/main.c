/*
 * BTHome Ultrasonic Distance Sensor Application
 * Based on BTHome v2 protocol for Home Assistant integration
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "hcsr04.h"

LOG_MODULE_REGISTER(bthome_ultrasonic, LOG_LEVEL_INF);

/* BTHome service UUID: 0xFCD2 */
#define BTHOME_SERVICE_UUID         0xFCD2

/* BTHome object IDs */
#define BTHOME_OBJECT_DISTANCE_MM   0x40    /* Distance in millimeters */

/* Measurement interval */
#define MEASUREMENT_INTERVAL_MS     5000    /* 5 seconds */

/* Platform-specific HC-SR04 GPIO pin definitions */
#if defined(CONFIG_BOARD_NRF52840DK_NRF52840) || defined(CONFIG_BOARD_UBX_EVKNINAB3_NRF52840)
    /* Nordic boards: Use dedicated free GPIO pins */
    #define TRIGGER_PIN         8   /* P0.08 */
    #define ECHO_PIN            6   /* P0.06 */
    #define GPIO_DEVICE         DEVICE_DT_GET(DT_NODELABEL(gpio0))
#elif defined(CONFIG_BOARD_ESP32_DEVKITC_WROOM)
    /* ESP32: Use GPIO18 and GPIO19 */
    #define TRIGGER_PIN         18
    #define ECHO_PIN            19  
    #define GPIO_DEVICE         DEVICE_DT_GET(DT_NODELABEL(gpio0))
#else
    #error "Unsupported board. Please add GPIO pin definitions for your board."
#endif

/* HC-SR04 sensor configuration */
static const struct hcsr04_config sensor_config = {
    .trigger_gpio = {.port = GPIO_DEVICE, .pin = TRIGGER_PIN, .dt_flags = GPIO_ACTIVE_HIGH},
    .echo_gpio = {.port = GPIO_DEVICE, .pin = ECHO_PIN, .dt_flags = GPIO_ACTIVE_HIGH},
    .max_distance_mm = 4000,    /* 4 meters maximum range */
    .timeout_us = 30000,        /* 30ms timeout */
};

static struct hcsr04_data sensor_data;

/* Bluetooth advertisement data */
static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xD2, 0xFC),  /* BTHome service UUID */
    BT_DATA(BT_DATA_SVC_DATA16, NULL, 0),  /* Will be updated with sensor data */
};

/* BTHome service data buffer */
static uint8_t bthome_data[10];

/**
 * @brief Create BTHome advertisement packet with distance measurement
 * 
 * @param distance_mm Distance in millimeters
 * @return Length of the created packet
 */
static uint8_t create_bthome_packet(uint16_t distance_mm)
{
    uint8_t *data = bthome_data;
    uint8_t len = 0;
    
    /* BTHome service UUID (little endian) */
    data[len++] = 0xD2;
    data[len++] = 0xFC;
    
    /* BTHome v2 info byte: 0x40 = encrypted=false, bindkey_verified=false, version=2 */
    data[len++] = 0x40;
    
    /* Distance object: Object ID (0x40) + 2 bytes distance in mm (little endian) */
    data[len++] = BTHOME_OBJECT_DISTANCE_MM;
    data[len++] = (uint8_t)(distance_mm & 0xFF);        /* Low byte */
    data[len++] = (uint8_t)((distance_mm >> 8) & 0xFF); /* High byte */
    
    return len;
}

/**
 * @brief Update advertisement with new distance measurement
 * 
 * @param distance_mm Distance in millimeters
 */
static void update_advertisement(uint16_t distance_mm)
{
    int err;
    uint8_t data_len;
    
    /* Create BTHome packet */
    data_len = create_bthome_packet(distance_mm);
    
    /* Update service data in advertisement */
    ad[2].data = bthome_data;
    ad[2].data_len = data_len;
    
    /* Stop previous advertising */
    err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_ERR("Failed to stop advertising: %d", err);
        return;
    }
    
    /* Start advertising with updated data */
    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        return;
    }
    
    if (distance_mm == 0xFFFF) {
        LOG_INF("📡 Advertising ERROR: No measurement available");
    } else {
        LOG_INF("📡 Advertising distance: %u mm (%.1f cm)", distance_mm, distance_mm / 10.0);
    }
}

/**
 * @brief Bluetooth ready callback
 */
static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("❌ Bluetooth init failed: %d", err);
        return;
    }
    
    LOG_INF("✅ Bluetooth initialized successfully");
    LOG_INF("📡 Starting BTHome advertisement...");
    LOG_INF("🔑 Service UUID: 0xFCD2 (BTHome v2)");
    
    /* Start initial advertisement with zero distance */
    update_advertisement(0);
}

/**
 * @brief Main measurement and advertisement loop
 */
static void measurement_thread(void)
{
    int32_t distance;
    
    LOG_INF("🚀 Starting measurement loop...");
    
    while (1) {
        LOG_DBG("📏 Performing distance measurement...");
        
        /* Perform distance measurement */
        distance = hcsr04_measure_distance(&sensor_config, &sensor_data);
        
        if (distance > 0) {
            /* Valid measurement - update advertisement */
            update_advertisement((uint16_t)distance);
            LOG_INF("✅ Distance measured: %d mm (%.1f cm)", distance, distance / 10.0);
        } else {
            /* Invalid measurement - log error but continue */
            switch (distance) {
                case -ETIMEDOUT:
                    LOG_WRN("⏰ Measurement timeout - no object detected");
                    break;
                case -ERANGE:
                    LOG_WRN("📐 Distance out of range");
                    break;
                case -EBUSY:
                    LOG_WRN("🔒 Sensor busy - measurement skipped");
                    break;
                default:
                    LOG_WRN("❌ Measurement failed with error: %d", distance);
                    break;
            }
            
            /* Send error value (0xFFFF = no measurement) */
            update_advertisement(0xFFFF);
        }
        
        /* Wait for next measurement */
        LOG_DBG("⏱️  Waiting %d seconds for next measurement...", MEASUREMENT_INTERVAL_MS / 1000);
        k_sleep(K_MSEC(MEASUREMENT_INTERVAL_MS));
    }
}

/* Define measurement thread */
K_THREAD_DEFINE(measurement_tid, 1024, measurement_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    int err;
    
    LOG_INF("🌟 BTHome Ultrasonic Distance Sensor starting...");
    LOG_INF("📋 Board: %s", CONFIG_BOARD);
    
    /* Check if GPIO device is ready */
    if (!device_is_ready(GPIO_DEVICE)) {
        LOG_ERR("❌ GPIO device not ready");
        return -ENODEV;
    }
    
    LOG_INF("🔧 GPIO Configuration:");
#if defined(CONFIG_BOARD_NRF52840DK_NRF52840) || defined(CONFIG_BOARD_UBX_EVKNINAB3_NRF52840)
    LOG_INF("   📌 Trigger Pin: P0.%02d", TRIGGER_PIN);
    LOG_INF("   📌 Echo Pin:    P0.%02d", ECHO_PIN);
#elif defined(CONFIG_BOARD_ESP32_DEVKITC_WROOM)
    LOG_INF("   📌 Trigger Pin: GPIO%d", TRIGGER_PIN);
    LOG_INF("   📌 Echo Pin:    GPIO%d", ECHO_PIN);
#endif
    
    /* Initialize HC-SR04 sensor */
    LOG_INF("🔧 Initializing HC-SR04 sensor...");
    err = hcsr04_init(&sensor_config, &sensor_data);
    if (err) {
        LOG_ERR("❌ Failed to initialize HC-SR04 sensor: %d", err);
        return err;
    }
    LOG_INF("✅ HC-SR04 sensor initialized successfully");
    
    /* Initialize Bluetooth */
    LOG_INF("📡 Initializing Bluetooth...");
    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("❌ Bluetooth init failed: %d", err);
        return err;
    }
    
    LOG_INF("🚀 Application initialized. Starting measurements...");
    LOG_INF("📊 Measurement interval: %d seconds", MEASUREMENT_INTERVAL_MS / 1000);
    LOG_INF("📏 Maximum range: %d mm (%.1f cm)", sensor_config.max_distance_mm, sensor_config.max_distance_mm / 10.0);
    
    /* Main thread handles system events */
    while (1) {
        k_sleep(K_SECONDS(1));
    }
    
    return 0;
}