#include "light.h"

#include <FastLED.h>
#include <LightComposer/lc.h>
#include <LightComposer/effects/lc_fx.h>
#include <LightComposer/color/lc_white.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <attotime.h>
#include <config.h>
#include <string.h>

/* --- HAL (FastLED) --- */

static CRGB raw_leds[CONFIG_LIGHT_LED_COUNT];

static void hal_set_pixel(uint16_t i, rgb_t c) {
	raw_leds[i].r = c.r;
	raw_leds[i].g = c.g;
	raw_leds[i].b = c.b;
}

static rgb_t hal_get_pixel(uint16_t i) {
	return (rgb_t){ raw_leds[i].r, raw_leds[i].g, raw_leds[i].b };
}

static void hal_show(void) {
	FastLED.show();
}

static const lc_hal_t hal = {
	.set_pixel = hal_set_pixel,
	.get_pixel = hal_get_pixel,
	.show      = hal_show,
};

/* --- Effect name table --- */

typedef struct {
	const char    *name;
	lc_effect_t   *effect;
} light_effect_entry_t;

static light_effect_entry_t effect_table[] = {
	{ "static",  &lc_fx_static  },
	{ "rainbow", &lc_fx_rainbow },
	{ "loading", &lc_fx_loading },
	{ "fill",    &lc_fx_fill    },
	{ "fire",    &lc_fx_fire    },
	{ "waves",   &lc_fx_waves   },
};

#define EFFECT_COUNT (sizeof(effect_table) / sizeof(effect_table[0]))

static const char *effect_names[EFFECT_COUNT];

static lc_effect_t *find_effect(const char *name) {
	for (size_t i = 0; i < EFFECT_COUNT; i++) {
		if (strcmp(effect_table[i].name, name) == 0) {
			return effect_table[i].effect;
		}
	}
	return NULL;
}

static const char *effect_name_of(const lc_effect_t *fx) {
	for (size_t i = 0; i < EFFECT_COUNT; i++) {
		if (effect_table[i].effect == fx) {
			return effect_table[i].name;
		}
	}
	return NULL;
}

/* --- State --- */

static rgb_t pixel_buf[CONFIG_LIGHT_LED_COUNT];

static QueueHandle_t cmd_queue;
static TaskHandle_t render_task_handle;

static light_state_t current_state;
static light_color_mode_t color_mode;
static const light_config_t *light_cfg;
static uint16_t white_mireds;
static uint8_t brightness_max;
static uint8_t requested_brightness;

static portMUX_TYPE state_mutex = portMUX_INITIALIZER_UNLOCKED;

/* --- Command handling --- */

static void apply_cmd(const light_cmd_t *cmd) {
	switch (cmd->type) {
	case LIGHT_CMD_POWER:
		lc_set_power(cmd->power);
		break;
	case LIGHT_CMD_BRIGHTNESS: {
		requested_brightness = cmd->brightness;
		lc_set_brightness(scale8_video(cmd->brightness, brightness_max));
		break;
	}
	case LIGHT_CMD_COLOR:
		color_mode = LIGHT_MODE_RGB;
		lc_set_color((rgb_t){ cmd->color.r, cmd->color.g, cmd->color.b });
		break;
	case LIGHT_CMD_COLOR_TEMP: {
		uint16_t temp = cmd->color_temp;
		if (temp < light_cfg->color_temp_cold || temp > light_cfg->color_temp_warm) {
			break;
		}
		color_mode = LIGHT_MODE_WHITE;
		white_mireds = temp;
		rgb_t white;
		lc_kelvin_to_rgb(lc_mireds_to_kelvin(temp), &white);
		lc_set_color(white);
		break;
	}
	case LIGHT_CMD_EFFECT: {
		if (cmd->effect_name == NULL) break;
		lc_effect_t *fx = find_effect(cmd->effect_name);
		if (fx != NULL && lc_get_effect() != fx) {
			lc_set_effect(fx);
		}
		break;
	}
	}
}

static void update_state(void) {
	rgb_t color = lc_get_color();

	portENTER_CRITICAL(&state_mutex);
	current_state.power      = lc_get_power();
	current_state.brightness = requested_brightness;
	current_state.color      = { color.r, color.g, color.b };
	current_state.color_temp = white_mireds;
	current_state.effect     = effect_name_of(lc_get_target_effect());
	current_state.color_mode = color_mode;
	portEXIT_CRITICAL(&state_mutex);
}

static uint32_t millis_u32(void) {
	return (uint32_t)millis();
}

static void render_task(void *) {
	for (;;) {
		light_cmd_t cmd;
		while (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
			apply_cmd(&cmd);
		}
		lc_tick();
		update_state();
		vTaskDelay(1);
	}
}

/* --- Public API --- */

void light_init(const light_config_t *cfg) {
	light_cfg = cfg;
	brightness_max = cfg->brightness_max;

	FastLED.addLeds<CONFIG_LIGHT_CONTROLLER, CONFIG_LIGHT_PIN_CTL, GRB>(
		raw_leds, CONFIG_LIGHT_LED_COUNT
	);
	FastLED.show();

	atto_init(millis_u32);

	for (size_t i = 0; i < EFFECT_COUNT; i++) {
		effect_names[i] = effect_table[i].name;
	}

	lc_config_t lc_cfg = {
		.hal             = &hal,
		.pixel_buf       = pixel_buf,
		.max_leds_count  = CONFIG_LIGHT_LED_COUNT,
		.leds_count      = cfg->led_count,
		.led_skip        = cfg->led_skip,
		.color_correction = LC_RGB_FROM_CODE(cfg->color_correction),
		.color_order     = (lc_color_order_t)cfg->color_order,
		.fps             = 60,
	};
	lc_init(&lc_cfg);

	rgb_t initial_white;
	white_mireds = cfg->color_temp_initial;
	lc_kelvin_to_rgb(lc_mireds_to_kelvin(cfg->color_temp_initial), &initial_white);

	lc_set_color(initial_white);
	lc_set_transition(cfg->transition_ms);
	lc_set_effect_force(&lc_fx_static);
	requested_brightness = cfg->brightness;
	lc_set_brightness(scale8_video(cfg->brightness, brightness_max));
	lc_set_power(true);
	color_mode = LIGHT_MODE_WHITE;

	cmd_queue = xQueueCreate(8, sizeof(light_cmd_t));

	xTaskCreatePinnedToCore(
		render_task,
		"light",
		4096,
		NULL,
		10,
		&render_task_handle,
		1
	);
}

void light_send_cmd(const light_cmd_t *cmd) {
	xQueueSend(cmd_queue, cmd, 0);
}

light_state_t light_get_state(void) {
	light_state_t copy;
	portENTER_CRITICAL(&state_mutex);
	copy = current_state;
	portEXIT_CRITICAL(&state_mutex);
	return copy;
}

const char **light_effect_names(void) {
	return effect_names;
}

uint8_t light_effect_count(void) {
	return EFFECT_COUNT;
}
