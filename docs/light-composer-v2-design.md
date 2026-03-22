# LightComposer v2 — Architecture Design

## 1. Goals

- **C-style API** — no classes, no templates, no virtual methods. Plain structs, function pointers, module-prefixed free functions.
- **Single external dependency** — `attotime` 2.0.0 (C API: `atto_timer_t`, `atto_progress_t`, etc.).
- **HAL decoupling** — output is abstracted via a function-pointer table (`lc_hal_t`). The library never touches FastLED or any hardware directly.
- **Effects are part of the library** — built-in effects live inside `LightComposer/effects/`. App code (`ha_light.cpp`) only maps human-readable names to effect pointers.
- **Per-effect color correction flag** — an effect declares whether color correction should be applied during output. The library applies it transparently when flushing pixels to the HAL.
- **Unified effect authoring** — every effect is a small struct of function pointers + an opaque context. Helper functions and macros make writing a new effect trivial.

---

## 2. Problems with the Current Design

| Problem | Consequence |
|---|---|
| Deep class hierarchy (`IRenderer`, `IPixelsRenderer`, `IPixels`, `IPixelsEffect`, `IBrightnessRenderer`) | Hard to follow control flow; vtable overhead on a microcontroller |
| `template <class Locator>` is always `<void>` | Unnecessary complexity; no locator is ever used |
| `ILightHAL` is an abstract C++ class | Ties the library to a specific inheritance hierarchy; not reusable from C |
| Effect names stored inside effects (`getName()`) | Library has to know about presentation concerns |
| Effects live outside the library (`src/effects/`) | Effects can't use internal helpers; duplication of boilerplate |
| `CONFIG_LIGHT_LED_COUNT` baked into fire effect's `heat_[]` array | Effect can't adapt to runtime LED count changes |
| Color correction applied at the HAL level globally | No per-effect control — some effects (e.g. rainbow) look wrong with correction meant for white |

---

## 3. Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────┐
│                         Application layer                            │
│  main.cpp / light.cpp / ha_light.cpp                                 │
│                                                                      │
│  ┌───────────────────────────────────┐                               │
│  │  effect_entry_t effect_table[] =  │  Name ↔ effect mapping        │
│  │    { "static",  &lc_fx_static },  │  lives here, not in library   │
│  │    { "fill",    &lc_fx_fill   },  │                               │
│  │    ...                            │                               │
│  └───────────────────────────────────┘                               │
│                         │                                            │
│            lc_init()    │   lc_set_effect()   lc_set_color()         │
│            lc_tick()    │   lc_set_brightness()  ...                 │
├─────────────────────────┼────────────────────────────────────────────┤
│                   LightComposer library                              │
│                         │                                            │
│  ┌──────────────────────▼──────────────────────┐                     │
│  │              lc (core module)                │                     │
│  │  - owns pixel buffer lc_rgb_t[max_leds]     │                     │
│  │  - frame deadline (atto_timer_t)            │                     │
│  │  - delegates to brightness + pixels modules │                     │
│  └────────┬─────────────────────┬──────────────┘                     │
│           │                     │                                    │
│  ┌────────▼────────┐   ┌───────▼────────────┐                       │
│  │  lc_brightness   │   │    lc_pixels        │                      │
│  │  (transitions)   │   │ (effect dispatch,   │                      │
│  │                  │   │  color state,       │                      │
│  │                  │   │  correction apply)  │                      │
│  └──────────────────┘   └───────┬─────────────┘                      │
│                                 │                                    │
│                    ┌────────────▼──────────────┐                     │
│                    │   lc_effect_t instances    │                     │
│                    │  (static, fill, rainbow,  │                     │
│                    │   loading, fire, waves)   │                     │
│                    └───────────────────────────┘                     │
│                                                                      │
│  Utilities: lc_math (scale8, blend8, lerp)                           │
│             lc_color (rgb, hsv, hsv2rgb, white/kelvin, gradient)     │
├──────────────────────────────────────────────────────────────────────┤
│                      lc_hal_t  (function pointers)                   │
│                         │                                            │
├─────────────────────────┼────────────────────────────────────────────┤
│                   Hardware / Platform                                 │
│                    FastLED, GPIO, etc.                                │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 4. Core Types

