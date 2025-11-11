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

#ifndef CO2_H
#define CO2_H

//-- Includes ------------------------------------------------------------------
#include "esp_err.h"
#include "types.h"
#include "sensirion_common.h"

#define LITTLE_ENDIAN


#define CHANNEL_HUM		0
#define CHANNEL_TEMP	1
#define CHANNEL_CO2		2
#define CHANNEL_PRE		3

typedef struct
{
	S16_T min;
	S16_T max;
} _RANGE_;

typedef struct
{
	uint16_t tem_org;
	uint16_t hum_org;
	uint16_t co2_org;
	int16_t tem_offset;
	int16_t hum_offset;
}Str_Mul_I2C;


typedef struct
{
	uint8_t deg_c_or_f;
	int16_t internal_temperature_c;
	int16_t internal_temperature_f;

	uint16_t pre_alarm_setpoint;
	uint16_t alarm_setpoint;
	uint8_t alarm_state;
	uint8_t pre_alarm_on_time;
	uint8_t pre_alarm_off_time;
	uint8_t alarm_delay_time;

	uint8_t output_auto_manual;
	uint16_t output_manual_value_temp;
	uint16_t output_manual_value_humi;
	uint16_t output_manual_value_co2;

	uint8_t output_mode; // 4_20ma 0-5v 0-10v

	_RANGE_ output_range_table[3];

	uint8_t backlight_keep_seconds;

	uint8_t lcd_i2c_sensor_index;
	uint8_t i2c_sensor_type[3];
	Str_Mul_I2C I2C_Sensor[3];

	uint8_t screenArea1;
	uint8_t screenArea2;
	uint8_t screenArea3;
	uint8_t enableScroll;
	uint8_t alarmEnable;

//	int16 external_operation_value;
//	uint8 external_operation_flag;  // 1- TEMP_CALIBRATION  2-HUM_CALIBRATION

	uint16 co2_asc; // 强制校正后的co2 read-only
	uint16 co2_frc; // 强制校正current co2 sensor的参考值
	uint8_t flag_co2_asc[3];// enable/disable 当前co2的自动校正功能

	uint16 analog_output[3];

//	uint8_t scd4x_perform_forced[3];
//	uint8_t scd4x_enable_asc[3];
//	uint8_t scd4x_disable_asc[3];
//	uint8_t scd4x_read_asc[3];



}STR_CO2_Reg;


typedef enum
{
	E_I2C_NO_SENSOR = 0,
	E_I2C_SHT3X = 1,
	E_I2C_SHT4X,//2
	E_I2C_SCD4X,//3
	E_I2C_SHT3X_SCD4X,// 4
	E_I2C_SHT4X_SCD4X,//5
}E_I2C_SENSOR;

// output mode
typedef enum
{
	E_OUT_FAULT = 0,
	E_0_10V = 1,
	E_0_5V,
	E_4_20MA
}E_OUTPUT_MODE;

enum
{
	SCREEN_AREA_TEMP = 0,
	SCREEN_AREA_HUMI = 1,
	SCREEN_AREA_CO2 = 2,
	SCREEN_AREA_NONE = 3,
	SCREEN_AREA_PM25 = 4,
	SCREEN_AREA_PRESSURE = 5,
	SCREEN_AREA_PM10 = 6,
	SCREEN_AREA_AQI = 7,
	SCREEN_AREA_LIGHT = 8,
	SCREEN_AREA_NULL = 9,
	SCREEN_AREA_TEMSETP = 10,
	SCREEN_AREA_HUMSETP = 11,
	SCREEN_AREA_CO2SETP = 12,
	MAX_SCREEN_AREA,
};

extern STR_CO2_Reg co2_data;
extern uint16 CO2_modbus_Addr;
extern int16 CO2_modbus_value;
extern uint8_t flag_write_i2c;
extern QueueHandle_t qSendCo2;
//extern uint8_t scd4x_perform_forced;
//extern uint16_t co2_asc;
//extern uint16_t co2_frc;


uint16_t read_co2_by_block(uint16_t addr);
void write_co2_by_block(uint16_t addr,uint8_t HeadLen,uint8_t *pData,uint8_t type);

uint8_t check_write_co2(uint16_t addr,uint16_t value);



#endif
