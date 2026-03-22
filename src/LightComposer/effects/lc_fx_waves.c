#include "lc_fx_waves.h"
#include <LightComposer/lc_pixels.h>
#include <LightComposer/math/lc_scale.h>
#include <LightComposer/lc_types.h>
#include <attotime.h>

#define WAVES_CYCLE_MS   1000
#define WAVES_FILL_FRACT 200

typedef struct {
    atto_progress_t progress;
    bool is_reverse;
    uint8_t fill_size;
    uint16_t max_offset;
} waves_ctx_t;

static bool waves_render(lc_state_t *s, void *raw) {
    waves_ctx_t *c = (waves_ctx_t *)raw;

    uint8_t shift = lc_scale8((uint8_t)c->max_offset, atto_progress_get(&c->progress));

    if (!LC_RGB_EQ(s->current_color, s->target_color)) {
        s->current_color = s->target_color;
    }

    lc_fill(s, LC_RGB_BLACK);
    uint8_t start = c->is_reverse ? (uint8_t)(c->max_offset - shift) : shift;
    for (uint8_t i = 0; i < c->fill_size; i++) {
        lc_set_pixel(s, start + i, s->current_color);
    }

    if (atto_progress_finished(&c->progress)) {
        c->is_reverse = !c->is_reverse;
        atto_progress_start(&c->progress, WAVES_CYCLE_MS);
    }

    return true;
}

static void waves_activate(lc_state_t *s, void *raw) {
    waves_ctx_t *c = (waves_ctx_t *)raw;
    c->is_reverse = false;
    c->fill_size = lc_scale8((uint8_t)(s->count - 1), WAVES_FILL_FRACT);
    c->max_offset = s->count - 1 - c->fill_size;
    atto_progress_set_max(&c->progress, 255);
    atto_progress_start(&c->progress, WAVES_CYCLE_MS);
}

static void waves_color_update(lc_state_t *s, void *raw) {
    waves_ctx_t *c = (waves_ctx_t *)raw;
    s->current_color = s->target_color;
    atto_progress_start(&c->progress, s->transition_ms);
}

LC_EFFECT_DEFINE(waves, LC_EFFECT_FLAG_NONE, waves_ctx_t);
