/*
 * Copyright (c) 2025 BTHome v2 for Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BTHOME_H_
#define ZEPHYR_INCLUDE_BTHOME_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief BTHome v2 API for Zephyr
 * 
 * This library provides a Zephyr-native implementation of BTHome v2 protocol
 * for creating Bluetooth LE sensors compatible with Home Assistant and other
 * BTHome-supporting platforms.
 * 
 * Based on the BTHome specification: https://bthome.io/
 * Inspired by: https://github.com/Chreece/BTHomeV2-ESP32-example
 */

/**
 * @defgroup bthome BTHome v2 API
 * @{
 */

/* BTHome v2 Protocol Constants */
#define BTHOME_SERVICE_UUID         0xFCD2  /**< BTHome service UUID */
#define BTHOME_MAX_PAYLOAD_SIZE     23      /**< Maximum payload size without encryption */
#define BTHOME_MAX_PAYLOAD_ENC      15      /**< Maximum payload size with encryption */

/* Use Kconfig for max measurements if available, otherwise default */
#ifdef CONFIG_BTHOME_MAX_MEASUREMENTS
#define BTHOME_MAX_MEASUREMENTS     CONFIG_BTHOME_MAX_MEASUREMENTS
#else
#define BTHOME_MAX_MEASUREMENTS     10      /**< Default max measurements per advertisement */
#endif

/* BTHome v2 Device Info Flags */
#define BTHOME_NO_ENCRYPT           0x40    /**< BTHome v2, no encryption */
#define BTHOME_NO_ENCRYPT_TRIGGER   0x44    /**< BTHome v2, no encryption, trigger-based */
#define BTHOME_ENCRYPT              0x41    /**< BTHome v2, with encryption */
#define BTHOME_ENCRYPT_TRIGGER      0x45    /**< BTHome v2, with encryption, trigger-based */

/* BTHome v2 Object IDs - Sensor Measurements */
#define BTHOME_ID_PACKET            0x00    /**< Packet ID (8-bit) */
#define BTHOME_ID_BATTERY           0x01    /**< Battery (8-bit, %) */
#define BTHOME_ID_TEMPERATURE_PRECISE 0x02  /**< Temperature precise (16-bit, 0.01°C) */
#define BTHOME_ID_HUMIDITY_PRECISE  0x03    /**< Humidity precise (16-bit, 0.01%) */
#define BTHOME_ID_PRESSURE          0x04    /**< Pressure (24-bit, 0.01 hPa) */
#define BTHOME_ID_ILLUMINANCE       0x05    /**< Illuminance (24-bit, 0.01 lux) */
#define BTHOME_ID_MASS              0x06    /**< Mass (16-bit, 0.01 kg) */
#define BTHOME_ID_MASS_LB           0x07    /**< Mass (16-bit, 0.01 lb) */
#define BTHOME_ID_DEWPOINT          0x08    /**< Dewpoint (16-bit, 0.01°C) */
#define BTHOME_ID_COUNT             0x09    /**< Count (8-bit) */
#define BTHOME_ID_ENERGY            0x0A    /**< Energy (24-bit, 0.001 kWh) */
#define BTHOME_ID_POWER             0x0B    /**< Power (24-bit, 0.01 W) */
#define BTHOME_ID_VOLTAGE           0x0C    /**< Voltage (16-bit, 0.001 V) */
#define BTHOME_ID_PM25              0x0D    /**< PM2.5 (16-bit, μg/m³) */
#define BTHOME_ID_PM10              0x0E    /**< PM10 (16-bit, μg/m³) */

/* BTHome v2 Object IDs - Binary States */
#define BTHOME_STATE_GENERIC_BOOLEAN 0x0F   /**< Generic boolean (8-bit) */
#define BTHOME_STATE_POWER_ON       0x10    /**< Power state (8-bit) */
#define BTHOME_STATE_OPENING        0x11    /**< Opening state (8-bit) */

/* BTHome v2 Object IDs - Gas and Air Quality */
#define BTHOME_ID_CO2               0x12    /**< CO2 (16-bit, ppm) */
#define BTHOME_ID_TVOC              0x13    /**< TVOC (16-bit, μg/m³) */
#define BTHOME_ID_MOISTURE_PRECISE  0x14    /**< Moisture precise (16-bit, 0.01%) */

