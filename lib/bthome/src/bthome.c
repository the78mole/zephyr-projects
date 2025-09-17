/*
 * Copyright (c) 2025 BTHome v2 for Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bthome/bthome.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <string.h>

LOG_MODULE_REGISTER(bthome, LOG_LEVEL_INF);

/* Forward declarations */
static void bthome_adv_work_handler(struct k_work *work);

/* Global persistent service data buffer */
static uint8_t g_service_data[BTHOME_MAX_PAYLOAD_SIZE + 8];

/* BTHome v2 Service Data Structure */
struct bthome_service_header {
    uint16_t service_uuid;         /* 0xFCD2 - BTHome Service UUID (little endian) */
    uint8_t device_info;           /* Device info flags */
} __packed;

/* Helper function to get data size for object ID */
static uint8_t bthome_get_data_size(uint8_t object_id)
{
    switch (object_id) {
    /* 8-bit values */
    case BTHOME_ID_PACKET:
    case BTHOME_ID_BATTERY:
    case BTHOME_ID_COUNT:
    case BTHOME_ID_HUMIDITY:
    case BTHOME_ID_MOISTURE:
    case BTHOME_ID_UV:
    case BTHOME_STATE_GENERIC_BOOLEAN:
    case BTHOME_STATE_POWER_ON:
    case BTHOME_STATE_OPENING:
    case BTHOME_STATE_BATTERY_LOW:
    case BTHOME_STATE_BATTERY_CHARGING:
    case BTHOME_STATE_CO:
    case BTHOME_STATE_COLD:
    case BTHOME_STATE_CONNECTIVITY:
    case BTHOME_STATE_DOOR:
    case BTHOME_STATE_GARAGE_DOOR:
    case BTHOME_STATE_GAS_DETECTED:
    case BTHOME_STATE_HEAT:
    case BTHOME_STATE_LIGHT:
    case BTHOME_STATE_LOCK:
    case BTHOME_STATE_MOISTURE:
    case BTHOME_STATE_MOTION:
    case BTHOME_STATE_MOVING:
    case BTHOME_STATE_OCCUPANCY:
    case BTHOME_STATE_PLUG:
    case BTHOME_STATE_PRESENCE:
    case BTHOME_STATE_PROBLEM:
    case BTHOME_STATE_RUNNING:
    case BTHOME_STATE_SAFETY:
    case BTHOME_STATE_SMOKE:
    case BTHOME_STATE_SOUND:
    case BTHOME_STATE_TAMPER:
    case BTHOME_STATE_VIBRATION:
    case BTHOME_STATE_WINDOW:
    case BTHOME_EVENT_BUTTON:
        return 1;

    /* 16-bit values */
    case BTHOME_ID_TEMPERATURE_PRECISE:
    case BTHOME_ID_HUMIDITY_PRECISE:
    case BTHOME_ID_DEWPOINT:
    case BTHOME_ID_VOLTAGE:
    case BTHOME_ID_PM25:
    case BTHOME_ID_PM10:
    case BTHOME_ID_CO2:
    case BTHOME_ID_TVOC:
    case BTHOME_ID_MOISTURE_PRECISE:
    case BTHOME_ID_MASS:
    case BTHOME_ID_MASS_LB:
    case BTHOME_ID_COUNT2:
    case BTHOME_ID_ROTATION:
    case BTHOME_ID_DISTANCE:
    case BTHOME_ID_DISTANCE_M:
    case BTHOME_ID_CURRENT:
    case BTHOME_ID_SPEED:
    case BTHOME_ID_TEMPERATURE:
    case BTHOME_ID_VOLUME1:
    case BTHOME_ID_VOLUME2:
    case BTHOME_ID_VOLUME_FLOW_RATE:
    case BTHOME_ID_VOLTAGE1:
        return 2;

    /* 24-bit values */
    case BTHOME_ID_PRESSURE:
    case BTHOME_ID_ILLUMINANCE:
    case BTHOME_ID_ENERGY:
    case BTHOME_ID_POWER:
    case BTHOME_ID_DURATION:
    case BTHOME_ID_GAS:
        return 3;

    /* 32-bit values */
    case BTHOME_ID_COUNT4:
    case BTHOME_ID_ENERGY4:
    case BTHOME_ID_GAS4:
    case BTHOME_ID_VOLUME:
    case BTHOME_ID_WATER:
    case BTHOME_ID_TIMESTAMP:
        return 4;

    default:
        LOG_WRN("Unknown object ID: 0x%02X, assuming 2 bytes", object_id);
        return 2;
    }
}

