#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r, g, b;
} rgb_t;

typedef struct {
    uint8_t h, s, v;
} hsv_t;

#define LC_RGB(rv, gv, bv) ((rgb_t){(uint8_t)(rv), (uint8_t)(gv), (uint8_t)(bv)})
#define LC_RGB_BLACK LC_RGB(0, 0, 0)
#define LC_RGB_WHITE LC_RGB(255, 255, 255)

#define LC_RGB_FROM_CODE(c) \
    LC_RGB(((c) >> 16) & 0xFF, ((c) >> 8) & 0xFF, ((c)) & 0xFF)

#define LC_RGB_EQ(c1, c2) ((c1).r == (c2).r && (c1).g == (c2).g && (c1).b == (c2).b)

typedef enum {
    LC_ORDER_RGB,
    LC_ORDER_RBG,
    LC_ORDER_GRB,
    LC_ORDER_GBR,
    LC_ORDER_BRG,
    LC_ORDER_BGR,
} lc_color_order_t;

typedef struct {
    rgb_t *pixels;
    uint16_t count;
    uint16_t center;
    rgb_t previous_color;
    rgb_t current_color;
    rgb_t target_color;
    uint16_t transition_ms;
} lc_state_t;

typedef struct {
    void (*set_pixel)(uint16_t index, rgb_t color);
    rgb_t (*get_pixel)(uint16_t index);
    void (*show)(void);
} lc_hal_t;

#define LC_EFFECT_FLAG_NONE 0
#define LC_EFFECT_FLAG_COLOR_CORRECTION (1 << 0)

typedef struct {
    bool (*render)(lc_state_t *state, void *ctx);
    void (*activate)(lc_state_t *state, void *ctx);
    void (*color_update)(lc_state_t *state, void *ctx);
    void *ctx;
    uint8_t flags;
} lc_effect_t;

#define LC_EFFECT_DECLARE(name) extern lc_effect_t lc_fx_##name

#define LC_EFFECT_DEFINE(name, flags_val, ctx_type) \
    static ctx_type name##_ctx;                     \
    lc_effect_t lc_fx_##name = {                    \
        .render = name##_render,                    \
        .activate = name##_activate,                \
        .color_update = name##_color_update,        \
        .ctx = &name##_ctx,                         \
        .flags = (flags_val),                       \
    }

#ifdef __cplusplus
}
#endif
