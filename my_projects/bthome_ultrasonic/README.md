# BTHome Ultrasonic Distance Sensor

This project implements a BTHome v2 compatible ultrasonic distance sensor using the HC-SR04 sensor and the nRF52840-DK development board with Zephyr RTOS.

## Overview

The application measures distance using an HC-SR04 ultrasonic sensor and broadcasts the measurements via Bluetooth LE using the BTHome v2 protocol. This makes it compatible with Home Assistant and other smart home systems that support BTHome devices.

## Hardware Requirements

- nRF52840-DK development board
- HC-SR04 ultrasonic distance sensor
- Jumper wires for connections

## Pin Connections

| HC-SR04 Pin | nRF52840-DK Pin | Description |
|-------------|-----------------|-------------|
| VCC         | 5V or 3.3V      | Power supply |
| GND         | GND             | Ground |
| Trig        | P0.11 (Button1) | Trigger signal |
| Echo        | P0.12 (Button2) | Echo signal |

**Note**: The current implementation uses the button pins (P0.11 and P0.12) for easy testing. For production use, consider using dedicated GPIO pins.

## Features

- **BTHome v2 Protocol**: Full compatibility with BTHome standard
- **Distance Measurement**: Range 2cm to 4m with HC-SR04 sensor
- **Bluetooth LE**: Low energy consumption with periodic advertisements
- **Home Assistant Integration**: Automatic discovery as distance sensor
- **Error Handling**: Robust measurement with timeout and validation
- **Configurable Timing**: Adjustable measurement intervals

## BTHome Protocol Details

The device sends BTHome v2 advertisements with:
- **Service UUID**: 0xFCD2 (BTHome)
- **Object ID**: 0x40 (Distance in millimeters)
- **Data Format**: 16-bit little-endian distance value
- **Advertisement Interval**: 5 seconds (configurable)

## Building and Flashing

1. **Set up Zephyr environment**:
   ```bash
   cd /path/to/zephyr
   source zephyr-env.sh
   ```

2. **Navigate to project directory**:
   ```bash
   cd my_projects/bthome_ultrasonic
   ```

3. **Build the application**:
   ```bash
   west build -b nrf52840dk_nrf52840
   ```

4. **Flash to board**:
   ```bash
   west flash
   ```

## Configuration

### Measurement Settings

Edit `src/main.c` to modify:
- `MEASUREMENT_INTERVAL_MS`: Time between measurements (default: 5000ms)
- `sensor_config.max_distance_mm`: Maximum detection range (default: 4000mm)
- `sensor_config.timeout_us`: Echo timeout (default: 30000µs)

### GPIO Pin Configuration

Modify `boards/nrf52840dk_nrf52840.overlay` to change pin assignments:
```dts
/* Change trigger pin */
trigger-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;

/* Change echo pin */
echo-gpios = <&gpio0 14 GPIO_ACTIVE_HIGH>;
```

## Usage

1. **Power on** the nRF52840-DK board
2. **Connect HC-SR04 sensor** according to pin connections
3. **Monitor logs** via UART (115200 baud):
   ```
   BTHome Ultrasonic Distance Sensor starting...
   HC-SR04 sensor initialized successfully
   Bluetooth initialized
   Distance: 1250 mm
   Advertising distance: 1250 mm
   ```

## Home Assistant Integration

The device will be automatically discovered in Home Assistant if BTHome integration is enabled:

1. **Enable BTHome integration** in Home Assistant
2. **Device discovery**: The sensor appears as "BTHome Distance Sensor"
3. **Entity**: Distance measurement in millimeters
4. **Unit conversion**: Home Assistant automatically converts to meters/centimeters

### Example Home Assistant Entity

```yaml
sensor:
  - platform: bthome
    name: "Ultrasonic Distance"
    device_class: distance
    unit_of_measurement: mm
    state_class: measurement
```

## Troubleshooting

### Common Issues

1. **No distance measurements**:
   - Check HC-SR04 connections
   - Verify power supply (5V recommended)
   - Ensure trigger/echo pins are correctly connected
   - Check for obstacles in sensor beam path

2. **Bluetooth not advertising**:
   - Verify Bluetooth stack initialization
   - Check for conflicting GPIO usage
   - Ensure sufficient power supply

3. **Inaccurate measurements**:
   - Calibrate timing constants in `hcsr04.c`
   - Check for electromagnetic interference
   - Ensure stable mounting of sensor

### Debug Output

Enable detailed logging by modifying `prj.conf`:
```
CONFIG_LOG_DEFAULT_LEVEL=4
CONFIG_HCSR04_LOG_LEVEL_DBG=y
```

### Error Codes

- **-ETIMEDOUT**: Echo signal timeout (no object or out of range)
- **-ERANGE**: Measurement outside valid range (< 20mm or > max_distance)
- **-EBUSY**: Previous measurement still in progress
- **-ENODEV**: GPIO device not ready

## Technical Details

### HC-SR04 Timing

The HC-SR04 sensor requires precise timing:
- **Trigger pulse**: 10µs high pulse
- **Echo measurement**: Time between echo rising and falling edge
- **Distance calculation**: `distance_mm = (echo_time_us * 10) / 58`

### Bluetooth Advertisement Format

BTHome v2 packet structure:
```
[Service UUID: 0xFCD2][Info: 0x40][Object ID: 0x40][Distance: 2 bytes LE]
```

Example for 1250mm distance:
```
D2 FC 40 40 E2 04
```

## Power Consumption

Typical power consumption:
- **Active measurement**: ~15mA for 100ms
- **Bluetooth advertising**: ~8mA for 1ms every 5s
- **Sleep mode**: ~5µA between measurements
- **Average power**: ~200µA (with 5s intervals)

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## Acknowledgments

- Based on original ESP32-S3 implementation from [the78mole/BThome-US-Ranging](https://github.com/the78mole/BThome-US-Ranging)
- BTHome protocol specification: [BTHome.io](https://bthome.io/)
- Zephyr RTOS documentation and samples