#include "ha_light_entity.h"

#include <lwjson/lwjson.h>
#include <lwjson/lwjson_serializer.h>
#include <string.h>

#define LWJSON_PARSE_TOKENS 32

/* Pass string literal as (ptr, len) pair without strlen */
#define LK(s) (s), (sizeof(s) - 1)

static char effect_buf[32];

static bool ser_finalize(lwjson_serializer_t *ser, char *buffer, size_t size) {
    size_t len = 0;
    if (lwjson_serializer_finalize(ser, &len) != lwjsonOK) {
        return false;
    }
    if (len < size) {
        buffer[len] = '\0';
    }
    return true;
}

bool ha_light_serialize_config(ha_entity_t *entity,
                               const ha_light_config_t *config,
                               char *buffer,
                               size_t size) {
    lwjson_serializer_t ser;
    if (lwjson_serializer_init(&ser, buffer, size - 1) != lwjsonOK) {
        return false;
    }

    lwjson_serializer_start_object(&ser, NULL, 0);

    lwjson_serializer_add_string(&ser, LK("name"), entity->name,
                                 strlen(entity->name));

    const char *id = ha_entity_id(entity);
    lwjson_serializer_add_string(&ser, LK("unique_id"), id, strlen(id));

    if (entity->icon != NULL) {
        lwjson_serializer_add_string(&ser, LK("icon"), entity->icon,
                                     strlen(entity->icon));
    }

    const char *st = ha_entity_state_topic(entity);
    lwjson_serializer_add_string(&ser, LK("state_topic"), st, strlen(st));

    const char *cmd = ha_entity_command_topic(entity);
    if (cmd != NULL) {
        lwjson_serializer_add_string(&ser, LK("command_topic"), cmd, strlen(cmd));
    }

    const char *avail = ha_entity_availability_topic(entity);
    lwjson_serializer_add_string(&ser, LK("availability_topic"), avail,
                                 strlen(avail));

    lwjson_serializer_start_object(&ser, LK("device"));
    lwjson_serializer_add_string(&ser, LK("name"), entity->device->name,
                                 strlen(entity->device->name));
    lwjson_serializer_start_array(&ser, LK("identifiers"));
    lwjson_serializer_add_string(&ser, NULL, 0, entity->device->id,
                                 strlen(entity->device->id));
    lwjson_serializer_end_array(&ser);
    lwjson_serializer_end_object(&ser);

    lwjson_serializer_add_string(&ser, LK("schema"), LK("json"));
    lwjson_serializer_add_bool(&ser, LK("brightness"), 1);
    lwjson_serializer_add_bool(&ser, LK("color_temp_kelvin"), 1);
    lwjson_serializer_add_uint(&ser, LK("min_kelvin"), config->min_kelvin);
    lwjson_serializer_add_uint(&ser, LK("max_kelvin"), config->max_kelvin);

    if (config->effect_count > 0) {
        lwjson_serializer_add_bool(&ser, LK("effect"), 1);
        lwjson_serializer_start_array(&ser, LK("effect_list"));
        for (uint16_t i = 0; i < config->effect_count; i++) {
            lwjson_serializer_add_string(&ser, NULL, 0, config->effects[i],
                                         strlen(config->effects[i]));
        }
        lwjson_serializer_end_array(&ser);
    }

    lwjson_serializer_start_array(&ser, LK("supported_color_modes"));
    lwjson_serializer_add_string(&ser, NULL, 0, LK("color_temp"));
    lwjson_serializer_add_string(&ser, NULL, 0, LK("rgb"));
    lwjson_serializer_end_array(&ser);

    lwjson_serializer_end_object(&ser);

    return ser_finalize(&ser, buffer, size);
}

