BOARD_TTY ?=
BOARD_HOST ?=
BAUD_RATE ?= 115200
OTA_PORT ?= 3232

ifneq ($(strip $(BOARD_TTY)),)
PIO_UPLOAD_PORT_ARG = --upload-port $(BOARD_TTY)
PIO_MONITOR_PORT_ARG = --port $(BOARD_TTY)
endif

PROVISIONING_PAGE_DIR = provisioning_page
ESPTOOL = pio pkg exec -- esptool.py
ESPOTA = python3 $(HOME)/.platformio/packages/framework-arduinoespressif32/tools/espota.py

define build_firmware
	make provisioning-page
	pio run -e $(1)
	cp .pio/build/$(1)/firmware.factory.bin .pio/build/$(1)-firmware.bin
endef

define upload_ota
	pio run -e $(1)
	$(ESPOTA) -i $(BOARD_HOST) -p $(OTA_PORT) -f .pio/build/$(1)/firmware.bin
endef

define flash_firmware
	make provisioning-page
	pio run -e $(1) -t upload $(PIO_UPLOAD_PORT_ARG)
endef

configure:
	@cd $(PROVISIONING_PAGE_DIR); bun install
	@pio run -t compiledb

.PHONY: monitor
monitor:
	@pio device monitor --no-reconnect $(PIO_MONITOR_PORT_ARG) --baud $(BAUD_RATE)

.PHONY: format
format:
	@find \
		lib/ \
		src/ \
		-iname '*.h' -o -iname '*.c' -o -iname '*.cpp' \
		| xargs clang-format -i

.PHONY: provisioning-page
provisioning-page:
	@cd $(PROVISIONING_PAGE_DIR); bun run build
	@python3 scripts/bin2source.py \
	    $(PROVISIONING_PAGE_DIR)/dist/index.html.gz \
		src/provisioning_page \
		provisioning_page

.PHONY: esp32dev-firmware
esp32dev-firmware:
	$(call build_firmware,esp32dev,esp32)

.PHONY: esp32dev-flash
esp32dev-flash: esp32dev-firmware
	$(call flash_firmware,esp32dev,esp32)

.PHONY: esp32dev-ota
esp32dev-ota:
	$(call upload_ota,esp32dev)

.PHONY: s2-mini-firmware
s2-mini-firmware:
	$(call build_firmware,s2-mini,esp32s2)

.PHONY: flash-s2-mini
s2-mini-flash: s2-mini-firmware
	$(call flash_firmware,s2-mini,esp32s2)

.PHONY: s2-mini-ota
s2-mini-ota:
	$(call upload_ota,s2-mini)