/* BTHome v2 Object IDs - Device States */
#define BTHOME_STATE_BATTERY_LOW    0x15    /**< Battery low (8-bit) */
#define BTHOME_STATE_BATTERY_CHARGING 0x16  /**< Battery charging (8-bit) */
#define BTHOME_STATE_CO             0x17    /**< CO detected (8-bit) */
#define BTHOME_STATE_COLD           0x18    /**< Cold detected (8-bit) */
#define BTHOME_STATE_CONNECTIVITY   0x19    /**< Connectivity (8-bit) */
#define BTHOME_STATE_DOOR           0x1A    /**< Door state (8-bit) */
#define BTHOME_STATE_GARAGE_DOOR    0x1B    /**< Garage door (8-bit) */
#define BTHOME_STATE_GAS_DETECTED   0x1C    /**< Gas detected (8-bit) */
#define BTHOME_STATE_HEAT           0x1D    /**< Heat detected (8-bit) */
#define BTHOME_STATE_LIGHT          0x1E    /**< Light state (8-bit) */
#define BTHOME_STATE_LOCK           0x1F    /**< Lock state (8-bit) */
#define BTHOME_STATE_MOISTURE       0x20    /**< Moisture detected (8-bit) */
#define BTHOME_STATE_MOTION         0x21    /**< Motion detected (8-bit) */
#define BTHOME_STATE_MOVING         0x22    /**< Moving state (8-bit) */
#define BTHOME_STATE_OCCUPANCY      0x23    /**< Occupancy (8-bit) */
#define BTHOME_STATE_PLUG           0x24    /**< Plug state (8-bit) */
#define BTHOME_STATE_PRESENCE       0x25    /**< Presence (8-bit) */
#define BTHOME_STATE_PROBLEM        0x26    /**< Problem detected (8-bit) */
#define BTHOME_STATE_RUNNING        0x27    /**< Running state (8-bit) */
#define BTHOME_STATE_SAFETY         0x28    /**< Safety state (8-bit) */
#define BTHOME_STATE_SMOKE          0x29    /**< Smoke detected (8-bit) */
#define BTHOME_STATE_SOUND          0x2A    /**< Sound detected (8-bit) */
#define BTHOME_STATE_TAMPER         0x2B    /**< Tamper detected (8-bit) */
#define BTHOME_STATE_VIBRATION      0x2C    /**< Vibration detected (8-bit) */
#define BTHOME_STATE_WINDOW         0x2D    /**< Window state (8-bit) */

/* BTHome v2 Object IDs - Additional Sensors */
#define BTHOME_ID_HUMIDITY          0x2E    /**< Humidity (8-bit, %) */
#define BTHOME_ID_MOISTURE          0x2F    /**< Moisture (8-bit, %) */

/* BTHome v2 Object IDs - Events */
#define BTHOME_EVENT_BUTTON         0x3A    /**< Button press event */
#define BTHOME_EVENT_DIMMER         0x3C    /**< Dimmer event */

/* BTHome v2 Object IDs - Extended Counters */
#define BTHOME_ID_COUNT2            0x3D    /**< Count (16-bit) */
#define BTHOME_ID_COUNT4            0x3E    /**< Count (32-bit) */

/* BTHome v2 Object IDs - Additional Measurements */
#define BTHOME_ID_ROTATION          0x3F    /**< Rotation (16-bit, 0.1°) */
#define BTHOME_ID_DISTANCE          0x40    /**< Distance (16-bit, mm) */
#define BTHOME_ID_DISTANCE_M        0x41    /**< Distance (16-bit, 0.1 m) */
#define BTHOME_ID_DURATION          0x42    /**< Duration (24-bit, 0.001 s) */
#define BTHOME_ID_CURRENT           0x43    /**< Current (16-bit, 0.001 A) */
#define BTHOME_ID_SPEED             0x44    /**< Speed (16-bit, 0.01 m/s) */
#define BTHOME_ID_TEMPERATURE       0x45    /**< Temperature (16-bit, 0.1°C) */
#define BTHOME_ID_UV                0x46    /**< UV index (8-bit, 0.1) */
#define BTHOME_ID_VOLUME1           0x47    /**< Volume (16-bit, 0.1 L) */
#define BTHOME_ID_VOLUME2           0x48    /**< Volume (16-bit, mL) */
#define BTHOME_ID_VOLUME_FLOW_RATE  0x49    /**< Volume flow rate (16-bit, m³/hr) */
#define BTHOME_ID_VOLTAGE1          0x4A    /**< Voltage (16-bit, 0.1 V) */
#define BTHOME_ID_GAS               0x4B    /**< Gas (24-bit, 0.001 m³) */
#define BTHOME_ID_GAS4              0x4C    /**< Gas (32-bit, 0.001 m³) */
#define BTHOME_ID_ENERGY4           0x4D    /**< Energy (32-bit, 0.001 kWh) */
#define BTHOME_ID_VOLUME            0x4E    /**< Volume (32-bit, 0.001 m³) */
#define BTHOME_ID_WATER             0x4F    /**< Water (32-bit, 0.001 L) */
#define BTHOME_ID_TIMESTAMP         0x50    /**< Timestamp (32-bit, Unix epoch) */

/* BTHome v2 Event Values */
#define BTHOME_EVENT_BUTTON_NONE             0x00  /**< No button event */
#define BTHOME_EVENT_BUTTON_PRESS            0x01  /**< Button press */
#define BTHOME_EVENT_BUTTON_DOUBLE_PRESS     0x02  /**< Button double press */
#define BTHOME_EVENT_BUTTON_TRIPLE_PRESS     0x03  /**< Button triple press */
#define BTHOME_EVENT_BUTTON_LONG_PRESS       0x04  /**< Button long press */
#define BTHOME_EVENT_BUTTON_LONG_DOUBLE_PRESS 0x05 /**< Button long double press */
#define BTHOME_EVENT_BUTTON_LONG_TRIPLE_PRESS 0x06 /**< Button long triple press */