### 4.1 Color types

```c
typedef struct {
    uint8_t r, g, b;
} lc_rgb_t;

typedef struct {
    uint8_t h, s, v;
} lc_hsv_t;

#define LC_RGB(r, g, b)    ((lc_rgb_t){ (r), (g), (b) })
#define LC_RGB_BLACK       LC_RGB(0, 0, 0)
#define LC_RGB_WHITE       LC_RGB(255, 255, 255)

#define LC_RGB_FROM_CODE(c) LC_RGB(((c) >> 16) & 0xFF, \
                                   ((c) >>  8) & 0xFF, \
                                   ((c)      ) & 0xFF)
```

### 4.2 Pixel state (passed to effects)

```c
typedef struct {
    lc_rgb_t *pixels;          // writable pixel buffer [0 .. count-1]
    uint16_t  count;           // active LED count (leds_count)
    uint16_t  center;          // ceil(count / 2)
    lc_rgb_t  previous_color;
    lc_rgb_t  current_color;
    lc_rgb_t  target_color;
    uint16_t  transition_ms;
} lc_state_t;
```

Rationale: combines the old `LightState<T>` and `IPixels` into one flat struct. No
allocations, no virtual calls. Effects receive a pointer to this struct and write
directly into `pixels[]`.

---

## 5. HAL Interface

```c
typedef struct {
    void      (*set_pixel)(uint16_t index, lc_rgb_t color);
    lc_rgb_t  (*get_pixel)(uint16_t index);
    void      (*set_brightness)(uint8_t brightness);
    void      (*show)(void);
} lc_hal_t;
```

The HAL operates in **physical** LED space (indices include `led_skip`). The library
translates logical pixel indices to physical during the flush phase:

```
physical_index = logical_index + led_skip
```

Rationale: the HAL is a leaf dependency injected at init time. It can wrap FastLED,
bare SPI, a test stub, etc. No header coupling to any platform.

### 5.1 FastLED HAL implementation example (lives in app code)

```c
#include <FastLED.h>

static CRGB raw_leds[MAX_LEDS];

static void fastled_set_pixel(uint16_t i, lc_rgb_t c) {
    raw_leds[i] = CRGB(c.r, c.g, c.b);
}

static lc_rgb_t fastled_get_pixel(uint16_t i) {
    return LC_RGB(raw_leds[i].r, raw_leds[i].g, raw_leds[i].b);
}

static void fastled_set_brightness(uint8_t b) {
    FastLED.setBrightness(b);
}

static void fastled_show(void) {
    FastLED.show();
}

static const lc_hal_t fastled_hal = {
    .set_pixel       = fastled_set_pixel,
    .get_pixel       = fastled_get_pixel,
    .set_brightness  = fastled_set_brightness,
    .show            = fastled_show,
};
```

---

## 6. Library Configuration & Initialization

```c
typedef struct {
    const lc_hal_t *hal;
    lc_rgb_t       *pixel_buf;       // caller-owned buffer, size >= max_leds_count
    uint16_t        max_leds_count;  // buffer capacity (compile-time constant at call site)
    uint16_t        leds_count;      // initial active LED count
    uint16_t        led_skip;        // physical LED offset
    lc_rgb_t        color_correction;// e.g. LC_RGB(255, 176, 240)
    uint8_t         fps;             // target frame rate (default 60)
} lc_config_t;
```

`pixel_buf` is allocated by the caller — typically a static array sized by a
project-level `#define`. The library never allocates memory.

```c
// In light.cpp
static lc_rgb_t pixel_buf[CONFIG_LIGHT_LED_COUNT];

static const lc_config_t composer_cfg = {
    .hal             = &fastled_hal,
    .pixel_buf       = pixel_buf,
    .max_leds_count  = CONFIG_LIGHT_LED_COUNT,
    .leds_count      = 60,
    .led_skip        = 5,
    .color_correction = LC_RGB(255, 176, 240),
    .fps             = 60,
};

lc_init(&composer_cfg);
```

