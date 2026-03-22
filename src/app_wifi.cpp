#include "app_wifi.h"
#include <WiFi.h>
#include <Arduino.h>

static const app_wifi_config_t *cfg;
static bool connected = false;
static unsigned long last_attempt = 0;

void wifi_init(const app_wifi_config_t *c) {
	cfg = c;
	WiFi.setHostname(cfg->hostname);
	WiFi.begin(cfg->ssid, cfg->password);
	last_attempt = millis();
}

void wifi_loop(void) {
	bool now_connected = (WiFi.status() == WL_CONNECTED);

	if (now_connected) {
		if (!connected) {
			Serial.println("[WiFi] connected");
		}
		connected = true;
		return;
	}

	connected = false;

	unsigned long now = millis();
	if (now - last_attempt < cfg->reconnect_interval_ms) {
		return;
	}

	Serial.println("[WiFi] reconnecting...");
	WiFi.begin(cfg->ssid, cfg->password);
	last_attempt = now;
}

bool wifi_is_connected(void) {
	return connected;
}
