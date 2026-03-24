#include <lc_types.h>
#include <lc_pixels.h>
#include <lc_math.h>
#include <lc_types.h>
#include <attotime.h>

typedef struct {
    atto_progress_t progress;
} fill_ctx_t;

static bool fill_render(lc_state_t *s, void *raw) {
    fill_ctx_t *c = (fill_ctx_t *)raw;

    if (LC_RGB_EQ(s->current_color, s->target_color)) {
        return false;
    }

    if (atto_progress_finished(&c->progress)) {
        s->current_color = s->target_color;
        lc_fill(s, s->current_color);
        return true;
    }

    uint16_t center = s->center;
    uint8_t fill_size = lc_scale8((uint8_t)center, atto_progress_get(&c->progress));

    for (uint8_t i = 0; i < fill_size; i++) {
        lc_set_pixel(s, center - i, s->target_color);
    }

    lc_mirror(s);
    return true;
}

static void fill_activate(lc_state_t *s, void *raw) {
    fill_ctx_t *c = (fill_ctx_t *)raw;
    atto_progress_set_max(&c->progress, 255);
    (void)s;
}

static void fill_color_update(lc_state_t *s, void *raw) {
    fill_ctx_t *c = (fill_ctx_t *)raw;
    atto_progress_start(&c->progress, s->transition_ms);
}

LC_EFFECT_DEFINE(fill, LC_EFFECT_FLAG_COLOR_CORRECTION, fill_ctx_t);
