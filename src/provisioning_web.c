#include "provisioning_web.h"
#include <cwebserver.h>
#include <lwjson/lwjson.h>
#include <lwjson/lwjson_serializer.h>
#include <string.h>

#include "config.h"
#include "wifi_manager.h"
#include "provisioning_page/dist/page.h"

#define LK(s) (s), (sizeof(s) - 1)
#define PARSE_TOKENS 32
#define JSON_BUF_SIZE 768
#define IP_BUF_SIZE 16

static persistent_data_t *s_pdata;
static light_config_t *s_light_cfg;
static web_configuration_changed_cb_t s_on_configuration_changed;
static char json_buf[JSON_BUF_SIZE];

/* --- Color order helpers --- */

static const char *color_order_names[] = {
	"rgb", "rbg", "grb", "gbr", "brg", "bgr"
};

static const char *color_order_to_str(uint8_t order) {
	if (order > 5) return "rgb";
	return color_order_names[order];
}

static uint8_t color_order_from_str(const char *str) {
	for (uint8_t i = 0; i < 6; i++) {
		if (strcasecmp(str, color_order_names[i]) == 0) return i;
	}
	return 0;
}

/* --- Serializer helper --- */

static bool ser_finalize(lwjson_serializer_t *ser) {
	size_t len = 0;
	if (lwjson_serializer_finalize(ser, &len) != lwjsonOK) {
		return false;
	}
	if (len < sizeof(json_buf)) {
		json_buf[len] = '\0';
	}
	return true;
}

/* --- Parser helpers --- */

static void copy_json_string(const lwjson_token_t *t, char *dst, size_t dst_size) {
	size_t slen;
	const char *val = lwjson_get_val_string(t, &slen);
	if (val == NULL) return;
	size_t n = slen < dst_size - 1 ? slen : dst_size - 1;
	memcpy(dst, val, n);
	dst[n] = '\0';
}

static const lwjson_token_t *find_field(
	lwjson_t *parser, const lwjson_token_t *parent, const char *key
) {
	if (parent != NULL) {
		return lwjson_find_ex(parser, parent, key);
	}
	return lwjson_find(parser, key);
}

static void parse_light_fields(lwjson_t *parser, const lwjson_token_t *parent) {
	const lwjson_token_t *t;

	t = find_field(parser, parent, "led_count");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		s_pdata->led_count = (uint16_t)lwjson_get_val_int(t);

	t = find_field(parser, parent, "skip_leds");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		s_pdata->skip_leds = (uint16_t)lwjson_get_val_int(t);

	t = find_field(parser, parent, "color_correction");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		s_pdata->color_correction = (uint32_t)lwjson_get_val_int(t);

	t = find_field(parser, parent, "brightness_min");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		s_pdata->brightness_min = (uint8_t)lwjson_get_val_int(t);

	t = find_field(parser, parent, "brightness_max");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		s_pdata->brightness_max = (uint8_t)lwjson_get_val_int(t);

	t = find_field(parser, parent, "color_order");
	if (t != NULL && t->type == LWJSON_TYPE_STRING) {
		size_t slen;
		const char *val = lwjson_get_val_string(t, &slen);
		if (val != NULL && slen > 0) {
			char order_str[8];
			size_t n = slen < sizeof(order_str) - 1 ? slen : sizeof(order_str) - 1;
			memcpy(order_str, val, n);
			order_str[n] = '\0';
			s_pdata->color_order = color_order_from_str(order_str);
		}
	}
}

/* --- Light config rebuild --- */

static void rebuild_config(void) {
	s_light_cfg->pin              = CONFIG_LIGHT_PIN_CTL;
	s_light_cfg->led_count        = s_pdata->led_count;
	s_light_cfg->led_skip         = s_pdata->skip_leds;
	s_light_cfg->color_correction = s_pdata->color_correction;
	s_light_cfg->color_order      = s_pdata->color_order;
	s_light_cfg->kelvin_warm      = CONFIG_LIGHT_COLOR_KELVIN_WARM;
	s_light_cfg->kelvin_cold      = CONFIG_LIGHT_COLOR_KELVIN_COLD;
	s_light_cfg->kelvin_initial   = CONFIG_LIGHT_COLOR_KELVIN_INITIAL;
	s_light_cfg->transition_ms    = CONFIG_LIGHT_TRANSITION_COLOR;
	s_light_cfg->brightness       = s_pdata->brightness_min;
	s_light_cfg->brightness_max   = s_pdata->brightness_max;
}

/* --- API handlers --- */

static void handle_page(void *user_data) {
	(void)user_data;
	cwebserver_send_header("Content-Encoding", "gzip", false);
	cwebserver_send_progmem(200, "text/html", provisioning_page, provisioning_page_len);
}

