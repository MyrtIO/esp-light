#include "lc_fx_static.h"
#include <lc_pixels.h>
#include <color/lc_color.h>
#include <attotime.h>

typedef struct {
    atto_progress_t progress;
    bool force_update;
} static_ctx_t;

static bool static_render(lc_state_t *s, void *raw) {
    static_ctx_t *c = (static_ctx_t *)raw;

    if (c->force_update) {
        lc_fill(s, s->target_color);
        s->current_color = s->target_color;
        c->force_update = false;
        return true;
    }

    if (LC_RGB_EQ(s->current_color, s->target_color)) {
        return true;
    }

    s->current_color = lc_blend_rgb(
        s->previous_color, s->target_color, atto_progress_get(&c->progress)
    );
    lc_fill(s, s->current_color);

    if (atto_progress_finished(&c->progress)) {
        s->current_color = s->target_color;
    }

    return true;
}

static void static_activate(lc_state_t *s, void *raw) {
    static_ctx_t *c = (static_ctx_t *)raw;
    c->force_update = true;
    atto_progress_set_max(&c->progress, 255);
    (void)s;
}

static void static_color_update(lc_state_t *s, void *raw) {
    static_ctx_t *c = (static_ctx_t *)raw;
    atto_progress_start(&c->progress, s->transition_ms);
}

LC_EFFECT_DEFINE(static, LC_EFFECT_FLAG_NONE, static_ctx_t);
