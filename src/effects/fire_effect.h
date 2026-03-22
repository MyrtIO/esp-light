#pragma once

#include <LightComposer/pixels/i_pixels_effect.h>
#include <Attotime.h>
#include <config.h>

typedef LightState<void> FireEffectState;

class FireEffect : public IPixelsEffect<void> {
public:
	const char* getName();
	bool handleFrame(FireEffectState& state, IPixels& pixels);
	void onColorUpdate(FireEffectState& state);

private:
	uint8_t heat_[CONFIG_LIGHT_LED_COUNT];
	uint32_t cooldown_;
	Timer frameReady_;
};

extern FireEffect FireFx;
