# Zephyr Project Setup Guide for nRF52840-DK

This guide describes setting up a Zephyr RTOS development workspace for the Nordic nRF52840-DK board.

## Prerequisites

- Linux system (tested on Ubuntu/Debian)
- Git installed
- Python 3.8+ installed
- uv for Python package management
- West (Zephyr's meta-tool) installed

## Setup Steps

### 0. Install Linux Packages

```bash
sudo apt update
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

### 1. West Installation

West was already installed with `uv`:
```bash
uv tool install west
```

### 2. Zephyr Workspace Initialization

```bash
# Navigate to desired workspace directory
cd /home/daniel/GIT/zephyr

# Initialize Zephyr workspace
west init -m https://github.com/zephyrproject-rtos/zephyr

# Download all Zephyr modules and dependencies
west update
```

**Status:** ✅ Completed
- Workspace successfully initialized
- All modules downloaded
- Directory structure created:
  - `.west/` - West configuration
  - `zephyr/` - Main repository
  - `modules/` - Additional modules
  - `bootloader/` - MCUboot bootloader
  - `tools/` - Development tools

### 3. Install Python Dependencies

Using `uv` for Python package management:

```bash
# Install Python dependencies with uv
uv pip install -r zephyr/scripts/requirements.txt
```

**Status:** ✅ Completed

### 4. Zephyr SDK Installation

The official Zephyr SDK can be installed in various ways:

**Option A: West SDK Install (recommended, but requires patoolib)**
```bash
# Install missing dependency
uv pip install patoolib  # If not available, see Option B

# Install Zephyr SDK automatically
west sdk install
```

**Option B: Manual Installation (Alternative)**
```bash
# Install ARM toolchain from Ubuntu repository
sudo apt update
sudo apt install -y gcc-arm-none-eabi

# Set environment variable for alternative toolchain
export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
export CROSS_COMPILE=/usr/bin/arm-none-eabi-
```

**Option C: Manual SDK Download (Recommended - Automatic Version)**
```bash
# Create SDK directory
mkdir -p ~/.local/zephyr-sdk
cd ~/.local/zephyr-sdk

# Automatically determine latest SDK version via GitHub API
LATEST_SDK_VERSION=$(curl -s https://api.github.com/repos/zephyrproject-rtos/sdk-ng/releases/latest | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
echo "Latest SDK Version: $LATEST_SDK_VERSION"

# Remove 'v' prefix for download URL
SDK_VERSION=${LATEST_SDK_VERSION#v}

# Lade aktuelle SDK-Version herunter (ca. 1.5GB)
wget "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/${LATEST_SDK_VERSION}/zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"

# Extrahiere und installiere
tar xvf "zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"
cd "zephyr-sdk-${SDK_VERSION}"
./setup.sh

# Setze Umgebungsvariablen (optional zu ~/.bashrc hinzufügen)
export ZEPHYR_SDK_INSTALL_DIR=~/.local/zephyr-sdk/zephyr-sdk-${SDK_VERSION}
```

**Nach der Installation: SDK Setup finalisieren**
```bash
# Wechsle ins SDK-Verzeichnis
cd ~/.local/zephyr-sdk/zephyr-sdk-${SDK_VERSION}

# Führe Setup-Skript aus (installiert udev-Regeln und registriert SDK)
./setup.sh

# Optional: Setze permanente Umgebungsvariablen
echo 'export ZEPHYR_SDK_INSTALL_DIR=~/.local/zephyr-sdk/zephyr-sdk-0.17.4' >> ~/.bashrc
echo 'export PATH="$ZEPHYR_SDK_INSTALL_DIR/sysroots/x86_64-pokysdk-linux/usr/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

**Verifizierung der Installation**
```bash
# Teste ob SDK gefunden wird
cd /path/to/your/zephyr/workspace
west build -b nrf52840dk/nrf52840 samples/hello_world
```

**Alternative: Manuelle Version (falls API nicht verfügbar)**
```bash
# Fallback für spezifische Version (Stand September 2025: v0.17.4)
SDK_VERSION="0.17.4"
wget "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${SDK_VERSION}/zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"
tar xvf "zephyr-sdk-${SDK_VERSION}_linux-x86_64.tar.xz"
cd "zephyr-sdk-${SDK_VERSION}"
./setup.sh
```

**Status:** ✅ SDK 0.17.4 erfolgreich installiert - API-basierte Version funktional

### 5. Zusätzliche Python-Abhängigkeiten für Build

Nach der SDK-Installation können noch zusätzliche Python-Module benötigt werden:

```bash
# Installiere pyelftools in der West Python-Umgebung (falls Build-Fehler auftreten)
uv pip install --python /home/daniel/.local/share/uv/tools/west/bin/python pyelftools
```

### 6. Erstes Hello World Projekt erstellen

```bash
# Erstelle ein neues Projekt-Verzeichnis
mkdir -p my_projects/hello_world
cd my_projects/hello_world

# Kopiere das Hello World Beispiel
cp -r ../../zephyr/samples/hello_world/* .

# Baue das Projekt für nRF52840-DK
west build -b nrf52840dk/nrf52840
```

**Status:** ✅ Projekt erstellt und erfolgreich kompiliert

### 7. Build-System testen

```bash
# Teste den Build-Prozess
west build -b nrf52840dk/nrf52840

# Flashe auf das Board (nRF52840-DK per USB verbunden)
west flash --runner openocd

# Optional: Serielle Verbindung für Debug-Ausgabe
west debug
```

**Status:** ✅ Build successful - LED blink project for nRF52840-DK compiled and flashed

## ✅ Setup Success

The Zephyr workspace setup is **fully completed**:

### What works:

- ✅ **Zephyr SDK 0.17.4** automatically detected and installed via GitHub API
- ✅ **West workspace** initialized with all modules
- ✅ **Python dependencies** installed with uv
- ✅ **LED blink project** successfully compiled and flashed for nRF52840-DK
- ✅ **PyOCD installed** as alternative flash solution
- ✅ **OpenOCD flash system** functional via Zephyr SDK
- ✅ **Board unlock** successfully performed

### Build and Flash Success:

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:       20456 B         1 MB      1.95%
             RAM:        4480 B       256 KB      1.71%

Flash result: 20,456 bytes successfully transferred (22.610 KiB/s)
Board: nRF52840-xxAA detected (1024kB Flash, 256kB RAM)
```

### LED1 Blink Functionality:

- **LED1 at P0.13** blinks every 1 second
- **GPIO control** via modern Zephyr Device Tree API
- **Debug output** available via serial interface

## Flash Troubleshooting

### Unlock board (if APPROTECT activated):

The nRF52840-DK can be factory-locked. To unlock:

```bash
# Unlock board with nrf52_recover
$HOME/.local/zephyr-sdk/zephyr-sdk-0.17.4/sysroots/x86_64-pokysdk-linux/usr/bin/openocd \
  -s $HOME/.local/zephyr-sdk/zephyr-sdk-0.17.4/sysroots/x86_64-pokysdk-linux/usr/share/openocd/scripts \
  -c 'source [find interface/jlink.cfg]' \
  -c 'transport select swd' \
  -c 'source [find target/nrf52.cfg]' \
  -c 'init' \
  -c 'nrf52_recover' \
  -c 'shutdown'

# Then flash normally
west flash --runner openocd
```

### Alternative Flash-Runner:

```bash
# Mit PyOCD (moderne Alternative)
west flash --runner pyocd

# Mit J-Link (wenn J-Link-Tools installiert)
west flash --runner jlink

# Standard Nordic-Tools (wenn nrfutil verfügbar)
west flash --runner nrfutil
```

## LED Blink Project (my_projects/hello_world)

### Implemented Features

The example project demonstrates:

- **GPIO configuration** via Device Tree (`led0` alias)
- **LED control** at P0.13 (LED1 of nRF52840-DK)  
- **Timer-based blinking** with `k_msleep(1000)`
- **Error handling** for GPIO operations
- **Debug output** via serial interface

### Project Structure

```
my_projects/hello_world/
├── CMakeLists.txt          # Build configuration
├── prj.conf               # Project configuration (GPIO enabled)
├── src/
│   └── main.c             # LED blink implementation
└── build/                 # Build artifacts
    └── zephyr/
        ├── zephyr.elf     # Executable file
        └── zephyr.hex     # Flash image
```

### Code Highlights

```c
// LED1 via Device Tree alias
#define LED1_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

// Configure GPIO as output
gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

// LED blink loop
while (1) {
    gpio_pin_set_dt(&led, 1);  // LED ON
    k_msleep(1000);
    gpio_pin_set_dt(&led, 0);  // LED OFF  
    k_msleep(1000);
}
```

## Current Challenges - SOLVED ✅

### ~~Toolchain Problem~~ → **SOLVED**
- ✅ Zephyr SDK 0.17.4 successfully installed and configured
- ✅ ARM-Toolchain für nRF52840 funktionsfähig
- ✅ Build-System vollständig einsatzbereit

### Flash-Herausforderungen → **GELÖST**
- ✅ Board-Entsperrung (APPROTECT) erfolgreich durchgeführt
- ✅ OpenOCD Flash-System funktioniert
- ✅ PyOCD als Alternative installiert
- ✅ LED-Blink-Programm erfolgreich geflasht

## Wichtige Verzeichnisse

- `zephyr/` - Zephyr RTOS Quellcode
- `zephyr/samples/` - Beispielprojekte
- `zephyr/boards/` - Board-Definitionen
- `modules/` - Externe Module und Bibliotheken
- `bootloader/` - MCUboot Bootloader
- `tools/` - Build-Tools und Utilities

## Board-spezifische Informationen: nRF52840-DK

- **Board-Identifier:** `nrf52840dk/nrf52840`
- **MCU:** Nordic nRF52840 (ARM Cortex-M4)
- **Flash:** 1 MB
- **RAM:** 256 KB
- **Debugging:** J-Link/SWD Interface

## Nützliche West-Befehle

```bash
# Zeige Workspace-Status
west status

# Update alle Repositories
west update

# Baue Projekt
west build -b <board_name>

# Flashe auf Board
west flash

# Debug-Session starten
west debug

# Konfiguration anzeigen
west config

# Hilfe anzeigen
west --help
```

## Umgebungsvariablen

Für optimale Entwicklung sollten folgende Umgebungsvariablen gesetzt werden:

```bash
# Füge zu ~/.bashrc oder ~/.zshrc hinzu
export ZEPHYR_BASE=/home/daniel/GIT/zephyr/zephyr
export GNUARMEMB_TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi
```

## Troubleshooting

### Häufige Probleme

1. **Build-Fehler:** Überprüfe ob alle Python-Abhängigkeiten installiert sind
2. **Board nicht gefunden:** Stelle sicher, dass der Board-Name korrekt ist
3. **Flash-Fehler:** Überprüfe USB-Verbindung und Berechtigungen

### Support-Ressourcen

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [Nordic nRF52840-DK Guide](https://docs.zephyrproject.org/latest/boards/nordic/nrf52840dk/doc/index.html)
- [Zephyr GitHub](https://github.com/zephyrproject-rtos/zephyr)

## 🎯 Summary

### Successfully Implemented

✅ **Complete Zephyr development environment** for nRF52840-DK
✅ **Modern package management** with uv instead of pip  
✅ **API-based SDK installation** with automatic version detection
✅ **LED blink project** successfully compiled and flashed
✅ **Multiple flash options** (OpenOCD, PyOCD) configured
✅ **Board unlock** documented and tested

### Next Development Steps

1. **Advanced GPIO projects** - Multiple LEDs, buttons, PWM
2. **Bluetooth Low Energy** - When nRF Command Line Tools installed  
3. **Integrate sensors** - Use I2C/SPI peripherals
4. **Customize Device Tree** - Custom board configurations
5. **Multi-Threading** - Use Zephyr RTOS features

### Support Resources

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [Nordic nRF52840-DK Guide](https://docs.zephyrproject.org/latest/boards/nordic/nrf52840dk/doc/index.html)
- [Zephyr GitHub](https://github.com/zephyrproject-rtos/zephyr)

---

*Last Updated: September 2025*
*Board: Nordic nRF52840-DK with LED1 blink functional*
*Zephyr Version: Latest Main Branch (4.2.99)*