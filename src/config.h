#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_MQTT_BUFFER_SIZE 1024
#define CONFIG_MQTT_RECONNECT_DELAY_MS 5000
#define CONFIG_PROVISIONING_HOLD_MS 3000
#define CONFIG_PROVISIONING_BUTTON_GPIO 0
#define CONFIG_LIGHT_LED_MAX_COUNT 256
#define CONFIG_LIGHT_COLOR_KELVIN_INITIAL 2857
#define CONFIG_LIGHT_COLOR_KELVIN_WARM 2000
#define CONFIG_LIGHT_COLOR_KELVIN_COLD 7407
#define CONFIG_LIGHT_TRANSITION_COLOR 300

#ifndef CONFIG_LIGHT_CTL_GPIO
#define CONFIG_LIGHT_CTL_GPIO 25
#endif

void device_id_init();
const char *device_id();
const char *device_hostname();
const char *device_name();

#ifdef __cplusplus
}
#endif
