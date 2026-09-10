#ifndef LED_STRIP_H
#define LED_STRIP_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* led_strip_dev_t;
typedef void* led_strip_handle_t;

typedef enum {
    LED_MODEL_WS2812,
} led_model_t;

typedef struct {
    int max_leds;
    led_strip_dev_t device; // old interface field
    int strip_gpio_num; // new interface field
    led_model_t led_model; // new interface field
    struct {
        bool invert_out;
    } flags; // new interface field
} led_strip_config_t;

#define LED_STRIP_DEFAULT_CONFIG(number, dev) { \
    .max_leds = number, \
    .device = dev, \
}

// Old interface struct
typedef struct led_strip_s led_strip_t;

struct led_strip_s {
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*del)(led_strip_t *strip);
};

#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 4

// Old interface function
led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);

// New interface config struct
typedef struct {
    int clk_src;
    uint32_t resolution_hz;
    size_t mem_block_symbols;
    struct {
        bool with_dma;
    } flags;
} led_strip_rmt_config_t;

// New interface functions
esp_err_t led_strip_new_rmt_device(const led_strip_config_t *config, const led_strip_rmt_config_t *rmt_config, led_strip_handle_t *ret_strip);
esp_err_t led_strip_clear(led_strip_handle_t strip);
esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
esp_err_t led_strip_refresh(led_strip_handle_t strip);
esp_err_t led_strip_del(led_strip_handle_t strip);

#ifdef __cplusplus
}
#endif

#endif // LED_STRIP_H
