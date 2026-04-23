#pragma once

#include <attotime.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool last_raw;
    bool stable;
    bool hold_reported;
    uint32_t debounce_ms;
    uint32_t stable_since_ms;
    atto_timer_t debounce;
} button_t;

#define BUTTON_INIT(debounce_ms_)   \
    {.last_raw = true,              \
     .stable = true,                \
     .hold_reported = false,        \
     .debounce_ms = (debounce_ms_), \
     .stable_since_ms = 0,          \
     .debounce = ATTO_TIMER_INIT}

/**
 * Feed the current raw GPIO level and detect a press edge.
 * Returns true exactly once per HIGH→LOW transition after debounce settles.
 */
bool button_pressed(button_t *btn, bool level);

/**
 * Feed the current raw GPIO level and detect a long press.
 * Returns true exactly once after the button stays pressed for hold_ms.
 */
bool button_held(button_t *btn, bool level, uint32_t hold_ms);

#ifdef __cplusplus
}
#endif
