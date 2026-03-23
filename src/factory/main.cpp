#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <config.h>
#include <lc.h>
#include <FastLED.h>

#include "persistent_data.h"
#include "light.h"
#include "page.h"

#define GPIO_LED     2
#define GPIO_BUTTON  0
#define LED_BLINK_MS 200

#define BUTTON_DEBOUNCE_MS 50

static WebServer server(80);
static persistent_data_t pdata;
static light_config_t light_cfg;

static unsigned long last_blink = 0;
static bool led_state = false;

static bool btn_last_raw = HIGH;
static bool btn_stable = HIGH;
static unsigned long btn_changed_at = 0;

/* --- Color order helpers --- */

static const char *color_order_names[] = {
	"rgb", "rbg", "grb", "gbr", "brg", "bgr"
};

static const char *color_order_to_str(uint8_t order) {
	if (order > 5) return "rgb";
	return color_order_names[order];
}

static uint8_t color_order_from_str(const char *str) {
	for (uint8_t i = 0; i < 6; i++) {
		if (strcasecmp(str, color_order_names[i]) == 0) return i;
	}
	return 0;
}

/* --- Build light_config_t from persistent_data --- */

static void build_light_config(void) {
	light_cfg.pin              = CONFIG_LIGHT_PIN_CTL;
	light_cfg.led_count        = pdata.led_count;
	light_cfg.led_skip         = pdata.skip_leds;
	light_cfg.color_correction = pdata.color_correction;
	light_cfg.color_order      = pdata.color_order;
	light_cfg.kelvin_warm      = CONFIG_LIGHT_COLOR_KELVIN_WARM;
	light_cfg.kelvin_cold      = CONFIG_LIGHT_COLOR_KELVIN_COLD;
	light_cfg.kelvin_initial   = CONFIG_LIGHT_COLOR_KELVIN_INITIAL;
	light_cfg.transition_ms    = CONFIG_LIGHT_TRANSITION_COLOR;
	light_cfg.brightness       = 200;
	light_cfg.brightness_max   = pdata.brightness_max;
}

/* --- WiFi AP --- */

static void wifi_ap_start(void) {
	uint8_t mac[6];
	WiFi.macAddress(mac);
	char ssid[64];
	snprintf(ssid, sizeof(ssid), "MyrtIO Светильник %02X%02X",
		mac[4], mac[5]);

	WiFi.mode(WIFI_AP);
	WiFi.softAP(ssid);
	Serial.printf("[WiFi] AP started: %s\n", ssid);
	Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
}

/* --- API handlers --- */

static void handle_page(void) {
	server.sendHeader("Content-Encoding", "gzip");
	server.send_P(200, "text/html", (const char *)factory_page, factory_page_len);
}

static void handle_get_configuration(void) {
	JsonDocument doc;

	JsonObject wifi = doc["wifi"].to<JsonObject>();
	wifi["ssid"]     = pdata.wifi_ssid;
	wifi["password"] = pdata.wifi_password;

	JsonObject mqtt = doc["mqtt"].to<JsonObject>();
	mqtt["host"]     = pdata.mqtt_host;
	mqtt["port"]     = pdata.mqtt_port;
	mqtt["username"] = pdata.mqtt_username;
	mqtt["password"] = pdata.mqtt_password;

	JsonObject light = doc["light"].to<JsonObject>();
	light["led_count"]         = pdata.led_count;
	light["skip_leds"]         = pdata.skip_leds;
	light["color_correction"]  = pdata.color_correction;
	light["brightness_min"]    = pdata.brightness_min;
	light["brightness_max"]    = pdata.brightness_max;
	light["color_order"]       = color_order_to_str(pdata.color_order);

	String body;
	serializeJson(doc, body);
	server.send(200, "application/json", body);
}

