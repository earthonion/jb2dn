# jb2dn

A Waveshare ESP32-S3-Zero USB HID auto-navigator for the PS5, with the Relapse direct-installer payload included.

## Included payload

- File: `relapse-direct-install.elf`
- Source: <http://45.56.67.85/payloads/payloads/relapse-direct-install.elf>
- SHA-256: `e8d9e6519d5d4de62ae03cc922f0f5d8b57b8b456c4c6d308be9b7749a1f6c59`

The payload C source, Makefiles, and `sce_sys` assets are included in `relapse-app/`. The bundled ELF is identical to the direct-install binary supplied with that source.

To rebuild the payload, set `PS5_PAYLOAD_SDK` to your PS5 payload SDK path and run:

```sh
cd relapse-app
make -f Makefile.direct
```

## Run the installer

1. Run `relapse-direct-install.elf` on the PS5 using your ELF loader, and wait for the installation to complete.
2. Open **Settings → Users and Accounts → Login Settings**.
3. Enable **Login Automatically**.

## Configure the ESP32 firmware

Open `jb2dn.ino` and replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` with your Wi-Fi credentials. Check that `PS5_IP` matches the PS5 and that the address passed to `WiFi.config()` is appropriate for your network.

## Customize key presses and delays

Edit `navigate_to_exploit()` in `jb2dn.ino` to change the navigation sequence. Add, remove, or reorder calls such as `press_up()`, `press_down()`, `press_left()`, `press_right()`, `press_enter()`, `press_esc()`, and `press_back()`.

All delays are in milliseconds (`1000` = 1 second). Pass a different value to a helper to change its delay for one call:

```cpp
press_down(300);    // Press Down, then wait 300 ms.
press_right(750);   // Press Right, then wait 750 ms.
press_enter(3500);  // Press Enter, then wait 3.5 seconds.
press_back();       // Use the default Backspace delay of 300 ms.
```

The HID keyboard helpers immediately above `navigate_to_exploit()` control the default timing. Change `post_delay = 300` to change a helper's default post-key delay. Change the second argument passed to `press_key()` to change how long that helper holds its key:

```cpp
void press_right(uint16_t post_delay = 500) {
    press_key(KEY_RIGHT_ARROW, 250);
    delay(post_delay);
}
```

This version holds Right for 250 ms and uses a default post-key delay of 500 ms. The shared `press_key()` function also waits 150 ms after releasing every key. For a key without a dedicated helper, call `press_key()` directly with a key constant and hold duration, such as `press_key(KEY_BACKSPACE, 100)`.

Run `make build` after your changes, then `make flash` to upload them to the ESP32.

## Build and flash

Install `arduino-cli` and the ESP32 board package, then run:

```sh
make build
make flash
```

For the first flash, connect the ESP32-S3-Zero while holding its **BOOT** button. Later flashes should reset into the bootloader automatically.

Use `make monitor` for the serial monitor and `make log` to connect to the ESP32 TCP log server.
