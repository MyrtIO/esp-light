#include "cwebserver.h"
#include <HTTP_Method.h>
#include <WebServer.h>
#include <new>
#include <string.h>
#include <vector>

struct route_entry_t {
	const char *uri;
	cwebserver_method_t method;
	cwebserver_handler_t handler;
	void *user_data;
};

struct not_found_entry_t {
	cwebserver_handler_t handler;
	void *user_data;
};

static WebServer *server = nullptr;
static uint16_t configured_port = 80;
static std::vector<route_entry_t> routes;
static not_found_entry_t not_found = {nullptr, nullptr};

static size_t copy_string(const String &value, char *buf, size_t buf_size) {
	if (buf == NULL || buf_size == 0) {
		return 0;
	}

	size_t len = value.length();
	size_t copy_len = (len < (buf_size - 1)) ? len : (buf_size - 1);
	memcpy(buf, value.c_str(), copy_len);
	buf[copy_len] = '\0';
	return copy_len;
}

static HTTPMethod to_http_method(cwebserver_method_t method) {
	switch (method) {
	case WEB_METHOD_GET:
		return HTTP_GET;
	case WEB_METHOD_POST:
		return HTTP_POST;
	case WEB_METHOD_PUT:
		return HTTP_PUT;
	case WEB_METHOD_PATCH:
		return HTTP_PATCH;
	case WEB_METHOD_DELETE:
		return HTTP_DELETE;
	case WEB_METHOD_OPTIONS:
		return HTTP_OPTIONS;
	case WEB_METHOD_HEAD:
		return HTTP_HEAD;
	case WEB_METHOD_ANY:
	default:
		return HTTP_ANY;
	}
}

static cwebserver_method_t from_http_method(HTTPMethod method) {
	switch (method) {
	case HTTP_GET:
		return WEB_METHOD_GET;
	case HTTP_POST:
		return WEB_METHOD_POST;
	case HTTP_PUT:
		return WEB_METHOD_PUT;
	case HTTP_PATCH:
		return WEB_METHOD_PATCH;
	case HTTP_DELETE:
		return WEB_METHOD_DELETE;
	case HTTP_OPTIONS:
		return WEB_METHOD_OPTIONS;
	case HTTP_HEAD:
		return WEB_METHOD_HEAD;
	default:
		return WEB_METHOD_ANY;
	}
}

static void register_route(const route_entry_t &route) {
	server->on(route.uri, to_http_method(route.method), [route]() {
		route.handler(route.user_data);
	});
}

void cwebserver_init(uint16_t port) {
	configured_port = port;

	if (server != nullptr) {
		delete server;
		server = nullptr;
	}

	server = new (std::nothrow) WebServer(configured_port);
	if (server == nullptr) {
		return;
	}

	for (const route_entry_t &route : routes) {
		register_route(route);
	}

	if (not_found.handler != nullptr) {
		server->onNotFound([]() {
			not_found.handler(not_found.user_data);
		});
	}
}

void cwebserver_start(void) {
	if (server != nullptr) {
		server->begin();
	}
}

void cwebserver_loop(void) {
	if (server != nullptr) {
		server->handleClient();
	}
}

void cwebserver_on(const char *uri,
                   cwebserver_method_t method,
                   cwebserver_handler_t handler,
                   void *user_data) {
	if (uri == NULL || handler == NULL) {
		return;
	}

	route_entry_t route = {uri, method, handler, user_data};
	routes.push_back(route);

	if (server != nullptr) {
		register_route(route);
	}
}

void cwebserver_on_not_found(cwebserver_handler_t handler, void *user_data) {
	not_found.handler = handler;
	not_found.user_data = user_data;

	if (server != nullptr && handler != nullptr) {
		server->onNotFound([]() {
			not_found.handler(not_found.user_data);
		});
	}
}

cwebserver_method_t cwebserver_method(void) {
	if (server == nullptr) {
		return WEB_METHOD_ANY;
	}
	return from_http_method(server->method());
}

bool cwebserver_has_arg(const char *name) {
	return server != nullptr && name != NULL && server->hasArg(name);
}

size_t cwebserver_arg(const char *name, char *buf, size_t buf_size) {
	if (server == nullptr || name == NULL) {
		if (buf != NULL && buf_size > 0) {
			buf[0] = '\0';
		}
		return 0;
	}

	return copy_string(server->arg(name), buf, buf_size);
}

size_t cwebserver_body(char *buf, size_t buf_size) {
	if (server == nullptr) {
		if (buf != NULL && buf_size > 0) {
			buf[0] = '\0';
		}
		return 0;
	}

	return copy_string(server->arg("plain"), buf, buf_size);
}

size_t cwebserver_uri(char *buf, size_t buf_size) {
	if (server == nullptr) {
		if (buf != NULL && buf_size > 0) {
			buf[0] = '\0';
		}
		return 0;
	}

	return copy_string(server->uri(), buf, buf_size);
}

void cwebserver_send(int code, const char *content_type, const char *content) {
	if (server != nullptr) {
		server->send(code, content_type, content);
	}
}

void cwebserver_send_header(const char *name, const char *value, bool first) {
	if (server != nullptr && name != NULL && value != NULL) {
		server->sendHeader(name, value, first);
	}
}

void cwebserver_send_progmem(int code,
                             const char *content_type,
                             const uint8_t *content,
                             size_t content_length) {
	if (server != nullptr && content_type != NULL && content != NULL) {
		server->send_P(code, content_type, reinterpret_cast<PGM_P>(content), content_length);
	}
}
