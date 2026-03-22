#include "ha_light.h"
#include "light.h"
#include "mqtt.h"

#include <HomeAssistantJSON/device.h>
#include <HomeAssistantJSON/light_entity.h>
#include <Arduino.h>
#include <config.h>

#define HA_BUFFER_SIZE        1024
#define HA_STATE_INTERVAL     30000
#define HA_CONFIG_INTERVAL    60000

static HomeAssistant::Device ha_device({
	.name = CONFIG_DEVICE_NAME,
	.id = CONFIG_DEVICE_ID,
	.mqttNamespace = CONFIG_MQTT_NAMESPACE
});

static HomeAssistant::LightEntity entity({
	.name = "Light",
	.identifier = "light",
	.icon = "mdi:led-strip-variant",
	.effects = nullptr,
	.effectCount = 0,
	.writable = true,
	.maxMireds = CONFIG_LIGHT_COLOR_MIREDS_WARM,
	.minMireds = CONFIG_LIGHT_COLOR_MIREDS_COLD
}, ha_device);

static char buffer[HA_BUFFER_SIZE];
static HomeAssistant::LightState ha_state;

static unsigned long last_state_report = 0;
static unsigned long last_config_report = 0;

static void publish_state(void) {
	light_state_t st = light_get_state();

	ha_state.enabled = st.power;
	ha_state.brightness = st.brightness;
	ha_state.colorTemp = st.color_temp;
	ha_state.effect = st.effect;
	ha_state.color = {
		.r = st.color.r,
		.g = st.color.g,
		.b = st.color.b
	};
	ha_state.colorMode = (st.color_mode == LIGHT_MODE_RGB)
		? HomeAssistant::ColorMode::RGBMode
		: HomeAssistant::ColorMode::ColorTempMode;

	entity.serializeValue(buffer, ha_state);
	mqtt_publish(entity.stateTopic(), buffer, false);
}

static void publish_config(void) {
	entity.serializeConfig(buffer);
	mqtt_publish(entity.configTopic(), buffer, false);
}

static bool is_color_black(const HomeAssistant::RGBColor &c) {
	return c.r == 0 && c.g == 0 && c.b == 0;
}

static void on_command(const uint8_t *payload, uint16_t length) {
	entity.parseValue(ha_state, (byte *)payload);

	light_state_t current = light_get_state();

	if (ha_state.enabled != current.power) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_POWER;
		cmd.power = ha_state.enabled;
		light_send_cmd(&cmd);
	} else if (ha_state.brightness != 0 && ha_state.brightness != current.brightness) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_BRIGHTNESS;
		cmd.brightness = ha_state.brightness;
		light_send_cmd(&cmd);
	}

	if (ha_state.effect != nullptr) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_EFFECT;
		cmd.effect_name = ha_state.effect;
		light_send_cmd(&cmd);
	}

	if (!is_color_black(ha_state.color) &&
		(ha_state.color.r != current.color.r ||
		 ha_state.color.g != current.color.g ||
		 ha_state.color.b != current.color.b)) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR;
		cmd.color = {ha_state.color.r, ha_state.color.g, ha_state.color.b};
		light_send_cmd(&cmd);
	} else if (ha_state.colorTemp != 0) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR_TEMP;
		cmd.color_temp = ha_state.colorTemp;
		light_send_cmd(&cmd);
	}

	publish_state();
}

void ha_light_init(void) {
	entity.setEffects(light_effect_names(), light_effect_count());
	mqtt_subscribe(entity.commandTopic(), on_command);
}

void ha_light_loop(void) {
	if (!mqtt_is_connected()) {
		return;
	}

	unsigned long now = millis();

	if (now - last_state_report >= HA_STATE_INTERVAL) {
		last_state_report = now;
		publish_state();
	}

	if (now - last_config_report >= HA_CONFIG_INTERVAL) {
		last_config_report = now;
		publish_config();
	}
}
