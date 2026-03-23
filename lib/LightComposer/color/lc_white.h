#pragma once

#include <lc_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t kelvin_t;

void lc_kelvin_to_rgb(kelvin_t temperature, rgb_t *out);

#ifdef __cplusplus
}
#endif
