#pragma once

#include "lc_math.h"
#include "lc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline rgb_t lc_scale_rgb(rgb_t c, uint8_t s) {
    return LC_RGB(lc_scale8(c.r, s), lc_scale8(c.g, s), lc_scale8(c.b, s));
}

static inline rgb_t lc_apply_correction(rgb_t pixel, rgb_t corr) {
    return LC_RGB(lc_scale8(pixel.r, corr.r), lc_scale8(pixel.g, corr.g),
                  lc_scale8(pixel.b, corr.b));
}

static inline rgb_t lc_blend_rgb(rgb_t a, rgb_t b, lc_fract_t frac) {
    return LC_RGB(lc_blend8(a.r, b.r, frac), lc_blend8(a.g, b.g, frac),
                  lc_blend8(a.b, b.b, frac));
}

void lc_hsv2rgb(hsv_t hsv, rgb_t *out);

typedef uint16_t kelvin_t;

void lc_kelvin_to_rgb(kelvin_t temperature, rgb_t *out);

typedef enum {
    LC_GRADIENT_FORWARD,
    LC_GRADIENT_BACKWARD,
    LC_GRADIENT_SHORTEST,
    LC_GRADIENT_LONGEST,
} lc_gradient_dir_t;

static inline void lc_fill_gradient(rgb_t *buf,
                                    uint16_t start_pos,
                                    hsv_t start_color,
                                    uint16_t end_pos,
                                    hsv_t end_color,
                                    lc_gradient_dir_t dir) {
    if (end_pos < start_pos) {
        uint16_t tp = end_pos;
        hsv_t tc = end_color;
        end_color = start_color;
        end_pos = start_pos;
        start_pos = tp;
        start_color = tc;
    }

    if (end_color.v == 0 || end_color.s == 0) {
        end_color.h = start_color.h;
    }
    if (start_color.v == 0 || start_color.s == 0) {
        start_color.h = end_color.h;
    }

    int16_t hue_distance87;
    int16_t sat_distance87;
    int16_t val_distance87;

    sat_distance87 = (end_color.s - start_color.s) << 7;
    val_distance87 = (end_color.v - start_color.v) << 7;

    uint8_t hue_delta = end_color.h - start_color.h;

    if (dir == LC_GRADIENT_SHORTEST) {
        dir = (hue_delta > 127) ? LC_GRADIENT_BACKWARD : LC_GRADIENT_FORWARD;
    }
    if (dir == LC_GRADIENT_LONGEST) {
        dir = (hue_delta < 128) ? LC_GRADIENT_BACKWARD : LC_GRADIENT_FORWARD;
    }

    if (dir == LC_GRADIENT_FORWARD) {
        hue_distance87 = hue_delta << 7;
    } else {
        hue_distance87 = (uint8_t)(256 - hue_delta) << 7;
        hue_distance87 = -hue_distance87;
    }

    uint16_t pixel_distance = end_pos - start_pos;
    int16_t divisor = pixel_distance ? pixel_distance : 1;

    int32_t hue_delta823 = (hue_distance87 * 65536) / divisor;
    int32_t sat_delta823 = (sat_distance87 * 65536) / divisor;
    int32_t val_delta823 = (val_distance87 * 65536) / divisor;

    hue_delta823 *= 2;
    sat_delta823 *= 2;
    val_delta823 *= 2;

    uint32_t hue824 = (uint32_t)(start_color.h) << 24;
    uint32_t sat824 = (uint32_t)(start_color.s) << 24;
    uint32_t val824 = (uint32_t)(start_color.v) << 24;

    for (uint16_t i = start_pos; i <= end_pos; ++i) {
        hsv_t c;
        c.h = (uint8_t)(hue824 >> 24);
        c.s = (uint8_t)(sat824 >> 24);
        c.v = (uint8_t)(val824 >> 24);
        lc_hsv2rgb(c, &buf[i]);
        hue824 += hue_delta823;
        sat824 += sat_delta823;
        val824 += val_delta823;
    }
}

static inline void lc_fill_gradient3(rgb_t *buf,
                                     uint16_t count,
                                     hsv_t c1,
                                     hsv_t c2,
                                     hsv_t c3,
                                     lc_gradient_dir_t dir) {
    uint16_t half = count / 2;
    uint16_t last = count - 1;
    lc_fill_gradient(buf, 0, c1, half, c2, dir);
    lc_fill_gradient(buf, half, c2, last, c3, dir);
}

#ifdef __cplusplus
}
#endif
