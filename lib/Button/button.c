#include "button.h"

bool button_pressed(button_t *btn, bool level) {
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
	return btn->stable == false;
}
