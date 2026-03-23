#pragma once

#include "lc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void  (*set_pixel)(uint16_t index, rgb_t color);
    rgb_t (*get_pixel)(uint16_t index);
    void  (*show)(void);
} lc_hal_t;

#ifdef __cplusplus
}
#endif
