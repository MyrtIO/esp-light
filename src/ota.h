#pragma once

#include <stdint.h>

struct ota_config_t {
	const char *hostname;
	uint16_t port;
};

void ota_init(const ota_config_t *cfg);
void ota_loop(void);
