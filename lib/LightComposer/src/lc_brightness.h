#pragma once

#include <attotime.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LC_BR_REASON_BRIGHTNESS,
    LC_BR_REASON_POWER,
    LC_BR_REASON_EFFECT,
} lc_brightness_reason_t;

typedef struct {
    bool enabled;
    bool effect_switched;
    uint8_t previous;
    uint8_t current;
    uint8_t target;
    uint8_t selected;
    atto_progress_t transition;
    lc_brightness_reason_t reason;
} lc_brightness_t;

#define LC_BRIGHTNESS_INIT             \
    {.enabled = true,                  \
     .effect_switched = false,         \
     .previous = 0,                    \
     .current = 0,                     \
     .target = 0,                      \
     .selected = 0,                    \
     .transition = ATTO_PROGRESS_INIT, \
     .reason = LC_BR_REASON_BRIGHTNESS}

typedef void (*lc_effect_switch_fn)(void);

void lc_brightness_init(lc_brightness_t *br);
bool lc_brightness_tick(lc_brightness_t *br, lc_effect_switch_fn on_switch);
uint8_t lc_brightness_get_value(const lc_brightness_t *br);

void lc_brightness_set(lc_brightness_t *br, uint8_t brightness);
void lc_brightness_set_power(lc_brightness_t *br, bool enabled);
void lc_brightness_request_effect_change(lc_brightness_t *br);

uint8_t lc_brightness_get(const lc_brightness_t *br);
bool lc_brightness_get_power(const lc_brightness_t *br);

#ifdef __cplusplus
}
#endif
