#include "device_id.h"
#include <esp_mac.h>
#include <stdio.h>

static char id_buf[32];
static char hostname_buf[32];
static char name_buf[48];

void device_id_init() {
	uint8_t mac[6];
	esp_efuse_mac_get_default(mac);
	snprintf(id_buf, sizeof(id_buf),
		"myrtio_light_%02X%02X", mac[4], mac[5]);
	snprintf(hostname_buf, sizeof(hostname_buf),
		"myrtio-light-%02x%02x", mac[4], mac[5]);
	snprintf(name_buf, sizeof(name_buf),
		"MyrtIO Светильник %02X%02X", mac[4], mac[5]);
}

const char *device_id() {
	return id_buf;
}

const char *device_hostname() {
	return hostname_buf;
}

const char *device_name() {
	return name_buf;
}
