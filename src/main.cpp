#include <Arduino.h>
#include <config.h>
#include "light.h"
#include "app_wifi.h"
#include "mqtt.h"
#include "ha_light.h"
#include "ota.h"

static const light_config_t light_cfg = {
	.pin            = CONFIG_LIGHT_PIN_CTL,
	.led_count      = CONFIG_LIGHT_LED_COUNT,
	.color_correction = CONFIG_LIGHT_COLOR_CORRECTION,
	.color_temp_warm  = CONFIG_LIGHT_COLOR_MIREDS_WARM,
	.color_temp_cold  = CONFIG_LIGHT_COLOR_MIREDS_COLD,
	.color_temp_initial = CONFIG_LIGHT_COLOR_MIREDS_INITIAL,
	.transition_ms  = CONFIG_LIGHT_TRANSITION_COLOR,
	.brightness     = CONFIG_LIGHT_BRIGHTNESS_INITIAL,
};

static const app_wifi_config_t wifi_cfg = {
	.ssid     = CONFIG_WIFI_SSID,
	.password = CONFIG_WIFI_PASSWORD,
	.hostname = CONFIG_DEVICE_NAME,
	.reconnect_interval_ms = 15000,
};

static const mqtt_config_t mqtt_cfg = {
	.client_id   = CONFIG_DEVICE_NAME,
	.host        = CONFIG_MQTT_HOST,
	.port        = CONFIG_MQTT_PORT,
	.buffer_size = CONFIG_MQTT_BUFFER_SIZE,
};

static const ota_config_t ota_cfg = {
	.hostname = CONFIG_DEVICE_NAME,
	.port     = CONFIG_OTA_PORT,
};

void setup() {
	Serial.begin(115200);

	light_init(&light_cfg);
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
}
