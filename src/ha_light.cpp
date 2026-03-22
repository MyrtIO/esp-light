#include "ha_light.h"
#include "light.h"
#include "mqtt.h"

#include <HomeAssistantJSON/ha_entity.h>
#include <HomeAssistantJSON/ha_light_entity.h>
#include <Arduino.h>
#include <config.h>

#define HA_BUFFER_SIZE        1024
#define HA_STATE_INTERVAL     30000
#define HA_CONFIG_INTERVAL    60000
#define HA_STATE_DELAY_MS     100

static const ha_device_t ha_device = {
	.name = CONFIG_DEVICE_NAME,
	.id = CONFIG_DEVICE_ID,
	.mqtt_namespace = CONFIG_MQTT_NAMESPACE,
};

static ha_entity_t entity = {
	.name = "Light",
	.icon = "mdi:led-strip-variant",
	.identifier = "light",
	.component = "light",
	.writable = true,
	.device = &ha_device,
};

static ha_light_config_t light_ha_config = {
	.effects = nullptr,
	.effect_count = 0,
	.max_mireds = CONFIG_LIGHT_COLOR_MIREDS_WARM,
	.min_mireds = CONFIG_LIGHT_COLOR_MIREDS_COLD,
};

static char buffer[HA_BUFFER_SIZE];
static ha_light_state_t ha_state;

static unsigned long last_state_report = 0;
static unsigned long last_config_report = 0;
static bool state_publish_pending = false;
static unsigned long state_publish_at = 0;

static void publish_state(void) {
	light_state_t st = light_get_state();

	ha_state.enabled = st.power;
	ha_state.brightness = st.brightness;
	ha_state.color_temp = st.color_temp;
	ha_state.effect = st.effect;
	ha_state.color = {
		.r = st.color.r,
		.g = st.color.g,
		.b = st.color.b,
	};
	ha_state.color_mode = (st.color_mode == LIGHT_MODE_RGB)
		? HA_COLOR_MODE_RGB
		: HA_COLOR_MODE_TEMP;

	ha_light_serialize_state(&ha_state, buffer, sizeof(buffer));
	mqtt_publish(ha_entity_state_topic(&entity), buffer, false);
}

static void publish_config(void) {
	ha_light_serialize_config(&entity, &light_ha_config, buffer, sizeof(buffer));
	mqtt_publish(ha_entity_config_topic(&entity), buffer, false);
}

static void on_command(const uint8_t *payload, uint16_t length) {
	uint8_t fields = ha_light_parse_command(payload, length, &ha_state);

	if (fields & HA_LIGHT_FIELD_STATE) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_POWER;
		cmd.power = ha_state.enabled;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_BRIGHTNESS) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_BRIGHTNESS;
		cmd.brightness = ha_state.brightness;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_EFFECT) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_EFFECT;
		cmd.effect_name = ha_state.effect;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_COLOR) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR;
		cmd.color = {ha_state.color.r, ha_state.color.g, ha_state.color.b};
		light_send_cmd(&cmd);
	} else if (fields & HA_LIGHT_FIELD_COLOR_TEMP) {
		light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR_TEMP;
		cmd.color_temp = ha_state.color_temp;
		light_send_cmd(&cmd);
	}

	state_publish_pending = true;
	state_publish_at = millis() + HA_STATE_DELAY_MS;
}

void ha_light_init(void) {
	light_ha_config.effects = light_effect_names();
	light_ha_config.effect_count = light_effect_count();
	mqtt_subscribe(ha_entity_command_topic(&entity), on_command);
}

void ha_light_loop(void) {
	if (!mqtt_is_connected()) {
		return;
	}

	unsigned long now = millis();

	if (state_publish_pending && (long)(now - state_publish_at) >= 0) {
		state_publish_pending = false;
		publish_state();
	}

	if (now - last_state_report >= HA_STATE_INTERVAL) {
		last_state_report = now;
		publish_state();
	}

	if (now - last_config_report >= HA_CONFIG_INTERVAL) {
		last_config_report = now;
		publish_config();
	}
}
