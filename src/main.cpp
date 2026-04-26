#include <Arduino.h>
#include <esp_system.h>
#include <button.h>
#include <cpubsub.h>
#include <string.h>

#include "config.h"
#include "persistent_data.h"
#include "light.h"
#include "cwifi_manager.h"
#include "mqtt_ha.h"
#include "provisioning_web.h"

static persistent_data_t pdata;
static light_saved_state_t saved_state;

static light_config_t light_cfg;
static cwifi_runtime_config_t wifi_cfg;
static cpubsub_config_t mqtt_cfg;

static button_t btn = BUTTON_INIT(50);

static bool manual_provisioning = false;
static bool runtime_reconfigure_pending = false;

static bool provisioning_enabled(void) {
	return manual_provisioning || !persistent_data_is_configured(&pdata);
}

static void build_light_config(void) {
	light_cfg = {
		.pin              = CONFIG_LIGHT_CTL_GPIO,
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
		.port        = (uint16_t)(pdata.mqtt_port != 0 ? pdata.mqtt_port : 1883),
		.buffer_size = CONFIG_MQTT_BUFFER_SIZE,
		.reconnect_delay = CONFIG_MQTT_RECONNECT_DELAY_MS,
		.username    = pdata.mqtt_username,
		.password    = pdata.mqtt_password,
	};
}

static void rebuild_runtime_config(void) {
	build_light_config();
	build_wifi_config();
	build_mqtt_config();
}

static void on_configuration_changed(void) {
	runtime_reconfigure_pending = true;
	Serial.println("[Main] runtime reconfigure scheduled");
}

static void apply_runtime_config(void) {
	rebuild_runtime_config();
	light_update_config(&light_cfg);
	cwifi_reconfigure(&wifi_cfg, provisioning_enabled());
	cpubsub_reconfigure(&mqtt_cfg);
}

static void set_default_light_state(void) {
	light_saved_state_t state = {};
	state.power = true;
	state.brightness = light_cfg.brightness;
	state.color_temp = light_cfg.kelvin_initial;
	state.color_mode = LIGHT_MODE_WHITE;
	state.r = 255;
	state.g = 255;
	state.b = 255;
	snprintf(state.effect, sizeof(state.effect), "%s", "static");

	light_restore_state(&state);
	light_state_save(&state);
}

static void toggle_provisioning(void) {
	if (!persistent_data_is_configured(&pdata)) {
		Serial.println("[Main] provisioning stays enabled until Wi-Fi and MQTT are configured");
		return;
	}

	manual_provisioning = !manual_provisioning;
	cwifi_set_provisioning(provisioning_enabled());
}

static uint32_t now_u32(void) {
	return (uint32_t)millis();
}

void setup() {
	Serial.begin(115200);
	delay(50);
	device_id_init();

	pinMode(CONFIG_PROVISIONING_BUTTON_GPIO, INPUT_PULLUP);

	persistent_data_load(&pdata);
	rebuild_runtime_config();
	bool has_saved_light_state = light_state_exists();

	atto_init(now_u32);
	cwifi_init(&wifi_cfg, provisioning_enabled());
	cpubsub_init(&mqtt_cfg);
	light_init(&light_cfg);
	if (has_saved_light_state) {
		light_state_load(&saved_state);
		light_restore_state(&saved_state);
	}
	light_start();

	if (!has_saved_light_state) {
		set_default_light_state();
	}

	mqtt_ha_init();
	web_init(&pdata, &light_cfg, on_configuration_changed);
	web_start();
}

void loop() {
	web_loop();

	if (runtime_reconfigure_pending) {
		runtime_reconfigure_pending = false;
		apply_runtime_config();
	}

	cwifi_loop();
	cpubsub_loop();
	mqtt_ha_loop();

	if (button_held(&btn, digitalRead(CONFIG_PROVISIONING_BUTTON_GPIO), CONFIG_PROVISIONING_HOLD_MS)) {
		toggle_provisioning();
	}
}
