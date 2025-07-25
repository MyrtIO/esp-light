#include "fasthal.h"
#include <FastLED.h>


void FastHAL::setup(RGBColor correction = RGBColor::White) {
	FastLED.addLeds<
        CONFIG_LIGHT_CONTROLLER,
        CONFIG_LIGHT_PIN_CTL,
        CONFIG_LIGHT_COLOR_ORDER
    >(
		reinterpret_cast<CRGB*>(&leds_[0]),
		CONFIG_LIGHT_LED_COUNT
	);
	FastLED.setCorrection(CRGB(
		correction.r,
		correction.g,
		correction.b
	));
	FastLED.show();
}

uint16_t FastHAL::count() {
	return CONFIG_LIGHT_LED_COUNT;
}

void FastHAL::setPixel(const uint16_t index, const RGBColor& color) {
#ifdef CONFIG_LIGHT_LED_SKIP
    if (index <= CONFIG_LIGHT_LED_SKIP) {
        return;
    }
#endif
	leds_[index] = color;
}

RGBColor FastHAL::getPixel(const uint16_t index) {
	return leds_[index];
}

void FastHAL::setBrightness(uint8_t brightness){
    brightness = scale8_video(brightness, CONFIG_LIGHT_BRIGHTNESS_MAX);
	FastLED.setBrightness(brightness);
}

void FastHAL::apply() {
	FastLED.show();
}
