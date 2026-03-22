#pragma once

#include "lc_types.h"
#include "lc_hal.h"
#include "lc_effect.h"
#include "lc_pixels.h"
#include "color/lc_color.h"
#include "color/lc_hsv2rgb.h"
#include "color/lc_white.h"
#include "color/lc_gradient.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const lc_hal_t  *hal;
    rgb_t           *pixel_buf;
    uint16_t         max_leds_count;
    uint16_t         leds_count;
    uint16_t         led_skip;
    rgb_t            color_correction;
    lc_color_order_t color_order;
    uint8_t          fps;
} lc_config_t;

/* Lifecycle */
void lc_init(const lc_config_t *cfg);
void lc_tick(void);

/* Brightness / power */
void    lc_set_brightness(uint8_t brightness);
void    lc_set_power(bool enabled);
uint8_t lc_get_brightness(void);
bool    lc_get_power(void);

/* Color */
void  lc_set_color(rgb_t color);
rgb_t lc_get_color(void);

/* Effect */
void lc_set_effect(lc_effect_t *effect);
void lc_set_effect_force(lc_effect_t *effect);
const lc_effect_t *lc_get_effect(void);
const lc_effect_t *lc_get_target_effect(void);

/* Transition */
void lc_set_transition(uint16_t ms);

/* Runtime parameters */
void lc_set_leds_count(uint16_t count);
void lc_set_led_skip(uint16_t skip);
void lc_set_color_correction(rgb_t corr);
void lc_set_color_order(lc_color_order_t order);

#ifdef __cplusplus
}
#endif
