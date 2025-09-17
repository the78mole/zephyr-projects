# Zephyr RTOS Development Workspace

This repository contains a fully configured Zephyr RTOS development environment for the **Nordic nRF52840-DK** board.

## 🚀 Quick Start

### Prerequisites

- **Linux** (Ubuntu/Mint recommended)
- **Python 3.10+** (Python 3.12 tested)
- **Git** for repository management
- **USB connection** for nRF52840-DK board

### 1. Clone Repository

```bash
git clone https://github.com/the78mole/zephyr-projects.git
cd zephyr-projects
```

### 2. Setup Python Environment

```bash
# Install uv package manager (modern pip alternative)
curl -LsSf https://astral.sh/uv/install.sh | sh
source ~/.bashrc

# Install West tool
uv tool install west
```

### 3. Initialize Zephyr Workspace

```bash
# Initialize west workspace
west init -l .

# Download all modules and dependencies
west update

# Install Python dependencies
uv pip install --python ~/.local/share/uv/tools/west/bin/python -r zephyr/scripts/requirements.txt
```

### 4. Install Zephyr SDK

```bash
# Automatic SDK detection via GitHub API
LATEST_SDK=$(curl -s https://api.github.com/repos/zephyrproject-rtos/sdk-ng/releases/latest | grep '"tag_name":' | cut -d '"' -f 4)
echo "Latest SDK Version: $LATEST_SDK"

# Download and install SDK
cd ~/.local
wget -O zephyr-sdk.tar.xz "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/${LATEST_SDK}/zephyr-sdk-${LATEST_SDK#v}_linux-x86_64.tar.xz"
tar -xf zephyr-sdk.tar.xz
rm zephyr-sdk.tar.xz

# Run SDK setup
cd zephyr-sdk-*/
./setup.sh
```

### 5. Install Build Tools

```bash
# Required system packages
sudo apt update
sudo apt install -y build-essential python3-dev libusb-1.0-0-dev

# USB permissions for flash tools
sudo usermod -a -G dialout $USER

# IMPORTANT: For immediate activation in current shell
newgrp dialout

# Install PyOCD flash tool (alternative to nrfutil)
uv tool install pyocd
```

## 📁 Project Structure

```
zephyr/
├── README.md                    # This file
├── zephyr/                      # Zephyr RTOS core
├── modules/                     # External modules
├── bootloader/                  # MCUboot bootloader
├── tools/                       # Build tools
├── my_projects/                 # Your own projects
│   └── hello_world/            # LED blink example project
└── docs/
    └── ZEPHYR_PROJECT_SETUP_GUIDE.md  # Detailed setup guide
```

## 🔨 Build & Flash

### Compile LED Blink Project

```bash
cd my_projects/hello_world

# Compile for nRF52840-DK board
west build -b nrf52840dk/nrf52840
```

**Build Success:**
```
Memory region         Used Size  Region Size  %age Used
           FLASH:       20456 B         1 MB      1.95%
             RAM:        4480 B       256 KB      1.71%
```

### Flash to Board

```bash
# Connect board via USB, then flash
west flash --runner openocd
```

**For locked board (APPROTECT):**
```bash
# Unlock board (required once)
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
# With PyOCD (modern alternative)
west flash --runner pyocd

# With J-Link (if J-Link tools installed)
west flash --runner jlink
```

## ✅ Test Functionality

After successful flash:

- **LED1** (green) on nRF52840-DK board should **blink every 1 second**
- **Serial output** "LED ON"/"LED OFF" available via debug interface

## 🆕 Create New Projects

```bash
# Create new project
mkdir my_projects/my_project
cd my_projects/my_project

# Create CMakeLists.txt and prj.conf
# (see hello_world as template)

# Compile and flash
west build -b nrf52840dk/nrf52840
west flash --runner openocd
```

## 🔧 Troubleshooting

### Common Issues

1. **"No J-Link device found"**
   - Connect board via USB
   - Check USB cable
   - `lsusb | grep SEGGER` should show J-Link

2. **"APPROTECT activated"**
   - Unlock board with `nrf52_recover` (see above)

3. **Build errors**
   - Check Python dependencies: `west update`
   - Validate SDK installation

4. **Permission denied (USB)**
   - Add user to dialout group
   - Restart session: `sudo usermod -a -G dialout $USER`

## 📚 Additional Information

- **Detailed setup guide:** `docs/ZEPHYR_PROJECT_SETUP_GUIDE.md`
- **Zephyr Documentation:** https://docs.zephyrproject.org/
- **nRF52840-DK Board Guide:** https://docs.zephyrproject.org/latest/boards/nordic/nrf52840dk/doc/index.html

---

**Board:** Nordic nRF52840-DK  
**Zephyr Version:** Latest Main Branch (4.2.99)  
**Status:** ✅ LED blink project successfully tested  
**Last Updated:** September 2025