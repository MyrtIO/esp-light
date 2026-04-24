#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ota_config_t {
    const char *hostname;
} ota_config_t;

void ota_init(const ota_config_t *cfg);
void ota_reconfigure(const ota_config_t *cfg);
void ota_loop(void);

#ifdef __cplusplus
}
#endif
