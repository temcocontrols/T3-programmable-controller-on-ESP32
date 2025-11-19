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
#include "co2.h"
#include "modbus.h"


extern STR_Task_Test task_test;

Str_points_ptr put_io_buf(Point_type_equate type, uint8 point);



/*const uint8 Var_label[13][9] = {
	
	"Baudrate",   //0
	"StnNumer",   //1
	"Protocol", //2
	"Instance",//3
	"Unit", //10 
	"Trgger_S",
	"Timer_S",
	"Trgger_L",
	"Timer_L",
	"Trgger_O",
	"Timer_O",
	"Trgger_C",
	"Timer_C",

};
const uint8 Var_Description[13][21] = {
	
	"baudrate select",   	//0
	"station number",   	//1
	"modbus/bacnet switch",    //2
	"instance number",		//3
	"temprature unit",//10
	"Trigger of Sound",
	"Timer of Sound",
	"Trigger of light",
	"Timer of light",
	"Trigger of OCC",
	"Timer of OCC",
	"Trigger of CO2",
	"Timer of CO2",
};
*/


QueueHandle_t qSendCo2;
uint8 isBlankScreen;
uint8_t display_config[5];

/*_RANGE_ output_range_table[3] =
{
	{0, 1000},	// hum, 0.1%
	{0, 1000},	// temp, 0.1c
	{0, 3000},	// co2, ppm
}; */



STR_CO2_Reg co2_data;

uint16 CO2_modbus_Addr;
int16 CO2_modbus_value;
uint8_t flag_write_i2c;

