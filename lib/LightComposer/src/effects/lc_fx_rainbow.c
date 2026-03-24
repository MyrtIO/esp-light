#include <lc_types.h>
#include <lc_pixels.h>
#include <lc_color.h>
#include <attotime.h>

#define RAINBOW_CYCLE_MS 12000

typedef struct {
    atto_progress_t progress;
    uint8_t hue_progress;
} rainbow_ctx_t;

static uint8_t hue_shift(rainbow_ctx_t *c, uint8_t shift) {
    return (c->hue_progress + shift) % 255;
}

static bool rainbow_render(lc_state_t *s, void *raw) {
    rainbow_ctx_t *c = (rainbow_ctx_t *)raw;

    c->hue_progress = atto_progress_get(&c->progress);

    hsv_t c1 = {hue_shift(c, 0), 255, 255};
    hsv_t c2 = {hue_shift(c, 60), 255, 255};
    hsv_t c3 = {hue_shift(c, 120), 255, 255};

    lc_fill_gradient3(s->pixels, s->center, c1, c2, c3, LC_GRADIENT_FORWARD);
    lc_mirror(s);

    if (atto_progress_finished(&c->progress)) {
        atto_progress_start(&c->progress, RAINBOW_CYCLE_MS);
    }

    return true;
}

static void rainbow_activate(lc_state_t *s, void *raw) {
    rainbow_ctx_t *c = (rainbow_ctx_t *)raw;
    atto_progress_set_max(&c->progress, 255);
    atto_progress_start(&c->progress, RAINBOW_CYCLE_MS);
    (void)s;
}

static void rainbow_color_update(lc_state_t *s, void *raw) {
    (void)s;
    (void)raw;
}

LC_EFFECT_DEFINE(rainbow, LC_EFFECT_FLAG_NONE, rainbow_ctx_t);
