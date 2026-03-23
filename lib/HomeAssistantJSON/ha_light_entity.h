#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ha_entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t r, g, b;
} ha_rgb_color_t;

typedef enum {
	HA_COLOR_MODE_RGB  = 0,
	HA_COLOR_MODE_TEMP = 1,
} ha_color_mode_t;

enum ha_light_field_t {
	HA_LIGHT_FIELD_STATE      = (1 << 0),
	HA_LIGHT_FIELD_BRIGHTNESS = (1 << 1),
	HA_LIGHT_FIELD_EFFECT     = (1 << 2),
	HA_LIGHT_FIELD_COLOR      = (1 << 3),
	HA_LIGHT_FIELD_COLOR_TEMP = (1 << 4),
};

typedef struct {
	const char **effects;
	uint16_t effect_count;
	uint16_t min_kelvin;
	uint16_t max_kelvin;
} ha_light_config_t;

typedef struct {
	bool enabled;
	uint16_t brightness;
	uint16_t color_temp;
	const char *effect;
	ha_rgb_color_t color;
	ha_color_mode_t color_mode;
} ha_light_state_t;

bool ha_light_serialize_config(
	ha_entity_t *entity,
	const ha_light_config_t *config,
	char *buffer,
	size_t size
);

bool ha_light_serialize_state(
	const ha_light_state_t *state,
	char *buffer,
	size_t size
);

uint8_t ha_light_parse_command(
	const uint8_t *payload,
	size_t len,
	ha_light_state_t *state
);

#ifdef __cplusplus
}
#endif
