/*
 * HC-SR04 Ultrasonic Distance Sensor Driver
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HCSR04_H
#define HCSR04_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief HC-SR04 sensor configuration structure
 */
struct hcsr04_config {
    struct gpio_dt_spec trigger_gpio;
    struct gpio_dt_spec echo_gpio;
    uint32_t max_distance_mm;  /* Maximum measurement distance in mm */
    uint32_t timeout_us;       /* Timeout for echo signal in microseconds */
};

/**
 * @brief HC-SR04 sensor data structure
 */
struct hcsr04_data {
    uint32_t last_distance_mm;
    bool measurement_valid;
    struct k_mutex lock;
    struct gpio_callback echo_cb;
    uint32_t echo_start_time;
    uint32_t echo_end_time;
    struct k_sem measurement_sem;
    const struct hcsr04_config *config;  /* Pointer to configuration */
};

/**
 * @brief Initialize HC-SR04 sensor
 * 
 * @param config Sensor configuration
 * @param data Sensor data structure
 * @return 0 on success, negative error code on failure
 */
int hcsr04_init(const struct hcsr04_config *config, struct hcsr04_data *data);

/**
 * @brief Measure distance with HC-SR04 sensor
 * 
 * @param config Sensor configuration
 * @param data Sensor data structure
 * @return Distance in millimeters, or -1 on error
 */
int32_t hcsr04_measure_distance(const struct hcsr04_config *config, struct hcsr04_data *data);

#endif /* HCSR04_H */