static void handle_post_configuration(void) {
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, server.arg("plain"));
	if (err) {
		server.send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	JsonObject wifi = doc["wifi"];
	if (wifi) {
		strlcpy(pdata.wifi_ssid,     wifi["ssid"] | "",     sizeof(pdata.wifi_ssid));
		strlcpy(pdata.wifi_password,  wifi["password"] | "", sizeof(pdata.wifi_password));
	}

	JsonObject mqtt = doc["mqtt"];
	if (mqtt) {
		strlcpy(pdata.mqtt_host,     mqtt["host"] | "",     sizeof(pdata.mqtt_host));
		pdata.mqtt_port =            mqtt["port"] | 1883;
		strlcpy(pdata.mqtt_username, mqtt["username"] | "",  sizeof(pdata.mqtt_username));
		strlcpy(pdata.mqtt_password, mqtt["password"] | "",  sizeof(pdata.mqtt_password));
	}

	JsonObject light = doc["light"];
	if (light) {
		pdata.led_count        = light["led_count"] | pdata.led_count;
		pdata.skip_leds        = light["skip_leds"] | pdata.skip_leds;
		pdata.color_correction = light["color_correction"] | pdata.color_correction;
		pdata.brightness_min   = light["brightness_min"] | pdata.brightness_min;
		pdata.brightness_max   = light["brightness_max"] | pdata.brightness_max;
		const char *order      = light["color_order"] | "";
		if (order[0]) {
			pdata.color_order = color_order_from_str(order);
		}
	}

	persistent_data_save(&pdata);
	build_light_config();
	light_update_config(&light_cfg);

	server.send(204);
}

static void handle_post_light_configuration(void) {
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, server.arg("plain"));
	if (err) {
		server.send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	pdata.led_count        = doc["led_count"] | pdata.led_count;
	pdata.skip_leds        = doc["skip_leds"] | pdata.skip_leds;
	pdata.color_correction = doc["color_correction"] | pdata.color_correction;
	pdata.brightness_min   = doc["brightness_min"] | pdata.brightness_min;
	pdata.brightness_max   = doc["brightness_max"] | pdata.brightness_max;
	const char *order      = doc["color_order"] | "";
	if (order[0]) {
		pdata.color_order = color_order_from_str(order);
	}

	build_light_config();
	light_update_config(&light_cfg);

	server.send(204);
}

static void handle_post_light_test(void) {
	JsonDocument doc;
	DeserializationError err = deserializeJson(doc, server.arg("plain"));
	if (err) {
		server.send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	light_cmd_t cmd;
	cmd.type = LIGHT_CMD_COLOR;
	cmd.color = {
		.r = (uint8_t)(doc["r"] | 0),
		.g = (uint8_t)(doc["g"] | 0),
		.b = (uint8_t)(doc["b"] | 0),
	};
	light_send_cmd(&cmd);

	cmd.type = LIGHT_CMD_BRIGHTNESS;
	cmd.brightness = doc["brightness"] | 128;
	light_send_cmd(&cmd);

	server.send(204);
}

static void handle_get_system(void) {
	JsonDocument doc;
	doc["build_version"] = __DATE__ " " __TIME__;

	uint8_t mac[6];
	WiFi.macAddress(mac);
	JsonArray mac_arr = doc["mac_address"].to<JsonArray>();
	for (int i = 0; i < 6; i++) {
		mac_arr.add(mac[i]);
	}

	String body;
	serializeJson(doc, body);
	server.send(200, "application/json", body);
}

static void handle_post_boot(void) {
	server.send(200, "application/json", "{\"ok\":true}");
	delay(200);
	boot_to_app();
}

/* --- Setup & Loop --- */

void setup() {
	Serial.begin(115200);

	pinMode(GPIO_LED, OUTPUT);
	pinMode(GPIO_BUTTON, INPUT_PULLUP);

	persistent_data_load(&pdata);
	build_light_config();
	light_init(&light_cfg);
	lc_set_brightness(scale8_video(200, light_cfg.brightness_max));
	lc_set_power(true);
	light_start();

	light_cmd_t cmd;
	cmd.type = LIGHT_CMD_COLOR;
	cmd.color = { 255, 255, 255 };
	light_send_cmd(&cmd);

	wifi_ap_start();

	server.on("/", HTTP_GET, handle_page);
	server.on("/api/configuration",       HTTP_GET,  handle_get_configuration);
	server.on("/api/configuration",       HTTP_POST, handle_post_configuration);
	server.on("/api/configuration/light", HTTP_POST, handle_post_light_configuration);
	server.on("/api/light/test",          HTTP_POST, handle_post_light_test);
	server.on("/api/system",              HTTP_GET,  handle_get_system);
	server.on("/api/boot",                HTTP_POST, handle_post_boot);
	server.begin();

	Serial.println("[Factory] initialized");
}

void loop() {
	server.handleClient();

	unsigned long now = millis();

	/* LED blink */
	if (now - last_blink >= LED_BLINK_MS) {
		last_blink = now;
		led_state = !led_state;
		digitalWrite(GPIO_LED, led_state);
	}

	/* GPIO0 button → boot to app */
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
