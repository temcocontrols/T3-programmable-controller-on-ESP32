#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pyq1548.h"
#include "driver/gpio.h"
#include "unistd.h"
#include "esp_system.h"
#include "sdkconfig.h"

pyq1548_config_t pyq1548Config;

#define PYQ1548_SERIAL_IN	32
#define PYQ1548_DIRECT_LINK	33
#define GPIO_INPUT_PIN_SEL  (1ULL<<PYQ1548_DIRECT_LINK)
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<PYQ1548_SERIAL_IN))//|(1ULL<<PYQ1548_DIRECT_LINK))

static void pyq1548_gpio_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
//	gpio_pad_select_gpio(PYQ1548_DIRECT_LINK);
//	gpio_set_direction(PYQ1548_DIRECT_LINK, GPIO_MODE_OUTPUT);

}

static void pyq1548_direct_pin_out(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	REG_WRITE(GPIO_ENABLE1_REG, BIT1);
	REG_WRITE(GPIO_OUT1_W1TS_REG, BIT1);
	REG_WRITE(GPIO_OUT1_W1TC_REG, BIT1);
//	gpio_pad_select_gpio(PYQ1548_DIRECT_LINK);
//	gpio_set_direction(PYQ1548_DIRECT_LINK, GPIO_MODE_INPUT);
}

static void pyq1548_direct_pin_in(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_INPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
}

static void pyq1548_read_out(void)
{
	bool read_bit;
	pyq1548_direct_pin_out();
	gpio_set_level(PYQ1548_DIRECT_LINK,1);
	usleep(120);
	for(int8_t i=39;i>=0;i--)
	{
		pyq1548_direct_pin_out();
		gpio_set_level(PYQ1548_DIRECT_LINK,0);
		usleep(3);
		gpio_set_level(PYQ1548_DIRECT_LINK,1);
		usleep(3);
		pyq1548_direct_pin_in();
		usleep(5);
		read_bit = gpio_get_level(PYQ1548_DIRECT_LINK);
		usleep(3);
	}
	pyq1548_direct_pin_out();
	gpio_set_level(PYQ1548_DIRECT_LINK,0);
}

void pyq1548_send_config(pyq1548_config_t config)
{
	uint32_t send=0x10;

	if(config.pulse_detection_mode)
		send |= 0x01;
	if(config.HPD_cut_off)
		send |= 0x04;
	switch(config.signal_source){
		case 0:  //PIR(BPF)
			break;
		case 1:	//PIR(LPF)
			send |= 0x20;
			break;
		case 2:	//Reserved
			send |= 0x40;
			break;
		case 3:	//Temperature sensor
			send |= 0x60;
			break;
		default:
			break;
	}
	switch(config.operation_mode){
		case 0:  // forced readout
		default:
			break;
		case 1:	//Interrupt readout
			send |= 0x80;
			break;
		case 2:
			send |= 0x0100;
			break;
		case 3:
			send |= 0x0180;
			break;
	}
	switch(config.window_time){
		case 0:  //2+0*2
		default:
			break;
		case 1:	//2+1*2
			send |= 0x200;
			break;
		case 2:	//2+2*2
			send |= 0x400;
			break;
		case 3:	//2+3*2
			send |= 0x600;
			break;
	}
	switch(config.pulse_counter){
		case 0:
		default:
			break;
		case 1:
			send |= 0x800;
			break;
		case 2:
			send |= 0x1000;
			break;
		case 3:
			send |= 0x1800;
			break;
	}
	if(config.blind_time){
		send |= (uint32_t)(config.blind_time<<13);
	}
	if(config.threshold){
		send |= (uint32_t)(config.threshold<<17);
	}

	usleep(2);

	for(int8_t i=24;i>=0;i--){
		gpio_set_level(PYQ1548_SERIAL_IN,1);
		usleep(5);
		if((send>>i)&1)
			gpio_set_level(PYQ1548_SERIAL_IN,1);
		else
			gpio_set_level(PYQ1548_SERIAL_IN,0);
		usleep(100);
		gpio_set_level(PYQ1548_SERIAL_IN,0);
		usleep(5);
	}
}

void pyq1548_set_config(bool pulse_detection_mode,bool hpf_cut_off,uint8_t signal_source, uint8_t operation_mode,
		uint8_t window_time, uint8_t pulse_counter, uint8_t blind_time, uint8_t threshold)
{
	pyq1548Config.pulse_detection_mode = pulse_detection_mode;
	pyq1548Config.HPD_cut_off = hpf_cut_off;
	pyq1548Config.signal_source = signal_source;
	pyq1548Config.operation_mode = operation_mode;
	pyq1548Config.window_time = window_time;
	pyq1548Config.pulse_counter = pulse_counter;
	pyq1548Config.blind_time = blind_time;
	pyq1548Config.threshold = threshold;
}



void pyq1548_task(void *arg)
{
	pyq1548_gpio_init();
	pyq1548_set_config(0,1,0,0,0,0,1,15);
	pyq1548_send_config(pyq1548Config);
	while(1){
		pyq1548_send_config(pyq1548Config);
		vTaskDelay(100 / portTICK_RATE_MS);
		pyq1548_read_out();
		/*gpio_set_level(PYQ1548_SERIAL_IN,1);
		gpio_set_level(PYQ1548_DIRECT_LINK,1);
		printf("SET PYQ1548_SERIAL_IN to HIGH\n");
		vTaskDelay(1000/portTICK_RATE_MS);
		gpio_set_level(PYQ1548_SERIAL_IN,0);
		gpio_set_level(PYQ1548_DIRECT_LINK,0);
		printf("SET PYQ1548_SERIAL_IN to LOW\n");*/
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}