### 6.1 Runtime-mutable parameters

```c
void lc_set_leds_count(uint16_t count);       // clamps to max_leds_count
void lc_set_led_skip(uint16_t skip);
void lc_set_color_correction(lc_rgb_t corr);
```

These can change at any time. `center` is recalculated when `leds_count` changes.

---

## 7. Effect System

### 7.1 Effect definition

```c
#define LC_EFFECT_FLAG_NONE             0
#define LC_EFFECT_FLAG_COLOR_CORRECTION (1 << 0)

typedef struct {
    bool (*render)(lc_state_t *state, void *ctx);
    void (*activate)(lc_state_t *state, void *ctx);
    void (*color_update)(lc_state_t *state, void *ctx);
    uint8_t flags;
} lc_effect_t;
```

- `render` — called every frame. Returns `true` if pixels changed and HAL flush is needed.
- `activate` — called once when the effect becomes active. Used to reset timers, fill initial state.
- `color_update` — called when target color changes. Used to start transitions.
- `flags` — bitmask. `LC_EFFECT_FLAG_COLOR_CORRECTION` tells the library to apply color correction during pixel flush.

All callbacks receive `lc_state_t *` (shared pixel/color state) and `void *ctx` (effect-private data).

### 7.2 Effect context pattern

Each effect defines its own context struct:

```c
// lc_fx_static.c
typedef struct {
    atto_progress_t progress;
    bool force_update;
} lc_fx_static_ctx_t;

static lc_fx_static_ctx_t ctx;

static bool render(lc_state_t *s, void *raw) {
    lc_fx_static_ctx_t *c = (lc_fx_static_ctx_t *)raw;
    // ...
}

static void activate(lc_state_t *s, void *raw) {
    lc_fx_static_ctx_t *c = (lc_fx_static_ctx_t *)raw;
    c->force_update = true;
}

static void color_update(lc_state_t *s, void *raw) {
    lc_fx_static_ctx_t *c = (lc_fx_static_ctx_t *)raw;
    atto_progress_start(&c->progress, s->transition_ms);
}
```

### 7.3 Effect declaration macro

```c
// In lc_effect.h

#define LC_EFFECT_DECLARE(name)  extern lc_effect_t lc_fx_##name

#define LC_EFFECT_DEFINE(name, flags_val, ctx_type)     \
    static ctx_type name##_ctx;                         \
    lc_effect_t lc_fx_##name = {                        \
        .render       = name##_render,                  \
        .activate     = name##_activate,                \
        .color_update = name##_color_update,            \
        .flags        = (flags_val),                    \
    }
```

Usage:

```c
// lc_fx_fill.h
LC_EFFECT_DECLARE(fill);

// lc_fx_fill.c
typedef struct { atto_progress_t progress; } fill_ctx_t;

static bool fill_render(lc_state_t *s, void *raw) { /* ... */ }
static void fill_activate(lc_state_t *s, void *raw) { /* ... */ }
static void fill_color_update(lc_state_t *s, void *raw) { /* ... */ }

LC_EFFECT_DEFINE(fill, LC_EFFECT_FLAG_COLOR_CORRECTION, fill_ctx_t);
```

Result: `lc_fx_fill` is a global `lc_effect_t` that the app code can reference.

### 7.4 Where `void *ctx` comes from

The `LC_EFFECT_DEFINE` macro creates a file-static context variable. When the library
calls `effect->render(state, ctx)`, it passes the `ctx` pointer stored inside the
`lc_effect_t`. This is set during init of the effect (see section 7.6).

### 7.5 Pixel helper functions (toolkit for effects)

```c
void lc_fill(lc_state_t *s, lc_rgb_t color);
void lc_fill_range(lc_state_t *s, lc_rgb_t color, uint16_t from, uint16_t to);
void lc_set_pixel(lc_state_t *s, uint16_t idx, lc_rgb_t color);
void lc_mirror(lc_state_t *s);   // mirror first half to second half
```

These are thin wrappers over `s->pixels[]` writes, with bounds checking against
`s->count`. Effects can also write `s->pixels[i]` directly.

