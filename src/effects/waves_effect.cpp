#include "waves_effect.h"
#include <LightComposer/math/scale.h>

const size_t kWavesCycleDuration = 1000;
const scale_t kWavesFillFract = 200;

const char* WavesEffect::getName() {
	return "waves";
}

void WavesEffect::onActivate(WavesEffectState& state, IPixels& pixels) {
	isReverse_ = false;
	progress_.start(kWavesCycleDuration);
	fillSize_ = scale8(pixels.count() - 1, kWavesFillFract);
	maxOffset_ = pixels.count() - 1 - fillSize_;
}

bool WavesEffect::handleFrame(WavesEffectState& state, IPixels& pixels) {
	uint8_t shift = scale8(maxOffset_, progress_.get());
	if (state.currentColor != state.targetColor) {
		state.currentColor = state.targetColor;
	}

	pixels.set(RGBColor::Black);
	uint8_t startOffset = isReverse_ ? maxOffset_ - shift : shift;
	for (uint8_t i = 0; i < fillSize_; i++) {
		pixels.set(state.currentColor, startOffset + i);
	}

	if (progress_.finished()) {
		isReverse_ = !isReverse_;
		progress_.start(kWavesCycleDuration);
	}
	return true;
}

void WavesEffect::onColorUpdate(WavesEffectState& state) {
	state.currentColor = state.targetColor;
	progress_.start(state.transitionMs);
}

WavesEffect WavesFx;
