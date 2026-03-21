BOARD_TTY = /dev/cu.wchusbserial58B90790221
BAUD_RATE = 115200

define ota
	python scripts/espota.py \
        -i $(1) \
        -p 3200 \
        -f .pio/build/release/firmware.bin
endef

define config
	python scripts/configgen.py \
		config/secrets.yaml \
		config/common.yaml \
		config/devices/$(1) \
		--output include/config.h
endef

.PHONY: build
build:
	@pio run

.PHONY: flash
flash:
@pio run -t upload --upload-port $(BOARD_TTY) -e release

.PHONY: bar-config
bar-config:
	@$(call config,bar.yaml)

.PHONY: bar-build
bar-build: bar-config
	@make build

.PHONY: bar-ota
bar-ota: bar-config
	@$(call ota,MyrtLightBar.lan)

.PHONY: bar-flash
bar-flash: bar-config
	@make flash

.PHONY: ceiling-config
ceiling-config:
	@$(call config,ceiling.yaml)

.PHONY: ceiling-build
ceiling-build: ceiling-config
	@make build

.PHONY: ceiling-ota
ceiling-ota: ceiling-config
	@$(call ota,MyrtLightCeiling.lan)

.PHONY: ceiling-flash
ceiling-flash: bar-config
	@make flash

.PHONY: curtain-config
curtain-config:
	@$(call config,curtain.yaml)

.PHONY: curtain-build
curtain-build: curtain-config
	@make build

.PHONY: curtain-ota
curtain-ota: curtain-config
	@$(call ota,MyrtLightCurtain.lan)

.PHONY: curtain-flash
curtain-flash: curtain-config
	@make flash

.PHONY: luminary-config
luminary-config:
	@$(call config,luminary.yaml)

.PHONY: luminary-build
luminary-build: luminary-config
	@make build

.PHONY: luminary-ota
luminary-ota: luminary-config
	@$(call ota,MyrtLightLuminary.lan)

.PHONY: luminary-flash
luminary-flash: luminary-config
	@make flash

.PHONY: configure
configure:
	@pio init --ide vscode
	@make build

.PHONY: format
format:
	@astyle --project=.astylerc -n \
		-r 'src/*.cc' \
		-r 'src/*.h' \
		-r 'lib/*.cc' \
		-r 'lib/*.h' \
		-r 'include/*.h'

.PHONY: cat-config
cat-config:
	@$(call config,cat.yaml)

.PHONY: cat-build
cat-build: cat-config
	@make build

.PHONY: cat-ota
cat-ota: cat-config
	@$(call ota,MyrtLightCat.lan)

.PHONY: cat-flash
cat-flash: cat-config
	@make flash