### 7.6 Effect initialization

When an `lc_effect_t` is defined with `LC_EFFECT_DEFINE`, the ctx pointer needs to
be associated with the effect struct. Updated macro:

```c
#define LC_EFFECT_DEFINE(name, flags_val, ctx_type)     \
    static ctx_type name##_ctx;                         \
    lc_effect_t lc_fx_##name = {                        \
        .render       = name##_render,                  \
        .activate     = name##_activate,                \
        .color_update = name##_color_update,            \
        .flags        = (flags_val),                    \
    }
```

And a companion init macro or the library just stores a `(effect, ctx)` pair:

```c
// The library stores the current effect + ctx pair
typedef struct {
    const lc_effect_t *def;
    void              *ctx;
} lc_active_effect_t;
```

**Revised approach** — embed `ctx` pointer in the effect struct itself:

```c
typedef struct {
    bool (*render)(lc_state_t *state, void *ctx);
    void (*activate)(lc_state_t *state, void *ctx);
    void (*color_update)(lc_state_t *state, void *ctx);
    void  *ctx;
    uint8_t flags;
} lc_effect_t;

#define LC_EFFECT_DEFINE(name, flags_val, ctx_type)     \
    static ctx_type name##_ctx;                         \
    lc_effect_t lc_fx_##name = {                        \
        .render       = name##_render,                  \
        .activate     = name##_activate,                \
        .color_update = name##_color_update,            \
        .ctx          = &name##_ctx,                    \
        .flags        = (flags_val),                    \
    }
```

Now the library calls:
```c
effect->render(state, effect->ctx);
```

Clean, zero-allocation, self-contained.

### 7.7 Built-in effects

| Effect | Flags | Description |
|--------|-------|-------------|
| `lc_fx_static` | `0` | Solid color with smooth blend transition |
| `lc_fx_fill` | `COLOR_CORRECTION` | Color wipe from center outward, mirrored |
| `lc_fx_rainbow` | `0` | Cycling 3-point HSV gradient |
| `lc_fx_loading` | `0` | Bouncing segment |
| `lc_fx_fire` | `0` | Fire simulation (heat map) |
| `lc_fx_waves` | `0` | Bouncing segment (variant) |

---

## 8. Color Correction

Color correction is an `lc_rgb_t` that acts as a per-channel scale factor:

```c
static inline lc_rgb_t lc_apply_correction(lc_rgb_t pixel, lc_rgb_t corr) {
    return LC_RGB(
        scale8(pixel.r, corr.r),
        scale8(pixel.g, corr.g),
        scale8(pixel.b, corr.b)
    );
}
```

**Applied during HAL flush**, not inside effects:

```c
// Inside lc_flush_pixels (internal)
for (uint16_t i = 0; i < leds_count; i++) {
    lc_rgb_t px = pixel_buf[i];
    if (current_effect->flags & LC_EFFECT_FLAG_COLOR_CORRECTION) {
        px = lc_apply_correction(px, color_correction);
    }
    hal->set_pixel(i + led_skip, px);
}
hal->show();
```

Rationale: effects that render procedural/decorative colors (rainbow, fire) should
not have their palette skewed by correction. Only effects that reproduce a
user-chosen color (fill, static in some cases) benefit from correction.

---

## 9. Brightness Management

The brightness subsystem is **internal** to the library. It handles three transition
types:

1. **Brightness change** — smooth lerp from old to new value
2. **Power on/off** — fade in from 0 / fade out to 0
3. **Effect switch** — fade out to 0, notify effect switch, fade in

```c
// Internal state (not exposed)
typedef struct {
    bool     enabled;
    bool     transitioning;
    bool     effect_switched;
    uint8_t  previous;
    uint8_t  current;
    uint8_t  target;
    uint8_t  selected;       // user-set value
    atto_progress_t transition;
    lc_brightness_reason_t reason;
} lc_brightness_t;
```

Public API:

```c
void lc_set_brightness(uint8_t brightness);
void lc_set_power(bool enabled);
uint8_t lc_get_brightness(void);
bool lc_get_power(void);
```

