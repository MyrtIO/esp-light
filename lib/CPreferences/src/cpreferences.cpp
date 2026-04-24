#include "cpreferences.h"
#include <Preferences.h>
#include <new>
#include <string.h>

struct cpreferences_t {
	Preferences prefs;
	bool opened;
};

static size_t copy_string(const String &value, char *buf, size_t buf_size) {
	if (buf == NULL || buf_size == 0) {
		return value.length();
	}

	size_t len = value.length();
	size_t copy_len = (len < (buf_size - 1)) ? len : (buf_size - 1);
	memcpy(buf, value.c_str(), copy_len);
	buf[copy_len] = '\0';
	return len;
}

cpreferences_t *cpreferences_create(void) {
	return new (std::nothrow) cpreferences_t{Preferences(), false};
}

void cpreferences_destroy(cpreferences_t *prefs) {
	if (prefs == NULL) {
		return;
	}

	if (prefs->opened) {
		prefs->prefs.end();
	}
	delete prefs;
}

bool cpreferences_begin(cpreferences_t *prefs, const char *namespace_name, bool read_only) {
	if (prefs == NULL || namespace_name == NULL) {
		return false;
	}

	if (prefs->opened) {
		prefs->prefs.end();
		prefs->opened = false;
	}

	prefs->opened = prefs->prefs.begin(namespace_name, read_only);
	return prefs->opened;
}

void cpreferences_end(cpreferences_t *prefs) {
	if (prefs == NULL || !prefs->opened) {
		return;
	}

	prefs->prefs.end();
	prefs->opened = false;
}

bool cpreferences_is_key(cpreferences_t *prefs, const char *key) {
	return prefs != NULL && prefs->opened && key != NULL && prefs->prefs.isKey(key);
}

size_t cpreferences_get_string(cpreferences_t *prefs,
                               const char *key,
                               char *buf,
                               size_t buf_size,
                               const char *default_value) {
	if (buf != NULL && buf_size > 0) {
		buf[0] = '\0';
	}

	if (prefs == NULL || !prefs->opened || key == NULL) {
		if (default_value != NULL) {
			return copy_string(String(default_value), buf, buf_size);
		}
		return 0;
	}

	String value = prefs->prefs.getString(key, default_value != NULL ? default_value : "");
	return copy_string(value, buf, buf_size);
}

bool cpreferences_put_string(cpreferences_t *prefs, const char *key, const char *value) {
	if (prefs == NULL || !prefs->opened || key == NULL || value == NULL) {
		return false;
	}
	return prefs->prefs.putString(key, value) > 0;
}

bool cpreferences_get_bool(cpreferences_t *prefs, const char *key, bool default_value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return default_value;
	}
	return prefs->prefs.getBool(key, default_value);
}

bool cpreferences_put_bool(cpreferences_t *prefs, const char *key, bool value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return false;
	}
	return prefs->prefs.putBool(key, value) > 0;
}

uint8_t cpreferences_get_uchar(cpreferences_t *prefs, const char *key, uint8_t default_value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return default_value;
	}
	return prefs->prefs.getUChar(key, default_value);
}

bool cpreferences_put_uchar(cpreferences_t *prefs, const char *key, uint8_t value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return false;
	}
	return prefs->prefs.putUChar(key, value) > 0;
}

uint16_t cpreferences_get_ushort(cpreferences_t *prefs, const char *key, uint16_t default_value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return default_value;
	}
	return prefs->prefs.getUShort(key, default_value);
}

bool cpreferences_put_ushort(cpreferences_t *prefs, const char *key, uint16_t value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return false;
	}
	return prefs->prefs.putUShort(key, value) > 0;
}

uint32_t cpreferences_get_ulong(cpreferences_t *prefs, const char *key, uint32_t default_value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return default_value;
	}
	return prefs->prefs.getULong(key, default_value);
}

bool cpreferences_put_ulong(cpreferences_t *prefs, const char *key, uint32_t value) {
	if (prefs == NULL || !prefs->opened || key == NULL) {
		return false;
	}
	return prefs->prefs.putULong(key, value) > 0;
}
