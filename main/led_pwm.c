#include <stdio.h>
#include "led_pwm.h"
//#include "driver/ledc.h"
#include "deviceparams.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "wifi.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (25)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO       (26)
#define LEDC_HS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL	   LEDC_CHANNEL_2
#define LEDC_HS_CH3_CHANNEL	   LEDC_CHANNEL_3
#define LEDC_HS_CH4_CHANNEL	   LEDC_CHANNEL_4
#define LEDC_HS_CH5_CHANNEL	   LEDC_CHANNEL_5


#define LEDC_TEST_CH_NUM       (2)
#define LEDC_TEST_DUTY         (100)
#define LEDC_TEST_FADE_TIME    (3000)

#define LED_HEART_BEAT	23
#define LED_FAN_SPEED	4
#define LED_WIFI		22
#define	LED_RS485_TX	19
#define LED_RS485_RX	18
#define LED_HEART_BEAT_SEL  (1ULL<<LED_HEART_BEAT)
#define LED_FAN_SPEED_SEL  (1ULL<<LED_FAN_SPEED)
#define LED_WIFI_SEL  		(1ULL<<LED_WIFI)
#define LED_RS485_TX_SEL  (1ULL<<LED_RS485_TX)
#define LED_RS485_RX_SEL  (1ULL<<LED_RS485_RX)

#define PULSE_COUNTER	5
#define PULSE_COUNTER_SEL	(1ULL<<PULSE_COUNTER)

#define ESP_INTR_FLAG_DEFAULT 0

static void periodic_timer_callback(void* arg);

static void fan_led_task(void* arg)
{
	uint32_t cnt=0;
	holding_reg_params.led_rx485_rx = 0;
	holding_reg_params.led_rx485_tx = 0;
    for(;;) {
		gpio_set_level(LED_HEART_BEAT, (cnt++) % 8);
		if((holding_reg_params.fan_module_pwm1==0))//||(holding_reg_params.fan_module_pwm2==255))
			gpio_set_level(LED_FAN_SPEED, 1);
		else if((holding_reg_params.fan_module_pwm1<50))//||(holding_reg_params.fan_module_pwm2>200))
			gpio_set_level(LED_FAN_SPEED, (cnt) % 6);
		else if((holding_reg_params.fan_module_pwm1<100))//||(holding_reg_params.fan_module_pwm2>150))
			gpio_set_level(LED_FAN_SPEED, (cnt) % 5);
		else if((holding_reg_params.fan_module_pwm1<150))//||(holding_reg_params.fan_module_pwm2>100))
			gpio_set_level(LED_FAN_SPEED, (cnt) % 4);
		else if((holding_reg_params.fan_module_pwm1<200))//||(holding_reg_params.fan_module_pwm2>50))
			gpio_set_level(LED_FAN_SPEED, (cnt) % 3);
		else if((holding_reg_params.fan_module_pwm1<256))//||(holding_reg_params.fan_module_pwm2>0))
			gpio_set_level(LED_FAN_SPEED, (cnt) % 2);
		if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
			gpio_set_level(LED_WIFI, 0);
		else
			gpio_set_level(LED_WIFI, 1);

		if(holding_reg_params.led_rx485_tx>0){
			gpio_set_level(LED_RS485_TX, 0);
			holding_reg_params.led_rx485_tx--;
		}
		else
			gpio_set_level(LED_RS485_TX, 1);

		if(holding_reg_params.led_rx485_rx>0)
		{
			gpio_set_level(LED_RS485_RX, 0);
			holding_reg_params.led_rx485_rx--;
		}
		else
			gpio_set_level(LED_RS485_RX, 1);

		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

/*void timer_init(void)
{
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            // name is optional, but may help identify the timer when debugging
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // The timer has been created but is not running yet

    // Start the timers
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000));
}*/

void led_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = LED_HEART_BEAT_SEL | LED_FAN_SPEED_SEL | LED_WIFI_SEL |
			LED_RS485_TX_SEL | LED_RS485_RX_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	xTaskCreate(fan_led_task, "fan_led_task", 2048, NULL, 1, NULL);
}

static xQueueHandle gpio_evt_queue = NULL;
static uint32_t pulseValue = 0;

static void IRAM_ATTR pulse_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void pulse_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            //printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        	pulseValue++;
        }
    }
}


void pulse_couter_init(void)
{
	gpio_config_t io_conf;
	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	//bit mask of the pins,
	io_conf.pin_bit_mask = PULSE_COUNTER_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(pulse_task, "pulse", 2048, NULL, 9, NULL);
	//change gpio intrrupt type for one pin
	//gpio_set_intr_type(PULSE_COUNTER, GPIO_INTR_ANYEDGE);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PULSE_COUNTER, pulse_isr_handler, (void*) PULSE_COUNTER);
}

void led_pwm_init(void)
{
	int ch;
	/*
	 * Prepare and set configuration of timers
	 * that will be used by LED Controller
	 */
	ledc_timer_config_t ledc_timer = {
		.duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
		.freq_hz = 10000,                      // frequency of PWM signal
		.speed_mode = LEDC_HS_MODE,           // timer mode
		.timer_num = LEDC_HS_TIMER,            // timer index
		.clk_cfg = LEDC_USE_APB_CLK,//LEDC_AUTO_CLK,              // Auto select the source clock
	};
	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);

	ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
		{
			.channel    = LEDC_HS_CH0_CHANNEL,
			.duty       = 0,
			.gpio_num   = LEDC_HS_CH0_GPIO,
			.speed_mode = LEDC_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = LEDC_HS_TIMER
		},
		{
			.channel    = LEDC_HS_CH1_CHANNEL,
			.duty       = 0,
			.gpio_num   = LEDC_HS_CH1_GPIO,
			.speed_mode = LEDC_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = LEDC_HS_TIMER
		},
		/*{
			.channel    = LEDC_HS_CH2_CHANNEL,
			.duty       = 0,
			.gpio_num   = LED_HEART_BEAT,
			.speed_mode = LEDC_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = LEDC_HS_TIMER
		},
		{
			.channel    = LEDC_HS_CH3_CHANNEL,
			.duty       = 0,
			.gpio_num   = LED_FAN_SPEED,
			.speed_mode = LEDC_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = LEDC_HS_TIMER
		},
		{
			.channel    = LEDC_HS_CH4_CHANNEL,
			.duty       = 0,
			.gpio_num   = LED_WIFI,
			.speed_mode = LEDC_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = LEDC_HS_TIMER
		},*/
	};
	for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
		ledc_channel_config(&ledc_channel[ch]);
	}

	ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, holding_reg_params.fan_module_pwm1);//LEDC_TEST_DUTY);//
	ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, holding_reg_params.fan_module_pwm2);
	ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
	ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
}
