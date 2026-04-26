#include "light_hal.h"

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "esp_log.h"

#define LIGHT_HAL_RMT_RESOLUTION_HZ 10000000U
#define LIGHT_HAL_RMT_TX_QUEUE_DEPTH 4U
#define LIGHT_HAL_WS2811_T0H_TICKS 5U
#define LIGHT_HAL_WS2811_T0L_TICKS 20U
#define LIGHT_HAL_WS2811_T1H_TICKS 12U
#define LIGHT_HAL_WS2811_T1L_TICKS 13U
#define LIGHT_HAL_WS2811_RESET_TICKS 250U

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define LIGHT_HAL_RMT_MEM_BLOCK_SYMBOLS 64U
#else
#define LIGHT_HAL_RMT_MEM_BLOCK_SYMBOLS 48U
#endif

typedef struct {
	rmt_encoder_t base;
	rmt_encoder_t *bytes_encoder;
	rmt_encoder_t *copy_encoder;
	int state;
	rmt_symbol_word_t reset_code;
} light_hal_strip_encoder_t;

_Static_assert(sizeof(rgb_t) == 3, "rgb_t must stay tightly packed for RMT output");

static const char *TAG = "LightHAL";

static rgb_t raw_leds[CONFIG_LIGHT_LED_MAX_COUNT];
static rmt_channel_handle_t s_rmt_channel = NULL;
static rmt_encoder_handle_t s_rmt_encoder = NULL;
static rmt_transmit_config_t s_rmt_tx_config = {
	.loop_count = 0,
};
static bool s_show_error_logged = false;

static size_t light_hal_encode(
	rmt_encoder_t *encoder,
	rmt_channel_handle_t channel,
	const void *data,
	size_t data_size,
	rmt_encode_state_t *ret_state
) {
	light_hal_strip_encoder_t *strip_encoder = __containerof(
		encoder, light_hal_strip_encoder_t, base
	);
	rmt_encode_state_t session_state = 0;
	rmt_encode_state_t state = 0;
	size_t encoded_symbols = 0;

	switch (strip_encoder->state) {
	case 0:
		encoded_symbols += strip_encoder->bytes_encoder->encode(
			strip_encoder->bytes_encoder,
			channel,
			data,
			data_size,
			&session_state
		);
		if (session_state & RMT_ENCODING_COMPLETE) {
			strip_encoder->state = 1;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
			goto out;
		}
		/* fall through */
	case 1:
		encoded_symbols += strip_encoder->copy_encoder->encode(
			strip_encoder->copy_encoder,
			channel,
			&strip_encoder->reset_code,
			sizeof(strip_encoder->reset_code),
			&session_state
		);
		if (session_state & RMT_ENCODING_COMPLETE) {
			strip_encoder->state = 0;
			state |= RMT_ENCODING_COMPLETE;
		}
		if (session_state & RMT_ENCODING_MEM_FULL) {
			state |= RMT_ENCODING_MEM_FULL;
		}
		break;
	default:
		strip_encoder->state = 0;
		state |= RMT_ENCODING_COMPLETE;
		break;
	}

out:
	*ret_state = state;
	return encoded_symbols;
}

static esp_err_t light_hal_delete_encoder(rmt_encoder_t *encoder) {
	light_hal_strip_encoder_t *strip_encoder = __containerof(
		encoder, light_hal_strip_encoder_t, base
	);

	rmt_del_encoder(strip_encoder->bytes_encoder);
	rmt_del_encoder(strip_encoder->copy_encoder);
	free(strip_encoder);
	return ESP_OK;
}

static esp_err_t light_hal_reset_encoder(rmt_encoder_t *encoder) {
	light_hal_strip_encoder_t *strip_encoder = __containerof(
		encoder, light_hal_strip_encoder_t, base
	);

	rmt_encoder_reset(strip_encoder->bytes_encoder);
	rmt_encoder_reset(strip_encoder->copy_encoder);
	strip_encoder->state = 0;
	return ESP_OK;
}

