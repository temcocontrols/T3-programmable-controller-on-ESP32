
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

#include "esp_attr.h"
#include "led_strip.h"
#include "driver/rmt.h"


#define PIR_NOTTRIGGERED   0
#define PIR_TRIGGERED   1

#define LIGHT_DEFAULT_VREF    1100//3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define LIGHT_NO_OF_SAMPLES   100//64          //Multisampling

extern uint16_t Test[50];
extern uint8 DEGCorF;
extern uint32_t PirSensorZero;
extern uint32_t Pir_Sensetivity;
extern uint16 pir_value;
extern uint8 pir_trigger;
extern uint8_t scd4x_perform_forced;
extern uint16_t co2_asc;
extern uint16_t co2_frc;
extern uint8 sensirion_co2_cmd_ForcedCalibration[8];
extern char debug_array[100];
void LS_led_task(void);

//-----------LIGHT SWITCH IO define
#define LS_SENSOR_EN		2
#define LS_SENSOR_EN_SEL  	(1ULL<<LS_SENSOR_EN)

#define LS_S7				12
#define LS_S7_SEL  			(1ULL<<LS_S7)

#define LS_S1_S2_SEL  		(1ULL<<33)
#define LS_S3_S4_SEL  		(1ULL<<32)
#define LS_S5_S6_SEL  		(1ULL<<36)
#define LS_AIN_TEMP_SET		(1ULL<<34)
#define LS_AIN_LIGHT_SET	(1ULL<<35)
#define LS_AIN_OCC_SET		(1ULL<<39)

portMUX_TYPE led_spinlock;

void Light_Switch_IO_Init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = LS_SENSOR_EN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
	
	gpio_set_level(LS_SENSOR_EN, 1);  // ENABLE SENSOR

	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_INPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = LS_S7_SEL | LS_S1_S2_SEL | LS_S3_S4_SEL | LS_S5_S6_SEL | LS_AIN_TEMP_SET | LS_AIN_LIGHT_SET | LS_AIN_OCC_SET;

	io_conf.pull_up_en = 1;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
}

// ADC
// 1. light
// 2. pir occ
// 3. temperature

static esp_adc_cal_characteristics_t *lightswitch_adc_chars;
static const adc_atten_t lightswitch_atten = ADC_ATTEN_DB_11;
static const adc_unit_t lightswitch_unit = ADC_UNIT_1;
static const adc_channel_t lightswitch_Light = ADC_CHANNEL_7;  // light  IO34
static const adc_channel_t lightswitch_PIR = ADC_CHANNEL_3;  //  occ	IO35
static const adc_channel_t lightswitch_TEMP = ADC_CHANNEL_6;  // temperature  IO39
static const adc_channel_t lightswitch_S1S2 = ADC_CHANNEL_5;  // S2 S1
static const adc_channel_t lightswitch_S3S4 = ADC_CHANNEL_4;  //  S4 S3
static const adc_channel_t lightswitch_S5S6 = ADC_CHANNEL_0;  //  S5 S6

uint8_t light_key[7];
uint8_t light_key_last[7];

static void Light_adc_task(void* arg);
void lightswitch_adc_init(void)
{
	//Configure ADC
	Light_Switch_IO_Init();

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(lightswitch_Light, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_PIR, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_TEMP, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S1S2, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S3S4, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S5S6, lightswitch_atten);

    xTaskCreate(Light_adc_task, "adc_task", 2048, NULL, 2, NULL);
}



