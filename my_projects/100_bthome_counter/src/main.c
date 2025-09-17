/*
 * Copyright (c) 2025 BTHome Counter Example
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(bthome_counter, LOG_LEVEL_DBG);

/* LED definitions for nRF52840-DK */
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* BTHome device information */
#define BTHOME_DEVICE_ID        0x1234  /* Unique device ID */
#define BTHOME_DEVICE_INFO      0x40    /* BTHome v2, no encryption, regular updates */

/* BTHome object IDs - see BTHome specification */
#define BTHOME_COUNT_8          0x09    /* Count (8-bit) */
#define BTHOME_COUNT_16         0x3D    /* Count (16-bit) */

/* Counter state */
static uint16_t counter_value = 0;

/* Generate a fixed MAC address based on nRF52840 FICR data */
static void get_fixed_mac_address(bt_addr_le_t *addr)
{
    /* Read FICR DEVICEADDR registers - CORRECT addresses! */
    uint32_t deviceaddr_low = *((volatile uint32_t *)0x100000A4);   /* FICR.DEVICEADDR[0] */
    uint32_t deviceaddr_high = *((volatile uint32_t *)0x100000A8);  /* FICR.DEVICEADDR[1] */
    
    LOG_INF("FICR.DEVICEADDR: 0x%08X%08X", deviceaddr_high, deviceaddr_low);
    
    /* Create a deterministic MAC address from the factory data */
    addr->type = BT_ADDR_LE_RANDOM;
    
    /* Use the factory device address to create a stable BLE MAC */
    addr->a.val[0] = (deviceaddr_low >> 0) & 0xFF;
    addr->a.val[1] = (deviceaddr_low >> 8) & 0xFF;
    addr->a.val[2] = (deviceaddr_low >> 16) & 0xFF;
    addr->a.val[3] = (deviceaddr_low >> 24) & 0xFF;
    addr->a.val[4] = (deviceaddr_high >> 0) & 0xFF;
    addr->a.val[5] = ((deviceaddr_high >> 8) & 0x3F) | 0xC0;  /* Static random address format */
    
    LOG_INF("Generated fixed MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
            addr->a.val[5], addr->a.val[4], addr->a.val[3], 
            addr->a.val[2], addr->a.val[1], addr->a.val[0]);
}

/* BTHome service data structure (without AD header) */
struct bthome_service_data {
    uint16_t uuid;         /* BTHome Service UUID (0xFCD2) */
    uint8_t device_info;   /* Device info byte */
    uint8_t object_id;     /* Object ID */
    uint16_t value;        /* Counter value (little endian) */
} __packed;

/* Build BTHome advertisement packet */
static void build_bthome_adv_data(struct bt_data *ad_data, struct bthome_service_data *bthome)
{
    /* Flags - required for BTHome v2 compatibility */
    static uint8_t flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
    
    /* BTHome service data */
    bthome->uuid = 0xFCD2;  /* BTHome service UUID (little endian) */
    bthome->device_info = BTHOME_DEVICE_INFO;  /* BTHome v2, no encryption, regular updates */
    bthome->object_id = BTHOME_COUNT_16;   /* 16-bit counter */
    bthome->value = counter_value;         /* Counter value (little endian) */

    /* Setup advertisement data array */
    /* AD Element 1: Flags (required by BTHome v2) */
    ad_data[0].type = BT_DATA_FLAGS;
    ad_data[0].data_len = 1;
    ad_data[0].data = &flags;

    /* AD Element 2: Service Data (16-bit UUID) - BTHome data */
    ad_data[1].type = BT_DATA_SVC_DATA16;
    ad_data[1].data_len = sizeof(struct bthome_service_data);
    ad_data[1].data = (uint8_t *)bthome;

    /* AD Element 3: Complete Device Name */
    ad_data[2].type = BT_DATA_NAME_COMPLETE;
    ad_data[2].data = "BTHome Counter";
    ad_data[2].data_len = strlen("BTHome Counter");
}

/* Bluetooth ready callback */
static void bt_ready(int err)
{
    bt_addr_le_t current_addr;
    size_t count = 1;
    
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");
    
    /* Check what identity we actually have */
    bt_id_get(&current_addr, &count);
    
    if (count > 0) {
        LOG_INF("Active MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                current_addr.a.val[5], current_addr.a.val[4], current_addr.a.val[3], 
                current_addr.a.val[2], current_addr.a.val[1], current_addr.a.val[0]);
    }
    
    LOG_INF("BTHome Counter starting with device ID 0x%04X", BTHOME_DEVICE_ID);
    LOG_INF("BTHome device info: 0x%02X (v2, no encryption, regular updates)", BTHOME_DEVICE_INFO);
}