static void handle_get_configuration(void *user_data) {
	(void)user_data;
	lwjson_serializer_t ser;
	if (lwjson_serializer_init(&ser, json_buf, sizeof(json_buf) - 1) != lwjsonOK) {
		cwebserver_send(500, "application/json", "{\"error\":\"serializer\"}");
		return;
	}

	lwjson_serializer_start_object(&ser, NULL, 0);

	lwjson_serializer_start_object(&ser, LK("wifi"));
	lwjson_serializer_add_string(&ser, LK("ssid"),
		s_pdata->wifi_ssid, strlen(s_pdata->wifi_ssid));
	lwjson_serializer_add_string(&ser, LK("password"),
		s_pdata->wifi_password, strlen(s_pdata->wifi_password));
	lwjson_serializer_end_object(&ser);

	lwjson_serializer_start_object(&ser, LK("mqtt"));
	lwjson_serializer_add_string(&ser, LK("host"),
		s_pdata->mqtt_host, strlen(s_pdata->mqtt_host));
	lwjson_serializer_add_uint(&ser, LK("port"), s_pdata->mqtt_port);
	lwjson_serializer_add_string(&ser, LK("username"),
		s_pdata->mqtt_username, strlen(s_pdata->mqtt_username));
	lwjson_serializer_add_string(&ser, LK("password"),
		s_pdata->mqtt_password, strlen(s_pdata->mqtt_password));
	lwjson_serializer_end_object(&ser);

	lwjson_serializer_start_object(&ser, LK("light"));
	lwjson_serializer_add_uint(&ser, LK("led_count"), s_pdata->led_count);
	lwjson_serializer_add_uint(&ser, LK("skip_leds"), s_pdata->skip_leds);
	lwjson_serializer_add_uint(&ser, LK("color_correction"), s_pdata->color_correction);
	lwjson_serializer_add_uint(&ser, LK("brightness_min"), s_pdata->brightness_min);
	lwjson_serializer_add_uint(&ser, LK("brightness_max"), s_pdata->brightness_max);
	const char *order = color_order_to_str(s_pdata->color_order);
	lwjson_serializer_add_string(&ser, LK("color_order"), order, strlen(order));
	lwjson_serializer_end_object(&ser);

	lwjson_serializer_end_object(&ser);

	if (!ser_finalize(&ser)) {
		cwebserver_send(500, "application/json", "{\"error\":\"serializer\"}");
		return;
	}
	cwebserver_send(200, "application/json", json_buf);
}

