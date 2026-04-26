#include "button.h"
#include <Arduino.h>

static bool button_update(button_t *btn, bool level) {
    if (level != btn->last_raw) {
        btn->last_raw = level;
        atto_timer_start(&btn->debounce, btn->debounce_ms);
    }

    if (!atto_timer_finished(&btn->debounce)) {
        return false;
    }

    if (level == btn->stable) {
        return false;
    }

    btn->stable = level;
    btn->stable_since_ms = millis();
    btn->hold_reported = false;
    return true;
}

bool button_pressed(button_t *btn, bool level) {
    return button_update(btn, level) && btn->stable == false;
}

bool button_held(button_t *btn, bool level, uint32_t hold_ms) {
    button_update(btn, level);

    if (btn->stable || btn->hold_reported) {
        return false;
    }

    if ((uint32_t)(millis() - btn->stable_since_ms) < hold_ms) {
        return false;
    }

    btn->hold_reported = true;
    return true;
}
