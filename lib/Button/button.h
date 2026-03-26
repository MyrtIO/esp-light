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
	uint32_t debounce_ms;
	atto_timer_t debounce;
} button_t;

#define BUTTON_INIT(debounce_ms_) \
	{.last_raw = true, .stable = true, .debounce_ms = (debounce_ms_), .debounce = ATTO_TIMER_INIT}

/**
 * Feed the current raw GPIO level and detect a press edge.
 * Returns true exactly once per HIGH→LOW transition after debounce settles.
 */
bool button_pressed(button_t *btn, bool level);

#ifdef __cplusplus
}
#endif
