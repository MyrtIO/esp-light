#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cpreferences_t cpreferences_t;

cpreferences_t *cpreferences_create(void);
void cpreferences_destroy(cpreferences_t *prefs);

bool cpreferences_begin(cpreferences_t *prefs, const char *namespace_name, bool read_only);
void cpreferences_end(cpreferences_t *prefs);
bool cpreferences_is_key(cpreferences_t *prefs, const char *key);

size_t cpreferences_get_string(cpreferences_t *prefs,
                               const char *key,
                               char *buf,
                               size_t buf_size,
                               const char *default_value);
bool cpreferences_put_string(cpreferences_t *prefs, const char *key, const char *value);

bool cpreferences_get_bool(cpreferences_t *prefs, const char *key, bool default_value);
bool cpreferences_put_bool(cpreferences_t *prefs, const char *key, bool value);

uint8_t cpreferences_get_uchar(cpreferences_t *prefs, const char *key, uint8_t default_value);
bool cpreferences_put_uchar(cpreferences_t *prefs, const char *key, uint8_t value);

uint16_t cpreferences_get_ushort(cpreferences_t *prefs, const char *key, uint16_t default_value);
bool cpreferences_put_ushort(cpreferences_t *prefs, const char *key, uint16_t value);

uint32_t cpreferences_get_ulong(cpreferences_t *prefs, const char *key, uint32_t default_value);
bool cpreferences_put_ulong(cpreferences_t *prefs, const char *key, uint32_t value);

#ifdef __cplusplus
}
#endif
