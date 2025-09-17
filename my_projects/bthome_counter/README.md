# BTHome Counter Project for nRF52840-DK

A Zephyr RTOS application that demonstrates BTHome (Bluetooth Home) protocol by broadcasting counter values via Bluetooth LE advertisements.

## Overview

This project implements a BTHome v2 compatible device that:

- **Broadcasts counter values** every 5 seconds via Bluetooth LE advertisements
- **Uses BTHome protocol** for Home Assistant and other smart home integration
- **Increments counter** starting from 1 (16-bit counter)
- **No pairing required** - data is sent via advertisements
- **Low power consumption** - only advertising, no connections

## BTHome Protocol

BTHome is an open standard for Bluetooth LE sensor data transmission, supported by:
- **Home Assistant** (native integration)
- **OpenHAB** (via BTHome binding)
- **ESPHome** (BTHome proxy)
- **Custom applications** using BTHome specification

### Packet Format

The project sends BTHome v2 service data with this structure:

```
Service UUID: 0xFCD2 (BTHome)
Data: [DeviceInfo][ObjectID][Value]
- DeviceInfo: 0x02 (BTHome v2, no encryption)  
- ObjectID: 0x3D (Count 16-bit)
- Value: Counter value (little endian, 16-bit)
```

## Hardware Requirements

- **Nordic nRF52840-DK** development board
- **USB cable** for programming and power
- **Bluetooth LE scanner** (smartphone app or Home Assistant) for testing

## Building and Running

### Prerequisites

Ensure you have the Zephyr development environment set up (see main [README.md](../../README.md)).

### Build

```bash
cd my_projects/bthome_counter

# Build for nRF52840-DK
west build -b nrf52840dk/nrf52840
```

**Build Success:**
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      132920 B         1 MB     12.68%
             RAM:       23488 B       256 KB      8.96%
```

### Flash

```bash
# Connect nRF52840-DK via USB, then flash
west flash --runner openocd
```

**Alternative flash methods:**
```bash
# With PyOCD
west flash --runner pyocd

# With J-Link
west flash --runner jlink
```

## Expected Behavior

After successful flashing:

1. **Bluetooth advertising** starts automatically
2. **Counter increments** every 5 seconds (1, 2, 3, ...)
3. **BTHome advertisements** are sent with counter value
4. **Serial output** shows current counter value and debug info

## Testing the BTHome Signal

### Option 1: Home Assistant

1. **Add BTHome integration** in Home Assistant
2. **Enable Bluetooth** integration
3. **Device appears automatically** as "BTHome Counter"
4. **Counter sensor** shows incrementing values

### Option 2: Bluetooth Scanner Apps

**Android:**
- nRF Connect (Nordic)
- Bluetooth Scanner
- BTHome Scanner

**iOS:**
- LightBlue
- Bluetooth Scanner

**Look for:**
- Device name: "BTHome Counter"
- Service UUID: 0xFCD2
- Service data with incrementing counter

### Option 3: Python Script

```python
# Example to scan for BTHome devices
import asyncio
from bleak import BleakScanner

async def scan_bthome():
    def detection_callback(device, advertisement_data):
        if 0xFCD2 in advertisement_data.service_data:
            data = advertisement_data.service_data[0xFCD2]
            if len(data) >= 3:
                counter = int.from_bytes(data[2:4], 'little')
                print(f"BTHome Counter: {counter}")
    
    scanner = BleakScanner(detection_callback=detection_callback)
    await scanner.start()
    await asyncio.sleep(30)  # Scan for 30 seconds
    await scanner.stop()

asyncio.run(scan_bthome())
```

## Serial Output Example

```
[00:00:00.234,000] <inf> bthome_counter: BTHome Counter Example for nRF52840-DK
[00:00:00.234,000] <inf> bthome_counter: Board: nrf52840dk_nrf52840
[00:00:01.234,000] <inf> bthome_counter: Bluetooth initialized
[00:00:01.234,000] <inf> bthome_counter: BTHome Counter starting with device ID 0x1234
[00:00:03.234,000] <inf> bthome_counter: BTHome advertisement sent: Counter = 1
[00:00:08.234,000] <inf> bthome_counter: BTHome advertisement sent: Counter = 2
[00:00:13.234,000] <inf> bthome_counter: BTHome advertisement sent: Counter = 3
```

## Configuration

### Bluetooth Settings (prj.conf)

```ini
# Basic Bluetooth LE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_BROADCASTER=y

# Device name
CONFIG_BT_DEVICE_NAME="BTHome Counter"

# Disable unnecessary features for power savings
CONFIG_BT_EXT_ADV=n
CONFIG_BT_PRIVACY=n
CONFIG_BT_MAX_CONN=1
```

### Customization Options

**Change counter increment interval:**
```c
// In main.c, modify this line:
k_work_schedule(&advertise_work, K_SECONDS(5));  // Change from 5 seconds
```

**Change device name:**
```c
// In prj.conf:
CONFIG_BT_DEVICE_NAME="My BTHome Device"

// In main.c:
ad_data[1].data = "My BTHome Device";
```

**Add different sensor types:**
```c
// BTHome object IDs (see BTHome specification):
#define BTHOME_TEMPERATURE  0x02  // Temperature (°C)
#define BTHOME_HUMIDITY     0x03  // Humidity (%)
#define BTHOME_BATTERY      0x01  // Battery (%)
#define BTHOME_COUNT_8      0x09  // Count (8-bit)
#define BTHOME_COUNT_16     0x3D  // Count (16-bit)
```

## Troubleshooting

### Common Issues

1. **No Bluetooth advertisements visible**
   - Check that Bluetooth is enabled on scanning device
   - Verify nRF52840-DK is powered and flashed
   - Use nRF Connect app to verify advertisements

2. **Home Assistant not discovering device**
   - Ensure BTHome integration is installed
   - Check Bluetooth integration is enabled
   - Device may take up to 30 seconds to appear

3. **Build errors**
   - Verify Bluetooth stack is properly configured
   - Check Zephyr SDK version compatibility

4. **High memory usage**
   - Bluetooth stack requires significant RAM/Flash
   - This is normal for BLE applications

## BTHome Specification

- **Official documentation:** https://bthome.io/
- **Object IDs reference:** https://bthome.io/format/
- **Home Assistant integration:** https://www.home-assistant.io/integrations/bthome/

## Power Consumption

**Estimated current consumption:**
- **Advertising mode:** ~5-15 mA (depends on interval)
- **Sleep between ads:** ~5-20 µA
- **Average:** ~1-3 mA (with 5-second intervals)

**Battery life estimation (CR2032 ~220mAh):**
- **Continuous operation:** ~3-9 days
- **With optimizations:** Up to several weeks possible

## Next Steps

1. **Add real sensors** (temperature, humidity, etc.)
2. **Implement low power modes** for battery operation
3. **Add button for manual counter reset**
4. **Implement multiple BTHome objects** in one advertisement
5. **Add encryption** for secure BTHome communication

## Related Documentation

- [Main Project README](../../README.md) - Full setup guide
- [BTHome Specification](https://bthome.io/)
- [Zephyr Bluetooth API](https://docs.zephyrproject.org/latest/connectivity/bluetooth/api/index.html)
- [nRF52840-DK Documentation](https://docs.zephyrproject.org/latest/boards/nordic/nrf52840dk/doc/index.html)

---

**Ready for testing when nRF52840-DK board is available!** 🚀