#include "light_hal.h"
#include <FastLED.h>

static CRGB raw_leds[CONFIG_LIGHT_LED_MAX_COUNT];

void light_hal_set_pixel(uint16_t i, rgb_t c) {
	raw_leds[i].r = c.r;
	raw_leds[i].g = c.g;
	raw_leds[i].b = c.b;
}

rgb_t light_hal_get_pixel(uint16_t i) {
	return (rgb_t){ raw_leds[i].r, raw_leds[i].g, raw_leds[i].b };
}

void light_hal_show(void) {
	FastLED.show();
}

void light_hal_init(void) {
    FastLED.addLeds<CONFIG_LIGHT_CONTROLLER, CONFIG_LIGHT_PIN_CTL, RGB>(
		raw_leds, CONFIG_LIGHT_LED_MAX_COUNT
	);
	FastLED.show();
}