/* Helper function to get scaling factor for object ID */
static uint16_t bthome_get_scale_factor(uint8_t object_id)
{
    switch (object_id) {
    /* 0.1 scaling */
    case BTHOME_ID_DISTANCE_M:
    case BTHOME_ID_ROTATION:
    case BTHOME_ID_TEMPERATURE:
    case BTHOME_ID_VOLTAGE1:
    case BTHOME_ID_VOLUME1:
    case BTHOME_ID_UV:
        return 10;

    /* 0.01 scaling */
    case BTHOME_ID_TEMPERATURE_PRECISE:
    case BTHOME_ID_HUMIDITY_PRECISE:
    case BTHOME_ID_DEWPOINT:
    case BTHOME_ID_ILLUMINANCE:
    case BTHOME_ID_MASS:
    case BTHOME_ID_MASS_LB:
    case BTHOME_ID_MOISTURE_PRECISE:
    case BTHOME_ID_POWER:
    case BTHOME_ID_PRESSURE:
    case BTHOME_ID_SPEED:
        return 100;

    /* 0.001 scaling */
    case BTHOME_ID_CURRENT:
    case BTHOME_ID_DURATION:
    case BTHOME_ID_ENERGY:
    case BTHOME_ID_ENERGY4:
    case BTHOME_ID_GAS:
    case BTHOME_ID_GAS4:
    case BTHOME_ID_VOLTAGE:
    case BTHOME_ID_VOLUME:
    case BTHOME_ID_WATER:
        return 1000;

    default:
        return 1;  /* No scaling */
    }
}

/* Platform-specific MAC address generation */
#if defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_NRF52833) || defined(CONFIG_SOC_NRF52832)
#include <hal/nrf_ficr.h>

int bthome_set_fixed_mac(void)
{
    bt_addr_le_t addr;
    uint32_t deviceaddr_low, deviceaddr_high;
    int err;

    /* Read Nordic FICR.DEVICEADDR registers */
    deviceaddr_low = nrf_ficr_deviceaddr_get(NRF_FICR, 0);
    deviceaddr_high = nrf_ficr_deviceaddr_get(NRF_FICR, 1);

    LOG_INF("FICR.DEVICEADDR: 0x%08X%08X", deviceaddr_high, deviceaddr_low);

    /* Generate deterministic MAC address */
    addr.type = BT_ADDR_LE_RANDOM;
    addr.a.val[0] = (deviceaddr_low >> 0) & 0xFF;
    addr.a.val[1] = (deviceaddr_low >> 8) & 0xFF;
    addr.a.val[2] = (deviceaddr_low >> 16) & 0xFF;
    addr.a.val[3] = (deviceaddr_low >> 24) & 0xFF;
    addr.a.val[4] = (deviceaddr_high >> 0) & 0xFF;
    addr.a.val[5] = ((deviceaddr_high >> 8) & 0x3F) | 0xC0;  /* Static random address */

    LOG_INF("Generated fixed MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            addr.a.val[5], addr.a.val[4], addr.a.val[3],
            addr.a.val[2], addr.a.val[1], addr.a.val[0]);

    /* Create identity with fixed MAC before bt_enable() */
    err = bt_id_create(&addr, NULL);
    if (err < 0) {
        LOG_ERR("Failed to create fixed identity: %d", err);
        return err;
    }

    LOG_INF("Fixed identity created successfully");
    return 0;
}

#elif defined(CONFIG_SOC_ESP32) || defined(CONFIG_SOC_ESP32C3) || defined(CONFIG_SOC_ESP32S3)
#include <esp_system.h>

int bthome_set_fixed_mac(void)
{
    bt_addr_le_t addr;
    uint8_t base_mac[6];
    int err;

    /* Get ESP32 base MAC address */
    err = esp_read_mac(base_mac, ESP_MAC_BT);
    if (err != 0) {
        LOG_ERR("Failed to read ESP32 MAC: %d", err);
        return -EIO;
    }

    /* Use base MAC as BLE MAC */
    addr.type = BT_ADDR_LE_RANDOM;
    memcpy(addr.a.val, base_mac, 6);
    addr.a.val[5] |= 0xC0;  /* Ensure static random address format */

    LOG_INF("Generated fixed MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            addr.a.val[5], addr.a.val[4], addr.a.val[3],
            addr.a.val[2], addr.a.val[1], addr.a.val[0]);

    /* Create identity with fixed MAC */
    err = bt_id_create(&addr, NULL);
    if (err < 0) {
        LOG_ERR("Failed to create fixed identity: %d", err);
        return err;
    }

    LOG_INF("Fixed identity created successfully");
    return 0;
}

#else
#warning "Platform-specific MAC generation not implemented, using random MAC"

int bthome_set_fixed_mac(void)
{
    LOG_WRN("Fixed MAC not supported on this platform, using random MAC");
    return 0;
}
#endif

