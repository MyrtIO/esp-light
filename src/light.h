#pragma once

#include <stdbool.h>
#include <stdint.h>

struct rgb_color_t {
  uint8_t r, g, b;
};

enum light_cmd_type_t {
  LIGHT_CMD_POWER,
  LIGHT_CMD_BRIGHTNESS,
  LIGHT_CMD_COLOR,
  LIGHT_CMD_COLOR_TEMP,
  LIGHT_CMD_EFFECT,
};

struct light_cmd_t {
  light_cmd_type_t type;
  union {
    bool power;
    uint8_t brightness;
    rgb_color_t color;
    uint16_t color_temp;
    const char *effect_name;
  };
};

enum light_color_mode_t {
  LIGHT_MODE_RGB = 0,
  LIGHT_MODE_WHITE = 1,
};

struct light_state_t {
  bool power;
  uint8_t brightness;
  rgb_color_t color;
  uint16_t color_temp;
  const char *effect;
  light_color_mode_t color_mode;
};

struct light_config_t {
  uint8_t pin;
  uint16_t led_count;
  uint16_t led_skip;
  uint32_t color_correction;
  uint8_t color_order;
  uint16_t kelvin_warm;
  uint16_t kelvin_cold;
  uint16_t kelvin_initial;
  uint16_t transition_ms;
  uint8_t brightness;
  uint8_t brightness_max;
};

struct light_saved_state_t;

void light_init(const light_config_t *cfg);
void light_start(void);
void light_update_config(const light_config_t *cfg);
void light_restore_state(const light_saved_state_t *state);
void light_send_cmd(const light_cmd_t *cmd);
light_state_t light_get_state(void);

const char **light_effect_names(void);
uint8_t light_effect_count(void);
