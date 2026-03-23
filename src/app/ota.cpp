#include "ota.h"
#include "wifi_sta.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Arduino.h>

static bool first_connect = true;

void ota_init(const ota_config_t *cfg) {
	ArduinoOTA.setHostname(cfg->hostname);
	ArduinoOTA.setPort(cfg->port);
	ArduinoOTA.setMdnsEnabled(false);
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.print("[OTA] error: ");
		Serial.println(error);
	});
}

void ota_loop(void) {
	if (!wifi_is_connected()) {
		return;
	}
	if (first_connect) {
		first_connect = false;
		ArduinoOTA.begin();
		Serial.println("[OTA] ready");
		return;
	}
	ArduinoOTA.handle();
}