static esp_err_t light_hal_new_encoder(rmt_encoder_handle_t *ret_encoder) {
	light_hal_strip_encoder_t *strip_encoder = calloc(1, sizeof(*strip_encoder));
	if (strip_encoder == NULL) {
		return ESP_ERR_NO_MEM;
	}

	strip_encoder->base.encode = light_hal_encode;
	strip_encoder->base.del = light_hal_delete_encoder;
	strip_encoder->base.reset = light_hal_reset_encoder;

	rmt_bytes_encoder_config_t bytes_encoder_config = {};
	bytes_encoder_config.bit0.level0 = 1;
	bytes_encoder_config.bit0.duration0 = LIGHT_HAL_WS2811_T0H_TICKS;
	bytes_encoder_config.bit0.level1 = 0;
	bytes_encoder_config.bit0.duration1 = LIGHT_HAL_WS2811_T0L_TICKS;
	bytes_encoder_config.bit1.level0 = 1;
	bytes_encoder_config.bit1.duration0 = LIGHT_HAL_WS2811_T1H_TICKS;
	bytes_encoder_config.bit1.level1 = 0;
	bytes_encoder_config.bit1.duration1 = LIGHT_HAL_WS2811_T1L_TICKS;
	bytes_encoder_config.flags.msb_first = 1;

	esp_err_t err = rmt_new_bytes_encoder(
		&bytes_encoder_config,
		&strip_encoder->bytes_encoder
	);
	if (err != ESP_OK) {
		free(strip_encoder);
		return err;
	}

	rmt_copy_encoder_config_t copy_encoder_config = {};
	err = rmt_new_copy_encoder(&copy_encoder_config, &strip_encoder->copy_encoder);
	if (err != ESP_OK) {
		rmt_del_encoder(strip_encoder->bytes_encoder);
		free(strip_encoder);
		return err;
	}

	strip_encoder->reset_code = (rmt_symbol_word_t){
		.level0 = 0,
		.duration0 = LIGHT_HAL_WS2811_RESET_TICKS,
		.level1 = 0,
		.duration1 = LIGHT_HAL_WS2811_RESET_TICKS,
	};

	*ret_encoder = &strip_encoder->base;
	return ESP_OK;
}

static void light_hal_release_backend(void) {
	if (s_rmt_encoder != NULL) {
		rmt_del_encoder(s_rmt_encoder);
		s_rmt_encoder = NULL;
	}

	if (s_rmt_channel != NULL) {
		rmt_del_channel(s_rmt_channel);
		s_rmt_channel = NULL;
	}
}

static esp_err_t light_hal_transmit(void) {
	esp_err_t err = rmt_enable(s_rmt_channel);
	if (err != ESP_OK) {
		return err;
	}

	err = rmt_transmit(
		s_rmt_channel,
		s_rmt_encoder,
		raw_leds,
		sizeof(raw_leds),
		&s_rmt_tx_config
	);
	if (err == ESP_OK) {
		err = rmt_tx_wait_all_done(s_rmt_channel, -1);
	}

	esp_err_t disable_err = rmt_disable(s_rmt_channel);
	if (err != ESP_OK) {
		return err;
	}
	return disable_err;
}

void light_hal_set_pixel(uint16_t i, rgb_t c) {
	raw_leds[i] = c;
}

rgb_t light_hal_get_pixel(uint16_t i) {
	return raw_leds[i];
}

void light_hal_show(void) {
	if (s_rmt_channel == NULL || s_rmt_encoder == NULL) {
		return;
	}

	esp_err_t err = light_hal_transmit();
	if (err != ESP_OK && !s_show_error_logged) {
		s_show_error_logged = true;
		ESP_LOGE(TAG, "rmt transmit failed: %s", esp_err_to_name(err));
	}
}

void light_hal_init(void) {
	memset(raw_leds, 0, sizeof(raw_leds));
	s_show_error_logged = false;

	if (s_rmt_channel != NULL && s_rmt_encoder != NULL) {
		light_hal_show();
		return;
	}

	light_hal_release_backend();

	rmt_tx_channel_config_t rmt_channel_config = {};
	rmt_channel_config.clk_src = RMT_CLK_SRC_DEFAULT;
	rmt_channel_config.gpio_num = (gpio_num_t)CONFIG_LIGHT_CTL_GPIO;
	rmt_channel_config.mem_block_symbols = LIGHT_HAL_RMT_MEM_BLOCK_SYMBOLS;
	rmt_channel_config.resolution_hz = LIGHT_HAL_RMT_RESOLUTION_HZ;
	rmt_channel_config.trans_queue_depth = LIGHT_HAL_RMT_TX_QUEUE_DEPTH;
	rmt_channel_config.flags.with_dma = true;
	rmt_channel_config.flags.invert_out = false;

	esp_err_t err = rmt_new_tx_channel(&rmt_channel_config, &s_rmt_channel);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "rmt channel init failed: %s", esp_err_to_name(err));
		light_hal_release_backend();
		return;
	}

	err = light_hal_new_encoder(&s_rmt_encoder);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "rmt encoder init failed: %s", esp_err_to_name(err));
		light_hal_release_backend();
		return;
	}

	err = light_hal_transmit();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "initial clear failed: %s", esp_err_to_name(err));
		return;
	}

	ESP_LOGI(
		TAG,
		"rmt strip ready gpio=%d symbols=%u bytes=%u",
		CONFIG_LIGHT_CTL_GPIO,
		(unsigned)LIGHT_HAL_RMT_MEM_BLOCK_SYMBOLS,
		(unsigned)sizeof(raw_leds)
	);
}