The effect-switch transition triggers an internal callback that tells the pixel
module to swap the active effect at the midpoint of the fade.

---

## 10. Public API Summary

```c
/* Lifecycle */
void     lc_init(const lc_config_t *cfg);
void     lc_tick(void);                        // call from main loop or RTOS task

/* Brightness / power */
void     lc_set_brightness(uint8_t brightness);
void     lc_set_power(bool enabled);
uint8_t  lc_get_brightness(void);
bool     lc_get_power(void);

/* Color */
void     lc_set_color(lc_rgb_t color);
lc_rgb_t lc_get_color(void);

/* Effect */
void     lc_set_effect(lc_effect_t *effect);
void     lc_set_effect_force(lc_effect_t *effect);   // skip fade transition
const lc_effect_t *lc_get_effect(void);

/* Transition */
void     lc_set_transition(uint16_t ms);

/* Runtime parameters */
void     lc_set_leds_count(uint16_t count);
void     lc_set_led_skip(uint16_t skip);
void     lc_set_color_correction(lc_rgb_t corr);

/* Color utilities (stateless, always available) */
lc_rgb_t lc_hsv_to_rgb(lc_hsv_t hsv);
lc_rgb_t lc_blend(lc_rgb_t a, lc_rgb_t b, uint8_t frac);
lc_rgb_t lc_kelvin_to_rgb(uint16_t kelvin);
```

---

## 11. Effect Names — App-Level Mapping

Effect name strings are **not** stored in the library. The application maintains the
mapping:

```c
// light.cpp
typedef struct {
    const char        *name;
    lc_effect_t       *effect;
} light_effect_entry_t;

static light_effect_entry_t effect_table[] = {
    { "static",  &lc_fx_static  },
    { "fill",    &lc_fx_fill    },
    { "rainbow", &lc_fx_rainbow },
    { "loading", &lc_fx_loading },
    { "fire",    &lc_fx_fire    },
    { "waves",   &lc_fx_waves   },
};

#define EFFECT_COUNT (sizeof(effect_table) / sizeof(effect_table[0]))

lc_effect_t *light_find_effect(const char *name) {
    for (size_t i = 0; i < EFFECT_COUNT; i++) {
        if (strcmp(effect_table[i].name, name) == 0) {
            return effect_table[i].effect;
        }
    }
    return NULL;
}

const char **light_effect_names(void) {
    static const char *names[EFFECT_COUNT];
    for (size_t i = 0; i < EFFECT_COUNT; i++) {
        names[i] = effect_table[i].name;
    }
    return names;
}
```

Rationale: the library is presentation-agnostic. Names are a UI/HA concern.

---

## 12. Full Usage Example

```c
#include <lc.h>
#include <lc/effects/lc_fx.h>
#include <FastLED.h>
#include <attotime.h>

/* --- HAL --- */

#define MAX_LEDS       120
#define ACTIVE_LEDS    60
#define LED_SKIP       5

static CRGB raw_leds[MAX_LEDS];
static lc_rgb_t pixel_buf[MAX_LEDS];

static void hal_set_pixel(uint16_t i, lc_rgb_t c) {
    raw_leds[i] = CRGB(c.r, c.g, c.b);
}
static lc_rgb_t hal_get_pixel(uint16_t i) {
    return LC_RGB(raw_leds[i].r, raw_leds[i].g, raw_leds[i].b);
}
static void hal_set_brightness(uint8_t b) { FastLED.setBrightness(b); }
static void hal_show(void) { FastLED.show(); }

static const lc_hal_t hal = {
    .set_pixel      = hal_set_pixel,
    .get_pixel      = hal_get_pixel,
    .set_brightness = hal_set_brightness,
    .show           = hal_show,
};

/* --- Effect table (app-level) --- */

static light_effect_entry_t effects[] = {
    { "static",  &lc_fx_static  },
    { "fill",    &lc_fx_fill    },
    { "rainbow", &lc_fx_rainbow },
};

/* --- Init --- */

void setup(void) {
    FastLED.addLeds<WS2812B, 4, GRB>(raw_leds, MAX_LEDS);
    atto_init(millis);

    lc_config_t cfg = {
        .hal             = &hal,
        .pixel_buf       = pixel_buf,
        .max_leds_count  = MAX_LEDS,
        .leds_count      = ACTIVE_LEDS,
        .led_skip        = LED_SKIP,
        .color_correction = LC_RGB(255, 176, 240),
        .fps             = 60,
    };
    lc_init(&cfg);

    lc_set_effect(&lc_fx_static);
    lc_set_color(LC_RGB(255, 200, 150));
    lc_set_brightness(200);
    lc_set_transition(500);
}

void loop(void) {
    lc_tick();
}
```

