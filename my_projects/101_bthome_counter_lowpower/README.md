# BTHome Low-Power Counter Example

This project demonstrates a battery-optimized BTHome v2 counter implementation using the BTHome Zephyr module. It's designed for minimal power consumption between advertisements.

## Features

- **Low Power Design**: Deep sleep mode between advertisements
- **Extended Battery Life**: 10-second advertisement intervals (configurable)
- **Minimal Logging**: Reduced to WARNING level only
- **Optional LED**: Can be disabled for maximum power savings
- **Peripheral Power Management**: GPIO and other peripherals suspended during sleep
- **BTHome v2 Compatible**: Works with Home Assistant and other BTHome platforms

## Power Optimizations

### Configuration Optimizations (`prj.conf`)
- `CONFIG_PM=y`: Enable power management
- `CONFIG_PM_DEVICE=y`: Device-level power management
- `CONFIG_PM_S2RAM=y`: Suspend-to-RAM support
- `CONFIG_PRINTK=n`: Disable printf for power savings
- `CONFIG_CONSOLE=n`: Disable console
- `CONFIG_SERIAL=n`: Disable serial interfaces
- Reduced stack sizes and buffer sizes

### Software Optimizations
- **10-second advertisement interval** (vs 5 seconds in normal version)
- **Deep sleep** between advertisements using `k_sleep()`
- **Minimal logging** (WARNING level only)
- **GPIO power management** - peripherals suspended during sleep
- **Optional LED** - can be completely disabled

## Advertisement Schedule

1. **Wake up** from deep sleep
2. **Initialize** sensors and prepare data
3. **Advertise** BTHome data for 2 seconds
4. **Enter deep sleep** for ~7 seconds
5. **Repeat** every 10 seconds

## Battery Life Estimation

With proper power management on nRF52840:
- **Active time**: ~2-3 seconds per 10-second cycle (advertisement + processing)
- **Sleep current**: ~2-5 µA (depending on configuration)
- **Active current**: ~5-10 mA during advertisement
- **Estimated battery life**: Several weeks to months (depending on battery capacity)

## Configuration

### Advertisement Interval

```c
#define ADV_INTERVAL_SEC 10  /* Send every 10 seconds */
#define ADV_DURATION_MS 2000 /* Advertise for 2 seconds */
```

### LED Control
```c
#define HAS_LED 1  /* Set to 0 to disable LED completely */
```

## Building and Flashing

```bash
cd /path/to/zephyr
west build -p -b nrf52840dk/nrf52840 my_projects/101_bthome_counter_lowpower
west flash --runner pyocd
```

## Power Measurement

For accurate power measurements:
1. Remove J6 jumper on nRF52840-DK
2. Connect ammeter between pins on J6
3. Monitor current consumption during sleep and active phases

## Home Assistant Integration

The device appears as "BTHome LowPower" and sends the same BTHome v2 counter data as the regular version, just at 10-second intervals for balanced power consumption and responsiveness.

## Customization

- Adjust `ADV_INTERVAL_SEC` for different power/update rate trade-offs
- Disable LED by setting `HAS_LED 0` for maximum power savings
- Add additional sensors as needed
- Modify power management settings in `prj.conf`