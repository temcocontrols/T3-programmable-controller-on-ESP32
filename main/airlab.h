//==============================================================================
//    S E N S I R I O N   AG,  Laubisruetistr. 44, CH-8712 Staefa, Switzerland
//==============================================================================
// Project   :  SHT3x Sample Code (V1.0)
// File      :  sht3x.h (V1.0)
// Author    :  RFU
// Date      :  16-Jun-2014
// Controller:  STM32F100RB
// IDE       :  �Vision V4.71.2.0
// Compiler  :  Armcc
// Brief     :  Sensor Layer: Definitions of commands and functions for sensor
//                            access.
//==============================================================================

#ifndef AIRLAB_H
#define AIRLAB_H

//-- Includes ------------------------------------------------------------------
#include "esp_err.h"
#include "types.h"
#include "sensirion_common.h"

#define LITTLE_ENDIAN


// for Sensor
extern uint8_t flag_pm25;


// for PM2.5
extern uint8 AQI_area;
extern uint8 AQI_level;
extern uint16 AQI_value;
extern uint16 aqi_background_color;
extern uint16  aq_calibration;
extern uint8	air_cal_point[4];
extern uint16 	aq_level_value[4];

extern uint8 update_flag;

extern volatile uint16 disp_pm25_weight_25;
extern volatile uint16 disp_pm25_number_25;
extern uint16 pm25_number_05;
extern uint16 pm25_number_10;
extern volatile uint16 pm25_number_25;
extern uint16 pm25_number_40;
extern volatile uint16 pm25_number_100;
extern uint16 pm25_weight_10;
extern volatile uint16 pm25_weight_25;
extern uint16 pm25_weight_40;
extern volatile uint16 pm25_weight_100;
extern uint16 typical_partical_size;

extern uint16 voc_value_raw;

extern uint32_t PirSensorZero;
extern uint32_t Pir_Sensetivity;


extern uint8_t scd4x_perform_forced;
extern uint16_t co2_asc;
extern uint16_t co2_frc;


void Airlab_init(void);

extern uint8_t input_state;
esp_err_t i2c_master_init1();
esp_err_t pca9536_write_register(uint8_t reg_addr, uint8_t data);
esp_err_t pca9536_read_register(uint8_t reg_addr, uint8_t *data);
esp_err_t pca9536_init();
esp_err_t pca9536_read_inputs(uint8_t *input_state);

void display_pm25w(uint16 value);
void display_pm25n(uint16 value);

uint16_t read_airlab_by_block(uint16_t addr);
void write_airlab_by_block(uint16_t addr,uint8_t HeadLen,uint8_t *pData,uint8_t type);
void vStartKeyTasks( unsigned char uxPriority);
//void vStartADCTasks( unsigned char uxPriority);

#define AIR_DEFAULT_VREF    1100//3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define AIR_NO_OF_SAMPLES   64          //Multisampling


#define THERM_METER_XPOS									39
#define TEMP_FIRST_BLANK						      0//30  //+= blank width
#define FIRST_CH_POS											TEMP_FIRST_BLANK + THERM_METER_XPOS
#define SECOND_CH_POS											FIRST_CH_POS+48
#define THIRD_CH_POS											SECOND_CH_POS+48+16
#define UNIT_POS													THIRD_CH_POS + 48+ 15
#define BUTTON_DARK_COLOR   							0X0BA7
#define BTN_OFFSET												CH_HEIGHT+7
#define TOP_AREA_DISP_ITEM_TEMPERATURE   	0
#define TOP_AREA_DISP_ITEM_HUM					 	1
#define TOP_AREA_DISP_ITEM_CO2				   	2
#define TOP_AREA_DISP_ITEM_NONE				   	10

#define TOP_AREA_DISP_UNIT_C   					 	0
#define TOP_AREA_DISP_UNIT_F					 	 	1
#define TOP_AREA_DISP_UNIT_PPM				   	2
#define TOP_AREA_DISP_UNIT_PERCENT			 	3
#define TOP_AREA_DISP_UNIT_Pa			 				4
#define TOP_AREA_DISP_UNIT_kPa						5
#define TOP_AREA_DISP_UNIT_RH							6

#define TOP_AREA_DISP_UNIT_NONE			 			100


#define TSTAT8_CH_COLOR   	0xffff //0xd6e0
#define TSTAT8_MENU_COLOR   0x7e17//0x3bef//0x43f2//0x14a9

#define PIR_NOTTRIGGERED   0
#define PIR_TRIGGERED   1

#define MIC_CARRIER_HI 	1365//1090 //1737//1911//
#define MIC_CARRIER_LO	1092//820 //310//1679//

uint16_t read_airlab_by_block(uint16_t addr);


extern uint8 flag_refresh_PM25;
enum
{
	GOOD = 0,
	MODERATE,
	POOL_FOR_SOME,
	UNHEALTHY,
	MORE_UNHEALTHY,
	HAZARDOUS,
};



#endif