uint16_t read_co2_by_block(uint16_t addr)
{
	uint8_t item;
	uint16_t *block;
	uint8_t *block1;
	uint8_t temp;
	/*uint8_t lcd_i2c_sensor_index;
	
	lcd_i2c_sensor_index = co2_data.lcd_i2c_sensor_index;*/

	if(addr == MODBUS_CO2_TEMPERATURE_DEGREE_C_OR_F)
	{
	  return co2_data.deg_c_or_f;
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_CELSIUS)
	{
	  return co2_data.internal_temperature_c;
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_FAHRENHEIT)
	{
	  return co2_data.internal_temperature_f;
	}
	/*else if(addr == MODBUS_CO2_EXTERNAL_TEMPERATURE_CELSIUS)
	{
	  return co2_data.I2C_Sensor[lcd_i2c_sensor_index].tem_org;
	}

	else if(addr == MODBUS_CO2_HUMIDITY)
	{
		return co2_data.I2C_Sensor[lcd_i2c_sensor_index].hum_org;
	}
	else if(addr == MODBUS_CO2_INTERNAL)
	{
		return co2_data.I2C_Sensor[lcd_i2c_sensor_index].co2_org;
	}*/
	else if(addr == MODBUS_CO2_PREALARM_SETPOINT)
	{
	  return co2_data.pre_alarm_setpoint;
	}
	else if(addr == MODBUS_CO2_ALARM_SETPOINT)
	{
	  return co2_data.alarm_setpoint;
	}
	else if(addr == MODBUS_CO2_ALARM_AUTO_MANUAL)
	{
	  return co2_data.alarm_state;
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_ON_TIME)
	{
	  return co2_data.pre_alarm_on_time;
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_OFF_TIME)
	{
	  return co2_data.pre_alarm_off_time;
	}
	else if(addr == MODBUS_CO2_ALARM_DELAY_TIME)
	{
	  return co2_data.alarm_delay_time;
	}
/*	else if(addr == MODBUS_CO2_OUTPUT_AUTO_MANUAL)
	{
	  return co2_data.output_auto_manual;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_TEM)
	{
	  return co2_data.output_manual_value_temp;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_HUM)
	{
	  return co2_data.output_manual_value_humi;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_CO2)
	{
	  return co2_data.output_manual_value_co2;
	}*/
	else if(addr == MODBUS_CO2_OUTPUT_MODE)
	{
	  return co2_data.output_mode;
	}
	/*else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_TEM)
	{
	  return co2_data.output_range_table[CHANNEL_TEMP].min;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_TEM)
	{
	  return co2_data.output_range_table[CHANNEL_TEMP].max;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_HUM)
	{
	  return co2_data.output_range_table[CHANNEL_HUM].min;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_HUM)
	{
	  return co2_data.output_range_table[CHANNEL_HUM].max;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_CO2)
	{
	  return co2_data.output_range_table[CHANNEL_CO2].min;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_CO2)
	{
	  return co2_data.output_range_table[CHANNEL_CO2].max;
	}*/
	else if(addr == MODBUS_CO2_BACKLIGHT_KEEP_SECONDS)
	{
	  return co2_data.backlight_keep_seconds;
	}
	/*else if(addr == MODBUS_CO2_LCD_I2C_SENSOR_Index)
	{
	  return co2_data.lcd_i2c_sensor_index;
	}*/
	else if(addr == MODBUS_CO2_I2C_SENOR1_TYPE)
	{
	  return co2_data.i2c_sensor_type[0];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM)
	{
	  return co2_data.I2C_Sensor[0].tem_org + co2_data.I2C_Sensor[0].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM)
	{
	  return co2_data.I2C_Sensor[0].hum_org + co2_data.I2C_Sensor[0].hum_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_CO2)
	{
	  return co2_data.I2C_Sensor[0].co2_org;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM_OFFSET)
	{
	  return co2_data.I2C_Sensor[0].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM_OFFSET)
	{
	  return co2_data.I2C_Sensor[0].hum_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TYPE)
	{
	  return co2_data.i2c_sensor_type[1];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM)
	{
	  return co2_data.I2C_Sensor[1].tem_org + co2_data.I2C_Sensor[1].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM)
	{
	  return co2_data.I2C_Sensor[1].hum_org + co2_data.I2C_Sensor[1].hum_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_CO2)
	{
	  return co2_data.I2C_Sensor[1].co2_org;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM_OFFSET)
	{
	  return co2_data.I2C_Sensor[1].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM_OFFSET)
	{
	  return co2_data.I2C_Sensor[1].hum_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TYPE)
	{
	  return co2_data.i2c_sensor_type[2];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM)
	{
	  return co2_data.I2C_Sensor[2].tem_org + co2_data.I2C_Sensor[2].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM)
	{
	  return co2_data.I2C_Sensor[2].hum_org + co2_data.I2C_Sensor[2].hum_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_CO2)
	{
	  return co2_data.I2C_Sensor[2].co2_org;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM_OFFSET)
	{
	  return co2_data.I2C_Sensor[2].tem_offset;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM_OFFSET)
	{
	  return co2_data.I2C_Sensor[2].hum_offset;
	}
	/*else if(addr == MODBUS_CO2_SCREEN_AREA_1)
	{
	  return co2_data.screenArea1;
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_2)
	{
	  return co2_data.screenArea2;
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_3)
	{
	  return co2_data.screenArea3;
	}
	else if(addr == MODBUS_CO2_ENABLE_SCROLL)
	{
	  return co2_data.enableScroll;
	}
	else if(addr == MODBUS_CO2_ALARM_ENABLE)
	{
	  return co2_data.alarmEnable;
	}
	else if(addr == MODBUS_CO2_FRC_GET)
	{
	  return co2_data.co2_asc;
	}
	else if(addr == MODBUS_CO2_FRC_VALUE)
	{
	  return co2_data.co2_frc;
	}
	else if(addr == MODBUS_CO2_CAL)
	{
	  return co2_data.flag_co2_asc[lcd_i2c_sensor_index];
	}*/
	else if(addr == MODBUS_OUTPUT_HUM)
	{
	  return co2_data.analog_output[0];
	}
	else if(addr == MODBUS_OUTPUT_TEMP)
	{
	  return co2_data.analog_output[1];
	}
	else if(addr == MODBUS_OUTPUT_CO2)
	{
	  return co2_data.analog_output[2];
	}
	return 0;
}

