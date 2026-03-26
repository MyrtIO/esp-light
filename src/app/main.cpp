#include <Arduino.h>
#include <config.h>
#include <light_composer.h>
#include <button.h>
#include "persistent_data.h"
#include "light.h"
#include "device_id.h"
#include "wifi_sta.h"
#include "mqtt.h"
#include "ha_light.h"

#define GPIO_BUTTON 0

static persistent_data_t pdata;
static light_saved_state_t saved_state;

static light_config_t light_cfg;
static app_wifi_config_t wifi_cfg;
static mqtt_config_t mqtt_cfg;

static button_t btn = BUTTON_INIT(50);

void setup() {
	Serial.begin(115200);
	device_id_init();

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
		.led_skip         = pdata.skip_leds,
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
		.hostname = device_id(),
		.reconnect_interval_ms = 15000,
	};

	mqtt_cfg = {
		.client_id   = device_id(),
		.host        = pdata.mqtt_host,
		.port        = pdata.mqtt_port,
		.buffer_size = CONFIG_MQTT_BUFFER_SIZE,
		.username    = pdata.mqtt_username,
		.password    = pdata.mqtt_password,
	};

	light_init(&light_cfg);
	if (light_state_exists()) {
		light_state_load(&saved_state);
		light_restore_state(&saved_state);
	} else {
		lc_set_brightness(lc_scale8_video(light_cfg.brightness, light_cfg.brightness_max));
		lc_set_power(true);
	}
	light_start();
	wifi_init(&wifi_cfg);
	mqtt_init(&mqtt_cfg);
	ha_light_init();

	Serial.println("[Main] initialized");
}

void loop() {
	wifi_loop();
	mqtt_loop();
	ha_light_loop();

	if (button_pressed(&btn, digitalRead(GPIO_BUTTON))) {
		Serial.println("[Main] button pressed, rebooting to factory...");
		boot_to_factory();
	}
}
