#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

static const wifi_runtime_config_t *cfg;
static bool connected = false;
static bool provisioning_enabled = false;
static unsigned long last_attempt = 0;

static bool wifi_has_sta_config(void) {
	return cfg != NULL && cfg->ssid != NULL && cfg->ssid[0] != '\0';
}

static bool wifi_ap_should_be_enabled(void) {
	return provisioning_enabled || !wifi_has_sta_config();
}

static size_t copy_ip_string(IPAddress ip, char *buf, size_t buf_size) {
	if (buf == NULL || buf_size == 0) {
		return 0;
	}

	String ip_string = ip.toString();
	size_t len = ip_string.length();
	size_t copy_len = (len < (buf_size - 1)) ? len : (buf_size - 1);
	memcpy(buf, ip_string.c_str(), copy_len);
	buf[copy_len] = '\0';
	return copy_len;
}

static void wifi_begin_sta(void) {
	if (!wifi_has_sta_config()) {
		connected = false;
		return;
	}

	WiFi.setHostname(cfg->hostname);
	WiFi.begin(cfg->ssid, cfg->password);
	last_attempt = millis();
}

static void wifi_apply_mode(void) {
	bool sta_enabled = wifi_has_sta_config();
	bool ap_enabled = wifi_ap_should_be_enabled();

	if (sta_enabled && ap_enabled) {
		WiFi.mode(WIFI_AP_STA);
	} else if (sta_enabled) {
		WiFi.mode(WIFI_STA);
	} else {
		WiFi.mode(WIFI_AP);
	}

	if (ap_enabled) {
		WiFi.softAP(cfg->ap_ssid);
		Serial.printf("[WiFi] AP started: %s\n", cfg->ap_ssid);
		Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
	} else {
		WiFi.softAPdisconnect(true);
		Serial.println("[WiFi] AP stopped");
	}

	if (sta_enabled) {
		wifi_begin_sta();
	} else {
		WiFi.disconnect();
		connected = false;
	}
}

void wifi_init(const wifi_runtime_config_t *c) {
	cfg = c;
	wifi_apply_mode();
}

void wifi_reconfigure(const wifi_runtime_config_t *c) {
	cfg = c;
	wifi_apply_mode();
}

void wifi_set_provisioning(bool enabled) {
	if (provisioning_enabled == enabled) {
		return;
	}

	provisioning_enabled = enabled;
	wifi_apply_mode();
}

void wifi_loop(void) {
	if (!wifi_has_sta_config()) {
		connected = false;
		return;
	}

	bool now_connected = (WiFi.status() == WL_CONNECTED);

	if (now_connected) {
		if (!connected) {
			Serial.printf("[WiFi] connected: %s\n", WiFi.localIP().toString().c_str());
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
	wifi_begin_sta();
}

bool wifi_sta_is_connected(void) {
	return connected;
}

bool wifi_ap_is_enabled(void) {
	return wifi_ap_should_be_enabled();
}

wifi_network_mode_t wifi_network_mode(void) {
	wifi_mode_t mode = WiFi.getMode();

	if ((mode & WIFI_AP) && (mode & WIFI_STA)) {
		return WIFI_NETWORK_AP_STA;
	}
	if (mode & WIFI_AP) {
		return WIFI_NETWORK_AP;
	}
	if (mode & WIFI_STA) {
		return WIFI_NETWORK_STA;
	}
	return WIFI_NETWORK_OFF;
}

const char *wifi_network_mode_string(void) {
	switch (wifi_network_mode()) {
	case WIFI_NETWORK_AP_STA:
		return "AP + STA";
	case WIFI_NETWORK_AP:
		return "AP";
	case WIFI_NETWORK_STA:
		return "STA";
	default:
		return "off";
	}
}

size_t wifi_sta_ip_string(char *buf, size_t buf_size) {
	if (!connected) {
		if (buf != NULL && buf_size > 0) {
			buf[0] = '\0';
		}
		return 0;
	}
	return copy_ip_string(WiFi.localIP(), buf, buf_size);
}

size_t wifi_ap_ip_string(char *buf, size_t buf_size) {
	if (!wifi_ap_should_be_enabled()) {
		if (buf != NULL && buf_size > 0) {
			buf[0] = '\0';
		}
		return 0;
	}
	return copy_ip_string(WiFi.softAPIP(), buf, buf_size);
}

void wifi_mac_address(uint8_t mac[6]) {
	WiFi.macAddress(mac);
}
