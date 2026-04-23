#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void device_id_init();
const char *device_id();
const char *device_hostname();
const char *device_name();

#ifdef __cplusplus
}
#endif
