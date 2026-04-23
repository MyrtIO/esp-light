#pragma once

struct ota_config_t {
    const char *hostname;
};

void ota_init(const ota_config_t *cfg);
void ota_reconfigure(const ota_config_t *cfg);
void ota_loop(void);