#define BTHOME_EVENT_DIMMER_NONE    0x00    /**< No dimmer event */
#define BTHOME_EVENT_DIMMER_LEFT    0x01    /**< Dimmer rotate left */
#define BTHOME_EVENT_DIMMER_RIGHT   0x02    /**< Dimmer rotate right */

/* BTHome v2 State Values */
#define BTHOME_STATE_OFF            0x00    /**< State: OFF */
#define BTHOME_STATE_ON             0x01    /**< State: ON */

/**
 * @brief BTHome device configuration
 */
struct bthome_config {
    const char *device_name;        /**< Device name for advertising */
    bool encryption;                /**< Enable encryption */
    bool trigger_based;             /**< Trigger-based device */
    uint8_t bind_key[16];          /**< Encryption key (if encryption enabled) */
};

/**
 * @brief BTHome sensor measurement
 */
struct bthome_measurement {
    uint8_t object_id;             /**< BTHome object ID */
    union {
        uint8_t u8;                /**< 8-bit value */
        uint16_t u16;              /**< 16-bit value */
        uint32_t u32;              /**< 32-bit value */
        uint64_t u64;              /**< 64-bit value */
        float f;                   /**< Float value */
        uint8_t *data;             /**< Raw data pointer */
    } value;                       /**< Measurement value */
    uint8_t data_size;             /**< Size of raw data (if applicable) */
};

/**
 * @brief BTHome device instance
 */
struct bthome_device {
    struct bthome_config config;    /**< Device configuration */
    uint8_t payload[BTHOME_MAX_PAYLOAD_SIZE]; /**< Advertisement payload */
    uint8_t payload_len;           /**< Current payload length */
    bool advertising;              /**< Advertising state */
    uint32_t encrypt_counter;      /**< Encryption counter */
    struct bt_data ad_data[3];     /**< Advertisement data elements */
    struct k_work_delayable adv_work; /**< Advertisement work item */
};

/**
 * @brief Initialize BTHome device
 * 
 * @param dev BTHome device instance
 * @param config Device configuration
 * @return 0 on success, negative error code on failure
 */
int bthome_init(struct bthome_device *dev, const struct bthome_config *config);

/**
 * @brief Reset measurement data
 * 
 * @param dev BTHome device instance
 */
void bthome_reset_measurements(struct bthome_device *dev);

/**
 * @brief Add a measurement to the current packet
 * 
 * @param dev BTHome device instance
 * @param measurement Measurement to add
 * @return 0 on success, negative error code on failure
 */
int bthome_add_measurement(struct bthome_device *dev, 
                          const struct bthome_measurement *measurement);

/**
 * @brief Add a state measurement (binary sensor)
 * 
 * @param dev BTHome device instance
 * @param object_id BTHome object ID
 * @param state State value (0 or 1)
 * @return 0 on success, negative error code on failure
 */
int bthome_add_state(struct bthome_device *dev, uint8_t object_id, uint8_t state);

/**
 * @brief Add a sensor value (automatically scaled)
 * 
 * @param dev BTHome device instance
 * @param object_id BTHome object ID
 * @param value Sensor value
 * @return 0 on success, negative error code on failure
 */
int bthome_add_sensor(struct bthome_device *dev, uint8_t object_id, float value);

/**
 * @brief Add an event measurement
 * 
 * @param dev BTHome device instance
 * @param object_id BTHome object ID
 * @param event Event value
 * @param steps Number of steps (for dimmer events, optional)
 * @return 0 on success, negative error code on failure
 */
int bthome_add_event(struct bthome_device *dev, uint8_t object_id, 
                     uint8_t event, uint8_t steps);

/**
 * @brief Send current measurements as advertisement
 * 
 * @param dev BTHome device instance
 * @param duration_ms Advertisement duration in milliseconds (0 = indefinite)
 * @return 0 on success, negative error code on failure
 */
int bthome_advertise(struct bthome_device *dev, uint32_t duration_ms);

/**
 * @brief Stop advertising
 * 
 * @param dev BTHome device instance
 * @return 0 on success, negative error code on failure
 */
int bthome_stop_advertising(struct bthome_device *dev);

/**
 * @brief Check if device is currently advertising
 * 
 * @param dev BTHome device instance
 * @return true if advertising, false otherwise
 */
bool bthome_is_advertising(const struct bthome_device *dev);

/**
 * @brief Set fixed MAC address based on device-specific hardware ID
 * 
 * This function generates a stable MAC address from the device's factory-programmed
 * unique identifier (e.g., Nordic FICR.DEVICEADDR).
 * 
 * @return 0 on success, negative error code on failure
 */
int bthome_set_fixed_mac(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BTHOME_H_ */