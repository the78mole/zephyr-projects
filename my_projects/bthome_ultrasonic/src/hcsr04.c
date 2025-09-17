/*
 * HC-SR04 Ultrasonic Distance Sensor Driver Implementation
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hcsr04.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hcsr04, LOG_LEVEL_INF);

/* HC-SR04 timing constants */
#define TRIGGER_PULSE_US    10      /* Trigger pulse duration in microseconds */
#define ECHO_TIMEOUT_US     30000   /* Echo timeout: 30ms for ~5m max range */
#define SOUND_SPEED_CM_US   58      /* Sound speed: 58 us/cm (round trip) */

static void echo_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct hcsr04_data *data = CONTAINER_OF(cb, struct hcsr04_data, echo_cb);
    const struct hcsr04_config *config = data->config;
    
    /* Get current time in microseconds using kernel ticks */
    uint32_t current_time = k_uptime_get_32() * 1000;  /* Convert ms to us */
    
    /* Check if echo pin is high (start of echo) or low (end of echo) */
    if (gpio_pin_get_dt(&config->echo_gpio)) {
        /* Echo started */
        data->echo_start_time = current_time;
    } else {
        /* Echo ended */
        data->echo_end_time = current_time;
        k_sem_give(&data->measurement_sem);
    }
}

int hcsr04_init(const struct hcsr04_config *config, struct hcsr04_data *data)
{
    int ret;
    
    /* Store configuration pointer */
    data->config = config;
    
    /* Initialize mutex and semaphore */
    k_mutex_init(&data->lock);
    k_sem_init(&data->measurement_sem, 0, 1);
    
    /* Initialize measurement state */
    data->last_distance_mm = 0;
    data->measurement_valid = false;
    data->echo_start_time = 0;
    data->echo_end_time = 0;
    
    /* Check if GPIO devices are ready */
    if (!gpio_is_ready_dt(&config->trigger_gpio)) {
        LOG_ERR("Trigger GPIO device not ready");
        return -ENODEV;
    }
    
    if (!gpio_is_ready_dt(&config->echo_gpio)) {
        LOG_ERR("Echo GPIO device not ready");
        return -ENODEV;
    }
    
    /* Configure trigger pin as output */
    ret = gpio_pin_configure_dt(&config->trigger_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure trigger GPIO: %d", ret);
        return ret;
    }
    
    /* Configure echo pin as input with pull-down */
    ret = gpio_pin_configure_dt(&config->echo_gpio, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret < 0) {
        LOG_ERR("Failed to configure echo GPIO: %d", ret);
        return ret;
    }
    
    /* Initialize GPIO callback for echo pin */
    gpio_init_callback(&data->echo_cb, echo_gpio_callback, BIT(config->echo_gpio.pin));
    
    /* Add callback to echo pin - trigger on both edges */
    ret = gpio_add_callback(config->echo_gpio.port, &data->echo_cb);
    if (ret < 0) {
        LOG_ERR("Failed to add GPIO callback: %d", ret);
        return ret;
    }
    
    /* Enable interrupt on echo pin */
    ret = gpio_pin_interrupt_configure_dt(&config->echo_gpio, GPIO_INT_EDGE_BOTH);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO interrupt: %d", ret);
        return ret;
    }
    
    LOG_INF("HC-SR04 sensor initialized successfully");
    return 0;
}

int32_t hcsr04_measure_distance(const struct hcsr04_config *config, struct hcsr04_data *data)
{
    int ret;
    uint32_t echo_duration_us;
    uint32_t distance_mm;
    
    /* Lock to ensure only one measurement at a time */
    ret = k_mutex_lock(&data->lock, K_MSEC(100));
    if (ret != 0) {
        LOG_WRN("Failed to acquire measurement lock");
        return -EBUSY;
    }
    
    /* Reset semaphore */
    k_sem_reset(&data->measurement_sem);
    
    /* Send trigger pulse */
    gpio_pin_set_dt(&config->trigger_gpio, 1);
    k_busy_wait(TRIGGER_PULSE_US);
    gpio_pin_set_dt(&config->trigger_gpio, 0);
    
    /* Wait for echo response with timeout */
    ret = k_sem_take(&data->measurement_sem, K_USEC(ECHO_TIMEOUT_US));
    if (ret != 0) {
        LOG_WRN("Echo timeout - no object detected or out of range");
        data->measurement_valid = false;
        k_mutex_unlock(&data->lock);
        return -ETIMEDOUT;
    }
    
    /* Calculate echo duration */
    if (data->echo_end_time > data->echo_start_time) {
        echo_duration_us = data->echo_end_time - data->echo_start_time;
    } else {
        /* Handle timer overflow */
        echo_duration_us = (UINT32_MAX - data->echo_start_time) + data->echo_end_time;
    }
    
    /* Convert to distance in millimeters */
    /* Distance = (echo_time_us * speed_of_sound) / 2 */
    /* Distance_cm = echo_time_us / 58 */
    /* Distance_mm = (echo_time_us * 10) / 58 */
    distance_mm = (echo_duration_us * 10) / SOUND_SPEED_CM_US;
    
    /* Validate measurement range */
    if (distance_mm > config->max_distance_mm || distance_mm < 20) {
        LOG_WRN("Distance out of valid range: %u mm", distance_mm);
        data->measurement_valid = false;
        k_mutex_unlock(&data->lock);
        return -ERANGE;
    }
    
    /* Store valid measurement */
    data->last_distance_mm = distance_mm;
    data->measurement_valid = true;
    
    k_mutex_unlock(&data->lock);
    
    LOG_DBG("Distance measured: %u mm (echo: %u us)", distance_mm, echo_duration_us);
    return distance_mm;
}