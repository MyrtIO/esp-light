BOARD_TTY = /dev/cu.wchusbserial58B90790221
BAUD_RATE = 115200

.PHONY: build
build:
	@pio run

.PHONY: flash
flash:
	@pio run -t upload --upload-port $(BOARD_TTY) -e release

.PHONY: format
format:
	@astyle --project=.astylerc -n \
		-r 'src/*.cc' \
		-r 'src/*.h' \
		-r 'lib/*.cc' \
		-r 'lib/*.h' \
		-r 'include/*.h'