bool ha_light_serialize_state(const ha_light_state_t *state,
                              char *buffer,
                              size_t size) {
    lwjson_serializer_t ser;
    if (lwjson_serializer_init(&ser, buffer, size - 1) != lwjsonOK) {
        return false;
    }

    lwjson_serializer_start_object(&ser, NULL, 0);

    if (state->color_mode == HA_COLOR_MODE_RGB) {
        lwjson_serializer_add_string(&ser, LK("color_mode"), LK("rgb"));
        lwjson_serializer_start_object(&ser, LK("color"));
        lwjson_serializer_add_uint(&ser, LK("r"), state->color.r);
        lwjson_serializer_add_uint(&ser, LK("g"), state->color.g);
        lwjson_serializer_add_uint(&ser, LK("b"), state->color.b);
        lwjson_serializer_end_object(&ser);
    } else {
        lwjson_serializer_add_string(&ser, LK("color_mode"), LK("color_temp"));
        lwjson_serializer_add_uint(&ser, LK("color_temp"), state->color_temp);
    }

    if (state->effect != NULL) {
        lwjson_serializer_add_string(&ser, LK("effect"), state->effect,
                                     strlen(state->effect));
    }
    lwjson_serializer_add_uint(&ser, LK("brightness"), state->brightness);
    lwjson_serializer_add_string(&ser, LK("state"), state->enabled ? "ON" : "OFF",
                                 state->enabled ? 2 : 3);

    lwjson_serializer_end_object(&ser);

    return ser_finalize(&ser, buffer, size);
}

uint8_t ha_light_parse_command(const uint8_t *payload,
                               size_t len,
                               ha_light_state_t *state) {
    uint8_t fields = 0;
    lwjson_token_t tokens[LWJSON_PARSE_TOKENS];
    lwjson_t parser;

    lwjson_init(&parser, tokens, LWJSON_ARRAYSIZE(tokens));
    if (lwjson_parse_ex(&parser, payload, len) != lwjsonOK) {
        return 0;
    }

    const lwjson_token_t *t;

    t = lwjson_find(&parser, "state");
    if (t != NULL && t->type == LWJSON_TYPE_STRING) {
        state->enabled = lwjson_string_compare(t, "ON");
        fields |= HA_LIGHT_FIELD_STATE;
    }

    t = lwjson_find(&parser, "brightness");
    if (t != NULL && t->type == LWJSON_TYPE_NUM_INT) {
        state->brightness = (uint16_t)lwjson_get_val_int(t);
        fields |= HA_LIGHT_FIELD_BRIGHTNESS;
    }

    t = lwjson_find(&parser, "effect");
    if (t != NULL && t->type == LWJSON_TYPE_STRING) {
        size_t fx_len = 0;
        const char *fx = lwjson_get_val_string(t, &fx_len);
        if (fx != NULL && fx_len > 0) {
            size_t n =
                fx_len < sizeof(effect_buf) - 1 ? fx_len : sizeof(effect_buf) - 1;
            memcpy(effect_buf, fx, n);
            effect_buf[n] = '\0';
            state->effect = effect_buf;
            fields |= HA_LIGHT_FIELD_EFFECT;
        }
    }

    t = lwjson_find(&parser, "color");
    if (t != NULL && t->type == LWJSON_TYPE_OBJECT) {
        const lwjson_token_t *r = lwjson_find_ex(&parser, t, "r");
        const lwjson_token_t *g = lwjson_find_ex(&parser, t, "g");
        const lwjson_token_t *b = lwjson_find_ex(&parser, t, "b");
        if (r != NULL && g != NULL && b != NULL) {
            state->color.r = (uint8_t)lwjson_get_val_int(r);
            state->color.g = (uint8_t)lwjson_get_val_int(g);
            state->color.b = (uint8_t)lwjson_get_val_int(b);
            state->color_mode = HA_COLOR_MODE_RGB;
            fields |= HA_LIGHT_FIELD_COLOR;
        }
    } else {
        t = lwjson_find(&parser, "color_temp");
        if (t != NULL && t->type == LWJSON_TYPE_NUM_INT) {
            state->color_temp = (uint16_t)lwjson_get_val_int(t);
            state->color_mode = HA_COLOR_MODE_TEMP;
            fields |= HA_LIGHT_FIELD_COLOR_TEMP;
        }
    }

    lwjson_free(&parser);
    return fields;
}
