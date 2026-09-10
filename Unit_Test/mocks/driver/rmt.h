#ifndef MOCK_DRIVER_RMT_H
#define MOCK_DRIVER_RMT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RMT_CHANNEL_0 = 0,
    RMT_CHANNEL_1,
    RMT_CHANNEL_2,
    RMT_CHANNEL_3,
    RMT_CHANNEL_MAX,
} rmt_channel_t;

typedef struct {
    rmt_channel_t channel;
    int gpio_num;
    int clk_div;
    int mem_block_num;
    struct {
        bool carrier_en;
        bool loop_en;
        int idle_level;
        bool idle_output_en;
    } tx_config;
} rmt_config_t;

#define RMT_DEFAULT_CONFIG_TX(gpio, chan) { \
    .channel = chan, \
    .gpio_num = gpio, \
}

esp_err_t rmt_config(const rmt_config_t *config);
esp_err_t rmt_driver_install(rmt_channel_t channel, size_t rx_buf_size, int intr_alloc_flags);

#ifdef __cplusplus
}
#endif

#endif // MOCK_DRIVER_RMT_H
