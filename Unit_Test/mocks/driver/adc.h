#ifndef MOCK_DRIVER_ADC_H
#define MOCK_DRIVER_ADC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_MAX,
} adc_channel_t;

typedef adc_channel_t adc1_channel_t;

typedef enum {
    ADC_ATTEN_DB_0 = 0,
    ADC_ATTEN_DB_2_5,
    ADC_ATTEN_DB_6,
    ADC_ATTEN_DB_11,
} adc_atten_t;

typedef enum {
    ADC_UNIT_1 = 1,
    ADC_UNIT_2 = 2,
    ADC_UNIT_BOTH = 3,
    ADC_UNIT_MAX,
} adc_unit_t;

typedef enum {
    ADC_WIDTH_BIT_9 = 0,
    ADC_WIDTH_BIT_10,
    ADC_WIDTH_BIT_11,
    ADC_WIDTH_BIT_12,
    ADC_WIDTH_MAX,
} adc_bits_width_t;

esp_err_t adc1_config_width(adc_bits_width_t width_bit);
esp_err_t adc1_config_channel_atten(adc1_channel_t channel, adc_atten_t atten);
int adc1_get_raw(adc1_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif // MOCK_DRIVER_ADC_H
