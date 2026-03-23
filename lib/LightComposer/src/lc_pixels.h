#pragma once

#include "lc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void lc_fill(lc_state_t *s, rgb_t color);
void lc_fill_range(lc_state_t *s, rgb_t color, uint16_t from, uint16_t to);
void lc_set_pixel(lc_state_t *s, uint16_t idx, rgb_t color);
void lc_mirror(lc_state_t *s);

#ifdef __cplusplus
}
#endif