uint8_t check_write_co2(uint16_t addr,uint16_t value)
{
/*	uint8_t lcd_i2c_sensor_index;

	lcd_i2c_sensor_index = co2_data.lcd_i2c_sensor_index;
	if(lcd_i2c_sensor_index == 255)
		lcd_i2c_sensor_index = 0;*/

	if(addr == MODBUS_CO2_TEMPERATURE_DEGREE_C_OR_F)
	{
		if(value == co2_data.deg_c_or_f)
		  return 1;
		else
				   return 0;
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_CELSIUS)
	{
		if(value == co2_data.internal_temperature_c)
		  return 1;
		else
				   return 0;
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_FAHRENHEIT)
	{
	   if(value == co2_data.internal_temperature_c)
		  return 1;
	   else
	   		   return 0;
	}
	/*else if(addr == MODBUS_CO2_EXTERNAL_TEMPERATURE_CELSIUS)
	{
		 if(value == co2_data.I2C_Sensor[lcd_i2c_sensor_index].tem_org)
			  return 1;
		 else
				   return 0;
	}
	else if(addr == MODBUS_CO2_HUMIDITY)
	{
		if(value == co2_data.I2C_Sensor[lcd_i2c_sensor_index].hum_org)
			return 1;
		else
				   return 0;
	}
	else if(addr == MODBUS_CO2_INTERNAL)
	{
		if(value == co2_data.I2C_Sensor[lcd_i2c_sensor_index].co2_org)
			return 1;
		else
				   return 0;
	}*/
	else if(addr == MODBUS_CO2_PREALARM_SETPOINT)
	{
	   if(value == co2_data.pre_alarm_setpoint)
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_ALARM_SETPOINT)
	{
	   if(value == co2_data.alarm_setpoint)
		  return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_ALARM_AUTO_MANUAL)
	{
	   if(value == co2_data.alarm_state)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_ON_TIME)
	{
	   if(value == co2_data.pre_alarm_on_time )
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_OFF_TIME)
	{
	   if(value == co2_data.pre_alarm_off_time)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_ALARM_DELAY_TIME)
	{
	   if(value == co2_data.alarm_delay_time)
		   return 1;
	   else
	   		   return 0;
	}
	/*else if(addr == MODBUS_CO2_OUTPUT_AUTO_MANUAL)
	{
	   if(value == co2_data.output_auto_manual )
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_TEM)
	{
	   if(value == co2_data.output_manual_value_temp)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_HUM)
	{
	   if(value == co2_data.output_manual_value_humi)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_CO2)
	{
	   if(value == co2_data.output_manual_value_co2)
		   return 1;
	   else
	   		   return 0;
	}*/
	else if(addr == MODBUS_CO2_OUTPUT_MODE)
	{
	   if(value == co2_data.output_mode)
		   return 1;
	   else
	   		   return 0;
	}
	/*else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_TEM)
	{
	   if(value == co2_data.output_range_table[CHANNEL_TEMP].min)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_TEM)
	{
	   if(value == co2_data.output_range_table[CHANNEL_TEMP].max)
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_HUM)
	{
	   if(value == co2_data.output_range_table[CHANNEL_HUM].min)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_HUM)
	{
	   if(value == co2_data.output_range_table[CHANNEL_HUM].max )
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_CO2)
	{
	   if(value == co2_data.output_range_table[CHANNEL_CO2].min)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_CO2)
	{
	   if(value == co2_data.output_range_table[CHANNEL_CO2].max)
		   return 1;
	   else
	   		   return 0;
	}*/
	else if(addr == MODBUS_CO2_BACKLIGHT_KEEP_SECONDS)
	{
	   if(value == co2_data.backlight_keep_seconds)
			return 1;
	   else
	   		   return 0;
	}
	/*else if(addr == MODBUS_CO2_LCD_I2C_SENSOR_Index)
	{
	   if(value == co2_data.lcd_i2c_sensor_index)
			return 1;
	   else
	   		   return 0;
	}*/
	else if(addr == MODBUS_CO2_I2C_SENOR1_TYPE)
	{
	   if(value == co2_data.i2c_sensor_type[0])
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM)
	{
	   if(value == co2_data.I2C_Sensor[0].tem_org)
		 return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM)
	{
	   if(value == co2_data.I2C_Sensor[0].hum_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_CO2)
	{
	   if(value == co2_data.I2C_Sensor[0].co2_org )
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[0].tem_offset)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[0].hum_offset)
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TYPE)
	{
	   if(value == co2_data.i2c_sensor_type[1] )
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM)
	{
	   if(value == co2_data.I2C_Sensor[1].tem_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM)
	{
	   if(value == co2_data.I2C_Sensor[1].hum_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_CO2)
	{
	   if(value == co2_data.I2C_Sensor[1].co2_org)
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[1].tem_offset)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[1].hum_offset)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TYPE)
	{
	   if(value == co2_data.i2c_sensor_type[2])
		   return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM)
	{
	   if(value == co2_data.I2C_Sensor[2].tem_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM)
	{
	   if(value == co2_data.I2C_Sensor[2].hum_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_CO2)
	{
	   if(value == co2_data.I2C_Sensor[2].co2_org)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[2].tem_offset)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM_OFFSET)
	{
	   if(value == co2_data.I2C_Sensor[2].hum_offset)
		   return 1;
	   else
	   		   return 0;
	}

	/*else if(addr == MODBUS_CO2_SCREEN_AREA_1)
	{
	   if(value == co2_data.screenArea1)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_2)
	{
	   if(value == co2_data.screenArea2)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_3)
	{
	   if(value == co2_data.screenArea3)
				return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_ENABLE_SCROLL)
	{
	   if(value == co2_data.enableScroll)
		return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_ALARM_ENABLE)
	{
	   if(value ==  co2_data.alarmEnable)
		 return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_FRC_GET)
	{
	   if(value == co2_data.co2_asc)
		return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_FRC_VALUE)
	{
	   if(value == co2_data.co2_frc)
			return 1;
	   else
	   		   return 0;
	}
	else if(addr == MODBUS_CO2_CAL)
	{
	   if(value == co2_data.flag_co2_asc[lcd_i2c_sensor_index])
			return 1;
	   else
		   return 0;
	}*/
	else
		return 1;
}

