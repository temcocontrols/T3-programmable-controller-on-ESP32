
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
#include "esp_attr.h"
#include "led_strip.h"
#include "driver/rmt.h"



static const char *TAG = "ws2812";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
//#define CONFIG_EXAMPLE_RMT_TX_GPIO 15
//#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 4




#define LED_BLACK	0
#define LED_RED		1
#define LED_GREEN	2
#define LED_BLUE	3
#define LED_WHITE	4

/* control 4 leds (0 - 15)
 * index : whic led
 * color :
 * 0: OFF 		1: red 			2: green 		3:blue		4: white ... we can defind more if necessary
 * */

typedef struct
{
	uint8_t pos[4];
	uint8_t color[4];
}LED_STR;

LED_STR led_status;
void LS_LED_Control(uint8_t index,uint8_t color1,uint8_t color2,uint8_t color3,uint8_t color4)
{
	memset(&led_status,0,sizeof(LED_STR));
	if(index & 0x01)
	{
		led_status.pos[0] = 1;
		led_status.color[0] = color1;
	}
	if(index & 0x02)
	{
		led_status.pos[1] = 1;
		led_status.color[1] = color2;
	}
	if(index & 0x04)
	{
		led_status.pos[2] = 1;
		led_status.color[2] = color3;
	}
	if(index & 0x08)
	{
		led_status.pos[3] = 1;
		led_status.color[3] = color4;
	}

}

extern uint16_t Test[50];
void LS_led_task(void)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;
	uint16_t t1 = 0;
	uint8_t t2 = 0;
	uint8_t ret1[7] = {0,0,0,0,0,0,0};
	//uart_init_test();
	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
	// set counter clock to 40MHz
	config.clk_div = 2;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
	// install ws2812 driver
	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
	led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
	if (!strip) {
		ESP_LOGE(TAG, "install WS2812 driver failed");
	}
	// Clear LED strip (turn off all LEDs)
	ESP_ERROR_CHECK(strip->clear(strip, 100));

	while (true) {
		//if(Test[44] != 0)
		{
			for(uint8_t i = 0; i < 4;i++)
			{
				if(led_status.pos[i] == 1)
				{
					if(led_status.color[i] == LED_RED)
						strip->set_pixel(strip, i, 255, 0, 0);
					else if(led_status.color[i] == LED_GREEN)
						strip->set_pixel(strip, i, 0, 255, 0);
					else if(led_status.color[i] == LED_BLUE)
						strip->set_pixel(strip, i, 0, 0, 255);
					else if(led_status.color[i] == LED_WHITE)
						strip->set_pixel(strip, i, 255, 255, 255);
					else
						strip->set_pixel(strip, i, 0, 0, 0);
				}
				else
					strip->set_pixel(strip, i, 0, 0, 0);

				strip->refresh(strip, 500);
				vTaskDelay(10 / portTICK_RATE_MS);
			}
			//Test[40] = 0;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
		strip->clear(strip, 500);
		vTaskDelay(1000 / portTICK_RATE_MS);

	}

}
