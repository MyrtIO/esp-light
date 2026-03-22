#include "lc_fx_fire.h"
#include <LightComposer/lc_pixels.h>
#include <LightComposer/math/lc_scale.h>
#include <attotime.h>
#include <stdlib.h>

#define FIRE_COOLING  55
#define FIRE_SPARKING 120
#define FIRE_INTERVAL 15

typedef struct {
    uint8_t heat[LC_MAX_LEDS];
    atto_timer_t frame_ready;
} fire_ctx_t;

static bool fire_render(lc_state_t *s, void *raw) {
    fire_ctx_t *c = (fire_ctx_t *)raw;

    if (!atto_timer_finished(&c->frame_ready)) {
        return false;
    }

    uint16_t count = s->count;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t cooldown = (uint32_t)rand() %
                            (((FIRE_COOLING * 10) / count) + 2);
        if (cooldown > c->heat[i]) {
            c->heat[i] = 0;
        } else {
            c->heat[i] = c->heat[i] - (uint8_t)cooldown;
        }
    }

    for (int k = (int)count - 1; k >= 2; k--) {
        c->heat[k] = (c->heat[k - 1] + c->heat[k - 2] + c->heat[k - 2]) / 3;
    }

    if ((uint8_t)(rand() % 256) < FIRE_SPARKING) {
        int y = rand() % 7;
        c->heat[y] = c->heat[y] + (uint8_t)(rand() % 96 + 160);
    }

    for (uint16_t j = 0; j < count; j++) {
        uint8_t t192 = (uint8_t)((c->heat[j] / 255.0f) * 191);
        uint8_t heat_ramp = t192 & 0x3F;
        heat_ramp <<= 2;

        if (t192 > 0x80) {
            lc_set_pixel(s, j, LC_RGB(255, 255, heat_ramp));
        } else if (t192 > 0x40) {
            lc_set_pixel(s, j, LC_RGB(255, heat_ramp, 0));
        } else {
            lc_set_pixel(s, j, LC_RGB(heat_ramp, 0, 0));
        }
    }

    atto_timer_start(&c->frame_ready, FIRE_INTERVAL);
    return true;
}

static void fire_activate(lc_state_t *s, void *raw) {
    fire_ctx_t *c = (fire_ctx_t *)raw;
    for (uint16_t i = 0; i < LC_MAX_LEDS; i++) {
        c->heat[i] = 0;
    }
    atto_timer_start(&c->frame_ready, 0);
    (void)s;
}

static void fire_color_update(lc_state_t *s, void *raw) {
    s->current_color = s->target_color;
    (void)raw;
}

LC_EFFECT_DEFINE(fire, LC_EFFECT_FLAG_NONE, fire_ctx_t);