void write_co2_by_block(uint16_t addr,uint8_t HeadLen,uint8_t *pData,uint8_t type)
{
	uint8_t item;
	uint16_t *block;
	uint8_t *block1;
	uint8_t temp;
/*	uint8_t lcd_i2c_sensor_index;
	

	lcd_i2c_sensor_index = co2_data.lcd_i2c_sensor_index;
	if(lcd_i2c_sensor_index == 255)
		lcd_i2c_sensor_index = 0;*/
	if(addr == MODBUS_CO2_TEMPERATURE_DEGREE_C_OR_F)
	{
	  co2_data.deg_c_or_f = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_CELSIUS)
	{
	  co2_data.internal_temperature_c = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_INTERNAL_TEMPERATURE_FAHRENHEIT)
	{
	  co2_data.internal_temperature_f = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	/*else if(addr == MODBUS_CO2_EXTERNAL_TEMPERATURE_CELSIUS)
	{
	 co2_data.I2C_Sensor[lcd_i2c_sensor_index].tem_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_HUMIDITY)
	{
		co2_data.I2C_Sensor[lcd_i2c_sensor_index].hum_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_INTERNAL)
	{
		co2_data.I2C_Sensor[lcd_i2c_sensor_index].co2_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}*/
	else if(addr == MODBUS_CO2_PREALARM_SETPOINT)
	{
	   co2_data.pre_alarm_setpoint = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_ALARM_SETPOINT)
	{
	   co2_data.alarm_setpoint = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_ALARM_AUTO_MANUAL)
	{
	   co2_data.alarm_state = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_ON_TIME)
	{
	   co2_data.pre_alarm_on_time = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_PRE_ALARM_SETTING_OFF_TIME)
	{
	   co2_data.pre_alarm_off_time = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_ALARM_DELAY_TIME)
	{
	   co2_data.alarm_delay_time = pData[HeadLen + 5];;
	}
	/*else if(addr == MODBUS_CO2_OUTPUT_AUTO_MANUAL)
	{
	   co2_data.output_auto_manual = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_TEM)
	{
	   co2_data.output_manual_value_temp = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_HUM)
	{
	   co2_data.output_manual_value_humi = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_MANUAL_VALUE_CO2)
	{
	   co2_data.output_manual_value_co2 = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}*/
	else if(addr == MODBUS_CO2_OUTPUT_MODE)
	{
	   co2_data.output_mode = pData[HeadLen + 5];
	}
	/*else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_TEM)
	{
	   co2_data.output_range_table[CHANNEL_TEMP].min = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_TEM)
	{
	   co2_data.output_range_table[CHANNEL_TEMP].max = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_HUM)
	{
	   co2_data.output_range_table[CHANNEL_HUM].min = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_HUM)
	{
	   co2_data.output_range_table[CHANNEL_HUM].max = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MIN_CO2)
	{
	   co2_data.output_range_table[CHANNEL_CO2].min = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_OUTPUT_RANGE_MAX_CO2)
	{
	   co2_data.output_range_table[CHANNEL_CO2].max = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}*/

	else if(addr == MODBUS_CO2_BACKLIGHT_KEEP_SECONDS)
	{
	   co2_data.backlight_keep_seconds = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	/*else if(addr == MODBUS_CO2_LCD_I2C_SENSOR_Index)
	{
	   co2_data.lcd_i2c_sensor_index = pData[HeadLen + 5];
	}*/
	else if(addr == MODBUS_CO2_I2C_SENOR1_TYPE)
	{
	   co2_data.i2c_sensor_type[0] = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM)
	{
	   co2_data.I2C_Sensor[0].tem_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM)
	{
	   co2_data.I2C_Sensor[0].hum_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_CO2)
	{
	   co2_data.I2C_Sensor[0].co2_org = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_TEM_OFFSET)
	{
	   co2_data.I2C_Sensor[0].tem_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR1_HUM_OFFSET)
	{
	   co2_data.I2C_Sensor[0].hum_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}

	else if(addr == MODBUS_CO2_I2C_SENOR2_TYPE)
	{
	   co2_data.i2c_sensor_type[1] = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM)
	{
	   co2_data.I2C_Sensor[1].tem_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM)
	{
	   co2_data.I2C_Sensor[1].hum_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_CO2)
	{
	   co2_data.I2C_Sensor[1].co2_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_TEM_OFFSET)
	{
	   co2_data.I2C_Sensor[1].tem_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR2_HUM_OFFSET)
	{
	   co2_data.I2C_Sensor[1].hum_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TYPE)
	{
	   co2_data.i2c_sensor_type[2] = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM)
	{
	   co2_data.I2C_Sensor[2].tem_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM)
	{
	   co2_data.I2C_Sensor[2].hum_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_CO2)
	{
	   co2_data.I2C_Sensor[2].co2_org = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_TEM_OFFSET)
	{
	   co2_data.I2C_Sensor[2].tem_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}
	else if(addr == MODBUS_CO2_I2C_SENOR3_HUM_OFFSET)
	{
	   co2_data.I2C_Sensor[2].hum_offset = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);;
	}

	/*else if(addr == MODBUS_CO2_SCREEN_AREA_1)
	{
	   co2_data.screenArea1 = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_2)
	{
	   co2_data.screenArea2 = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_SCREEN_AREA_3)
	{
	   co2_data.screenArea3 = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_ENABLE_SCROLL)
	{
	   co2_data.enableScroll = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_ALARM_ENABLE)
	{
	   co2_data.alarmEnable = pData[HeadLen + 5];
	}
	else if(addr == MODBUS_CO2_FRC_GET)
	{
	   co2_data.co2_asc = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_FRC_VALUE)
	{
	   co2_data.co2_frc = pData[HeadLen + 5] + (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CO2_CAL)
	{
	   co2_data.flag_co2_asc[lcd_i2c_sensor_index] = pData[HeadLen + 5];
	}*/

	flag_write_i2c = 1;
	CO2_modbus_Addr = addr;
	CO2_modbus_value = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);

	xQueueSend(qSendCo2, &flag_write_i2c, 0);

	return 0;
}



