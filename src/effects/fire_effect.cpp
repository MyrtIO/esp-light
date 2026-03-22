#include "fire_effect.h"
#include <LightComposer/math/scale.h>

const uint8_t kFireEffectCooling = 55;
const uint8_t kFireEffectSparking = 120;
const uint8_t kFireEffectInterval = 15;

const char* FireEffect::getName() {
	return "fire";
}

bool FireEffect::handleFrame(FireEffectState& state, IPixels& pixels) {
	if (!frameReady_.finished()) {
		return false;
	}

	for (int i = 0; i < pixels.count(); i++) {
		cooldown_ = random(0, ((kFireEffectCooling * 10) / pixels.count()) + 2);
		if (cooldown_ > heat_[i]) {
			heat_[i] = 0;
		} else {
			heat_[i] = heat_[i] - cooldown_;
		}
	}

	for (int k = pixels.count() - 1; k >= 2; k--) {
		heat_[k] = (heat_[k - 1] + heat_[k - 2] + heat_[k - 2]) / 3;
	}

	if (random(255) < kFireEffectSparking) {
		int y = random(7);
		heat_[y] = heat_[y] + random(160, 255);
	}

	for (int j = 0; j < pixels.count(); j++) {
		byte t192 = round((heat_[j] / 255.0) * 191);
		byte heatRamp = t192 & 0x3F;
		heatRamp <<= 2;

		if (t192 > 0x80) {
			pixels.set(RGBColor(255, 255, heatRamp), j);
		} else if (t192 > 0x40) {
			pixels.set(RGBColor(255, heatRamp, 0), j);
		} else {
			pixels.set(RGBColor(heatRamp, 0, 0), j);
		}
	}

	frameReady_.start(kFireEffectInterval);
	return true;
}

void FireEffect::onColorUpdate(FireEffectState& state) {
	state.currentColor = state.targetColor;
}

FireEffect FireFx;
