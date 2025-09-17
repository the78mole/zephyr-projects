#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_test, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int counter = 0;
    bool led_state = false;
    
    /* Immediate output to test UART */
    printk("\n\n");
    printk("========================================\n");
    printk("nRF52840-DK UART Test\n");
    printk("Board: %s\n", CONFIG_BOARD);
    printk("Testing UART Console Output\n");
    printk("========================================\n");
    
    LOG_INF("Logger test: System starting");
    
    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        printk("ERROR: LED device not ready\n");
        LOG_ERR("LED device not ready");
        return -1;
    }
    
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERROR: Failed to configure LED: %d\n", ret);
        LOG_ERR("Failed to configure LED: %d", ret);
        return ret;
    }
    
    printk("LED configured successfully\n");
    LOG_INF("LED configured on pin P0.13");
    
    /* Test UART device */
    const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    if (!device_is_ready(uart_dev)) {
        printk("ERROR: UART device not ready\n");
        LOG_ERR("UART device not ready");
    } else {
        printk("UART device ready: %s\n", uart_dev->name);
        LOG_INF("UART device ready: %s", uart_dev->name);
    }
    
    printk("Starting LED blink with UART output...\n");
    LOG_INF("Starting main loop");
    
    while (1) {
        counter++;
        led_state = !led_state;
        
        gpio_pin_set_dt(&led, led_state);
        
        printk("[%d] LED %s\n", counter, led_state ? "ON" : "OFF");
        LOG_INF("Blink %d: LED %s", counter, led_state ? "ON" : "OFF");
        
        if (counter % 5 == 0) {
            printk("--- Status: %d blinks completed ---\n", counter);
        }
        
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}