#pragma once

#include <stdint.h>
#include <stdbool.h>

struct persistent_data_t {
	char wifi_ssid[33];
	char wifi_password[65];
	char mqtt_host[65];
	uint16_t mqtt_port;
	char mqtt_username[33];
	char mqtt_password[65];
	uint16_t led_count;
	uint16_t skip_leds;
	uint32_t color_correction;
	uint8_t brightness_min;
	uint8_t brightness_max;
	uint8_t color_order;
};

struct light_saved_state_t {
	bool power;
	uint8_t brightness;
	uint8_t r, g, b;
	uint16_t color_temp;
	uint8_t color_mode;
	char effect[16];
};

void persistent_data_load(persistent_data_t *data);
void persistent_data_save(const persistent_data_t *data);
bool persistent_data_is_configured(const persistent_data_t *data);

bool light_state_exists(void);
void light_state_load(light_saved_state_t *state);
void light_state_save(const light_saved_state_t *state);

void boot_to_factory(void);
void boot_to_app(void);