static void handle_post_configuration(void *user_data) {
	(void)user_data;
	size_t body_len = cwebserver_body(json_buf, sizeof(json_buf));

	lwjson_token_t tokens[PARSE_TOKENS];
	lwjson_t parser;
	lwjson_init(&parser, tokens, LWJSON_ARRAYSIZE(tokens));

	if (lwjson_parse_ex(&parser, json_buf, body_len) != lwjsonOK) {
		cwebserver_send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	const lwjson_token_t *wifi = lwjson_find(&parser, "wifi");
	if (wifi != NULL && wifi->type == LWJSON_TYPE_OBJECT) {
		const lwjson_token_t *t;
		t = lwjson_find_ex(&parser, wifi, "ssid");
		if (t != NULL && t->type == LWJSON_TYPE_STRING)
			copy_json_string(t, s_pdata->wifi_ssid, sizeof(s_pdata->wifi_ssid));
		t = lwjson_find_ex(&parser, wifi, "password");
		if (t != NULL && t->type == LWJSON_TYPE_STRING)
			copy_json_string(t, s_pdata->wifi_password, sizeof(s_pdata->wifi_password));
	}

	const lwjson_token_t *mqtt = lwjson_find(&parser, "mqtt");
	if (mqtt != NULL && mqtt->type == LWJSON_TYPE_OBJECT) {
		const lwjson_token_t *t;
		t = lwjson_find_ex(&parser, mqtt, "host");
		if (t != NULL && t->type == LWJSON_TYPE_STRING)
			copy_json_string(t, s_pdata->mqtt_host, sizeof(s_pdata->mqtt_host));
		t = lwjson_find_ex(&parser, mqtt, "port");
		if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
			s_pdata->mqtt_port = (uint16_t)lwjson_get_val_int(t);
		t = lwjson_find_ex(&parser, mqtt, "username");
		if (t != NULL && t->type == LWJSON_TYPE_STRING)
			copy_json_string(t, s_pdata->mqtt_username, sizeof(s_pdata->mqtt_username));
		t = lwjson_find_ex(&parser, mqtt, "password");
		if (t != NULL && t->type == LWJSON_TYPE_STRING)
			copy_json_string(t, s_pdata->mqtt_password, sizeof(s_pdata->mqtt_password));
	}

	const lwjson_token_t *light = lwjson_find(&parser, "light");
	if (light != NULL && light->type == LWJSON_TYPE_OBJECT) {
		parse_light_fields(&parser, light);
	}

	lwjson_free(&parser);

	persistent_data_save(s_pdata);
	rebuild_config();
	light_update_config(s_light_cfg);

	cwebserver_send(204, "text/plain", "");
	if (s_on_configuration_changed != NULL) {
		s_on_configuration_changed();
	}
}

static void handle_post_light_configuration(void *user_data) {
	(void)user_data;
	size_t body_len = cwebserver_body(json_buf, sizeof(json_buf));

	lwjson_token_t tokens[PARSE_TOKENS];
	lwjson_t parser;
	lwjson_init(&parser, tokens, LWJSON_ARRAYSIZE(tokens));

	if (lwjson_parse_ex(&parser, json_buf, body_len) != lwjsonOK) {
		cwebserver_send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	parse_light_fields(&parser, NULL);
	lwjson_free(&parser);

	rebuild_config();
	light_update_config(s_light_cfg);

	cwebserver_send(204, "text/plain", "");
}

static void handle_post_light_test(void *user_data) {
	(void)user_data;
	size_t body_len = cwebserver_body(json_buf, sizeof(json_buf));

	lwjson_token_t tokens[PARSE_TOKENS];
	lwjson_t parser;
	lwjson_init(&parser, tokens, LWJSON_ARRAYSIZE(tokens));

	if (lwjson_parse_ex(&parser, json_buf, body_len) != lwjsonOK) {
		cwebserver_send(400, "application/json", "{\"error\":\"invalid json\"}");
		return;
	}

	light_cmd_t cmd;
	cmd.type = LIGHT_CMD_COLOR;
	cmd.color.r = 0;
	cmd.color.g = 0;
	cmd.color.b = 0;

	const lwjson_token_t *t;
	t = lwjson_find(&parser, "r");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		cmd.color.r = (uint8_t)lwjson_get_val_int(t);
	t = lwjson_find(&parser, "g");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		cmd.color.g = (uint8_t)lwjson_get_val_int(t);
	t = lwjson_find(&parser, "b");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		cmd.color.b = (uint8_t)lwjson_get_val_int(t);
	light_send_cmd(&cmd);

	cmd.type = LIGHT_CMD_BRIGHTNESS;
	cmd.brightness = 128;
	t = lwjson_find(&parser, "brightness");
	if (t != NULL && t->type == LWJSON_TYPE_NUM_INT)
		cmd.brightness = (uint8_t)lwjson_get_val_int(t);
	light_send_cmd(&cmd);

	lwjson_free(&parser);
	cwebserver_send(204, "text/plain", "");
}

static void handle_get_system(void *user_data) {
	(void)user_data;
	lwjson_serializer_t ser;
	if (lwjson_serializer_init(&ser, json_buf, sizeof(json_buf) - 1) != lwjsonOK) {
		cwebserver_send(500, "application/json", "{\"error\":\"serializer\"}");
		return;
	}

	lwjson_serializer_start_object(&ser, NULL, 0);

	const char *ver = __DATE__ " " __TIME__;
	lwjson_serializer_add_string(&ser, LK("build_version"), ver, strlen(ver));

	const char *network_mode = wifi_network_mode_string();
	lwjson_serializer_add_string(&ser, LK("network_mode"), network_mode, strlen(network_mode));

	char sta_ip[IP_BUF_SIZE];
	size_t sta_ip_len = wifi_sta_ip_string(sta_ip, sizeof(sta_ip));
	lwjson_serializer_add_string(&ser, LK("sta_ip"), sta_ip, sta_ip_len);

	char ap_ip[IP_BUF_SIZE];
	size_t ap_ip_len = wifi_ap_ip_string(ap_ip, sizeof(ap_ip));
	lwjson_serializer_add_string(&ser, LK("ap_ip"), ap_ip, ap_ip_len);

	uint8_t mac[6];
	wifi_mac_address(mac);
	lwjson_serializer_start_array(&ser, LK("mac_address"));
	for (int i = 0; i < 6; i++) {
		lwjson_serializer_add_uint(&ser, NULL, 0, mac[i]);
	}
	lwjson_serializer_end_array(&ser);

	lwjson_serializer_end_object(&ser);

	if (!ser_finalize(&ser)) {
		cwebserver_send(500, "application/json", "{\"error\":\"serializer\"}");
		return;
	}
	cwebserver_send(200, "application/json", json_buf);
}

/* --- Public API --- */

void web_init(
	persistent_data_t *pdata,
	light_config_t *light_cfg,
	web_configuration_changed_cb_t on_configuration_changed
) {
	cwebserver_init(80);
	s_pdata = pdata;
	s_light_cfg = light_cfg;
	s_on_configuration_changed = on_configuration_changed;
	rebuild_config();

	cwebserver_on("/", WEB_METHOD_GET, handle_page, NULL);
	cwebserver_on("/api/configuration", WEB_METHOD_GET, handle_get_configuration, NULL);
	cwebserver_on("/api/configuration", WEB_METHOD_POST, handle_post_configuration, NULL);
	cwebserver_on("/api/configuration/light", WEB_METHOD_POST, handle_post_light_configuration, NULL);
	cwebserver_on("/api/light/test", WEB_METHOD_POST, handle_post_light_test, NULL);
	cwebserver_on("/api/system", WEB_METHOD_GET, handle_get_system, NULL);
}

void web_start(void) {
	cwebserver_start();
}

void web_loop(void) {
	cwebserver_loop();
}
