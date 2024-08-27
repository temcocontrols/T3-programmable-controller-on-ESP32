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
#include "driver/uart.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "modbus.h"
#include "i2c_task.h"
#include "user_data.h"

#define PIR_NOTTRIGGERED   0
#define PIR_TRIGGERED   1

#define LIGHT_DEFAULT_VREF    1100//3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define LIGHT_NO_OF_SAMPLES   64          //Multisampling

extern uint8 DEGCorF;
extern uint32_t PirSensorZero;
extern uint32_t Pir_Sensetivity;
extern uint16 pir_value;
extern uint8 pir_trigger;
extern uint8_t scd4x_perform_forced;
extern uint16_t co2_asc;
extern uint16_t co2_frc;
extern uint8 sensirion_co2_cmd_ForcedCalibration[8];


//-----------LIGHT SWITCH IO define
#define LS_SENSOR_EN		2
#define LS_SENSOR_EN_SEL  	(1ULL<<LS_SENSOR_EN)

#define LS_S1				12
#define LS_S1_SEL  			(1ULL<<LS_S1)


#define LS_S2_S3_SEL  		(1ULL<<36)
#define LS_S4_S6_SEL  		(1ULL<<32)
#define LS_S5_S7_SEL  		(1ULL<<33)
#define LS_AIN_TEMP_SET		(1ULL<<34)
#define LS_AIN_LIGHT_SET	(1ULL<<35)
#define LS_AIN_OCC_SET		(1ULL<<39)

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
	io_conf.pin_bit_mask = LS_S1_SEL | LS_S2_S3_SEL | LS_S4_S6_SEL | LS_S5_S7_SEL | LS_AIN_TEMP_SET | LS_AIN_LIGHT_SET | LS_AIN_OCC_SET;

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
static const adc_channel_t lightswitch_S2S3 = ADC_CHANNEL_0;  // S2 S3
static const adc_channel_t lightswitch_S4S6 = ADC_CHANNEL_4;  //  S4 S6
static const adc_channel_t lightswitch_S5S7 = ADC_CHANNEL_5;  // S5 S7

uint8_t light_key[7];

static void Light_adc_task(void* arg);
void lightswitch_adc_init(void)
{
	// EN SENSOR_IO IO2
	
	
    //Configure ADC

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(lightswitch_Light, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_PIR, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_TEMP, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S2S3, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S4S6, lightswitch_atten);
	adc1_config_channel_atten(lightswitch_S5S7, lightswitch_atten);

    xTaskCreate(Light_adc_task, "adc_task", 2048*2, NULL, 2, NULL);
}



static void Light_adc_task(void* arg)
{
	//uint32_t adc_reading = 0;
	//uint32_t adc_temp = 0;
	uint32_t voltage =0;
	uint32_t adc_light = 0;
	uint32_t adc_pir = 0;
	uint32_t adc_tempertature = 0;
	uint32_t adc_S2S3 = 0;
	uint32_t adc_S4S6 = 0;
	uint32_t adc_S5S7 = 0;
	Str_points_ptr ptr;



	uint32_t vol_light = 0;
	uint32_t vol_pir = 0;
	uint32_t vol_tempertature = 0;
	uint32_t vol_S2S3 = 0;
	uint32_t vol_S4S6 = 0;
	uint32_t vol_S5S7 = 0;
	
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
            	adc_S2S3 += adc1_get_raw((adc1_channel_t)lightswitch_S2S3);
            	adc_S4S6 += adc1_get_raw((adc1_channel_t)lightswitch_S4S6);
            	adc_S5S7 += adc1_get_raw((adc1_channel_t)lightswitch_S5S7);
            }
        }

        adc_light /= LIGHT_NO_OF_SAMPLES;
        adc_pir /= LIGHT_NO_OF_SAMPLES;
        adc_tempertature /= LIGHT_NO_OF_SAMPLES;
        adc_S2S3 /= LIGHT_NO_OF_SAMPLES;
        adc_S4S6 /= LIGHT_NO_OF_SAMPLES;
        adc_S5S7 /= LIGHT_NO_OF_SAMPLES;
        
        vol_light = esp_adc_cal_raw_to_voltage(adc_light, lightswitch_adc_chars);
        vol_pir = esp_adc_cal_raw_to_voltage(adc_pir, lightswitch_adc_chars);
        vol_tempertature = esp_adc_cal_raw_to_voltage(adc_tempertature, lightswitch_adc_chars);
        vol_S2S3 = esp_adc_cal_raw_to_voltage(adc_S2S3, lightswitch_adc_chars);
        vol_S4S6 = esp_adc_cal_raw_to_voltage(adc_S4S6, lightswitch_adc_chars);
        vol_S5S7 = esp_adc_cal_raw_to_voltage(adc_S5S7, lightswitch_adc_chars);

        light_key[0] = gpio_get_level(LS_S1);
        if(vol_S2S3 < 1)	{light_key[1] = 1;light_key[2] = 1;}// 1/4
        else if(vol_S2S3 < 1.8)	{light_key[1] = 1;light_key[2] = 0;}// 1/2
        else if(vol_S2S3 < 2.4)	{light_key[1] = 1;light_key[2] = 0;}// 2/3
        else 					{light_key[1] = 0;light_key[2] = 0;}// 1

        if(vol_S4S6 < 1)	{light_key[3] = 1;light_key[5] = 1;}// 1/4
		else if(vol_S4S6 < 1.8)	{light_key[3] = 1;light_key[5] = 0;}// 1/2
		else if(vol_S4S6 < 2.4)	{light_key[3] = 1;light_key[5] = 0;}// 2/3
		else 					{light_key[3] = 0;light_key[5] = 0;}// 1

        if(vol_S5S7 < 1)	{light_key[4] = 1;light_key[6] = 1;}// 1/4
		else if(vol_S5S7 < 1.8)	{light_key[4] = 1;light_key[6] = 0;}// 1/2
		else if(vol_S5S7 < 2.4)	{light_key[4] = 1;light_key[6] = 0;}// 2/3
		else 					{light_key[4] = 0;light_key[6] = 0;}// 1

        if(abs(vol_pir - PirSensorZero) > Pir_Sensetivity) //occupied
		{
			pir_trigger = PIR_TRIGGERED;
		}
		else
		{
			pir_trigger = PIR_NOTTRIGGERED;
		}
        
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


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


