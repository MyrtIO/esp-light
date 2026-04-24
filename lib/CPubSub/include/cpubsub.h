#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CPUBSUB_MAX_SUBSCRIPTIONS
#define CPUBSUB_MAX_SUBSCRIPTIONS 8
#endif

typedef void (*mqtt_handler_t)(const uint8_t *payload, uint16_t length);

typedef struct mqtt_config_t {
    const char *client_id;
    const char *host;
    uint16_t port;
    uint16_t buffer_size;
    uint16_t reconnect_delay;
    const char *username;
    const char *password;
} mqtt_config_t;

void mqtt_init(const mqtt_config_t *cfg);
void mqtt_reconfigure(const mqtt_config_t *cfg);
void mqtt_disconnect(void);
void mqtt_set_lwt(const char *topic, const char *message);
void mqtt_loop(void);
void mqtt_subscribe(const char *topic, mqtt_handler_t handler);
void mqtt_publish(const char *topic, const char *payload, bool retain);
bool mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif
