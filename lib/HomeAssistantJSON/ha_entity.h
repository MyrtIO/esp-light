#pragma once

#include <stdbool.h>
#include "ha_device.h"

#define HA_AVAILABILITY_ONLINE  "online"
#define HA_AVAILABILITY_OFFLINE "offline"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *name;
	const char *icon;
	const char *identifier;
	const char *component;
	bool writable;
	const ha_device_t *device;

	char *unique_id;
	char *state_topic;
	char *command_topic;
	char *config_topic;
	char *availability_topic;
} ha_entity_t;

const char *ha_entity_id(ha_entity_t *e);
const char *ha_entity_state_topic(ha_entity_t *e);
const char *ha_entity_command_topic(ha_entity_t *e);
const char *ha_entity_config_topic(ha_entity_t *e);
const char *ha_entity_availability_topic(ha_entity_t *e);

#ifdef __cplusplus
}
#endif
