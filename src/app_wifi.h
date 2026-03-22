#pragma once

struct app_wifi_config_t {
	const char *ssid;
	const char *password;
	const char *hostname;
	unsigned long reconnect_interval_ms;
};

void wifi_init(const app_wifi_config_t *cfg);
void wifi_loop(void);
bool wifi_is_connected(void);
