SKETCH = scruffy.ino
BUILD_ROOT = build
UPLOAD_PORT ?= /dev/ttyUSB0
ESP_ROOT = ${HOME}/arduino/arduino-1.8.5/hardware/espressif/esp32
CHIP = esp32
BOARD = featheresp32
LIBS = $(ESP_LIBS)/WiFiClientSecure \
	$(ESP_LIBS)/WiFi \
	$(ESP_LIBS)/HTTPClient


.PHONY: monitor
monitor:
	miniterm $(UPLOAD_PORT) 115200

.PHONY: config
config: dep
	scripts/gen-config.sh > config.h

.PHONY: dep
dep:
	git clone https://github.com/plerup/makeEspArduino .makeEspArduino

-include .makeEspArduino/makeEspArduino.mk

