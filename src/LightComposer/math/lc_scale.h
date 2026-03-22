#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t lc_scale_t;

static inline uint8_t lc_scale8(uint8_t x, lc_scale_t scale) {
    return (((uint16_t)x) * (1 + (uint16_t)(scale))) >> 8;
}

static inline uint8_t lc_scale8_video(uint8_t x, lc_scale_t scale) {
    return (((int)x * (int)scale) >> 8) + ((x && scale) ? 1 : 0);
}

#ifdef __cplusplus
}
#endif
