/*
 * Simple LED Toggle Test for nRF52840-DK
 * Tests basic GPIO functionality and serial logging
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(led_toggle, LOG_LEVEL_DBG);

/* nRF52840-DK has 4 LEDs connected to these pins */
#define LED0_NODE    DT_ALIAS(led0)
#define LED1_NODE    DT_ALIAS(led1) 
#define LED2_NODE    DT_ALIAS(led2)
#define LED3_NODE    DT_ALIAS(led3)

/* Check if LED aliases exist in device tree */
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "LED0 devicetree alias is not defined or not enabled"
#endif

/* Define LED GPIO specs from device tree */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* Toggle state */
static bool led_state = false;
static int led_counter = 0;

int main(void)
{
    int ret;
    
    printk("🚀 LED Toggle Test for nRF52840-DK starting...\n");
    printk("📋 Board: %s\n", CONFIG_BOARD);
    
    LOG_INF("🚀 LED Toggle Test for nRF52840-DK starting...");
    LOG_INF("📋 Board: %s", CONFIG_BOARD);
    LOG_INF("🌐 Zephyr version: %s", STRINGIFY(KERNEL_VERSION_MAJOR) "." STRINGIFY(KERNEL_VERSION_MINOR));
    
    /* Check if LED GPIO devices are ready */
    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("❌ LED0 GPIO device not ready");
        return -ENODEV;
    }
    
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("❌ LED1 GPIO device not ready");
        return -ENODEV;
    }
    
    if (!gpio_is_ready_dt(&led2)) {
        LOG_ERR("❌ LED2 GPIO device not ready");
        return -ENODEV;
    }
    
    if (!gpio_is_ready_dt(&led3)) {
        LOG_ERR("❌ LED3 GPIO device not ready");
        return -ENODEV;
    }
    
    LOG_INF("✅ All LED GPIO devices are ready");
    
    /* Configure LED pins as outputs */
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("❌ Failed to configure LED0: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("❌ Failed to configure LED1: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("❌ Failed to configure LED2: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("❌ Failed to configure LED3: %d", ret);
        return ret;
    }
    
    LOG_INF("🔧 All LEDs configured as outputs");
    LOG_INF("💡 Starting LED toggle sequence...");
    
    /* Turn all LEDs off initially */
    gpio_pin_set_dt(&led0, 0);
    gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&led3, 0);
    
    /* Main loop - toggle LEDs */
    while (1) {
        led_counter++;
        led_state = !led_state;
        
        printk("🔄 Toggle #%d: LEDs %s\n", led_counter, led_state ? "ON" : "OFF");
        LOG_INF("🔄 Toggle #%d: LEDs %s", led_counter, led_state ? "ON 💡" : "OFF ⚫");
        
        /* Toggle all LEDs together */
        gpio_pin_set_dt(&led0, led_state);
        gpio_pin_set_dt(&led1, led_state);
        gpio_pin_set_dt(&led2, led_state);
        gpio_pin_set_dt(&led3, led_state);
        
        /* Log detailed state */
        LOG_DBG("📊 LED States: LED0=%d, LED1=%d, LED2=%d, LED3=%d", 
                gpio_pin_get_dt(&led0), 
                gpio_pin_get_dt(&led1),
                gpio_pin_get_dt(&led2), 
                gpio_pin_get_dt(&led3));
        
        /* Wait 1 second */
        k_sleep(K_MSEC(1000));
        
        /* Every 10 toggles, show a status message */
        if (led_counter % 10 == 0) {
            LOG_INF("📈 Status: %d toggles completed, system running normally", led_counter);
        }
    }
    
    return 0;
}