void CO2_check_calibration(uint8_t i)
{
	Str_points_ptr ptr;
	uint16 addr = 0;
	uint16 value = 0;
	int16_t cal;
	ptr = put_io_buf(IN,i);

	cal = (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo) / 10;
	if(i == 0 || i == 1 || i == 3 || i == 4 || i == 6 || i == 7)
	{

		if( ptr.pin->calibration_sign ) // 0->postive 		1->negtive
			cal = -cal;
		value = cal;
	}
	else  // co2 value
	{
		value = ptr.pin->value / 1000;
	}

	Test[25]++;


	if(i == 0)
	{
		addr = MODBUS_CO2_I2C_SENOR1_TEM_OFFSET;
	}
	if(i == 1)
	{
		addr = MODBUS_CO2_I2C_SENOR1_HUM_OFFSET;
	}
	if(i == 2)
	{
		addr = MODBUS_CO2_I2C_SENOR1_CO2;
	}
	if(i == 3)
	{
		addr = MODBUS_CO2_I2C_SENOR2_TEM_OFFSET;
	}
	if(i == 4)
	{
		addr = MODBUS_CO2_I2C_SENOR2_HUM_OFFSET;
	}
	if(i == 5)
	{
		addr = MODBUS_CO2_I2C_SENOR2_CO2;
	}
	if(i == 6)
	{
		addr = MODBUS_CO2_I2C_SENOR3_TEM_OFFSET;
	}
	if(i == 7)
	{
		addr = MODBUS_CO2_I2C_SENOR3_HUM_OFFSET;
	}
	if(i == 8)
	{
		addr = MODBUS_CO2_I2C_SENOR3_CO2;
	}

	flag_write_i2c = 1;
	CO2_modbus_Addr = addr;
	CO2_modbus_value = value;
//	delay_ms(1000);
	//xQueueSend(qSendCo2, &flag_write_i2c, 0);

}