void LS_LED_Control(uint32_t* color);
static void Light_adc_task(void* arg)
{
	//uint32_t adc_reading = 0;
	//uint32_t adc_temp = 0;
	uint32_t voltage =0;
	uint32_t adc_light = 0;
	uint32_t adc_pir = 0;
	uint32_t adc_tempertature = 0;
	uint32_t adc_S1S2 = 0;
	uint32_t adc_S3S4 = 0;
	uint32_t adc_S5S6 = 0;
	Str_points_ptr ptr;
	uint32_t key_refresh_timer[7] = {0,0,0,0,0,0,0};
	uint8_t key_temp[7]= {0,0,0,0,0,0,0};


	uint32_t vol_light = 0;
	uint32_t vol_pir = 0;
	uint32_t vol_tempertature = 0;
	uint32_t vol_S1S2 = 0;
	uint32_t vol_S3S4 = 0;
	uint32_t vol_S5S6 = 0;

	int i = 0;
    //Continuously sample ADC1//Characterize ADC
	lightswitch_adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(lightswitch_unit, lightswitch_atten, ADC_WIDTH_BIT_12, LIGHT_DEFAULT_VREF, lightswitch_adc_chars);


    while (1) {

           //Multisampling
        for (i = 0; i < LIGHT_NO_OF_SAMPLES; i++) {
            if (lightswitch_unit == ADC_UNIT_1) {
            	adc_light += adc1_get_raw((adc1_channel_t)lightswitch_Light);
            	adc_pir += adc1_get_raw((adc1_channel_t)lightswitch_PIR);
            	adc_tempertature += adc1_get_raw((adc1_channel_t)lightswitch_TEMP);
            	adc_S1S2 += adc1_get_raw((adc1_channel_t)lightswitch_S1S2);
            	adc_S3S4 += adc1_get_raw((adc1_channel_t)lightswitch_S3S4);
            	adc_S5S6 += adc1_get_raw((adc1_channel_t)lightswitch_S5S6);
            }
        }

        adc_light /= LIGHT_NO_OF_SAMPLES;
        adc_pir /= LIGHT_NO_OF_SAMPLES;
        adc_tempertature /= LIGHT_NO_OF_SAMPLES;
        adc_S1S2 /= LIGHT_NO_OF_SAMPLES;
        adc_S3S4 /= LIGHT_NO_OF_SAMPLES;
        adc_S5S6 /= LIGHT_NO_OF_SAMPLES;
        

        vol_light = esp_adc_cal_raw_to_voltage(adc_light, lightswitch_adc_chars);
        vol_pir = esp_adc_cal_raw_to_voltage(adc_pir, lightswitch_adc_chars);
        vol_tempertature = esp_adc_cal_raw_to_voltage(adc_tempertature, lightswitch_adc_chars);
        vol_S1S2 = esp_adc_cal_raw_to_voltage(adc_S1S2, lightswitch_adc_chars);
        vol_S3S4 = esp_adc_cal_raw_to_voltage(adc_S3S4, lightswitch_adc_chars);
        vol_S5S6 = esp_adc_cal_raw_to_voltage(adc_S5S6, lightswitch_adc_chars);


        if(gpio_get_level(LS_S7) == 0)
        {
        	if(key_temp[6] == 1)
			{
				key_temp[6] = 0;
				key_refresh_timer[6] = xTaskGetTickCount();
			}
        	if(xTaskGetTickCount() - key_refresh_timer[6] > 20)
			{
				light_key[6] = 0; key_temp[6] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[6] > 1000)
			{
				if(xTaskGetTickCount() - key_refresh_timer[6] > 1000)
				{// long press
					ptr = put_io_buf(2/*VAR*/,25);
					ptr.pvar->range = 0/*OFF_ON*/;
					ptr.pvar->digital_analog = 1;
					ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[6] - 1000);
					memcpy(ptr.pvar->description,"press time of button7", strlen("press time of button7"));
					memcpy(ptr.pvar->label,"HT_KEY7",strlen("HT_KEY7"));
				}
			}
        }
        else
        {
        	if(xTaskGetTickCount() - key_refresh_timer[6] > 500)
        	{
        		light_key[6] = 1;key_temp[6] = 1;
#if 0
        		ptr = put_io_buf(2/*VAR*/,25);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
        	}
        }


        if(vol_S1S2 < 500)
        {
        	if(xTaskGetTickCount() - key_refresh_timer[0] > 500)
        	{
        		light_key[0] = 1;key_temp[0] = 1;
#if 0
        		ptr = put_io_buf(2/*VAR*/,19);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
        	}
        	if(xTaskGetTickCount() - key_refresh_timer[1] > 500)
        	{
        		light_key[1] = 1;key_temp[1] = 1;
#if 0
        		ptr = put_io_buf(2/*VAR*/,20);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
        	}
        }// 1/4
        else if(vol_S1S2 < 1800)
        {
        	if(key_temp[1] == 1)
        	{
        		key_temp[1] = 0;
        		key_refresh_timer[1] = xTaskGetTickCount();
        	}
			if(xTaskGetTickCount() - key_refresh_timer[1] > 20)
			{
				light_key[0] = 1; light_key[1] = 0; key_temp[1] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[1] > 1000)
			{
				if(xTaskGetTickCount() - key_refresh_timer[1] > 1000)
				{// long press
					ptr = put_io_buf(2/*VAR*/,20);
					ptr.pvar->range = 0/*OFF_ON*/;
					ptr.pvar->digital_analog = 1;
					ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[1] - 1000);
					memcpy(ptr.pvar->description,"press time of button2", strlen("press time of button2"));
					memcpy(ptr.pvar->label,"HT_KEY2",strlen("HT_KEY2"));
				}
			}
        }// 1/2
        else if(vol_S1S2 < 2400)
        {
        	if(key_temp[0] == 1)
			{
				key_temp[0] = 0;
				key_refresh_timer[0] = xTaskGetTickCount();
			}
			if(xTaskGetTickCount() - key_refresh_timer[0] > 20)
			{
				light_key[1] = 1; light_key[0] = 0; key_temp[0] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[0] > 1000)
			{
				if(xTaskGetTickCount() - key_refresh_timer[0] > 1000)
				{// long press
					ptr = put_io_buf(2/*VAR*/,19);
					ptr.pvar->range = 0/*OFF_ON*/;
					ptr.pvar->digital_analog = 1;
					ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[0] - 1000);
					memcpy(ptr.pvar->description,"press time of button1", strlen("press time of button1"));
					memcpy(ptr.pvar->label,"HT_KEY1",strlen("HT_KEY1"));
				}
			}
        }// 2/3
        else 						{light_key[0] = 0; key_temp[0] = 0; light_key[1] = 0; key_temp[1] = 0; key_refresh_timer[0] = xTaskGetTickCount(); key_refresh_timer[1] = xTaskGetTickCount();}// 1


		if(vol_S3S4 < 500)	{
			if(xTaskGetTickCount() - key_refresh_timer[2] > 500)
			{
				light_key[2] = 1; key_temp[2] = 1;
#if 0
				ptr = put_io_buf(2/*VAR*/,21);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
			}
			if(xTaskGetTickCount() - key_refresh_timer[3] > 500)
			{
				light_key[3] = 1; key_temp[3] = 1;
#if 0
				ptr = put_io_buf(2/*VAR*/,22);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
			}

		}// 1/4
		else if(vol_S3S4 < 1800)
		{
			if(key_temp[3] == 1)
			{
				key_temp[3] = 0;
				key_refresh_timer[3] = xTaskGetTickCount();
			}
			if(xTaskGetTickCount() - key_refresh_timer[3] > 20)
			{
				light_key[2] = 1;	light_key[3] = 0;	key_temp[3] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[3] > 1000)
			{// long press
				ptr = put_io_buf(2/*VAR*/,22);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[3] - 1000);
				memcpy(ptr.pvar->description,"press time of button4", strlen("press time of button4"));
				memcpy(ptr.pvar->label,"HT_KEY4",strlen("HT_KEY4"));
			}
		}// 1/2
		else if(vol_S3S4 < 2400)
		{
			if(key_temp[2] == 1)
			{
				key_temp[2] = 0;
				key_refresh_timer[2] = xTaskGetTickCount();
			}
			if(xTaskGetTickCount() - key_refresh_timer[2] > 20)
			{
				light_key[3] = 1;	light_key[2] = 0;	key_temp[2] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[2] > 1000)
			{// long press
				ptr = put_io_buf(2/*VAR*/,21);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[2] - 1000);
				memcpy(ptr.pvar->description,"press time of button3", strlen("press time of button3"));
				memcpy(ptr.pvar->label,"HT_KEY3",strlen("HT_KEY3"));
			}// 2/3
		}
		else 						{ light_key[2] = 0; key_temp[2] = 0; light_key[3] = 0; key_temp[3] = 0;	key_refresh_timer[2] = xTaskGetTickCount();	key_refresh_timer[3] = xTaskGetTickCount();}// 1


        if(vol_S5S6 < 500)	{
        	if(xTaskGetTickCount() - key_refresh_timer[4] > 500)
        	{
        		light_key[4] = 1;key_temp[4] = 1;
#if 0
        		ptr = put_io_buf(2/*VAR*/,23);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
        	}
        	if(xTaskGetTickCount() - key_refresh_timer[5] > 500)
        	{
        		light_key[5] = 1;key_temp[5] = 1;
#if 0
        		ptr = put_io_buf(2/*VAR*/,24);
				ptr.pvar->range = 0/*OFF_ON*/;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->value = 0;
#endif
        	}
        }// 1/4
		else if(vol_S5S6 < 1800)
		{
			if(key_temp[5] == 1)
			{
				key_temp[5] = 0;
				key_refresh_timer[5] = xTaskGetTickCount();
			}
			if(xTaskGetTickCount() - key_refresh_timer[5] > 20)
			{
				light_key[4] = 1; light_key[5] = 0; key_temp[5] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[5] > 2000)
			{
				if(xTaskGetTickCount() - key_refresh_timer[5] > 1000)
				{// long press
					ptr = put_io_buf(2/*VAR*/,24);
					ptr.pvar->range = 0/*OFF_ON*/;
					ptr.pvar->digital_analog = 1;
					ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[5] - 1000);
					memcpy(ptr.pvar->description,"press time of button6", strlen("press time of button6"));
					memcpy(ptr.pvar->label,"HT_KEY6",strlen("HT_KEY6"));
				}
			}
		}// 1/2
		else if(vol_S5S6 < 2400)
		{
			if(key_temp[4] == 1)
			{
				key_temp[4] = 0;
				key_refresh_timer[4] = xTaskGetTickCount();
			}
			if(xTaskGetTickCount() - key_refresh_timer[4] > 20)
			{
				light_key[5] = 1; light_key[4] = 0; key_temp[4] = 0;
			}
			if(xTaskGetTickCount() - key_refresh_timer[4] > 2000)
			{
				if(xTaskGetTickCount() - key_refresh_timer[4] > 1000)
				{// long press
					ptr = put_io_buf(2/*VAR*/,23);
					ptr.pvar->range = 0/*OFF_ON*/;
					ptr.pvar->digital_analog = 1;
					ptr.pvar->value = (xTaskGetTickCount() - key_refresh_timer[4] - 1000);
					memcpy(ptr.pvar->description,"press time of button5", strlen("press time of button5"));
					memcpy(ptr.pvar->label,"HT_KEY5",strlen("HT_KEY5"));
				}
			}
		}// 2/3
		else 	{	light_key[4] = 0;key_temp[4] = 0; light_key[5] = 0;	key_temp[5] = 0; key_refresh_timer[4] = xTaskGetTickCount();	key_refresh_timer[5] = xTaskGetTickCount();}// 1



        for(i = 0;i < 7;i++)
        {
        	ptr = put_io_buf(2/*VAR*/,i);
        	ptr.pvar->range = 1/*OFF_ON*/;
        	ptr.pvar->digital_analog = 0;
        	if((light_key[i] == 0) && (light_key_last[i] == 1))
        	{
        		Test[10 + i]++;
        		ptr.pvar->control = ~ptr.pvar->control;
        		// save it to flash

        	}
        	light_key_last[i] = light_key[i];
        }

        // control led
