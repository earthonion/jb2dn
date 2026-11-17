# jb_deeznutz — ESP32-S3-Zero HID Auto-Navigator
# Waveshare ESP32-S3-Zero — USB HID keyboard for PS5 exploit navigation
#
# Usage:
#   make build          — compile
#   make flash          — compile + upload
#   make monitor        — serial monitor (115200 baud)
#   make flash monitor  — compile + upload + monitor
#   make log            — connect to TCP log server (port 9999)
#   make clean          — remove build artifacts
#
# First time: plug in ESP32-S3-Zero holding BOOT button, then 'make flash'
# After first flash: auto-resets into bootloader, no button needed

SKETCH    := jb_deeznutz.ino
FQBN      := esp32:esp32:waveshare_esp32_s3_zero
BAUD      := 921600
MON_BAUD  := 115200
BUILD_DIR := build

# USB mode: USB-OTG for HID keyboard
BOARD_OPTS := USBMode=hwcdc,CDCOnBoot=cdc,MSCOnBoot=default,DFUOnBoot=default,UploadMode=cdc,PSRAM=disabled

# Auto-detect port: try ttyACM* first (native USB), then ttyUSB* (UART)
PORT := $(shell ls /dev/ttyACM0 2>/dev/null || ls /dev/ttyUSB0 2>/dev/null || echo "/dev/ttyACM0")

# ESP32-S3 IP on WiFi (for TCP log)
ESP_IP := 192.168.0.123

.PHONY: build flash monitor log clean ports help

help:
	@echo "ESP32-S3-Zero HID Auto-Navigator"
	@echo ""
	@echo "  make build     — compile only"
	@echo "  make flash     — compile + upload (PORT=$(PORT))"
	@echo "  make monitor   — serial monitor"
	@echo "  make log       — TCP log ($(ESP_IP):9999)"
	@echo "  make clean     — remove build dir"
	@echo "  make ports     — list available serial ports"
	@echo ""
	@echo "Override port:  make flash PORT=/dev/ttyACM1"

build:
	arduino-cli compile \
		--fqbn "$(FQBN)" \
		--board-options "$(BOARD_OPTS)" \
		--build-path "$(BUILD_DIR)" \
		.

flash: build
	@echo "Flashing to $(PORT)..."
	arduino-cli upload \
		--fqbn "$(FQBN)" \
		--port "$(PORT)" \
		--input-dir "$(BUILD_DIR)" \
		.

monitor:
	arduino-cli monitor --port "$(PORT)" --config baudrate=$(MON_BAUD)

log:
	@echo "Connecting to ESP32 log server at $(ESP_IP):9999..."
	@nc $(ESP_IP) 9999

ports:
	@echo "Serial ports:"
	@ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  (none found — plug in ESP32-S3)"
	@echo ""
	@arduino-cli board list 2>/dev/null || true

clean:
	rm -rf "$(BUILD_DIR)"