---

## 13. Proposed Directory Structure

```
src/LightComposer/
├── lc.h                        # Main public API (includes everything below)
├── lc.c                        # Core: init, tick, flush
│
├── lc_types.h                  # lc_rgb_t, lc_hsv_t, lc_state_t, macros
├── lc_hal.h                    # lc_hal_t definition
├── lc_effect.h                 # lc_effect_t, flags, LC_EFFECT_DECLARE/DEFINE
│
├── lc_brightness.h             # Internal brightness transition state + API
├── lc_brightness.c
│
├── lc_pixels.h                 # Pixel helpers: lc_fill, lc_mirror, etc.
├── lc_pixels.c
│
├── math/
│   ├── lc_scale.h              # scale8, scale8_video
│   └── lc_blend.h              # blend8, lerp8by8
│
├── color/
│   ├── lc_color.h              # lc_blend_rgb, lc_apply_correction
│   ├── lc_hsv2rgb.h
│   ├── lc_hsv2rgb.c
│   ├── lc_white.h              # kelvin/mireds conversion
│   ├── lc_white.c
│   └── lc_gradient.h           # fillGradient (C version)
│
└── effects/
    ├── lc_fx.h                 # Includes all effect headers
    ├── lc_fx_static.h / .c
    ├── lc_fx_fill.h / .c
    ├── lc_fx_rainbow.h / .c
    ├── lc_fx_loading.h / .c
    ├── lc_fx_fire.h / .c
    └── lc_fx_waves.h / .c
```

---

## 14. Data Flow Diagram

```
lc_tick()
  │
  ├─► atto_timer_finished(&frame_deadline)?  ── no ──► return
  │       │ yes
  │       ▼
  ├─► lc_brightness_render()
  │       │ returns: has_changes (bool)
  │       │ side effect: hal->set_brightness(value)
  │       │ at midpoint of effect-switch: swaps active effect
  │       │
  ├─► lc_pixels_render()
  │       │ calls: effect->render(state, effect->ctx)
  │       │ returns: has_changes (bool)
  │       │
  ├─► if (has_changes) lc_flush()
  │       │ for i in [0, leds_count):
  │       │     px = pixel_buf[i]
  │       │     if (effect.flags & COLOR_CORRECTION):
  │       │         px = apply_correction(px, color_correction)
  │       │     hal->set_pixel(i + led_skip, px)
  │       │ hal->show()
  │       │
  └─► atto_timer_start(&frame_deadline, frame_time)
```

---

## 15. Effect Authoring — Step by Step

Creating a new effect requires **3 functions** and **1 macro invocation**:

### Step 1: Define context struct

```c
// lc_fx_pulse.c
#include <lc_effect.h>
#include <lc_pixels.h>
#include <attotime.h>

typedef struct {
    atto_progress_t cycle;
    bool growing;
} pulse_ctx_t;
```

### Step 2: Implement the three callbacks

```c
static bool pulse_render(lc_state_t *s, void *raw) {
    pulse_ctx_t *c = (pulse_ctx_t *)raw;
    uint8_t progress = atto_progress_get(&c->cycle);

    uint8_t brightness = c->growing ? progress : (255 - progress);
    lc_rgb_t color = lc_blend(LC_RGB_BLACK, s->target_color, brightness);
    lc_fill(s, color);

    if (atto_progress_finished(&c->cycle)) {
        c->growing = !c->growing;
        atto_progress_start(&c->cycle, 1000);
    }
    return true;
}

static void pulse_activate(lc_state_t *s, void *raw) {
    pulse_ctx_t *c = (pulse_ctx_t *)raw;
    c->growing = true;
    atto_progress_start(&c->cycle, 1000);
    lc_fill(s, LC_RGB_BLACK);
}

static void pulse_color_update(lc_state_t *s, void *raw) {
    (void)raw;
    s->current_color = s->target_color;
}
```

