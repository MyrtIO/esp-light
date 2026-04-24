#include "ha_entity.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *ha_entity_id(ha_entity_t *e) {
    if (e->unique_id == NULL) {
        size_t len = strlen(e->device->id) + strlen(e->identifier) + 2;
        e->unique_id = (char *)malloc(len);
        snprintf(e->unique_id, len, "%s_%s", e->device->id, e->identifier);
    }
    return e->unique_id;
}

const char *ha_entity_state_topic(ha_entity_t *e) {
    if (e->state_topic == NULL) {
        size_t len = strlen(e->device->mqtt_namespace) + strlen(e->identifier) + 2;
        e->state_topic = (char *)malloc(len);
        snprintf(e->state_topic, len, "%s/%s", e->device->mqtt_namespace,
                 e->identifier);

        if (e->writable) {
            size_t cmd_len = len + 4;
            e->command_topic = (char *)malloc(cmd_len);
            snprintf(e->command_topic, cmd_len, "%s/set", e->state_topic);
        }
    }
    return e->state_topic;
}

const char *ha_entity_command_topic(ha_entity_t *e) {
    if (e->writable && e->command_topic == NULL) {
        ha_entity_state_topic(e);
    }
    return e->command_topic;
}

const char *ha_entity_config_topic(ha_entity_t *e) {
    if (e->config_topic == NULL) {
        const char *id = ha_entity_id(e);
        size_t len = strlen(e->component) + strlen(id) + 23;
        e->config_topic = (char *)malloc(len);
        snprintf(e->config_topic, len, "homeassistant/%s/%s/config", e->component,
                 id);
    }
    return e->config_topic;
}

const char *ha_entity_availability_topic(ha_entity_t *e) {
    if (e->availability_topic == NULL) {
        const char *ns = e->device->mqtt_namespace;
        size_t len = strlen(ns) + sizeof("/availability");
        e->availability_topic = (char *)malloc(len);
        snprintf(e->availability_topic, len, "%s/availability", ns);
    }
    return e->availability_topic;
}
