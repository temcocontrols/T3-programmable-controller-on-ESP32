#ifndef MOCK_ESP_ADC_CAL_H
#define MOCK_ESP_ADC_CAL_H

#include "esp_err.h"
#include "driver/adc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_ADC_CAL_VAL_EFUSE_VREF = 0,
    ESP_ADC_CAL_VAL_EFUSE_TP,
    ESP_ADC_CAL_VAL_DEFAULT_VREF,
    ESP_ADC_CAL_VAL_MAX,
} esp_adc_cal_value_t;

typedef struct {
    adc_unit_t adc_num;
    adc_atten_t atten;
    adc_bits_width_t bit_width;
    uint32_t vref;
} esp_adc_cal_characteristics_t;

esp_adc_cal_value_t esp_adc_cal_characterize(
    adc_unit_t adc_num,
    adc_atten_t atten,
    adc_bits_width_t bit_width,
    uint32_t default_vref,
    esp_adc_cal_characteristics_t *chars
);

uint32_t esp_adc_cal_raw_to_voltage(uint32_t adc_reading, const esp_adc_cal_characteristics_t *chars);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_ADC_CAL_H