/* Advertisement work handler */
static void advertise_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(advertise_work, advertise_work_handler);

/* Global advertisement data - persistent across updates */
static struct bt_data ad_data[3];  /* Flags + Service Data + Name */
static struct bthome_service_data bthome;
static bool advertising_started = false;

static void advertise_work_handler(struct k_work *work)
{
    int err;

    /* LED an - Advertisement beginnt */
    gpio_pin_set_dt(&led1, 1);

    /* Increment counter */
    counter_value++;
    
    /* Update BTHome advertisement data */
    build_bthome_adv_data(ad_data, &bthome);

    if (!advertising_started) {
        /* Start advertising for the first time with USE_IDENTITY flag */
        err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                                              BT_GAP_ADV_SLOW_INT_MIN,
                                              BT_GAP_ADV_SLOW_INT_MAX,
                                              NULL),
                              ad_data, ARRAY_SIZE(ad_data), NULL, 0);
        if (err) {
            LOG_ERR("Failed to start advertising (err %d)", err);
        } else {
            LOG_INF("BTHome advertising started successfully");
            advertising_started = true;
        }
    } else {
        /* Update existing advertisement data */
        err = bt_le_adv_update_data(ad_data, ARRAY_SIZE(ad_data), NULL, 0);
        if (err) {
            LOG_ERR("Failed to update advertising data (err %d)", err);
            /* Try to restart advertising on update failure */
            bt_le_adv_stop();
            k_sleep(K_MSEC(50));
            err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,
                                                  BT_GAP_ADV_SLOW_INT_MIN,
                                                  BT_GAP_ADV_SLOW_INT_MAX,
                                                  NULL),
                                  ad_data, ARRAY_SIZE(ad_data), NULL, 0);
            if (err) {
                LOG_ERR("Failed to restart advertising (err %d)", err);
                advertising_started = false;
            }
        }
    }

    if (advertising_started) {
        LOG_INF("BTHome advertisement updated: Counter = %u", counter_value);
        
        /* Log the raw BTHome service data for debugging */
        LOG_HEXDUMP_DBG(&bthome, sizeof(bthome), "BTHome service data:");
    }

    /* LED aus nach kurzer Zeit - Advertisement beendet */
    k_sleep(K_MSEC(100));
    gpio_pin_set_dt(&led1, 0);

    /* Schedule next advertisement in 5 seconds */
    k_work_schedule(&advertise_work, K_SECONDS(5));
}

int main(void)
{
    int err;
    bt_addr_le_t fixed_addr;

    LOG_INF("BTHome Counter Example for nRF52840-DK");
    LOG_INF("Board: %s", CONFIG_BOARD_TARGET);

    /* Initialize LED1 for visual feedback */
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED1 device not ready");
        return -1;
    }
    
    err = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (err < 0) {
        LOG_ERR("Failed to configure LED1 pin: %d", err);
        return -1;
    }
    
    LOG_INF("LED1 initialized successfully");

    /* WICHTIG: MAC-Adresse VOR bt_enable() setzen! */
    get_fixed_mac_address(&fixed_addr);
    
    /* Create identity with fixed MAC before enabling Bluetooth */
    err = bt_id_create(&fixed_addr, NULL);
    if (err < 0) {
        LOG_ERR("Failed to create fixed identity: %d", err);
        /* Continue anyway - system will use default */
    } else {
        LOG_INF("Fixed identity created successfully with ID: %d", err);
    }

    /* Initialize Bluetooth - now it will use our fixed MAC */
    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Wait for Bluetooth to be ready */
    k_sleep(K_SECONDS(2));

    /* Start the advertising work */
    k_work_schedule(&advertise_work, K_SECONDS(3));

    LOG_INF("BTHome Counter is running...");
    LOG_INF("Sending counter values every 5 seconds");
    LOG_INF("Use a BTHome-compatible app (e.g., Home Assistant) to receive data");

    /* Main loop - just keep the system running */
    while (1) {
        k_sleep(K_SECONDS(10));
        LOG_INF("System running, current counter: %u", counter_value);
    }

    return 0;
}