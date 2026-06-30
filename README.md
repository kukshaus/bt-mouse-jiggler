# Logi-Mouse

BLE mouse jiggler for ESP32-S3. Keeps your computer awake by moving the cursor a minimal amount at configurable intervals.

## Features

- **BLE HID Mouse** – pairs as a Bluetooth mouse
- **Stealth name** – advertises as a real, on-market Logitech model (manufacturer "Logitech"), chosen from a built-in list
- **Random jiggler** – moves the cursor at a random interval (default 1–2 min) so the timing isn't perfectly periodic
- **Schedule** – optional start/end time window during which jiggling is active (e.g. 09:00–17:00)
- **LED status** – shows connection state
- **Serial menu** – configure via USB (115200 baud)
- **Demo mode** – mouse draws a circle (100px, 3s)
- **Persistent settings** – model, interval range, distance and schedule saved via Preferences

### Time / schedule note

The ESP32 has no RTC or network time here, so the schedule needs a time reference: set
the current time once via the serial menu (**5 – Set current time**). The firmware then
tracks the time of day with `millis()`. After a power-cycle the clock must be set again
(if it isn't set, the schedule is ignored and the jiggler stays always-on).

## Hardware

- MCU: ESP32-S3
- Bluetooth: BLE HID
- Power: 5V via USB

## Quick start

1. Flash the `.ino` to an ESP32-S3 (see **Build & flash** below)
2. Power the ESP32 via USB
3. Connect to **MX Master 3S** (the default Logitech name) via Bluetooth
4. Done – cursor movement starts automatically (random 1–2 min interval)

## Build & flash

> The T-vK `BleMouse` library does **not** compile against ESP32 core 3.x.
> Use an ESP32 core in the **2.0.x** line (tested with `esp32:esp32@2.0.17`).

### Arduino IDE
1. **Boards Manager** → install **esp32 by Espressif**, then select version **2.0.17**
2. Install the library: download [T-vK/ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse)
   → *Sketch → Include Library → Add .ZIP Library*
3. Board: **ESP32S3 Dev Module**, select the serial port, click **Upload**

### arduino-cli
```bash
# one-time setup
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli config set library.enable_unsafe_install true
arduino-cli lib install --git-url https://github.com/T-vK/ESP32-BLE-Mouse.git

# compile + upload (the sketch folder must match the .ino name, so use a temp folder)
mkdir -p /tmp/bt-mouse-jiggler
cp bt-mouse.ino /tmp/bt-mouse-jiggler/bt-mouse-jiggler.ino
arduino-cli compile -u -p <YOUR_PORT> --fqbn esp32:esp32:esp32s3 /tmp/bt-mouse-jiggler
```
Find `<YOUR_PORT>` with `arduino-cli board list` (e.g. `/dev/cu.usbmodem...` on macOS,
`/dev/ttyACM0` on Linux, `COM3` on Windows).

## Testing

1. **Open the serial menu** at 115200 baud and press Enter:
   ```bash
   arduino-cli monitor -p <YOUR_PORT> -c baudrate=115200
   ```
   Type `1` to show current settings.
2. **Pair it**: on your computer's Bluetooth settings, connect to **MX Master 3S**
   (or whichever Logitech model you picked). Pick a different model with menu `2`.
3. **Verify jiggling**: with the device connected, the cursor nudges every 1–2 minutes.
   Press `8` to jiggle immediately, or `9` to run the circle demo.
4. **Test the schedule**: set the clock with `5` (e.g. your current `HH:MM`), then `6`
   to enable a window. Show settings with `1` – it reports `aktiv` (active) inside the
   window and `pausiert` (paused) outside it.

## Configuration

Connect via serial terminal (115200 baud):

```
1 – Show settings
2 – Choose device model (Logitech list)
3 – Change interval range (random, seconds)
4 – Change distance (pixels)
5 – Set current time (HH:MM)
6 – Configure schedule (enable + start/end HH:MM)
7 – Reset to defaults
8 – Jiggle manually
9 – Demo: draw a circle
h – Help
```

Changing the model or resetting reboots the ESP32 so the BLE stack re-initializes
cleanly under the new name.

## Dependencies

- [BleMouse](https://github.com/T-vK/ESP32-BLE-Mouse) – BLE HID mouse implementation

## License

MIT