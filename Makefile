BOARD_TTY = /dev/cu.wchusbserial58B90790221
BAUD_RATE = 115200

ESPTOOL = pio pkg exec -- esptool.py

BOOTLOADER_BIN = .pio/build/factory/bootloader.bin
PARTITIONS_BIN = .pio/build/factory/partitions.bin
BOOT_APP_BIN   = $(HOME)/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
FACTORY_BIN    = .pio/build/factory/firmware.bin
APP_BIN        = .pio/build/app/firmware.bin

.PHONY: build
build: build-app build-factory

.PHONY: build-app
build-app:
	@pio run -e app

.PHONY: build-factory
build-factory: factory-page
	@pio run -e factory

.PHONY: factory-page
factory-page:
	@cd src/factory/page; bun run build
	@python3 scripts/bin2source.py \
	    src/factory/page/dist/index.html.gz \
		src/factory/page \
		factory_page

.PHONY: flash
flash: build
	@$(ESPTOOL) --chip esp32 --port $(BOARD_TTY) --baud 460800 \
		--before default_reset --after hard_reset \
		write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
		0x1000   $(BOOTLOADER_BIN) \
		0x8000   $(PARTITIONS_BIN) \
		0xD000   $(BOOT_APP_BIN) \
		0x10000  $(FACTORY_BIN) \
		0x110000 $(APP_BIN)

.PHONY: flash-factory
flash-factory:
	@pio run -t upload --upload-port $(BOARD_TTY) -e factory

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
