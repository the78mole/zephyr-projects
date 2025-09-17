/*
 * Minimal LED blink with serial output test
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* LED0 from device tree */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    bool led_state = true;
    int counter = 0;
    
    /* Print startup message immediately via UART and RTT */
    printk("\n\n=== nRF52840-DK Simple Blink Test ===\n");
    printk("Board: %s\n", CONFIG_BOARD);
    printk("UART Console Test - if you see this, UART works!\n");
    printk("Starting LED blink...\n");
    
#ifdef CONFIG_RTT_CONSOLE
    /* Also print via RTT for debugging */
    printk("RTT Console also enabled\n");
#endif
    
    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        printk("ERROR: LED device not ready\n");
        return -1;
    }
    
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    printk("LED configured successfully\n");
    
    /* Main loop */
    while (1) {
        counter++;
        led_state = !led_state;
        
        gpio_pin_set_dt(&led, led_state);
        
        printk("Blink %d: LED %s\n", counter, led_state ? "ON" : "OFF");
        
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}