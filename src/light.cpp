#include "light.h"
#include "effects/effects.h"

#include <FastLED.h>
#include <LightComposer.h>
#include <LightComposer/brightness/brightness_renderer.h>
#include <LightComposer/pixels/pixels_renderer.h>
#include <LightComposer/pixels/effect_vector.h>
#include <LightComposer/color/white_color.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <config.h>

// FastLED HAL — implements ILightHAL from LightComposer
class FastHAL : public ILightHAL {
public:
	void setup(RGBColor correction) {
		FastLED.addLeds<
			CONFIG_LIGHT_CONTROLLER,
			CONFIG_LIGHT_PIN_CTL,
			CONFIG_LIGHT_COLOR_ORDER
		>(
			reinterpret_cast<CRGB*>(&leds_[0]),
			CONFIG_LIGHT_LED_COUNT
		);
		FastLED.setCorrection(CRGB(correction.r, correction.g, correction.b));
		FastLED.show();
	}

	uint16_t count() { return CONFIG_LIGHT_LED_COUNT; }

	void setPixel(const uint16_t index, const RGBColor& color) {
#ifdef CONFIG_LIGHT_LED_SKIP
		if (index <= CONFIG_LIGHT_LED_SKIP) {
			return;
		}
#endif
		leds_[index] = color;
	}

	RGBColor getPixel(const uint16_t index) { return leds_[index]; }

	void setBrightness(uint8_t brightness) {
		brightness = scale8_video(brightness, CONFIG_LIGHT_BRIGHTNESS_MAX);
		FastLED.setBrightness(brightness);
	}

	void apply() { FastLED.show(); }

#ifdef CONFIG_LIGHT_LED_SKIP
	RGBColor* pixels() { return &leds_[CONFIG_LIGHT_LED_SKIP]; }
#else
	RGBColor* pixels() { return &leds_[0]; }
#endif

private:
	RGBColor leds_[CONFIG_LIGHT_LED_COUNT];
};

static FastHAL hal;
static BrightnessRenderer brightness;
static PixelsRenderer<void> pixels;
static LightComposer<void> composer;
static EffectList<void> effects;
static WhiteColor white_color;

static QueueHandle_t cmd_queue;
static TaskHandle_t render_task_handle;

static light_state_t current_state;
static light_color_mode_t color_mode;
static const light_config_t *light_cfg;

static portMUX_TYPE state_mutex = portMUX_INITIALIZER_UNLOCKED;

static void apply_cmd(const light_cmd_t *cmd) {
	switch (cmd->type) {
	case LIGHT_CMD_POWER:
		brightness.setPower(cmd->power);
		break;
	case LIGHT_CMD_BRIGHTNESS:
		brightness.setBrightness(cmd->brightness);
		break;
	case LIGHT_CMD_COLOR:
		color_mode = LIGHT_MODE_RGB;
		pixels.setColor(RGBColor(cmd->color.r, cmd->color.g, cmd->color.b));
		break;
	case LIGHT_CMD_COLOR_TEMP: {
		uint16_t temp = cmd->color_temp;
		if (temp < light_cfg->color_temp_cold || temp > light_cfg->color_temp_warm) {
			break;
		}
		color_mode = LIGHT_MODE_WHITE;
		white_color.setMireds(temp);
		pixels.setColor(white_color);
		break;
	}
	case LIGHT_CMD_EFFECT: {
		if (cmd->effect_name == nullptr) break;
		auto *fx = effects.find(cmd->effect_name);
		if (fx != nullptr && pixels.getEffect() != fx) {
			pixels.setEffect(fx);
		}
		break;
	}
	}
}

static void update_state(void) {
	auto color = pixels.getColor();
	auto *fx = pixels.getEffect();

	portENTER_CRITICAL(&state_mutex);
	current_state.power = brightness.getPower();
	current_state.brightness = brightness.getBrightness();
	current_state.color = { color.r, color.g, color.b };
	current_state.color_temp = white_color.mireds();
	current_state.effect = (fx != nullptr) ? fx->getName() : nullptr;
	current_state.color_mode = color_mode;
	portEXIT_CRITICAL(&state_mutex);
}

static void render_task(void *) {
	for (;;) {
		light_cmd_t cmd;
		while (xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
			apply_cmd(&cmd);
		}
		composer.loop();
		update_state();
		vTaskDelay(1);
	}
}

void light_init(const light_config_t *cfg) {
	light_cfg = cfg;

	RGBColor correction;
	correction.r = (cfg->color_correction >> 16) & 0xFF;
	correction.g = (cfg->color_correction >> 8) & 0xFF;
	correction.b = cfg->color_correction & 0xFF;
	hal.setup(correction);

	effects
		.insert(&StaticFx)
		.insert(&RainbowFx)
		.insert(&LoadingFx)
		.insert(&FillFx)
		.insert(&FireFx)
		.insert(&WavesFx);

	white_color.setMireds(cfg->color_temp_initial);
	pixels.setColor(white_color);
	pixels.setTransition(cfg->transition_ms);
	pixels.setEffect(&StaticFx);
	brightness.setBrightness(cfg->brightness);
	brightness.setPower(true);
	color_mode = LIGHT_MODE_WHITE;

	composer.start(hal, brightness, pixels);

	cmd_queue = xQueueCreate(8, sizeof(light_cmd_t));

	xTaskCreatePinnedToCore(
		render_task,
		"light",
		4096,
		nullptr,
		10,
		&render_task_handle,
		1
	);
}

void light_send_cmd(const light_cmd_t *cmd) {
	xQueueSend(cmd_queue, cmd, 0);
}

light_state_t light_get_state(void) {
	light_state_t copy;
	portENTER_CRITICAL(&state_mutex);
	copy = current_state;
	portEXIT_CRITICAL(&state_mutex);
	return copy;
}

const char **light_effect_names(void) {
	return effects.names();
}

uint8_t light_effect_count(void) {
	return effects.size();
}
