#include <Arduino.h>
#include <config.h>
#include <lc.h>
#include <FastLED.h>
#include "persistent_data.h"
#include "light.h"
#include "wifi_sta.h"
#include "mqtt.h"
#include "ha_light.h"
#include "ota.h"

#define GPIO_BUTTON        0
#define BUTTON_DEBOUNCE_MS 50

static persistent_data_t pdata;
static light_saved_state_t saved_state;

static light_config_t light_cfg;
static app_wifi_config_t wifi_cfg;
static mqtt_config_t mqtt_cfg;

static const ota_config_t ota_cfg = {
	.hostname = CONFIG_DEVICE_NAME,
	.port     = CONFIG_OTA_PORT,
};

static bool btn_last_raw = HIGH;
static bool btn_stable = HIGH;
static unsigned long btn_changed_at = 0;

void setup() {
	Serial.begin(115200);

	pinMode(GPIO_BUTTON, INPUT_PULLUP);

	persistent_data_load(&pdata);
	if (!persistent_data_is_configured(&pdata)) {
		Serial.println("[Main] not configured, rebooting to factory...");
		boot_to_factory();
		return;
	}

	light_cfg = {
		.pin              = CONFIG_LIGHT_PIN_CTL,
		.led_count        = pdata.led_count,
#ifdef CONFIG_LIGHT_LED_SKIP
		.led_skip         = CONFIG_LIGHT_LED_SKIP,
#else
		.led_skip         = pdata.skip_leds,
#endif
		.color_correction = pdata.color_correction,
		.color_order      = pdata.color_order,
		.kelvin_warm      = CONFIG_LIGHT_COLOR_KELVIN_WARM,
		.kelvin_cold      = CONFIG_LIGHT_COLOR_KELVIN_COLD,
		.kelvin_initial   = CONFIG_LIGHT_COLOR_KELVIN_INITIAL,
		.transition_ms    = CONFIG_LIGHT_TRANSITION_COLOR,
		.brightness       = pdata.brightness_min,
		.brightness_max   = pdata.brightness_max,
	};

	wifi_cfg = {
		.ssid     = pdata.wifi_ssid,
		.password = pdata.wifi_password,
		.hostname = CONFIG_DEVICE_NAME,
		.reconnect_interval_ms = 15000,
	};

	mqtt_cfg = {
		.client_id   = CONFIG_DEVICE_NAME,
		.host        = pdata.mqtt_host,
		.port        = pdata.mqtt_port,
		.buffer_size = CONFIG_MQTT_BUFFER_SIZE,
	};

	light_init(&light_cfg);
	if (light_state_exists()) {
		light_state_load(&saved_state);
		light_restore_state(&saved_state);
	} else {
		lc_set_brightness(scale8_video(light_cfg.brightness, light_cfg.brightness_max));
		lc_set_power(true);
	}
	light_start();
	wifi_init(&wifi_cfg);
	mqtt_init(&mqtt_cfg);
	ha_light_init();
	ota_init(&ota_cfg);

	Serial.println("[Main] initialized");
}

void loop() {
	wifi_loop();
	mqtt_loop();
	ha_light_loop();
	ota_loop();

	/* GPIO0 button → boot to factory */
	unsigned long now = millis();
	bool btn_now = digitalRead(GPIO_BUTTON);
	if (btn_now != btn_last_raw) {
		btn_changed_at = now;
		btn_last_raw = btn_now;
	}
	if ((now - btn_changed_at) > BUTTON_DEBOUNCE_MS && btn_now != btn_stable) {
		btn_stable = btn_now;
		if (btn_stable == LOW) {
			Serial.println("[Main] button pressed, rebooting to factory...");
			boot_to_factory();
		}
	}
}
