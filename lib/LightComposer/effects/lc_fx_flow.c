#include "lc_fx_flow.h"
#include <lc_pixels.h>
#include <color/lc_color.h>
#include <attotime.h>

/* ── Palettes ──────────────────────────────────────────────────────── */

static const rgb_t neon_palette[] = {
    LC_RGB_FROM_CODE(0x002EB8), // Deep blue
    LC_RGB_FROM_CODE(0x00FFD4), // Teal
    LC_RGB_FROM_CODE(0x14FF78), // Green
    LC_RGB_FROM_CODE(0x00C8FF), // Cyan
    LC_RGB_FROM_CODE(0x8800FF), // Violet
    LC_RGB_FROM_CODE(0xFF0090), // Pink/magenta
};

static const rgb_t lava_lamp_palette[] = {
    LC_RGB_FROM_CODE(0x3C0014), // Dark magenta
    LC_RGB_FROM_CODE(0xD10038), // Deep red
    LC_RGB_FROM_CODE(0xFF5000), // Orange
    LC_RGB_FROM_CODE(0xFF972E), // Bright yellow
    LC_RGB_FROM_CODE(0xF2039F), // Purple accent
};

static const rgb_t sunset_palette[] = {
    LC_RGB_FROM_CODE(0x0B1026), // Night Sky
    LC_RGB_FROM_CODE(0x2B1B54), // Deep Purple
    LC_RGB_FROM_CODE(0x8C2155), // Magenta
    LC_RGB_FROM_CODE(0xD94E1F), // Red-Orange
    LC_RGB_FROM_CODE(0xF28C22), // Orange
    LC_RGB_FROM_CODE(0xFFD878), // Sun Yellow
};

#define NEON_PALETTE_SIZE      (sizeof(neon_palette) / sizeof(neon_palette[0]))
#define LAVA_LAMP_PALETTE_SIZE (sizeof(lava_lamp_palette) / sizeof(lava_lamp_palette[0]))
#define SUNSET_PALETTE_SIZE    (sizeof(sunset_palette) / sizeof(sunset_palette[0]))

/* ── Timing ────────────────────────────────────────────────────────── */

#define FLOW_LAYER1_PERIOD_MS 8000
#define FLOW_LAYER2_PERIOD_MS 5000
#define FLOW_LAYER3_PERIOD_MS 13000

/* ── Spatial tuning (LEDs per noise cell) ──────────────────────────── */

#define FLOW_MIN_CELL1 12
#define FLOW_MIN_CELL2  6
#define FLOW_MIN_CELL3 18

#define FLOW_MAX_CELL1 40
#define FLOW_MAX_CELL2 18
#define FLOW_MAX_CELL3 60

/* ── Variant ───────────────────────────────────────────────────────── */

typedef enum {
    FLOW_VARIANT_NEON,
    FLOW_VARIANT_LAVA_LAMP,
    FLOW_VARIANT_SUNSET,
} flow_variant_t;

typedef struct {
    flow_variant_t variant;
} flow_ctx_t;

/* ── Helpers ───────────────────────────────────────────────────────── */

