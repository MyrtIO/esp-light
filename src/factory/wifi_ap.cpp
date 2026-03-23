#include "wifi_ap.h"
#include <Arduino.h>
#include <WiFi.h>
#include "device_id.h"

void wifi_ap_init(void) {
	WiFi.mode(WIFI_AP);
	WiFi.softAP(device_name());
	Serial.printf("[WiFi] AP started: %s\n", device_name());
	Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
}
