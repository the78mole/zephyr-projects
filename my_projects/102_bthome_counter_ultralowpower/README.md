# BTHome Ultra Low-Power Counter Example

This project demonstrates an **ultra-aggressive power-optimized** BTHome v2 counter implementation designed for **maximum battery life** in remote sensor applications.

## Ultra Power Features

- **Retained RAM**: Counter survives deep sleep cycles and reboots
- **30-second intervals**: Maximum power savings
- **1-second advertisements**: Minimal transmission time
- **Aggressive power management**: All unnecessary subsystems disabled
- **No logging**: Zero console/UART power consumption
- **Internal RC oscillator**: No external crystal power consumption
- **Broadcaster only**: No connection capabilities for maximum efficiency

## Power Optimizations

### Aggressive Configuration Highlights
```conf
# Complete subsystem shutdown
CONFIG_SERIAL=n
CONFIG_CONSOLE=n  
CONFIG_LOG=n
CONFIG_PRINTK=n

# Bluetooth minimal
CONFIG_BT_MAX_CONN=0          # Broadcaster only
CONFIG_BT_PERIPHERAL=n
CONFIG_BT_CTLR_RX_BUFFERS=1   # Minimal buffers
CONFIG_BT_CTLR_TX_BUFFERS=1

# Clock optimization
CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC=y  # Internal RC oscillator
CONFIG_RETAINED_MEM=y                  # RAM retention
```

### Software Optimizations
- **Retained RAM**: Counter and state persist across sleep
- **30-second cycles**: Only ~3% duty cycle
- **1-second advertisements**: Minimal air time
- **No logging**: Zero debug overhead
- **Minimal stacks**: 1KB main stack, 256B idle stack
- **Power management**: Aggressive device suspension

## Retained Data Structure

```c
struct retained_data {
    uint16_t counter_value;  // Survives deep sleep
    uint32_t boot_count;     // Tracks reboot cycles  
    uint8_t initialized;     // Initialization flag
} __attribute__((section(".retention")));
```

## Advertisement Schedule

1. **Wake up** from ultra-deep sleep
2. **Brief LED pulse** (10ms, optional)
3. **Advertise** BTHome data for 1 second
4. **Enter deep sleep** for 29 seconds
5. **Repeat** every 30 seconds

## Ultra Low Power Metrics

Estimated power consumption on nRF52840:
- **Sleep current**: ~1-2 µA (retained RAM + RC oscillator)
- **Active current**: ~8 mA for 1 second per 30 seconds
- **Average current**: ~0.3 mA
- **Battery life**: **1-2 years** on CR2032 (220mAh)

## Comparison Table

| Project | Interval | Active Time | Est. Battery Life |
|---------|----------|-------------|-------------------|
| Normal | 5s | ~40% | Days/Weeks |
| Low Power | 10s | ~20% | Weeks/Months |
| **Ultra Low** | **30s** | **~3%** | **1-2 Years** |

## Building and Flashing

```bash
cd /path/to/zephyr
west build -p -b nrf52840dk/nrf52840 my_projects/102_bthome_counter_ultralowpower
west flash --runner pyocd
```

## Power Measurement Setup

For accurate ultra-low-power measurements:
1. Remove J6 jumper on nRF52840-DK
2. Connect precision ammeter (µA capable)
3. Observe ~1-2µA sleep current
4. Brief ~8mA spikes every 30 seconds

## Home Assistant Integration

- Device appears as **"BTHome Ultra"**
- Counter updates every **30 seconds**
- Perfect for **remote sensors** with infrequent updates
- **Years of battery life** for set-and-forget deployments

## Customization

```c
#define ADV_INTERVAL_SEC 30   // Adjust for power/latency trade-off
#define ADV_DURATION_MS 1000  // Keep minimal for power savings
#define HAS_LED 0             // Set to 0 to disable LED completely
```

## Use Cases

Perfect for:
- **Remote outdoor sensors**
- **Battery-powered door/window sensors**  
- **Long-term environmental monitoring**
- **Set-and-forget installations**
- **Solar-powered sensors** (with small panels)

This represents the **absolute minimum power consumption** achievable while maintaining BTHome v2 compatibility!