#pragma once

#include <LightComposer/pixels/i_pixels_effect.h>
#include <Attotime.h>

typedef LightState<void> WavesEffectState;

class WavesEffect : public IPixelsEffect<void> {
public:
	const char* getName();
	bool handleFrame(WavesEffectState& state, IPixels& pixels);
	void onColorUpdate(WavesEffectState& state);
	void onActivate(WavesEffectState& state, IPixels& pixels);

private:
	Progress progress_ = Progress();
	bool isReverse_ = false;
	uint8_t fillSize_ = 0;
	size_t maxOffset_ = 0;
};

extern WavesEffect WavesFx;
