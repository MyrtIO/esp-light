#pragma once

#include <MyrtIO/device/unit.h>
#include <LightComposer.h>
#include <LightComposer/brightness/brightness_renderer.h>
#include <LightComposer/pixels/pixels_renderer.h>
#include <LightComposer/pixels/effect_vector.h>
#include <LightComposer/color/white_color.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <config.h>
#include "i_light_platform.h"
#include "fasthal.h"

struct LightPlatformParams {
	uint8_t ledCount;
	uint8_t brightness;
	uint32_t colorCorrection;
    mireds_t temperatureInitial;
    mireds_t temperatureCold;
    mireds_t temperatureWarm;
	uint16_t transitionMs;
};

class LightPlatform : public io::Unit, public ILightPlatform {
  public:
	LightPlatform(const LightPlatformParams& params) : params_(params) {};

	const char* getName() override;
	void setup() override;
	void loop() override;

	void setColor(RGBColor color) override;
	RGBColor getColor() override;
	void setColorTemperature(mireds_t temperature) override;
	mireds_t getTemperature() override;
	color_mode_t getMode() override;

	void setBrightness(uint8_t brightness) override;
	uint8_t getBrightness() override;

	void setPower(bool enabled) override;
	bool getPower() override;

	bool setEffect(const char* effect, bool force = false) override;
	const char* getEffect() override;
	virtual const char** effects();
	virtual uint8_t effectCount();

	void setFPS(uint8_t fps);

  private:
	static void renderTask(void* arg);

	FastHAL hal_;
	BrightnessRenderer brightness_;
	PixelsRenderer<void> pixels_;
	LightComposer<void> composer_;
	LightPlatformParams params_;
	LightMode mode_ = LightMode::RGBMode;
	mireds_t temperature_ = 0;
	EffectList<void> effects_;
	WhiteColor whiteColor_;
	TaskHandle_t renderTaskHandle_ = nullptr;
};
