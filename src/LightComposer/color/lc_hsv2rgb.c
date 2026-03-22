#include "lc_hsv2rgb.h"
#include <LightComposer/math/lc_scale.h>

#define FORCE_REFERENCE(var) asm volatile("" : : "r"(var))

#define K255 255
#define K171 171
#define K170 170
#define K85  85

void lc_hsv2rgb(hsv_t hsv, rgb_t *out) {
    const uint8_t Y1 = 1;
    const uint8_t Y2 = 0;
    const uint8_t G2 = 0;
    const uint8_t Gscale = 0;

    uint8_t hue = hsv.h;
    uint8_t sat = hsv.s;
    uint8_t val = hsv.v;

    uint8_t offset = hue & 0x1F;
    uint8_t offset8 = offset;
    offset8 <<= 3;

    uint8_t third = lc_scale8(offset8, (256 / 3));

    uint8_t r, g, b;

    if (!(hue & 0x80)) {
        if (!(hue & 0x40)) {
            if (!(hue & 0x20)) {
                r = K255 - third;
                g = third;
                b = 0;
                FORCE_REFERENCE(b);
            } else {
                if (Y1) {
                    r = K171;
                    g = K85 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if (Y2) {
                    r = K170 + third;
                    uint8_t twothirds = lc_scale8(offset8, ((256 * 2) / 3));
                    g = K85 + twothirds;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            }
        } else {
            if (!(hue & 0x20)) {
                if (Y1) {
                    uint8_t twothirds = lc_scale8(offset8, ((256 * 2) / 3));
                    r = K171 - twothirds;
                    g = K170 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if (Y2) {
                    r = K255 - offset8;
                    g = K255;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            } else {
                r = 0;
                FORCE_REFERENCE(r);
                g = K255 - third;
                b = third;
            }
        }
    } else {
        if (!(hue & 0x40)) {
            if (!(hue & 0x20)) {
                r = 0;
                FORCE_REFERENCE(r);
                uint8_t twothirds = lc_scale8(offset8, ((256 * 2) / 3));
                g = K171 - twothirds;
                b = K85 + twothirds;
            } else {
                r = third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K255 - third;
            }
        } else {
            if (!(hue & 0x20)) {
                r = K85 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K171 - third;
            } else {
                r = K170 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K85 - third;
            }
        }
    }

    if (G2) g = g >> 1;
    if (Gscale) g = lc_scale8_video(g, Gscale);

    if (sat != 255) {
        if (sat == 0) {
            r = 255; b = 255; g = 255;
        } else {
            uint8_t desat = 255 - sat;
            desat = lc_scale8_video(desat, desat);
            uint8_t satscale = 255 - desat;
            r = lc_scale8(r, satscale);
            g = lc_scale8(g, satscale);
            b = lc_scale8(b, satscale);
            uint8_t brightness_floor = desat;
            r += brightness_floor;
            g += brightness_floor;
            b += brightness_floor;
        }
    }

    if (val != 255) {
        val = lc_scale8_video(val, val);
        if (val == 0) {
            r = 0; g = 0; b = 0;
        } else {
            r = lc_scale8(r, val);
            g = lc_scale8(g, val);
            b = lc_scale8(b, val);
        }
    }

    out->r = r;
    out->g = g;
    out->b = b;
}
