#ifndef MOCK_DRIVER_PCNT_H
#define MOCK_DRIVER_PCNT_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PCNT_UNIT_0,
    PCNT_UNIT_1,
    PCNT_UNIT_2,
    PCNT_UNIT_3,
    PCNT_UNIT_MAX,
} pcnt_unit_t;

typedef enum {
    PCNT_CHANNEL_0,
    PCNT_CHANNEL_1,
    PCNT_CHANNEL_MAX,
} pcnt_channel_t;

typedef enum {
    PCNT_COUNT_DIS,
    PCNT_COUNT_INC,
    PCNT_COUNT_DEC,
} pcnt_count_mode_t;

typedef enum {
    PCNT_MODE_KEEP,
    PCNT_MODE_REVERSE,
    PCNT_MODE_DISABLE,
} pcnt_ctrl_mode_t;

typedef struct {
    int pulse_gpio_num;
    int ctrl_gpio_num;
    pcnt_ctrl_mode_t lctrl_mode;
    pcnt_ctrl_mode_t hctrl_mode;
    pcnt_count_mode_t pos_mode;
    pcnt_count_mode_t neg_mode;
    int16_t counter_h_lim;
    int16_t counter_l_lim;
    pcnt_unit_t unit;
    pcnt_channel_t channel;
} pcnt_config_t;

typedef void* pcnt_isr_handle_t;
typedef void (*pcnt_isr_t)(void *arg);

typedef struct {
    struct {
        uint32_t val;
    } int_st;
    struct {
        uint32_t val;
    } status_unit[8];
    struct {
        uint32_t val;
    } int_clr;
} pcnt_dev_t;

extern pcnt_dev_t PCNT;

esp_err_t pcnt_unit_config(const pcnt_config_t *pcnt_config);
esp_err_t pcnt_set_filter_value(pcnt_unit_t pcnt_unit, uint16_t filter_val);
esp_err_t pcnt_filter_enable(pcnt_unit_t pcnt_unit);
esp_err_t pcnt_counter_pause(pcnt_unit_t pcnt_unit);
esp_err_t pcnt_counter_clear(pcnt_unit_t pcnt_unit);
esp_err_t pcnt_counter_resume(pcnt_unit_t pcnt_unit);
esp_err_t pcnt_get_counter_value(pcnt_unit_t pcnt_unit, int16_t* count);
esp_err_t pcnt_isr_register(pcnt_isr_t fn, void *arg, int intr_alloc_flags, pcnt_isr_handle_t *handle);
esp_err_t pcnt_intr_enable(pcnt_unit_t pcnt_unit);
esp_err_t esp_intr_free(void* handle);

#ifdef __cplusplus
}
#endif

#endif // MOCK_DRIVER_PCNT_H
