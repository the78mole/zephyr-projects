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

LOG_MODULE_REGISTER(bthome_counter, LOG_LEVEL_DBG);

/* BTHome device information */
#define BTHOME_DEVICE_ID        0x1234  /* Unique device ID */
#define BTHOME_VERSION          0x02    /* BTHome v2 */

/* BTHome object IDs - see BTHome specification */
#define BTHOME_COUNT_8          0x09    /* Count (8-bit) */
#define BTHOME_COUNT_16         0x3D    /* Count (16-bit) */

/* Counter state */
static uint16_t counter_value = 0;

/* BTHome advertisement data structure */
struct bthome_data {
    uint8_t length;         /* Length of data */
    uint8_t type;          /* AD Type (0x16 = Service Data) */
    uint16_t uuid;         /* BTHome Service UUID (0xFCD2) */
    uint8_t device_info;   /* Device info byte */
    uint8_t object_id;     /* Object ID */
    uint16_t value;        /* Counter value (little endian) */
} __packed;

/* Build BTHome advertisement packet */
static void build_bthome_adv_data(struct bt_data *ad_data, struct bthome_data *bthome)
{
    /* BTHome service data */
    bthome->length = sizeof(struct bthome_data) - 1;  /* Exclude length byte */
    bthome->type = BT_DATA_SVC_DATA16;
    bthome->uuid = 0xFCD2;  /* BTHome service UUID (little endian) */
    bthome->device_info = BTHOME_VERSION;  /* BTHome v2, no encryption */
    bthome->object_id = BTHOME_COUNT_16;   /* 16-bit counter */
    bthome->value = counter_value;         /* Counter value (little endian) */

    /* Setup advertisement data */
    ad_data[0].type = BT_DATA_SVC_DATA16;
    ad_data[0].data_len = sizeof(struct bthome_data) - 2;  /* Exclude length and type */
    ad_data[0].data = (uint8_t *)&bthome->uuid;

    /* Device name */
    ad_data[1].type = BT_DATA_NAME_COMPLETE;
    ad_data[1].data = "BTHome Counter";
    ad_data[1].data_len = strlen("BTHome Counter");
}

/* Bluetooth ready callback */
static void bt_ready(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");
    LOG_INF("BTHome Counter starting with device ID 0x%04X", BTHOME_DEVICE_ID);
}

/* Advertisement work handler */
static void advertise_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(advertise_work, advertise_work_handler);

static void advertise_work_handler(struct k_work *work)
{
    static struct bt_data ad_data[2];
    static struct bthome_data bthome;
    int err;

    /* Increment counter */
    counter_value++;
    
    /* Build BTHome advertisement */
    build_bthome_adv_data(ad_data, &bthome);

    /* Stop any existing advertising */
    err = bt_le_adv_stop();
    if (err && err != -EALREADY) {
        LOG_WRN("Failed to stop advertising (err %d)", err);
    }

    /* Start advertising with BTHome data */
    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad_data, ARRAY_SIZE(ad_data), NULL, 0);
    if (err) {
        LOG_ERR("Failed to start advertising (err %d)", err);
    } else {
        LOG_INF("BTHome advertisement sent: Counter = %u", counter_value);
        
        /* Log the raw BTHome data for debugging */
        LOG_HEXDUMP_DBG(&bthome, sizeof(bthome), "BTHome packet:");
    }

    /* Schedule next advertisement in 5 seconds */
    k_work_schedule(&advertise_work, K_SECONDS(5));
}

int main(void)
{
    int err;

    LOG_INF("BTHome Counter Example for nRF52840-DK");
    LOG_INF("Board: %s", CONFIG_BOARD_TARGET);

    /* Initialize Bluetooth */
    err = bt_enable(bt_ready);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Wait for Bluetooth to be ready */
    k_sleep(K_SECONDS(1));

    /* Start the advertising work */
    k_work_schedule(&advertise_work, K_SECONDS(2));

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