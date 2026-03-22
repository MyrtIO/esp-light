#pragma once

#include <LightComposer/lc_types.h>
#include <LightComposer/math/lc_scale.h>
#include <LightComposer/math/lc_blend.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline rgb_t lc_scale_rgb(rgb_t c, uint8_t s) {
    return LC_RGB(lc_scale8(c.r, s), lc_scale8(c.g, s), lc_scale8(c.b, s));
}

static inline rgb_t lc_apply_correction(rgb_t pixel, rgb_t corr) {
    return LC_RGB(
        lc_scale8(pixel.r, corr.r),
        lc_scale8(pixel.g, corr.g),
        lc_scale8(pixel.b, corr.b)
    );
}

static inline rgb_t lc_blend_rgb(rgb_t a, rgb_t b, lc_fract_t frac) {
    return LC_RGB(
        lc_blend8(a.r, b.r, frac),
        lc_blend8(a.g, b.g, frac),
        lc_blend8(a.b, b.b, frac)
    );
}

#ifdef __cplusplus
}
#endif
