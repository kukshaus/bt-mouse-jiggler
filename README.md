# Logi-Mouse

BLE mouse jiggler for ESP32-S3. Keeps your computer awake by moving the cursor a minimal amount at configurable intervals.

## Features

- **Plug & play** – powered by USB, auto-reconnects to a paired host and starts jiggling on its own
- **BLE HID Mouse** – pairs as a Bluetooth mouse
- **Stealth name** – advertises as a real, on-market Logitech model (manufacturer "Logitech"), chosen from a built-in list
- **Profiles** – pick a predefined config stack (model + interval + schedule) by number
- **Random jiggler** – moves the cursor at a random interval (default 1–2 min) so the timing isn't perfectly periodic
- **Schedule** – optional start/end time window during which jiggling is active (e.g. off after 17:30)
- **Button control** – onboard BOOT button (GPIO0): short click = next profile, long press = on/off
- **Profile feedback** – green LED blinks the active profile number (1–4×), also on every boot
- **Status LED** – onboard RGB LED, always solid (no blinking): 🟢 green = on, 🔴 red = off
- **Serial menu** – configure via USB (115200 baud)
- **Demo mode** – mouse draws a circle (100px, 3s)
- **Persistent settings** – model, interval range, distance, schedule and on/off state saved via Preferences

### Profiles

| # | Profile | Bluetooth name (Logitech model) | Interval | Schedule |
|---|---------|--------------------------------|----------|----------|
| 1 | Standard | MX Master 3S | 1–2 min | always on |
| 2 | Mein PC | MX Anywhere 3 | 22–119 s | on until **17:30** |
| 3 | Stealth schnell | G Pro X Superlight | 30–90 s | always on |
| 4 | Büro | M720 Triathlon | 45–90 s | 08:00–17:30 |

The "Bluetooth name" is what the host sees when pairing. Switching to a profile with a
different model changes the BLE name and triggers a clean reboot. You can also override the
model independently of the profile with serial menu `3`.

Apply a profile with serial menu `2`, or cycle through them with the BOOT button (see below).
Edit the `PROFILES[]` array in `bt-mouse.ino` to add your own (the first field after the
label is the index into the Logitech model list).

### Status LED & button

**RGB LED** (GPIO48) — the light is **always steady, never blinks** during normal use:
- 🟢 **bright green** – on, connected, jiggling (doing the job)
- 🟢 **dim green** – on, but still waiting for the Bluetooth connection
- 🔴 **red** – off (toggled off) **or** outside the schedule window (e.g. after 17:30)
- 🟢 **green blinks 1–4×** – a short signal of the active **profile number**, shown when you
  switch profiles and once on every boot (the only time the LED blinks)

**BOOT button** (GPIO0):
- **Short click** → switch to the next profile; the LED blinks the new profile's number
- **Long press (~0.7 s)** → toggle the jiggler on/off (same as serial menu `8`)

Switching to a profile that uses a different mouse model briefly reboots (to re-init BLE
cleanly under the new name); the profile number is blinked again right after the reboot.
Want dedicated buttons instead? Wire push-buttons from a free GPIO to GND and adapt the
button handling. The first ~1.5 s after boot the button is ignored, so the board's
auto-reset circuit can't trigger a false press.

### Time / schedule note

The ESP32 has no RTC or network time here, so the schedule needs a time reference: set
the current time once via the serial menu (**6 – Set current time**). The firmware then
tracks the time of day with `millis()`. As long as the board stays powered (e.g. from a
PC that keeps its USB alive), the clock — and the "off at 17:30" — stays correct all day.
If power is fully cut, set the time again after boot. If the clock isn't set, the schedule
is ignored and the jiggler stays always-on.

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
   (or whichever Logitech model you picked). Pick a different model with menu `3`.
3. **Pick a profile**: menu `2` → choose e.g. *Mein PC (22–119 s, off at 17:30)*.
4. **Verify jiggling**: with the device connected, the cursor nudges at the configured
   interval. Press `9` to jiggle immediately, or `0` to run the circle demo.
5. **Test the button / LED**:
   - **Short click** the onboard **BOOT** button → profile switches and the LED blinks
     the new profile's number (1–4×).
   - **Long press** (~0.7 s) → jiggler toggles; the RGB LED switches 🟢 green ⇄ 🔴 red
     and the cursor stops/starts.
6. **Test the schedule**: set the clock with `6` (your current `HH:MM`), then `7`
   to enable a window. Show settings with `1` – it reports `aktiv` (active) inside the
   window and `pausiert` (paused) outside it; the LED turns red outside the window.

## Configuration

Connect via serial terminal (115200 baud):

```
1 – Show settings (incl. green/red status)
2 – Choose profile
3 – Choose device model (Logitech list)
4 – Change interval range (random, seconds)
5 – Change distance (pixels)
6 – Set current time (HH:MM)
7 – Configure schedule (enable + start/end HH:MM)
8 – Toggle jiggler on/off (= BOOT button long press; short press = next profile)
9 – Jiggle manually
0 – Demo: draw a circle
r – Reset to defaults
h – Help
```

Changing the model, applying a profile with a different model, or resetting reboots the
ESP32 so the BLE stack re-initializes cleanly under the new name.

## Dependencies

- [BleMouse](https://github.com/T-vK/ESP32-BLE-Mouse) – BLE HID mouse implementation

## License

MIT