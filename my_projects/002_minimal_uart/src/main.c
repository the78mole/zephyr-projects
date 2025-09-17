#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int counter = 0;
    bool led_state = false;
    
    // Immediately print something
    printk("HELLO FROM nRF52840-DK!\n");
    printk("This is a minimal UART test\n");
    printk("Board: %s\n", CONFIG_BOARD);
    
    // Configure LED
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        printk("LED configured OK\n");
    } else {
        printk("LED config FAILED\n");
    }
    
    while (1) {
        counter++;
        led_state = !led_state;
        
        gpio_pin_set_dt(&led, led_state);
        
        printk("%d: LED %s\n", counter, led_state ? "ON" : "OFF");
        
        k_sleep(K_MSEC(2000));  // 2 seconds for easier monitoring
    }
}