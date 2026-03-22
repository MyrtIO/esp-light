#pragma once

#include <stdint.h>
#include <LightComposer/lc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t mireds_t;
typedef uint16_t kelvin_t;

static inline kelvin_t lc_mireds_to_kelvin(mireds_t mireds) {
    return 1000000 / mireds;
}

static inline mireds_t lc_kelvin_to_mireds(kelvin_t kelvin) {
    return 1000000 / kelvin;
}

void lc_kelvin_to_rgb(kelvin_t temperature, rgb_t *out);

#ifdef __cplusplus
}
#endif
