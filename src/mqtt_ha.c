#include "mqtt_ha.h"

#include <string.h>
#include <cpubsub.h>
#include <lc_color.h>
#include <attotime.h>
#include <homeassistant_json.h>

#include "config.h"
#include "light.h"
#include "persistent_data.h"

#define HA_BUFFER_SIZE        1024
#define HA_STATE_INTERVAL     30000
#define HA_CONFIG_INTERVAL    60000
#define HA_STATE_DELAY_MS     100
#define HA_SAVE_DELAY_MS      2000

static ha_device_t ha_device;

static ha_entity_t entity = {
	.name = "Свет",
	.icon = "mdi:led-strip-variant",
	.identifier = "light",
	.component = "light",
	.writable = true,
	.device = &ha_device,
};

static ha_light_config_t light_ha_config = {
	.effects = NULL,
	.effect_count = 0,
	.min_kelvin = LC_KELVIN_MIN,
	.max_kelvin = LC_KELVIN_MAX,
};

static char buffer[HA_BUFFER_SIZE];
static ha_light_state_t ha_state;

static unsigned long last_state_report = 0;
static unsigned long last_config_report = 0;
static bool state_publish_pending = false;
static unsigned long state_publish_at = 0;
static bool state_save_pending = false;
static unsigned long state_save_at = 0;
static bool was_connected = false;

static void publish_state(void) {
	light_state_t st = light_get_state();

	ha_state.enabled = st.power;
	ha_state.brightness = st.brightness;
	ha_state.color_temp = st.color_temp;
	ha_state.effect = st.effect;

	ha_state.color.r = st.color.r;
	ha_state.color.g = st.color.g;
	ha_state.color.b = st.color.b;

	ha_state.color_mode = (st.color_mode == LIGHT_MODE_RGB)
		? HA_COLOR_MODE_RGB
		: HA_COLOR_MODE_TEMP;

	ha_light_serialize_state(&ha_state, buffer, sizeof(buffer));
	cpubsub_publish(ha_entity_state_topic(&entity), buffer, false);
}

static void publish_discovery(void) {
	ha_light_serialize_config(&entity, &light_ha_config, buffer, sizeof(buffer));
	cpubsub_publish(ha_entity_config_topic(&entity), buffer, true);
}

static void save_light_state(void) {
	light_state_t st = light_get_state();
	light_saved_state_t saved;
	saved.power = st.power;
	saved.brightness = st.brightness;
	saved.r = st.color.r;
	saved.g = st.color.g;
	saved.b = st.color.b;
	saved.color_temp = st.color_temp;
	saved.color_mode = (uint8_t)st.color_mode;
	memset(saved.effect, 0, sizeof(saved.effect));
	if (st.effect) {
		strncpy(saved.effect, st.effect, sizeof(saved.effect) - 1);
	}
	light_state_save(&saved);
}

static void on_command(const uint8_t *payload, uint16_t length) {
	uint8_t fields = ha_light_parse_command(payload, length, &ha_state);

	if (fields & HA_LIGHT_FIELD_STATE) {
		struct light_cmd_t cmd;
		cmd.type = LIGHT_CMD_POWER;
		cmd.power = ha_state.enabled;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_BRIGHTNESS) {
		struct light_cmd_t cmd;
		cmd.type = LIGHT_CMD_BRIGHTNESS;
		cmd.brightness = ha_state.brightness;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_EFFECT) {
		struct light_cmd_t cmd;
		cmd.type = LIGHT_CMD_EFFECT;
		cmd.effect_name = ha_state.effect;
		light_send_cmd(&cmd);
	}

	if (fields & HA_LIGHT_FIELD_COLOR) {
		struct light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR;
		cmd.color.r = ha_state.color.r;
		cmd.color.g = ha_state.color.g;
		cmd.color.b = ha_state.color.b;
		light_send_cmd(&cmd);
	} else if (fields & HA_LIGHT_FIELD_COLOR_TEMP) {
		struct light_cmd_t cmd;
		cmd.type = LIGHT_CMD_COLOR_TEMP;
		cmd.color_temp = ha_state.color_temp;
		light_send_cmd(&cmd);
	}

	state_publish_pending = true;
	state_publish_at = atto_now() + HA_STATE_DELAY_MS;

	state_save_pending = true;
	state_save_at = atto_now() + HA_SAVE_DELAY_MS;
}

static void on_ha_status(const uint8_t *payload, uint16_t length) {
	if (length != strlen(HA_AVAILABILITY_ONLINE)) return;
	if (memcmp(payload, HA_AVAILABILITY_ONLINE, length) != 0) return;
	publish_discovery();
	publish_state();
}

void mqtt_ha_init(void) {
	ha_device.name = device_name();
	ha_device.id = device_id();
	ha_device.mqtt_namespace = device_id();

	light_ha_config.effects = light_effect_names();
	light_ha_config.effect_count = light_effect_count();
	cpubsub_set_lwt(
		ha_entity_availability_topic(&entity),
		HA_AVAILABILITY_OFFLINE
	);
	cpubsub_subscribe(ha_entity_command_topic(&entity), on_command);
	cpubsub_subscribe(HA_STATUS_TOPIC, on_ha_status);
}

void mqtt_ha_loop(void) {
	if (!cpubsub_is_connected()) {
		was_connected = false;
		return;
	}

	unsigned long now = atto_now();

	if (!was_connected) {
		was_connected = true;
		cpubsub_publish(ha_entity_availability_topic(&entity), HA_AVAILABILITY_ONLINE, true);
		publish_discovery();
		publish_state();
		last_config_report = now;
		last_state_report = now;
		return;
	}

	if (state_publish_pending && (long)(now - state_publish_at) >= 0) {
		state_publish_pending = false;
		publish_state();
	}

	if (state_save_pending && (long)(now - state_save_at) >= 0) {
		state_save_pending = false;
		save_light_state();
	}

	if (now - last_state_report >= HA_STATE_INTERVAL) {
		last_state_report = now;
		publish_state();
	}

	if (now - last_config_report >= HA_CONFIG_INTERVAL) {
		last_config_report = now;
		publish_discovery();
	}
}
