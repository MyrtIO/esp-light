#pragma once

#include <IPAddress.h>
#include <stdint.h>

enum wifi_network_mode_t {
    WIFI_NETWORK_OFF,
    WIFI_NETWORK_AP,
    WIFI_NETWORK_STA,
    WIFI_NETWORK_AP_STA,
};

struct wifi_runtime_config_t {
    const char *ssid;
    const char *password;
    const char *hostname;
    const char *ap_ssid;
    unsigned long reconnect_interval_ms;
};

void wifi_init(const wifi_runtime_config_t *cfg);
void wifi_reconfigure(const wifi_runtime_config_t *cfg);
void wifi_set_provisioning(bool enabled);
void wifi_loop(void);
bool wifi_sta_is_connected(void);
bool wifi_ap_is_enabled(void);
wifi_network_mode_t wifi_network_mode(void);
const char *wifi_network_mode_string(void);
IPAddress wifi_sta_ip(void);
IPAddress wifi_ap_ip(void);
void wifi_mac_address(uint8_t mac[6]);
