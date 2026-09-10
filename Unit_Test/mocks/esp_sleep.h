#ifndef ESP_SLEEP_H
#define ESP_SLEEP_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_SLEEP_WAKEUP_UNDEFINED,
    ESP_SLEEP_WAKEUP_ALL,
    ESP_SLEEP_WAKEUP_EXT0,
    ESP_SLEEP_WAKEUP_EXT1,
    ESP_SLEEP_WAKEUP_TIMER,
    ESP_SLEEP_WAKEUP_TOUCHPAD,
    ESP_SLEEP_WAKEUP_ULP,
    ESP_SLEEP_WAKEUP_GPIO,
    ESP_SLEEP_WAKEUP_UART,
    ESP_SLEEP_WAKEUP_WIFI,
    ESP_SLEEP_WAKEUP_COCPU,
    ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG,
    ESP_SLEEP_WAKEUP_BT,
} esp_sleep_source_t;

esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us);
esp_err_t esp_sleep_enable_ext0_wakeup(int gpio_num, int level);
esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t mask, int mode);
esp_err_t esp_sleep_enable_gpio_wakeup(void);
void esp_deep_sleep_start(void) __attribute__((noreturn));
int esp_light_sleep_start(void);
esp_sleep_source_t esp_sleep_get_wakeup_cause(void);

#ifdef __cplusplus
}
#endif

#endif // ESP_SLEEP_H
