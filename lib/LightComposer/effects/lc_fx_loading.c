#include "lc_fx_loading.h"
#include <lc_pixels.h>
#include <math/lc_scale.h>
#include <lc_types.h>
#include <attotime.h>

#define LOADING_CYCLE_MS   1000
#define LOADING_FILL_FRACT 200

typedef struct {
    atto_progress_t progress;
    bool is_reverse;
    uint8_t fill_size;
    uint16_t max_offset;
} loading_ctx_t;

static bool loading_render(lc_state_t *s, void *raw) {
    loading_ctx_t *c = (loading_ctx_t *)raw;

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
        atto_progress_start(&c->progress, LOADING_CYCLE_MS);
    }

    return true;
}

static void loading_activate(lc_state_t *s, void *raw) {
    loading_ctx_t *c = (loading_ctx_t *)raw;
    c->is_reverse = false;
    c->fill_size = lc_scale8((uint8_t)(s->count - 1), LOADING_FILL_FRACT);
    c->max_offset = s->count - 1 - c->fill_size;
    atto_progress_set_max(&c->progress, 255);
    atto_progress_start(&c->progress, LOADING_CYCLE_MS);
}

static void loading_color_update(lc_state_t *s, void *raw) {
    loading_ctx_t *c = (loading_ctx_t *)raw;
    s->current_color = s->target_color;
    atto_progress_start(&c->progress, s->transition_ms);
}

LC_EFFECT_DEFINE(loading, LC_EFFECT_FLAG_NONE, loading_ctx_t);
