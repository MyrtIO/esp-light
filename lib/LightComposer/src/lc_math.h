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

typedef uint8_t lc_fract_t;

static inline uint8_t lc_blend8(uint8_t a, uint8_t b, lc_fract_t amount_of_b) {
    uint16_t partial;
    uint8_t result;

    partial = (a << 8) | b;
    partial += (b * amount_of_b);
    partial -= (a * amount_of_b);
    result = partial >> 8;

    return result;
}

static inline uint8_t lc_lerp8by8(uint8_t a, uint8_t b, lc_fract_t frac) {
    uint8_t result;
    if (b > a) {
        uint8_t delta = b - a;
        uint8_t scaled = lc_scale8(delta, frac);
        result = a + scaled;
    } else {
        uint8_t delta = a - b;
        uint8_t scaled = lc_scale8(delta, frac);
        result = a - scaled;
    }
    return result;
}

#ifdef __cplusplus
}
#endif
