# Logi-Mouse

BLE mouse jiggler for ESP32-S3. Keeps your computer awake by moving the cursor a minimal amount at configurable intervals.

## Features

- **BLE HID Mouse** – pairs as a Bluetooth mouse
- **Jiggler** – moves the cursor every 3 minutes by 3 pixels (configurable)
- **LED status** – shows connection state
- **Serial menu** – configure via USB (115200 baud)
- **Demo mode** – mouse draws a circle (100px, 3s)
- **Persistent settings** – name, interval, distance saved via Preferences

## Hardware

- MCU: ESP32-S3
- Bluetooth: BLE HID
- Power: 5V via USB

## Quick start

1. Flash the `.ino` to an ESP32-S3 using Arduino IDE / PlatformIO
2. Power the ESP32 via USB
3. Connect to "Logi-Mouse" via Bluetooth
4. Done – cursor movement starts automatically

## Configuration

Connect via serial terminal (115200 baud):

```
1 – Show settings
2 – Change device name
3 – Change interval (seconds)
4 – Change distance (pixels)
5 – Reset to defaults
6 – Jiggle manually
7 – Demo: draw a circle
h – Help
```

## Dependencies

- [BleMouse](https://github.com/T-vK/ESP32-BLE-Mouse) – BLE HID mouse implementation

## License

MIT