#if 1
        uint32_t temp_color[6] = {0,0,0,0,0,0};
        for(i = 0;i < 6;i++)
		{
			ptr = put_io_buf(2/*VAR*/,i+10);
			ptr.pvar->range = 0/*OFF_ON*/;
			ptr.pvar->digital_analog = 1;
			temp_color[i] = ptr.pvar->value;
		}

        LS_LED_Control(temp_color);
#endif

        if(abs(vol_pir - PirSensorZero) > Pir_Sensetivity) //occupied
		{
			pir_trigger = PIR_TRIGGERED;
		}
		else
		{
			pir_trigger = PIR_NOTTRIGGERED;
		}

        vTaskDelay(20 / portTICK_RATE_MS);

    }
}

#if 1
uint16_t read_lightswitch_by_block(uint16_t addr)
{
	uint8_t item;
	uint16_t *block;
	uint8_t *block1;
	uint8_t temp;
	
	if(addr == DEGC_OR_F)
	{
	  return DEGCorF;
	}
	else if(addr == TEMPRATURE_CHIP)
	{
	  return g_sensors.temperature;
	}
	else if(addr == EXTERNAL_SENSOR1)  // CO2
	{
	  return g_sensors.co2;
	}
	else if(addr == EXTERNAL_SENSOR2) // HUM
	{
	  return g_sensors.humidity;
	}
	else if(addr == CALIBRATION)
	{
	  return 0;
	}
	else if(addr == CO2_CALIBRATION)
	{
	  return 0;
	}
	else if(addr == HUM_CALIBRATION)
	{
	  return 0;
	}
	// LIGHT
	else if(addr == LIGHT_SENSOR)
	{
	  return 0;
	}
	// PIR
	else if(addr == PIR_SENSOR_VALUE)
	{
	  return pir_value;
	}
	else if(addr == PIR_SENSOR_ZERO) 
	{
	  return PirSensorZero;
	}
	else if(addr == PIR_SPARE)
	{
	  return pir_trigger;
	}
	
// VOC	
	else if((addr >= VOC_BASELINE1) && (addr <= VOC_BASELINE1))
	{
	  return g_sensors.voc_baseline[addr - VOC_BASELINE1];
	}
	else if(addr == VOC_DATA)
	{
	  return g_sensors.voc_value;
	}	
	else if(addr == MODBUS_PATICAL_SIZE)
	{
	  return 0;
	}
	else if(addr == MODBUS_WBGT)
	{
	  return 0;
	}
	// CO2
	else if(addr == CO2_ASC_ENABLE)
	{
	  return co2_asc;
	}
	else if(addr == CO2_FRC_VALUE)
	{
	  return co2_frc;
	}
	// DISPLAY CONFIG
	
	else
	  return 0;
}