int bthome_init(struct bthome_device *dev, const struct bthome_config *config)
{
    if (!dev || !config) {
        return -EINVAL;
    }

    memset(dev, 0, sizeof(*dev));
    memcpy(&dev->config, config, sizeof(*config));

    /* Initialize work queue for advertisement timeout */
    k_work_init_delayable(&dev->adv_work, bthome_adv_work_handler);

    LOG_INF("BTHome device initialized: %s", config->device_name);
    LOG_INF("Encryption: %s, Trigger-based: %s",
            config->encryption ? "enabled" : "disabled",
            config->trigger_based ? "yes" : "no");

    return 0;
}

void bthome_reset_measurements(struct bthome_device *dev)
{
    if (!dev) {
        return;
    }

    dev->payload_len = 0;
    LOG_DBG("Measurements reset");
}

static int bthome_add_data(struct bthome_device *dev, uint8_t object_id,
                          const void *data, uint8_t size)
{
    uint8_t required_space = 1 + size;  /* Object ID + data */
    uint8_t max_payload = dev->config.encryption ? 
                         BTHOME_MAX_PAYLOAD_ENC : BTHOME_MAX_PAYLOAD_SIZE;

    if (!dev || !data) {
        return -EINVAL;
    }

    if (dev->payload_len + required_space > max_payload) {
        LOG_WRN("Payload full, cannot add object 0x%02X", object_id);
        return -ENOSPC;
    }

    /* Add object ID */
    dev->payload[dev->payload_len++] = object_id;

    /* Add data */
    memcpy(&dev->payload[dev->payload_len], data, size);
    dev->payload_len += size;

    LOG_DBG("Added object 0x%02X, size %u, total payload: %u",
            object_id, size, dev->payload_len);

    return 0;
}

int bthome_add_measurement(struct bthome_device *dev,
                          const struct bthome_measurement *measurement)
{
    uint8_t data_size;
    uint8_t data_buf[8];

    if (!dev || !measurement) {
        return -EINVAL;
    }

    data_size = bthome_get_data_size(measurement->object_id);

    /* Convert value to little-endian byte array */
    switch (data_size) {
    case 1:
        data_buf[0] = measurement->value.u8;
        break;
    case 2:
        sys_put_le16(measurement->value.u16, data_buf);
        break;
    case 3:
        sys_put_le24(measurement->value.u32, data_buf);
        break;
    case 4:
        sys_put_le32(measurement->value.u32, data_buf);
        break;
    default:
        /* Raw data */
        if (measurement->data_size > sizeof(data_buf)) {
            return -EINVAL;
        }
        memcpy(data_buf, measurement->value.data, measurement->data_size);
        data_size = measurement->data_size;
        break;
    }

    return bthome_add_data(dev, measurement->object_id, data_buf, data_size);
}

int bthome_add_state(struct bthome_device *dev, uint8_t object_id, uint8_t state)
{
    struct bthome_measurement measurement = {
        .object_id = object_id,
        .value.u8 = state ? BTHOME_STATE_ON : BTHOME_STATE_OFF,
    };

    return bthome_add_measurement(dev, &measurement);
}

int bthome_add_sensor(struct bthome_device *dev, uint8_t object_id, float value)
{
    struct bthome_measurement measurement;
    uint16_t scale_factor;
    uint8_t data_size;

    if (!dev) {
        return -EINVAL;
    }

    measurement.object_id = object_id;
    scale_factor = bthome_get_scale_factor(object_id);
    data_size = bthome_get_data_size(object_id);

    /* Scale and convert value */
    uint64_t scaled_value = (uint64_t)(value * scale_factor);

    switch (data_size) {
    case 1:
        measurement.value.u8 = (uint8_t)scaled_value;
        break;
    case 2:
        measurement.value.u16 = (uint16_t)scaled_value;
        break;
    case 3:
    case 4:
        measurement.value.u32 = (uint32_t)scaled_value;
        break;
    default:
        return -EINVAL;
    }

    measurement.data_size = data_size;

    LOG_INF("Adding sensor: OID=0x%02X, value=%.2f, scaled=%llu, size=%u bytes", 
            object_id, (double)value, scaled_value, data_size);

    return bthome_add_measurement(dev, &measurement);
}

int bthome_add_event(struct bthome_device *dev, uint8_t object_id,
                     uint8_t event, uint8_t steps)
{
    int err;
    struct bthome_measurement measurement = {
        .object_id = object_id,
        .value.u8 = event,
    };

    if (!dev) {
        return -EINVAL;
    }

    /* Add event value */
    err = bthome_add_measurement(dev, &measurement);
    if (err) {
        return err;
    }

    /* Add steps if dimmer event with steps */
    if (object_id == BTHOME_EVENT_DIMMER && event != BTHOME_EVENT_DIMMER_NONE && steps > 0) {
        measurement.value.u8 = steps;
        err = bthome_add_data(dev, object_id, &steps, 1);
    }

    return err;
}

