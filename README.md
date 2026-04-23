# ESP Light

ESP32 firmware for a WS2812 light/strip with a built-in factory access point, web-based setup, MQTT, and Home Assistant auto-discovery.

## Features

- Factory mode with Wi-Fi AP and web UI
- Runtime control over MQTT
- Home Assistant MQTT discovery
- Persistent Wi-Fi, MQTT, and light settings in NVS

### WS2812 Wiring

#### esp32dev

- `ESP32 GPIO25` -> `WS2812 DIN`
- `ESP32 GND` -> `WS2812 GND`
- External `5V` PSU -> `WS2812 5V`
- Keep ESP32 and LED strip grounds common
- For stable operation, a level shifter and a `330-470 Ohm` series resistor on the data line are recommended

Do not power longer LED strips from the ESP32 board directly.

#### lolin s2 mini

Same as `esp32dev`, but `WS2812 DIN` should be connected to `GPIO 18`.

## Prerequisites

- `PlatformIO` CLI
- `bun`
- `python3`
- USB serial access to the ESP32 board

## Build and Flash

The default serial port is hardcoded in [`Makefile`](./Makefile). In most cases, override it from the command line:

### esp32dev

```sh
make esp32dev-firmware
make BOARD_TTY=/dev/ttyUSB0 esp32dev-flash
```

### lolin s2 mini

```sh
make s2-mini-firmware
make BOARD_TTY=/dev/ttyUSB0 s2-mini-flash
```

## First Setup

1. Flash the device.
2. On first boot, the device starts in factory mode if Wi-Fi is not configured.
3. Connect to the AP named `MyrtIO Светильник XXYY`.
4. Open `http://192.168.4.1/` or the IP printed in the serial log.
5. Fill in the Wi-Fi `SSID` and password, MQTT host/port/credentials, and the LED strip settings: `led_count`, `skip_leds`, `color_order`, `brightness_min`, `brightness_max`, `color_correction`.
6. Save the configuration and boot the app firmware.

Press the button on `GPIO0` to switch between factory mode and application mode.

## Wi-Fi And MQTT

- Wi-Fi credentials are stored in NVS and used on every boot
- MQTT default port is `1883`
- MQTT client ID and topic namespace are generated as `myrtio_light_XXYY`
- Home Assistant discovery is published automatically after MQTT connection

Main MQTT topics:

- State: `myrtio_light_XXYY/light`
- Command: `myrtio_light_XXYY/light/set`
- Availability: `myrtio_light_XXYY/availability`
- Discovery: `homeassistant/light/myrtio_light_XXYY_light/config`

## License

[MIT](./LICENSE)