void write_lightswitch_by_block(uint16_t addr,uint8_t HeadLen,uint8_t *pData,uint8_t type)
{
	if(addr == DEGC_OR_F)
	{
		DEGCorF = pData[HeadLen + 5];
	}
	
	else if(addr == CALIBRATION)
	{
	  
	}
	else if(addr == CO2_CALIBRATION)
	{
	  
	}
	else if(addr == HUM_CALIBRATION)
	{
	  
	}
	
	// PIR
	else if(addr == PIR_SENSOR_VALUE)
	{
	  
	}
	else if(addr == PIR_SENSOR_ZERO) 
	{
	  PirSensorZero = pData[HeadLen + 5];
	}
	
// PM2.5
	
// VOC		
	
	
	// CO2	
	else if(addr == CO2_FRC_VALUE)
	{
		uint16 tmp;
		sensirion_co2_cmd_ForcedCalibration[4] = pData[HeadLen + 4];	
		sensirion_co2_cmd_ForcedCalibration[5] = pData[HeadLen + 5];
		tmp = (uint16)(sensirion_co2_cmd_ForcedCalibration[4]<<8 )|sensirion_co2_cmd_ForcedCalibration[5];
		if((tmp >= 0) && (tmp <= 5000))
		{
			co2_frc = tmp;
			/*if(co2_present == SCD30)
			{
				co2_cmd_status = CMD_SET_FRC;
			}
			else*/
			{
				if(scd4x_perform_forced == 0)
				{
					scd4x_perform_forced = 1;
					//scd4x_perform_forced_count = 0;
				}
			}					
		}
	}	
}
#endif

