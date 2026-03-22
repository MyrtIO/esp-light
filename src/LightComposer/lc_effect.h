#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LC_EFFECT_FLAG_NONE             0
#define LC_EFFECT_FLAG_COLOR_CORRECTION (1 << 0)

typedef struct {
    bool (*render)(lc_state_t *state, void *ctx);
    void (*activate)(lc_state_t *state, void *ctx);
    void (*color_update)(lc_state_t *state, void *ctx);
    void    *ctx;
    uint8_t  flags;
} lc_effect_t;

#define LC_EFFECT_DECLARE(name) extern lc_effect_t lc_fx_##name

#define LC_EFFECT_DEFINE(name, flags_val, ctx_type) \
    static ctx_type name##_ctx;                     \
    lc_effect_t lc_fx_##name = {                    \
        .render       = name##_render,              \
        .activate     = name##_activate,            \
        .color_update = name##_color_update,        \
        .ctx          = &name##_ctx,                \
        .flags        = (flags_val),                \
    }

#ifdef __cplusplus
}
#endif
