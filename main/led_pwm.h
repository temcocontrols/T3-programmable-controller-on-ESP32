#ifndef __LED_PWM_H
#define __LED_PWM_H


#include "driver/ledc.h"

extern ledc_channel_config_t ledc_channel[];
extern void led_pwm_init(void);
extern void led_init(void);
extern void pcnt_init(void);
extern void adc_init(void);

#endif
