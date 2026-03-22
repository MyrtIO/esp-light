#include "lc_white.h"
#include <math.h>

#define CLAMP8(a) ((uint8_t)((a) < 0.0 ? 0.0 : ((a) > 255.0 ? 255.0 : (a))))

void lc_kelvin_to_rgb(kelvin_t temperature, rgb_t *out) {
    double t = temperature / 100.0;
    double acc;

    if (t <= 66.0) {
        out->r = 255;
    } else {
        acc = t - 60.0;
        acc = 329.698727466 * pow(acc, -0.1332047592);
        out->r = CLAMP8(acc);
    }

    if (t <= 66.0) {
        acc = t;
        acc = 99.4708025861 * log(acc) - 161.1195681661;
        out->g = CLAMP8(acc);
    } else {
        acc = t - 60.0;
        acc = 288.1221695283 * pow(acc, -0.0755148492);
        out->g = CLAMP8(acc);
    }

    if (t >= 66.0) {
        out->b = 255;
    } else {
        if (t <= 19.0) {
            out->b = 0;
        } else {
            acc = t - 10.0;
            acc = 138.5177312231 * log(acc) - 305.0447927307;
            out->b = CLAMP8(acc);
        }
    }
}