### Step 3: Define the effect instance

```c
LC_EFFECT_DEFINE(pulse, LC_EFFECT_FLAG_NONE, pulse_ctx_t);
```

### Step 4: Declare in header (optional, for external access)

```c
// lc_fx_pulse.h
#pragma once
#include <lc_effect.h>
LC_EFFECT_DECLARE(pulse);
```

### Result

`lc_fx_pulse` is ready to use:

```c
lc_set_effect(&lc_fx_pulse);
```

---

## 16. Migration Checklist

| Current | New |
|---|---|
| `LightComposer<void>` class | `lc_init()` + `lc_tick()` free functions |
| `ILightHAL` abstract class | `lc_hal_t` struct of function pointers |
| `IPixelsEffect<void>` virtual class | `lc_effect_t` struct of function pointers |
| `PixelsRenderer<void>` class | Internal `lc_pixels` module |
| `BrightnessRenderer` class | Internal `lc_brightness` module |
| `Pixels` class | `lc_state_t` + `lc_fill/lc_mirror` helpers |
| `LightState<void>` template struct | `lc_state_t` plain struct |
| `RGBColor` struct with operators | `lc_rgb_t` plain struct + `LC_RGB()` macro |
| `HSVColor` struct | `lc_hsv_t` plain struct |
| `WhiteColor` class | `lc_kelvin_to_rgb()` free function |
| `EffectList<void>` template class | App-level `effect_table[]` array |
| `effect->getName()` | App-level name-to-effect mapping |
| `Timer` / `Progress` (old Attotime) | `atto_timer_t` / `atto_progress_t` (attotime 2.0) |
| `FastLED.setCorrection()` global | Per-effect `LC_EFFECT_FLAG_COLOR_CORRECTION` |
| `CONFIG_LIGHT_LED_COUNT` in fire effect | `s->count` from `lc_state_t` (runtime) |
| Effects in `src/effects/` | Effects in `LightComposer/effects/` |

---

## 17. Design Justifications

### Why C-style over C++?

The codebase targets ESP32 (embedded, single application). C-style code:
- Has predictable memory layout and no hidden vtable costs.
- Is easier to audit for stack/heap usage.
- Compiles faster (no template instantiation).
- Can be called from both C and C++ translation units (`extern "C"`).

### Why function pointers instead of virtual methods?

Function pointer structs (`lc_hal_t`, `lc_effect_t`) achieve the same polymorphism
without inheritance hierarchies. The struct is visible and inspectable — there is no
hidden vtable.

### Why caller-owned pixel buffer?

On an embedded target, all memory should be accounted for at link time. The caller
allocates `lc_rgb_t pixel_buf[N]` as a static array, so the memory appears in the
linker map. The library never calls `malloc`.

### Why per-effect color correction flag?

Global correction distorts procedural effects (fire, rainbow) that compute their own
colors. Per-effect flags let the library apply correction only where it makes sense
(fill, static — effects that faithfully reproduce a user-chosen color).

### Why effect names outside the library?

Effect names are a presentation/integration concern (Home Assistant entity, UI). The
rendering library should not be coupled to display names. The app layer maps
`"static"` → `&lc_fx_static` — this mapping is trivial, explicit, and easy to
extend or localize.

### Why effects inside the library?

Built-in effects use internal helpers (`lc_fill`, `lc_mirror`, `scale8`, `blend8`,
gradient functions). Keeping them inside the library means they can rely on internal
APIs without exposing them publicly, and new effects are immediately available to any
project using the library.

### Why `lc_state_t` combines color state and pixel buffer?

The old design split state (`LightState`) from pixel access (`IPixels`), requiring
every effect to accept two parameters and mentally map between them. A single struct
is simpler: one pointer, one source of truth.
