# LED Blink Project for nRF52840-DK

A Zephyr RTOS sample that demonstrates GPIO control by blinking LED1 on the Nordic nRF52840-DK board.

## Overview

This project is based on the Zephyr "Hello World" sample but extends it to:

- **Blink LED1** at P0.13 on the nRF52840-DK board
- **GPIO configuration** using modern Zephyr Device Tree API
- **Timer-based control** with 1-second intervals
- **Error handling** for GPIO operations
- **Debug output** via serial console

## Hardware Requirements

- **Nordic nRF52840-DK** development board
- **USB cable** for programming and power
- **LED1** (built-in green LED at P0.13)

## Building and Running

### Prerequisites

Ensure you have the Zephyr development environment set up (see main [README.md](../../README.md)).

### Build

```bash
cd my_projects/hello_world

# Build for nRF52840-DK
west build -b nrf52840dk/nrf52840
```

### Flash

```bash
# Connect nRF52840-DK via USB, then flash
west flash --runner openocd
```

**If board is locked (APPROTECT):**

```bash
# Unlock board (one-time operation)
~/.local/zephyr-sdk/zephyr-sdk-*/sysroots/x86_64-pokysdk-linux/usr/bin/openocd \
  -s ~/.local/zephyr-sdk/zephyr-sdk-*/sysroots/x86_64-pokysdk-linux/usr/share/openocd/scripts \
  -c 'source [find interface/jlink.cfg]' \
  -c 'transport select swd' \
  -c 'source [find target/nrf52.cfg]' \
  -c 'init' \
  -c 'nrf52_recover' \
  -c 'shutdown'

# Then flash normally
west flash --runner openocd
```

### Alternative Flash Methods

```bash
# With PyOCD
west flash --runner pyocd

# With J-Link (if available)
west flash --runner jlink
```

## Expected Behavior

After successful flashing:

1. **LED1** (green LED) on the nRF52840-DK board blinks every 1 second
2. **Serial output** shows "LED ON" and "LED OFF" messages
3. **Memory usage**: ~20KB Flash (1.95%), ~4KB RAM (1.71%)

## Sample Output

### Serial Console

```
LED Blink Demo für nRF52840-DK Board: nrf52840dk_nrf52840
LED1 an P0.13 wird blinken...
LED ON
LED OFF
LED ON
LED OFF
...
```

### Build Output

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       20456 B         1 MB      1.95%
             RAM:        4480 B       256 KB      1.71%
```

## Code Structure

### Key Files

- **`src/main.c`** - Main application with LED blink logic
- **`prj.conf`** - Project configuration (GPIO driver enabled)
- **`CMakeLists.txt`** - Build configuration

### GPIO Configuration

```c
// LED1 via Device Tree alias
#define LED1_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

// Configure as output
gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
```

### Blink Loop

```c
while (1) {
    gpio_pin_set_dt(&led, 1);  // LED ON
    k_msleep(1000);            // Wait 1 second
    gpio_pin_set_dt(&led, 0);  // LED OFF
    k_msleep(1000);            // Wait 1 second
}
```

## Troubleshooting

### Common Issues

1. **"No J-Link device found"**
   - Ensure nRF52840-DK is connected via USB
   - Check that `lsusb | grep SEGGER` shows the device

2. **"APPROTECT activated"**
   - Run the unlock command shown above

3. **LED not blinking**
   - Verify the board is properly flashed
   - Check serial output for error messages

4. **Build errors**
   - Ensure Zephyr SDK is properly installed
   - Check that GPIO driver is enabled in `prj.conf`

## Related Documentation

- [Main Project README](../../README.md) - Full setup guide
- [Zephyr GPIO API](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [nRF52840-DK Board Documentation](https://docs.zephyrproject.org/latest/boards/nordic/nrf52840dk/doc/index.html)

## License

This project follows the Zephyr Project licensing (Apache 2.0).