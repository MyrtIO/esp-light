#include <Arduino.h>
#include <button.h>
#include <cpubsub.h>

#include "config.h"
#include "persistent_data.h"
#include "light.h"
#include "wifi_manager.h"
#include "ota.h"
#include "mqtt_ha.h"
#include "provisioning_web.h"

#define GPIO_BUTTON 0
#define BUTTON_HOLD_MS 3000

static persistent_data_t pdata;
static light_saved_state_t saved_state;

static light_config_t light_cfg;
static wifi_runtime_config_t wifi_cfg;
static mqtt_config_t mqtt_cfg;
static ota_config_t ota_cfg;

static button_t btn = BUTTON_INIT(50);

static bool manual_provisioning = false;
static bool runtime_reconfigure_pending = false;

static void build_light_config(void) {
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
}

static void build_wifi_config(void) {
	wifi_cfg = {
		.ssid                  = pdata.wifi_ssid,
		.password              = pdata.wifi_password,
		.hostname              = device_hostname(),
		.ap_ssid               = device_name(),
		.reconnect_interval_ms = 15000,
	};
}

static void build_mqtt_config(void) {
	mqtt_cfg = {
		.client_id   = device_id(),
		.host        = pdata.mqtt_host,
		.port        = pdata.mqtt_port,
		.buffer_size = CONFIG_MQTT_BUFFER_SIZE,
		.username    = pdata.mqtt_username,
		.password    = pdata.mqtt_password,
	};
}

static void build_ota_config(void) {
	ota_cfg = {
		.hostname = device_hostname(),
	};
}

static void rebuild_runtime_config(void) {
	build_light_config();
	build_wifi_config();
	build_mqtt_config();
	build_ota_config();
}

static void on_configuration_changed(void) {
	runtime_reconfigure_pending = true;
}

static void apply_runtime_config(void) {
	rebuild_runtime_config();
	light_update_config(&light_cfg);
	wifi_reconfigure(&wifi_cfg);
	wifi_set_provisioning(manual_provisioning);
	mqtt_reconfigure(&mqtt_cfg);
	ota_reconfigure(&ota_cfg);

	Serial.println("[Main] runtime configuration applied");
}

static void set_default_light_state(void) {
	light_cmd_t cmd = {
		.type = LIGHT_CMD_BRIGHTNESS,
		.brightness = light_cfg.brightness,
	};
	light_send_cmd(&cmd);

	cmd.type = LIGHT_CMD_POWER;
	cmd.power = true;
	light_send_cmd(&cmd);
}

static void toggle_provisioning(void) {
	if (!persistent_data_is_configured(&pdata)) {
		Serial.println("[Main] provisioning stays enabled until Wi-Fi is configured");
		return;
	}

	manual_provisioning = !manual_provisioning;
	wifi_set_provisioning(manual_provisioning);
	Serial.printf(
		"[Main] provisioning %s\n",
		manual_provisioning ? "enabled" : "disabled"
	);
}

static uint32_t now_u32(void) {
	return (uint32_t)millis();
}

void setup() {
	Serial.begin(115200);
	device_id_init();

	pinMode(GPIO_BUTTON, INPUT_PULLUP);

	persistent_data_load(&pdata);
	rebuild_runtime_config();
	bool has_saved_light_state = light_state_exists();

	atto_init(now_u32);

	light_init(&light_cfg);
	if (has_saved_light_state) {
		light_state_load(&saved_state);
		light_restore_state(&saved_state);
	}
	light_start();

	if (!has_saved_light_state) {
		set_default_light_state();
	}

	wifi_init(&wifi_cfg);
	mqtt_init(&mqtt_cfg);
	ota_init(&ota_cfg);
	mqtt_ha_init();

	web_init(&pdata, &light_cfg, on_configuration_changed);
	web_start();

	Serial.println("[Main] initialized");
}

void loop() {
	web_loop();

	if (runtime_reconfigure_pending) {
		runtime_reconfigure_pending = false;
		apply_runtime_config();
	}

	wifi_loop();
	mqtt_loop();
	ota_loop();
	mqtt_ha_loop();

	if (button_held(&btn, digitalRead(GPIO_BUTTON), BUTTON_HOLD_MS)) {
		toggle_provisioning();
	}
}
