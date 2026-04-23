BOARD_TTY = /dev/cu.wchusbserial58B90790221
BAUD_RATE = 115200

ESPTOOL = pio pkg exec -- esptool.py

ESP32_MERGED_BIN     = .pio/build/esp_light.bin
BOOT_APP_BIN   = $(HOME)/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin

define build_firmware
	make factory-page
	pio run -e $(1)-app
	pio run -e $(1)-factory
	$(ESPTOOL) --chip $(2) \
		merge_bin \
		--output .pio/build/$(1)-firmware.bin \
		0x1000   .pio/build/$(1)-factory/bootloader.bin \
		0x8000   .pio/build/$(1)-factory/partitions.bin \
		0xD000   $(BOOT_APP_BIN) \
		0x10000  .pio/build/$(1)-factory/firmware.bin \
		0x110000 .pio/build/$(1)-app/firmware.bin
endef

define flash_firmware
	$(ESPTOOL) --chip $(2) --baud $(BAUD_RATE) \
		--before default_reset \
		--after hard_reset \
		write_flash -z \
		--flash_mode dio \
		--flash_freq 40m \
		--flash_size 4MB \
		0x0 .pio/build/$(1)-firmware.bin
endef

compiledb:
	@pio run -t compiledb

.PHONY: monitor
monitor:
	@pio device monitor --no-reconnect --port $(BOARD_TTY) --baud $(BAUD_RATE)

.PHONY: format
format:
	@astyle --project=.astylerc -n \
		-r 'src/*.cc' \
		-r 'src/*.h' \
		-r 'lib/*.cc' \
		-r 'lib/*.h' \
		-r 'include/*.h'

.PHONY: factory-page
factory-page:
	@cd src/factory/page; bun run build
	@python3 scripts/bin2source.py \
	    src/factory/page/dist/index.html.gz \
		src/factory/page \
		factory_page

.PHONY: esp32dev-firmware
esp32dev-firmware:
	$(call build_firmware,esp32dev,esp32)

esp32dev-flash: esp32dev-firmware
	$(call flash_firmware,esp32dev,esp32)

.PHONY: build-s2-mini
s2-mini-firmware:
	$(call build_firmware,s2-mini,esp32s2)

.PHONY: flash-s2-mini
s2-mini-flash: s2-mini-firmware
	$(call flash_firmware,s2-mini,esp32s2)