static int bthome_build_advertisement(struct bthome_device *dev)
{
    struct bthome_service_header header;
    uint8_t flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
    uint8_t service_data_len;

    if (!dev) {
        return -EINVAL;
    }

    /* Build BTHome service data header */
    header.service_uuid = sys_cpu_to_le16(BTHOME_SERVICE_UUID);
    
    if (dev->config.trigger_based) {
        header.device_info = dev->config.encryption ? 
                           BTHOME_ENCRYPT_TRIGGER : BTHOME_NO_ENCRYPT_TRIGGER;
    } else {
        header.device_info = dev->config.encryption ? 
                           BTHOME_ENCRYPT : BTHOME_NO_ENCRYPT;
    }

    /* Copy complete header into global service data */
    memcpy(g_service_data, &header, sizeof(header));
    service_data_len = sizeof(header);

    /* Copy payload */
    memcpy(&g_service_data[service_data_len], dev->payload, dev->payload_len);
    service_data_len += dev->payload_len;

    /* Clear advertisement data array */
    memset(dev->ad_data, 0, sizeof(dev->ad_data));

    /* Setup advertisement data */
    /* Element 1: Flags */
    dev->ad_data[0].type = BT_DATA_FLAGS;
    dev->ad_data[0].data_len = 1;
    dev->ad_data[0].data = &flags;

    /* Element 2: Service Data */
    dev->ad_data[1].type = BT_DATA_SVC_DATA16;
    dev->ad_data[1].data_len = service_data_len;
    dev->ad_data[1].data = g_service_data;  /* Use global service data */

    /* Element 3: Complete Device Name */
    dev->ad_data[2].type = BT_DATA_NAME_COMPLETE;
    dev->ad_data[2].data = dev->config.device_name;
    dev->ad_data[2].data_len = strlen(dev->config.device_name);

    LOG_INF("Advertisement built: payload=%u bytes, total=%u elements",
            dev->payload_len, 3);  // Now 3 elements again
    LOG_HEXDUMP_INF(g_service_data, service_data_len, "Service data:");
    
    /* Debug: Print advertisement structure */
    LOG_INF("AD Element 1 (Flags): type=0x%02X, len=%u, data=0x%02X", 
            dev->ad_data[0].type, dev->ad_data[0].data_len, flags);
    LOG_INF("AD Element 2 (Service Data): type=0x%02X, len=%u", 
            dev->ad_data[1].type, dev->ad_data[1].data_len);
    // LOG_INF("AD Element 3 (Name): type=0x%02X, len=%u, name='%s'", 
    //         dev->ad_data[2].type, dev->ad_data[2].data_len, (char*)dev->ad_data[2].data);

    return 0;
}

int bthome_advertise(struct bthome_device *dev, uint32_t duration_ms)
{
    int err;

    if (!dev) {
        return -EINVAL;
    }

    if (dev->payload_len == 0) {
        LOG_WRN("No measurements to advertise");
        return -ENODATA;
    }

    err = bthome_build_advertisement(dev);
    if (err) {
        return err;
    }

    /* Start advertising */
    struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
        BT_LE_ADV_OPT_USE_IDENTITY,
        BT_GAP_ADV_SLOW_INT_MIN,
        BT_GAP_ADV_SLOW_INT_MAX,
        NULL);

    err = bt_le_adv_start(&adv_param, dev->ad_data, 3, NULL, 0);  // 3 elements: Flags + Service Data + Name
    if (err) {
        LOG_ERR("Failed to start advertising: %d", err);
        return err;
    }

    dev->advertising = true;
    LOG_INF("BTHome advertising started (payload: %u bytes)", dev->payload_len);

    /* Stop advertising after duration if specified */
    if (duration_ms > 0) {
        k_work_schedule(&dev->adv_work, K_MSEC(duration_ms));
    }

    return 0;
}

static void bthome_adv_work_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct bthome_device *dev = CONTAINER_OF(dwork, struct bthome_device, adv_work);

    bthome_stop_advertising(dev);
}

int bthome_stop_advertising(struct bthome_device *dev)
{
    int err;

    if (!dev) {
        return -EINVAL;
    }

    if (!dev->advertising) {
        return 0;
    }

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising: %d", err);
        return err;
    }

    dev->advertising = false;
    k_work_cancel_delayable(&dev->adv_work);
    
    LOG_INF("BTHome advertising stopped");
    return 0;
}

bool bthome_is_advertising(const struct bthome_device *dev)
{
    return dev ? dev->advertising : false;
}