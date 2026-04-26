#include "persistent_data.h"
#include "config.h"
#include <cpreferences.h>
#include <esp_log.h>
#include <string.h>

#define NVS_NAMESPACE "config"
#define NVS_LIGHT_STATE "light_state"

static const char *TAG = "Persistent";

static void persistent_data_set_defaults(persistent_data_t *data) {
    memset(data, 0, sizeof(persistent_data_t));
    data->mqtt_port = 1883;
    data->led_count = 128;
    data->skip_leds = 0;
    data->color_correction = 0xFFFFFF;
    data->brightness_min = 10;
    data->brightness_max = 255;
    data->color_order = 2;
}

static void light_state_set_defaults(light_saved_state_t *state) {
    memset(state, 0, sizeof(light_saved_state_t));
    state->power = true;
    state->brightness = 200;
    state->r = 255;
    state->g = 255;
    state->b = 255;
    strcpy(state->effect, "static");
}

static void load_string(cpreferences_t *prefs,
                        const char *key,
                        char *dst,
                        size_t max_len) {
    cpreferences_get_string(prefs, key, dst, max_len, "");
}

static bool cpreferences_begin_read_or_create(cpreferences_t *prefs,
                                              const char *namespace_name) {
    if (cpreferences_begin(prefs, namespace_name, true)) {
        return true;
    }

    return cpreferences_begin(prefs, namespace_name, false);
}

void persistent_data_sanitize(persistent_data_t *data) {
    if (data == NULL) {
        return;
    }

    if (data->led_count == 0) {
        data->led_count = 1;
    }

    if (data->skip_leds >= CONFIG_LIGHT_LED_MAX_COUNT) {
        data->skip_leds = CONFIG_LIGHT_LED_MAX_COUNT - 1;
    }

    uint16_t available_leds = CONFIG_LIGHT_LED_MAX_COUNT - data->skip_leds;
    if (available_leds == 0) {
        data->skip_leds = 0;
        available_leds = CONFIG_LIGHT_LED_MAX_COUNT;
    }

    if (data->led_count > available_leds) {
        data->led_count = available_leds;
    }

    if (data->brightness_min > data->brightness_max) {
        data->brightness_min = data->brightness_max;
    }
}

void persistent_data_load(persistent_data_t *data) {
    persistent_data_set_defaults(data);

    cpreferences_t *prefs = cpreferences_create();
    if (prefs == NULL) {
        ESP_LOGW(TAG, "preferences create failed during load, using defaults");
        return;
    }

    if (!cpreferences_begin_read_or_create(prefs, NVS_NAMESPACE)) {
        ESP_LOGW(TAG, "failed to open namespace '%s', using defaults",
                 NVS_NAMESPACE);
        cpreferences_destroy(prefs);
        return;
    }

    load_string(prefs, "wifi_ssid", data->wifi_ssid, sizeof(data->wifi_ssid));
    load_string(prefs, "wifi_pass", data->wifi_password,
                sizeof(data->wifi_password));
    load_string(prefs, "mqtt_host", data->mqtt_host, sizeof(data->mqtt_host));
    load_string(prefs, "mqtt_user", data->mqtt_username,
                sizeof(data->mqtt_username));
    load_string(prefs, "mqtt_pass", data->mqtt_password,
                sizeof(data->mqtt_password));

    data->mqtt_port = cpreferences_get_ushort(prefs, "mqtt_port", 1883);
    data->led_count = cpreferences_get_ushort(prefs, "led_count", 128);
    data->skip_leds = cpreferences_get_ushort(prefs, "skip_leds", 0);
    data->color_correction = cpreferences_get_ulong(prefs, "color_corr", 0xFFFFFF);
    data->brightness_min = cpreferences_get_uchar(prefs, "bright_min", 10);
    data->brightness_max = cpreferences_get_uchar(prefs, "bright_max", 255);
    data->color_order = cpreferences_get_uchar(prefs, "color_order", 2);
    persistent_data_sanitize(data);

    cpreferences_destroy(prefs);
}