#if 0
static const char *TAG = "ws2812";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
//#define CONFIG_EXAMPLE_RMT_TX_GPIO 15
//#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 4






void LS_led_task(void)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;
	uint16_t t1 = 0;
	uint8_t t2 = 0;
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
	// Show simple rainbow chasing pattern
	//ESP_LOGI(TAG, "LED Rainbow Chase Start");

	while (true) {

		//Test[16] = t2;
		if(t1++ > 20)
		{
			//start_fw_update();
		}
		switch(t2)
		{
		case 0:
			strip->set_pixel(strip, 0, 255, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 1, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 2, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 3, 0, 0, 0);strip->refresh(strip, 100);
			break;
		case 2:
			strip->set_pixel(strip, 0, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 1, 0, 255, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 2, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 3, 0, 0, 0);strip->refresh(strip, 100);
			break;
		case 4:
			strip->set_pixel(strip, 0, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 1, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 2, 0, 0, 255);strip->refresh(strip, 100);
			strip->set_pixel(strip, 3, 0, 0, 0);strip->refresh(strip, 100);
			break;
		case 6:
			strip->set_pixel(strip, 0, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 1, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 2, 0, 0, 0);strip->refresh(strip, 100);
			strip->set_pixel(strip, 3, 255, 255, 255);strip->refresh(strip, 100);
			break;
		case 1:
		case 3:
		case 5:
		case 7:
			strip->clear(strip, 100);
		default:
			break;
		}
		t2++;
		if(t2 >= 8) t2 = 0;
		vTaskDelay(1000 / portTICK_RATE_MS);

	}

}

#endif
