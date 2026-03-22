#include "lc_pixels.h"

void lc_fill(lc_state_t *s, rgb_t color) {
    for (uint16_t i = 0; i < s->count; i++) {
        s->pixels[i] = color;
    }
}

void lc_fill_range(lc_state_t *s, rgb_t color, uint16_t from, uint16_t to) {
    if (to > s->count) {
        to = s->count;
    }
    for (uint16_t i = from; i < to; i++) {
        s->pixels[i] = color;
    }
}

void lc_set_pixel(lc_state_t *s, uint16_t idx, rgb_t color) {
    if (idx < s->count) {
        s->pixels[idx] = color;
    }
}

void lc_mirror(lc_state_t *s) {
    for (uint16_t i = 0; i < s->center; i++) {
        s->pixels[s->count - 1 - i] = s->pixels[i];
    }
}