void persistent_data_save(const persistent_data_t *data) {
    persistent_data_t copy = *data;
    persistent_data_sanitize(&copy);

    cpreferences_t *prefs = cpreferences_create();
    if (prefs == NULL) {
        ESP_LOGW(TAG, "preferences create failed during save");
        return;
    }

    if (!cpreferences_begin(prefs, NVS_NAMESPACE, false)) {
        ESP_LOGW(TAG, "failed to open namespace '%s' for write", NVS_NAMESPACE);
        cpreferences_destroy(prefs);
        return;
    }

    cpreferences_put_string(prefs, "wifi_ssid", copy.wifi_ssid);
    cpreferences_put_string(prefs, "wifi_pass", copy.wifi_password);
    cpreferences_put_string(prefs, "mqtt_host", copy.mqtt_host);
    cpreferences_put_string(prefs, "mqtt_user", copy.mqtt_username);
    cpreferences_put_string(prefs, "mqtt_pass", copy.mqtt_password);
    cpreferences_put_ushort(prefs, "mqtt_port", copy.mqtt_port);
    cpreferences_put_ushort(prefs, "led_count", copy.led_count);
    cpreferences_put_ushort(prefs, "skip_leds", copy.skip_leds);
    cpreferences_put_ulong(prefs, "color_corr", copy.color_correction);
    cpreferences_put_uchar(prefs, "bright_min", copy.brightness_min);
    cpreferences_put_uchar(prefs, "bright_max", copy.brightness_max);
    cpreferences_put_uchar(prefs, "color_order", copy.color_order);

    cpreferences_destroy(prefs);
}

bool persistent_data_is_configured(const persistent_data_t *data) {
    return data->wifi_ssid[0] != '\0' && data->mqtt_host[0] != '\0';
}

bool light_state_exists(void) {
    cpreferences_t *prefs = cpreferences_create();
    if (prefs == NULL) {
        ESP_LOGW(TAG, "preferences create failed while checking light state");
        return false;
    }

    if (!cpreferences_begin_read_or_create(prefs, NVS_LIGHT_STATE)) {
        ESP_LOGW(TAG, "failed to open namespace '%s' while checking light state",
                 NVS_LIGHT_STATE);
        cpreferences_destroy(prefs);
        return false;
    }

    bool exists = cpreferences_is_key(prefs, "brightness");
    cpreferences_destroy(prefs);
    return exists;
}

void light_state_load(light_saved_state_t *state) {
    light_state_set_defaults(state);

    cpreferences_t *prefs = cpreferences_create();
    if (prefs == NULL) {
        ESP_LOGW(
            TAG,
            "preferences create failed during light state load, using defaults");
        return;
    }

    if (!cpreferences_begin_read_or_create(prefs, NVS_LIGHT_STATE)) {
        ESP_LOGW(TAG, "failed to open namespace '%s', using default light state",
                 NVS_LIGHT_STATE);
        cpreferences_destroy(prefs);
        return;
    }

    state->power = cpreferences_get_bool(prefs, "power", true);
    state->brightness = cpreferences_get_uchar(prefs, "brightness", 200);
    state->r = cpreferences_get_uchar(prefs, "r", 255);
    state->g = cpreferences_get_uchar(prefs, "g", 255);
    state->b = cpreferences_get_uchar(prefs, "b", 255);
    state->color_temp = cpreferences_get_ushort(prefs, "color_temp", 0);
    state->color_mode = cpreferences_get_uchar(prefs, "color_mode", 0);
    cpreferences_get_string(prefs, "effect", state->effect, sizeof(state->effect),
                            "static");

    cpreferences_destroy(prefs);
}

void light_state_save(const light_saved_state_t *state) {
    cpreferences_t *prefs = cpreferences_create();
    if (prefs == NULL) {
        ESP_LOGW(TAG, "preferences create failed during light state save");
        return;
    }

    if (!cpreferences_begin(prefs, NVS_LIGHT_STATE, false)) {
        ESP_LOGW(TAG, "failed to open namespace '%s' for light state write",
                 NVS_LIGHT_STATE);
        cpreferences_destroy(prefs);
        return;
    }

    cpreferences_put_bool(prefs, "power", state->power);
    cpreferences_put_uchar(prefs, "brightness", state->brightness);
    cpreferences_put_uchar(prefs, "r", state->r);
    cpreferences_put_uchar(prefs, "g", state->g);
    cpreferences_put_uchar(prefs, "b", state->b);
    cpreferences_put_ushort(prefs, "color_temp", state->color_temp);
    cpreferences_put_uchar(prefs, "color_mode", state->color_mode);
    cpreferences_put_string(prefs, "effect", state->effect);

    cpreferences_destroy(prefs);
}
