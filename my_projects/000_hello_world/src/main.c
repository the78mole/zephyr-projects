/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

/* LED1 on nRF52840-DK is connected to P0.13 */
#define LED1_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

int main(void)
{
	int ret;

	printf("LED Blink Demo für nRF52840-DK Board: %s\n", CONFIG_BOARD_TARGET);

	/* Prüfe ob LED verfügbar ist */
	if (!gpio_is_ready_dt(&led)) {
		printf("Fehler: LED device %s ist nicht bereit\n", led.port->name);
		return -1;
	}

	/* Konfiguriere LED als Output */
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printf("Fehler: Kann LED nicht als Output konfigurieren (%d)\n", ret);
		return -1;
	}

	printf("LED1 an P0.13 wird blinken...\n");

	/* Blink-Loop */
	while (1) {
		/* LED einschalten */
		ret = gpio_pin_set_dt(&led, 1);
		if (ret < 0) {
			printf("Fehler beim LED einschalten\n");
			return -1;
		}
		
		printf("LED ON\n");
		k_msleep(1000);  /* 1 Sekunde warten */

		/* LED ausschalten */
		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			printf("Fehler beim LED ausschalten\n");
			return -1;
		}
		
		printf("LED OFF\n");
		k_msleep(1000);  /* 1 Sekunde warten */
	}

	return 0;
}
