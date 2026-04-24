#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cwebserver_method_t {
    WEB_METHOD_ANY = 0,
    WEB_METHOD_GET,
    WEB_METHOD_POST,
    WEB_METHOD_PUT,
    WEB_METHOD_PATCH,
    WEB_METHOD_DELETE,
    WEB_METHOD_OPTIONS,
    WEB_METHOD_HEAD,
} cwebserver_method_t;

typedef void (*cwebserver_handler_t)(void *user_data);

void cwebserver_init(uint16_t port);
void cwebserver_start(void);
void cwebserver_loop(void);
void cwebserver_on(const char *uri,
                   cwebserver_method_t method,
                   cwebserver_handler_t handler,
                   void *user_data);
void cwebserver_on_not_found(cwebserver_handler_t handler, void *user_data);

cwebserver_method_t cwebserver_method(void);
bool cwebserver_has_arg(const char *name);
size_t cwebserver_arg(const char *name, char *buf, size_t buf_size);
size_t cwebserver_body(char *buf, size_t buf_size);
size_t cwebserver_uri(char *buf, size_t buf_size);

void cwebserver_send(int code, const char *content_type, const char *content);
void cwebserver_send_header(const char *name, const char *value, bool first);
void cwebserver_send_progmem(int code,
                             const char *content_type,
                             const uint8_t *content,
                             size_t content_length);

#ifdef __cplusplus
}
#endif
