#include "persistent_data.h"
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <Arduino.h>
#include <string.h>

#define NVS_NAMESPACE "config"

static void load_string(Preferences &prefs, const char *key, char *dst, size_t max_len) {
	String val = prefs.getString(key, "");
	strncpy(dst, val.c_str(), max_len - 1);
	dst[max_len - 1] = '\0';
}

void persistent_data_load(persistent_data_t *data) {
	memset(data, 0, sizeof(persistent_data_t));

	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, true);

	load_string(prefs, "wifi_ssid",   data->wifi_ssid,     sizeof(data->wifi_ssid));
	load_string(prefs, "wifi_pass",   data->wifi_password,  sizeof(data->wifi_password));
	load_string(prefs, "mqtt_host",   data->mqtt_host,      sizeof(data->mqtt_host));
	load_string(prefs, "mqtt_user",   data->mqtt_username,  sizeof(data->mqtt_username));
	load_string(prefs, "mqtt_pass",   data->mqtt_password,  sizeof(data->mqtt_password));

	data->mqtt_port        = prefs.getUShort("mqtt_port",    1883);
	data->led_count        = prefs.getUShort("led_count",    1);
	data->skip_leds        = prefs.getUShort("skip_leds",    0);
	data->color_correction = prefs.getULong("color_corr",    0xFFFFFF);
	data->brightness_min   = prefs.getUChar("bright_min",    10);
	data->brightness_max   = prefs.getUChar("bright_max",    255);
	data->color_order      = prefs.getUChar("color_order",   2); // GRB

	prefs.end();
}

void persistent_data_save(const persistent_data_t *data) {
	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, false);

	prefs.putString("wifi_ssid",  data->wifi_ssid);
	prefs.putString("wifi_pass",  data->wifi_password);
	prefs.putString("mqtt_host",  data->mqtt_host);
	prefs.putString("mqtt_user",  data->mqtt_username);
	prefs.putString("mqtt_pass",  data->mqtt_password);
	prefs.putUShort("mqtt_port",  data->mqtt_port);
	prefs.putUShort("led_count",  data->led_count);
	prefs.putUShort("skip_leds",  data->skip_leds);
	prefs.putULong("color_corr",  data->color_correction);
	prefs.putUChar("bright_min",  data->brightness_min);
	prefs.putUChar("bright_max",  data->brightness_max);
	prefs.putUChar("color_order", data->color_order);

	prefs.end();
}

bool persistent_data_is_configured(const persistent_data_t *data) {
	return data->wifi_ssid[0] != '\0';
}

void boot_to_factory(void) {
	const esp_partition_t *p = esp_partition_find_first(
		ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
	if (p) {
		esp_ota_set_boot_partition(p);
	}
	esp_restart();
}

void boot_to_app(void) {
	const esp_partition_t *p = esp_partition_find_first(
		ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
	if (p) {
		esp_ota_set_boot_partition(p);
	}
	esp_restart();
}
