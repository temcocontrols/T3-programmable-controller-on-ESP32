
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "types.h"
#include "define.h"
//#include "driver/uart.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "modbus.h"
#include "i2c_task.h"
#include "user_data.h"//?????????????????????
#include "driver/ledc.h"

#include "esp_attr.h"

extern uint16_t get_output_raw(uint8_t point);

#define IO13_PIN 13
#define IO13_PIN_SEL (1ULL << IO13_PIN)


void Light_PWM_Init(void)
{
	Str_points_ptr ptr;
    // 配置 LEDC 定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE, // 高速模式
        .timer_num        = LEDC_TIMER_0,        // 定时器 0
        .duty_resolution  = LEDC_TIMER_13_BIT,   // 占空比分辨率为 13 位
        .freq_hz          = 5000,               // PWM 频率为 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK       // 自动选择时钟源
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 配置 LEDC 通道
    ledc_channel_config_t ledc_channel[] = {
        {
            .gpio_num       = GPIO_NUM_2,                 // GPIO 2
            .speed_mode     = LEDC_HIGH_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,   // 通道 0
            .timer_sel      = LEDC_TIMER_0,     // 使用定时器 0
            .duty           = 0,                // 初始占空比为 0
            .hpoint         = 0                 // 高点设置为 0
        },
        {
            .gpio_num       = GPIO_NUM_12,                // GPIO 12
            .speed_mode     = LEDC_HIGH_SPEED_MODE,
            .channel        = LEDC_CHANNEL_1,   // 通道 1
            .timer_sel      = LEDC_TIMER_0,     // 使用定时器 0
            .duty           = 0,                // 初始占空比为 0
            .hpoint         = 0                 // 高点设置为 0
        },
        {
            .gpio_num       = GPIO_NUM_15,                // GPIO 15
            .speed_mode     = LEDC_HIGH_SPEED_MODE,
            .channel        = LEDC_CHANNEL_2,   // 通道 2
            .timer_sel      = LEDC_TIMER_0,     // 使用定时器 0
            .duty           = 0,                // 初始占空比为 0
            .hpoint         = 0                 // 高点设置为 0
        },
        {
            .gpio_num       = GPIO_NUM_32,                // GPIO 32
            .speed_mode     = LEDC_HIGH_SPEED_MODE,
            .channel        = LEDC_CHANNEL_3,   // 通道 3
            .timer_sel      = LEDC_TIMER_0,     // 使用定时器 0
            .duty           = 0,                // 初始占空比为 0
            .hpoint         = 0                 // 高点设置为 0
        }
    };

    // 配置每个通道
    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[i]));
        ptr = put_io_buf(OUT,i);
        ptr.pout->digital_analog = 1; // analog
        ptr.pout->switch_status = SW_AUTO;
        ptr.pout->auto_manual = 0;
        ptr.pout->range = P0_100_PWM;
    }
    Count_OUT_Object_Number();
    
    // 配置 IO13 为输出模式
   gpio_config_t io_conf = {
	   .pin_bit_mask = IO13_PIN_SEL,  // 选择 IO13
	   .mode = GPIO_MODE_OUTPUT,     // 设置为输出模式
	   .pull_up_en = GPIO_PULLUP_DISABLE,  // 禁用上拉
	   .pull_down_en = GPIO_PULLDOWN_DISABLE,  // 禁用下拉
	   .intr_type = GPIO_INTR_DISABLE  // 禁用中断
   };
   gpio_config(&io_conf);

   // 设置 IO13 默认高电平
   gpio_set_level(IO13_PIN, 1);
   
}

//PWM 的占空比范围由分辨率决定，例如 13 位分辨率的最大值为 2^13 - 1 = 8191。
//50% 占空比为 8191 / 2 = 4095。
void Set_PWM_Duty(ledc_channel_t channel, uint32_t duty)
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);
}

void Light_PWM_AO_Update(void)
{
	uint8_t i;
	for(i = 0;i < 4; i++)
	{
		check_output_priority_HOA(i);
		Test[34 + i] = get_output_raw(i);
		if(get_output_raw(i) <= 500)  // 50%
		{
			Set_PWM_Duty(i,get_output_raw(i) * 2);
		}
		else
		{
			Set_PWM_Duty(i,1000 + (get_output_raw(i) - 500) * 10);
		}
	}

}
