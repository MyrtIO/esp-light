#include "lc_brightness.h"
#include "math/lc_blend.h"

#define BRIGHTNESS_TRANSITION_MS 800

void lc_brightness_init(lc_brightness_t *br) {
    br->enabled = true;
    br->effect_switched = false;
    br->previous = 0;
    br->current = 0;
    br->target = 0;
    br->selected = 0;
    br->reason = LC_BR_REASON_BRIGHTNESS;
    atto_progress_reset(&br->transition);
}

static uint8_t map_range(uint8_t x, uint8_t in_min, uint8_t in_max,
                         uint8_t out_min, uint8_t out_max) {
    if (in_max == in_min) return out_min;
    return (uint8_t)(((uint16_t)(x - in_min) * (out_max - out_min)) /
                     (in_max - in_min) + out_min);
}

bool lc_brightness_tick(lc_brightness_t *br, lc_effect_switch_fn on_switch) {
    if (atto_progress_finished(&br->transition) && br->current == br->target) {
        return false;
    }

    uint8_t progress = atto_progress_get(&br->transition);

    if (br->reason == LC_BR_REASON_EFFECT) {
        if (progress < 128) {
            progress = map_range(progress, 0, 127, 0, 255);
            br->previous = br->selected;
            br->target = 0;
        } else {
            if (!br->effect_switched) {
                if (on_switch) {
                    on_switch();
                }
                br->effect_switched = true;
            }
            progress = map_range(progress, 128, 255, 0, 255);
            br->previous = 0;
            br->target = br->selected;
        }
    }

    br->current = lc_lerp8by8(br->previous, br->target, progress);
    return true;
}

uint8_t lc_brightness_get_value(const lc_brightness_t *br) {
    return br->current;
}

void lc_brightness_set(lc_brightness_t *br, uint8_t brightness) {
    if (br->selected == brightness) {
        return;
    }
    br->previous = br->selected;
    br->selected = brightness;
    br->target = brightness;
    br->reason = LC_BR_REASON_BRIGHTNESS;
    atto_progress_start(&br->transition, BRIGHTNESS_TRANSITION_MS);
}

void lc_brightness_set_power(lc_brightness_t *br, bool enabled) {
    if (br->enabled == enabled) {
        return;
    }
    br->enabled = enabled;
    if (enabled) {
        br->previous = 0;
        br->target = br->selected;
    } else {
        br->previous = br->selected;
        br->target = 0;
    }
    br->reason = LC_BR_REASON_POWER;
    atto_progress_start(&br->transition, BRIGHTNESS_TRANSITION_MS);
}

void lc_brightness_request_effect_change(lc_brightness_t *br) {
    br->effect_switched = false;
    br->reason = LC_BR_REASON_EFFECT;
    atto_progress_start(&br->transition, BRIGHTNESS_TRANSITION_MS);
}

uint8_t lc_brightness_get(const lc_brightness_t *br) {
    return br->selected;
}

bool lc_brightness_get_power(const lc_brightness_t *br) {
    return br->enabled;
}
