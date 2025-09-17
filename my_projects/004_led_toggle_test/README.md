# LED Toggle Test for nRF52840-DK

Ein einfaches Testprojekt zum Überprüfen der Grundfunktionen des nRF52840-DK:
- GPIO-Steuerung der LEDs
- Serielle Ausgabe über UART
- Basis-Logging-System

## Hardware

Das nRF52840-DK verfügt über 4 LEDs:
- LED1 (P0.13)
- LED2 (P0.14) 
- LED3 (P0.15)
- LED4 (P0.16)

## Funktionen

- Alle 4 LEDs blinken synchron alle 1 Sekunde
- Serielle Ausgabe zeigt den Status und Toggle-Counter
- Debug-Level Logging zeigt detaillierte GPIO-Zustände

## Kompilieren und Flashen

```bash
cd /home/daniel/GIT/zephyr/my_projects/led_toggle_test
west build -b nrf52840dk/nrf52840
west flash -r pyocd
```

## Serielle Ausgabe überwachen

```bash
# Eine der beiden seriellen Schnittstellen verwenden:
cat /dev/ttyACM0
# oder
cat /dev/ttyACM1
```

## Erwartete Ausgabe

```
🚀 LED Toggle Test for nRF52840-DK starting...
📋 Board: nrf52840dk_nrf52840
✅ All LED GPIO devices are ready
🔧 All LEDs configured as outputs
💡 Starting LED toggle sequence...
🔄 Toggle #1: LEDs ON 💡
🔄 Toggle #2: LEDs OFF ⚫
...
```