static uint32_t flow_clamp_u32(uint32_t v, uint32_t lo, uint32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static uint8_t flow_ease_in_out_quad(uint8_t i) {
    uint8_t j  = (i & 0x80) ? (255 - i) : i;
    uint8_t jj = lc_scale8(j, j);
    uint8_t jj2 = jj << 1;
    return (i & 0x80) ? (255 - jj2) : jj2;
}

/* SplitMix64-style deterministic hash, folded to 32 bits */
static uint32_t flow_hash(uint64_t x) {
    uint64_t z = x + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return (uint32_t)(z ^ (z >> 31));
}

/* 1-D value noise. pos_fp is 16.16 fixed-point. Returns 0-255. */
static uint8_t flow_value_noise(uint64_t pos_fp) {
    uint64_t cell = pos_fp >> 16;
    uint8_t  frac = (uint8_t)((pos_fp >> 8) & 0xFF);

    uint8_t v0 = (uint8_t)(flow_hash(cell)     & 0xFF);
    uint8_t v1 = (uint8_t)(flow_hash(cell + 1) & 0xFF);

    uint8_t t = flow_ease_in_out_quad(frac);
    return lc_blend8(v0, v1, t);
}

/* Sample a palette at position t (0-255) with linear interpolation */
static rgb_t flow_sample_palette(const rgb_t *palette, uint8_t size, uint8_t t) {
    uint8_t segments = size - 1;
    if (segments == 0) {
        return palette[0];
    }

    uint16_t scaled  = (uint16_t)t * segments;
    uint8_t  segment = (uint8_t)(scaled >> 8);
    if (segment >= segments) segment = segments - 1;
    uint8_t  local_t = (uint8_t)(scaled & 0xFF);

    return lc_blend_rgb(palette[segment], palette[segment + 1], local_t);
}

static void flow_get_palette(flow_variant_t variant,
                             const rgb_t **out_palette,
                             uint8_t *out_size)
{
    switch (variant) {
    case FLOW_VARIANT_LAVA_LAMP:
        *out_palette = lava_lamp_palette;
        *out_size    = LAVA_LAMP_PALETTE_SIZE;
        return;
    case FLOW_VARIANT_SUNSET:
        *out_palette = sunset_palette;
        *out_size    = SUNSET_PALETTE_SIZE;
        return;
    default:
        *out_palette = neon_palette;
        *out_size    = NEON_PALETTE_SIZE;
        return;
    }
}

/* Combine 3 noise layers into a single 0-255 value */
static uint8_t flow_combined_noise(uint32_t i, uint32_t len, uint64_t time_ms) {
    uint32_t cell1 = flow_clamp_u32(len / 6,  FLOW_MIN_CELL1, FLOW_MAX_CELL1);
    uint32_t cell2 = flow_clamp_u32(len / 12, FLOW_MIN_CELL2, FLOW_MAX_CELL2);
    uint32_t cell3 = flow_clamp_u32(len / 4,  FLOW_MIN_CELL3, FLOW_MAX_CELL3);
    if (cell1 == 0) cell1 = 1;
    if (cell2 == 0) cell2 = 1;
    if (cell3 == 0) cell3 = 1;

    uint64_t i64 = (uint64_t)i;
    uint64_t x1 = (i64 << 16) / cell1;
    uint64_t x2 = (i64 << 16) / cell2;
    uint64_t x3 = (i64 << 16) / cell3;

    /* 16.16 fixed-point phase offsets */
    uint64_t p1 = (time_ms << 16) / FLOW_LAYER1_PERIOD_MS;
    uint64_t p2 = (time_ms << 16) / FLOW_LAYER2_PERIOD_MS;
    uint64_t p3 = (time_ms << 16) / FLOW_LAYER3_PERIOD_MS;

    /* Layer directions differ for parallax depth */
    uint8_t n1 = flow_value_noise(x1 + p1);
    uint8_t n2 = flow_value_noise(x2 - p2);
    uint8_t n3 = flow_value_noise(x3 + p3 * 2);

    /* 50% base, 30% detail, 20% shimmer */
    uint16_t combined = ((uint16_t)n1 * 128 + (uint16_t)n2 * 77 + (uint16_t)n3 * 51) >> 8;
    return (uint8_t)combined;
}

/* ── Effect callbacks ──────────────────────────────────────────────── */

static bool flow_render(lc_state_t *s, void *raw) {
    flow_ctx_t *c = (flow_ctx_t *)raw;

    const rgb_t *palette;
    uint8_t palette_size;
    flow_get_palette(c->variant, &palette, &palette_size);

    uint64_t time_ms = (uint64_t)atto_now();
    uint32_t len = s->count;

    for (uint16_t i = 0; i < s->count; i++) {
        uint8_t noise = flow_combined_noise(i, len, time_ms);

        rgb_t base_color = flow_sample_palette(palette, palette_size, noise);

        /* Brightness modulation: 75%-100% range for a silky feel */
        uint16_t bm = (uint16_t)lc_scale8(noise, 64) + 191;
        uint8_t brightness_mod = (bm > 255) ? 255 : (uint8_t)bm;

        lc_set_pixel(s, i, lc_scale_rgb(base_color, brightness_mod));
    }

    return true;
}

static void flow_activate(lc_state_t *s, void *raw) {
    (void)s;
    (void)raw;
}

static void flow_color_update(lc_state_t *s, void *raw) {
    s->current_color = s->target_color;
    (void)raw;
}

/* ── Effect definitions (one per variant) ──────────────────────────── */

static flow_ctx_t flow_neon_ctx      = { .variant = FLOW_VARIANT_NEON };
static flow_ctx_t flow_lava_lamp_ctx = { .variant = FLOW_VARIANT_LAVA_LAMP };
static flow_ctx_t flow_sunset_ctx    = { .variant = FLOW_VARIANT_SUNSET };

lc_effect_t lc_fx_flow_neon = {
    .render       = flow_render,
    .activate     = flow_activate,
    .color_update = flow_color_update,
    .ctx          = &flow_neon_ctx,
    .flags        = LC_EFFECT_FLAG_NONE,
};

lc_effect_t lc_fx_flow_lava_lamp = {
    .render       = flow_render,
    .activate     = flow_activate,
    .color_update = flow_color_update,
    .ctx          = &flow_lava_lamp_ctx,
    .flags        = LC_EFFECT_FLAG_NONE,
};

lc_effect_t lc_fx_flow_sunset = {
    .render       = flow_render,
    .activate     = flow_activate,
    .color_update = flow_color_update,
    .ctx          = &flow_sunset_ctx,
    .flags        = LC_EFFECT_FLAG_NONE,
};
