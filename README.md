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

## Build and flash

Install `arduino-cli` and the ESP32 board package, then run:

```sh
make build
make flash
```

For the first flash, connect the ESP32-S3-Zero while holding its **BOOT** button. Later flashes should reset into the bootloader automatically.

Use `make monitor` for the serial monitor and `make log` to connect to the ESP32 TCP log server.
