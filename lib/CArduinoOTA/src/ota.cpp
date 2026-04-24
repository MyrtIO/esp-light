#include "ota.h"
#include "wifi_manager.h"
#include <ArduinoOTA.h>
#include <Arduino.h>

static const ota_config_t *cfg;
static bool started = false;
static uint8_t last_logged_progress = 0;

static bool ota_network_available(void) {
	return wifi_sta_is_connected() || wifi_ap_is_enabled();
}

static const char *ota_error_to_str(ota_error_t error) {
	switch (error) {
	case OTA_AUTH_ERROR:
		return "auth";
	case OTA_BEGIN_ERROR:
		return "begin";
	case OTA_CONNECT_ERROR:
		return "connect";
	case OTA_RECEIVE_ERROR:
		return "receive";
	case OTA_END_ERROR:
		return "end";
	default:
		return "unknown";
	}
}

static void ota_start(void) {
	if (started || cfg == NULL || cfg->hostname == NULL || cfg->hostname[0] == '\0') {
		return;
	}

	last_logged_progress = 0;
	ArduinoOTA.setHostname(cfg->hostname);
	ArduinoOTA.onStart([]() {
		last_logged_progress = 0;
		Serial.println("[OTA] start");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		if (total == 0) {
			return;
		}

		uint8_t percent = (uint8_t)((progress * 100U) / total);
		uint8_t progress_step = (uint8_t)((percent / 10U) * 10U);
		if (progress_step == last_logged_progress && percent < 100) {
			return;
		}

		last_logged_progress = progress_step;
		Serial.printf("[OTA] progress: %u%%\n", percent);
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("[OTA] end");
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("[OTA] error: %s\n", ota_error_to_str(error));
	});
	ArduinoOTA.begin();
	started = true;
}

static void ota_stop(void) {
	if (!started) {
		return;
	}

	ArduinoOTA.end();
	started = false;
}

void ota_init(const ota_config_t *c) {
	cfg = c;
}

void ota_reconfigure(const ota_config_t *c) {
	bool should_restart = started;
	if (should_restart) {
		ota_stop();
	}

	cfg = c;

	if (should_restart && ota_network_available()) {
		ota_start();
	}
}

void ota_loop(void) {
	if (!ota_network_available()) {
		ota_stop();
		return;
	}

	if (!started) {
		ota_start();
	}

	ArduinoOTA.handle();
}
