#pragma once

#include "persistent_data.h"
#include "light.h"

void api_init(persistent_data_t *pdata, light_config_t *light_cfg);
void api_start(void);
void api_loop(void);
