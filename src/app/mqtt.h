#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*mqtt_handler_t)(const uint8_t *payload, uint16_t length);

struct mqtt_config_t {
	const char *client_id;
	const char *host;
	uint16_t port;
	uint16_t buffer_size;
};

void mqtt_init(const mqtt_config_t *cfg);
void mqtt_loop(void);
void mqtt_subscribe(const char *topic, mqtt_handler_t handler);
void mqtt_publish(const char *topic, const char *payload, bool retain);
bool mqtt_is_connected(void);
