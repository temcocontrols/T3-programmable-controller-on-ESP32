
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
#include "driver/rmt_tx.h"


static const char *TAG = "ws2812";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
//#define CONFIG_EXAMPLE_RMT_TX_GPIO 15
//#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 4

extern uint16_t Test[50];


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
	uint8_t pos[6];
	uint8_t color_r[6];
	uint8_t color_g[6];
	uint8_t color_b[6];
}LED_STR;

LED_STR led_status;
void LS_LED_Control(uint32_t* color)
{
	uint8_t i;
	memset(&led_status,0,sizeof(LED_STR));
	for(i = 0;i < 6;i++)
	{
		if(color[i] > 0)
		{
			led_status.pos[i] = 1;
			led_status.color_r[i] = (uint8_t)(color[i] >> 16);
			led_status.color_g[i] = (uint8_t)(color[i] >> 8);
			led_status.color_b[i] = (uint8_t)color[i];

		}
	}
}

void LS_led_task(void *pvParameters)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    uint16_t t1 = 0;
    uint8_t t2 = 0;
    uint8_t ret1[7] = {0,0,0,0,0,0,0};

    /* ---------------- Driver Init (Replaces old RMT init) ---------------- */

    led_strip_handle_t strip;

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_EXAMPLE_RMT_TX_GPIO,
        .max_leds = CONFIG_EXAMPLE_STRIP_LED_NUMBER,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &strip)
    );

    /* Clear LED strip (turn off all LEDs) */
    ESP_ERROR_CHECK(led_strip_clear(strip));

    while (true)
    {
        /* Same as: strip->clear(strip, 50); */
        led_strip_clear(strip);
        vTaskDelay(10 / portTICK_PERIOD_MS);

        for(uint8_t i = 0; i < 6; i++)
        {
            if(led_status.pos[i] == 1)
            {
                /* Same as strip->set_pixel() */
                led_strip_set_pixel(strip, i,
                                    led_status.color_r[i],
                                    led_status.color_g[i],
                                    led_status.color_b[i]);
            }
            else
            {
                led_strip_set_pixel(strip, i, 0, 0, 0);
            }
        }

        /* Same as strip->refresh(strip, 100); */
        led_strip_refresh(strip);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/* End of file */
