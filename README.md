# ESP Light

ESP32 firmware for a WS2812 light/strip with a built-in setup access point, web-based setup, MQTT, and Home Assistant auto-discovery.

## Features

- Single firmware with Wi-Fi AP provisioning and STA web UI
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

## OTA Update

The firmware uses standard `ArduinoOTA` / `espota` over both the STA connection and the provisioning AP. After the first wired flash, upload new builds over Wi-Fi:

### esp32dev

```sh
make BOARD_HOST=myrtio-light-xxyy.local esp32dev-ota
```

### lolin s2 mini

```sh
make BOARD_HOST=myrtio-light-xxyy.local s2-mini-ota
```

`BOARD_HOST` may be either the device IP address or the mDNS host name. The OTA host name format is `myrtio-light-xxyy.local`.
While the provisioning AP is enabled, OTA is also available at `192.168.4.1`.
After switching to this partition layout, the first installation should be done over USB serial.

## First Setup

1. Flash the device.
2. On first boot, the device starts the setup AP if Wi-Fi is not configured.
3. Connect to the AP named `MyrtIO Светильник XXYY`.
4. Open `http://192.168.4.1/` or the IP printed in the serial log.
5. Fill in the Wi-Fi `SSID` and password, MQTT host/port/credentials, and the LED strip settings: `led_count`, `skip_leds`, `color_order`, `brightness_min`, `brightness_max`, `color_correction`.
6. Save the configuration. The device will apply the settings in the same firmware and connect over STA.

Hold the button on `GPIO0` for about 3 seconds to enable or disable the setup AP on a configured device.

## Wi-Fi And MQTT

- Wi-Fi credentials are stored in NVS and used on every boot
- The web setup remains available over the STA IP address when the device is connected to Wi-Fi
- OTA updates are available over `espota` over both STA and the provisioning AP
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
