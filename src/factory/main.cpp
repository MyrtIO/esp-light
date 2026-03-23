#include <Arduino.h>
#include <config.h>
#include <lc.h>
#include <FastLED.h>

#include "persistent_data.h"
#include "light.h"
#include "device_id.h"
#include "wifi_ap.h"
#include "api.h"

#define GPIO_LED     2
#define GPIO_BUTTON  0
#define LED_BLINK_MS 200

#define BUTTON_DEBOUNCE_MS 50

static persistent_data_t pdata;
static light_config_t light_cfg;

static unsigned long last_blink = 0;
static bool led_state = false;

static bool btn_last_raw = HIGH;
static bool btn_stable = HIGH;
static unsigned long btn_changed_at = 0;

void setup() {
	Serial.begin(115200);
	device_id_init();

	pinMode(GPIO_LED, OUTPUT);
	pinMode(GPIO_BUTTON, INPUT_PULLUP);

	persistent_data_load(&pdata);
	api_init(&pdata, &light_cfg);

	light_init(&light_cfg);
	lc_set_brightness(scale8_video(200, light_cfg.brightness_max));
	lc_set_power(true);
	light_start();

	light_cmd_t cmd;
	cmd.type = LIGHT_CMD_COLOR;
	cmd.color = { 255, 255, 255 };
	light_send_cmd(&cmd);

	wifi_ap_init();
	api_start();

	Serial.println("[Factory] initialized");
}

void loop() {
	api_loop();

	unsigned long now = millis();

	if (now - last_blink >= LED_BLINK_MS) {
		last_blink = now;
		led_state = !led_state;
		digitalWrite(GPIO_LED, led_state);
	}

	bool btn_now = digitalRead(GPIO_BUTTON);
	if (btn_now != btn_last_raw) {
		btn_changed_at = now;
		btn_last_raw = btn_now;
	}
	if ((now - btn_changed_at) > BUTTON_DEBOUNCE_MS && btn_now != btn_stable) {
		btn_stable = btn_now;
		if (btn_stable == LOW) {
			Serial.println("[Factory] button pressed, booting app...");
			boot_to_app();
		}
	}
}
