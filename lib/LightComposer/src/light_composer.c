#include "light_composer.h"
#include "lc_brightness.h"
#include "lc_math.h"
#include <attotime.h>
#include <string.h>

static const lc_hal_t *hal;
static rgb_t *pixel_buf;
static uint16_t max_leds_count;
static uint16_t led_skip;
static rgb_t color_correction;
static lc_color_order_t color_order;

static lc_state_t state;
static lc_brightness_t brightness = LC_BRIGHTNESS_INIT;
static lc_effect_t *current_effect;
static lc_effect_t *next_effect;
static atto_timer_t frame_deadline = ATTO_TIMER_INIT;
static uint16_t frame_time_ms;

static void recalc_center(void) {
    state.center = state.count / 2;
    if (state.count % 2 != 0) {
        state.center += 1;
    }
}

static void on_effect_switch(void) {
    if (next_effect == NULL) {
        return;
    }
    current_effect = next_effect;
    next_effect = NULL;
    if (current_effect->activate) {
        current_effect->activate(&state, current_effect->ctx);
    }
}

static rgb_t reorder_rgb(rgb_t c, lc_color_order_t order) {
    switch (order) {
        case LC_ORDER_RGB:
            return c;
        case LC_ORDER_RBG:
            return LC_RGB(c.r, c.b, c.g);
        case LC_ORDER_GRB:
            return LC_RGB(c.g, c.r, c.b);
        case LC_ORDER_GBR:
            return LC_RGB(c.g, c.b, c.r);
        case LC_ORDER_BRG:
            return LC_RGB(c.b, c.r, c.g);
        case LC_ORDER_BGR:
            return LC_RGB(c.b, c.g, c.r);
        default:
            return c;
    }
}

static bool strip_dirty;

static void flush(void) {
    uint8_t br = lc_brightness_get_value(&brightness);
    bool apply_corr =
        current_effect && (current_effect->flags & LC_EFFECT_FLAG_COLOR_CORRECTION);

    if (strip_dirty) {
        rgb_t black = LC_RGB(0, 0, 0);
        for (uint16_t i = 0; i < max_leds_count; i++) {
            hal->set_pixel(i, black);
        }
        strip_dirty = false;
    }

    for (uint16_t i = 0; i < state.count; i++) {
        rgb_t px = pixel_buf[i];
        px = LC_RGB(lc_scale8(px.r, br), lc_scale8(px.g, br), lc_scale8(px.b, br));
        if (apply_corr) {
            px = LC_RGB(lc_scale8(px.r, color_correction.r),
                        lc_scale8(px.g, color_correction.g),
                        lc_scale8(px.b, color_correction.b));
        }
        px = reorder_rgb(px, color_order);
        hal->set_pixel(i + led_skip, px);
    }

    hal->show();
}

void lc_init(const lc_config_t *cfg) {
    hal = cfg->hal;
    pixel_buf = cfg->pixel_buf;
    max_leds_count = cfg->max_leds_count;
    led_skip = cfg->led_skip;
    color_correction = cfg->color_correction;
    color_order = cfg->color_order;

    state.pixels = pixel_buf;
    state.count = cfg->leds_count;
    recalc_center();
    state.previous_color = LC_RGB_BLACK;
    state.current_color = LC_RGB_BLACK;
    state.target_color = LC_RGB_BLACK;
    state.transition_ms = 0;

    memset(pixel_buf, 0, sizeof(rgb_t) * max_leds_count);

    lc_brightness_init(&brightness);
    current_effect = NULL;
    next_effect = NULL;

    frame_time_ms = cfg->fps > 0 ? (1000 / cfg->fps) : 16;
    atto_timer_start(&frame_deadline, frame_time_ms);
}

void lc_tick(void) {
    if (!atto_timer_finished(&frame_deadline)) {
        return;
    }

    bool changed = false;

    changed |= lc_brightness_tick(&brightness, on_effect_switch);

    if (current_effect && current_effect->render) {
        changed |= current_effect->render(&state, current_effect->ctx);
    }

    if (changed || strip_dirty) {
        flush();
    }

    atto_timer_start(&frame_deadline, frame_time_ms);
}

/* Brightness / power */

void lc_set_brightness(uint8_t value) {
    lc_brightness_set(&brightness, value);
}

void lc_set_power(bool enabled) {
    lc_brightness_set_power(&brightness, enabled);
}

uint8_t lc_get_brightness(void) {
    return lc_brightness_get(&brightness);
}

bool lc_get_power(void) {
    return lc_brightness_get_power(&brightness);
}

/* Color */

void lc_set_color(rgb_t color) {
    state.previous_color = state.current_color;
    state.target_color = color;
    if (current_effect && current_effect->color_update) {
        current_effect->color_update(&state, current_effect->ctx);
    }
}

rgb_t lc_get_color(void) {
    return state.target_color;
}

/* Effect */

void lc_set_effect(lc_effect_t *effect) {
    if (current_effect == NULL) {
        current_effect = effect;
        if (current_effect->activate) {
            current_effect->activate(&state, current_effect->ctx);
        }
        return;
    }
    next_effect = effect;
    lc_brightness_request_effect_change(&brightness);
}

void lc_set_effect_force(lc_effect_t *effect) {
    current_effect = effect;
    next_effect = NULL;
    if (current_effect && current_effect->activate) {
        current_effect->activate(&state, current_effect->ctx);
    }
}

const lc_effect_t *lc_get_effect(void) {
    return current_effect;
}

const lc_effect_t *lc_get_target_effect(void) {
    return next_effect != NULL ? next_effect : current_effect;
}

/* Transition */

void lc_set_transition(uint16_t ms) {
    state.transition_ms = ms;
}

/* Runtime parameters */

void lc_set_leds_count(uint16_t count) {
    uint16_t limit = max_leds_count - led_skip;
    if (count > limit) {
        count = limit;
    }
    uint16_t old_count = state.count;
    state.count = count;
    recalc_center();
    if (count < old_count) {
        memset(&pixel_buf[count], 0, sizeof(rgb_t) * (old_count - count));
        strip_dirty = true;
    }
}

void lc_set_led_skip(uint16_t skip) {
    if (skip != led_skip) {
        strip_dirty = true;
    }
    led_skip = skip;
}

void lc_set_color_correction(rgb_t corr) {
    color_correction = corr;
}

void lc_set_color_order(lc_color_order_t order) {
    color_order = order;
}
