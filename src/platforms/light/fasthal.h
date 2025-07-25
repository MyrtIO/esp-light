#pragma once

#include <LightComposer/i_light_hal.h>
#include <config.h>

class FastHAL : public ILightHAL {
public:
	FastHAL() {};

	void setup(const RGBColor correction);
	uint16_t count();

	void setPixel(const uint16_t index, const RGBColor& color);
	RGBColor getPixel(const uint16_t index);

	void setBrightness(const uint8_t brightness);

	void apply();

#ifdef CONFIG_LIGHT_LED_SKIP
	RGBColor* pixels() { return &leds_[CONFIG_LIGHT_LED_SKIP]; }
#else
	RGBColor* pixels() { return &leds_[0]; }
#endif

private:
	RGBColor leds_[CONFIG_LIGHT_LED_COUNT];
};
