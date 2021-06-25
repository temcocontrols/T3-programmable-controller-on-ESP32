#include "co2_cal.h"
#include "flash.h"
//#include "nvs.h"

uint16_t   co2_bkcal_day =0;  	//how many days the calibration will run, default is 14 days
uint16_t   co2_level = 0; 	    //lowest co2 level in current area, default is 400 ppm
uint8_t    co2_1h_timer;      	//one hour counter, the lowest co2 value need to keep at leat one hour
uint8_t    min_co2_adj;       	//the minimal adjustable co2 value,default is 1ppm
uint8_t    co2_bkcal_onoff;   	//co2 back ground calibration on/off flag, 0: off, 1:on
int16_t    co2_bkcal_value;   	//co2 back ground calibration value, this need to be stored in different place with factory calibration
uint16_t   co2_lowest_value=0;  	//the lowest co2 value during back ground calibration
uint16_t   co2_temp;          	//temporary co2 value
uint8_t    value_keep_time;   	//how long the lowest value need to keep

static void co2_bkcal_start(void)
{
	co2_lowest_value = co2_level;
	co2_temp = 1000;
}

void co2_cal_initial(void)
{
	esp_err_t err;
  	err = read_uint16_from_falsh(FLASH_CO2_CAL_DAYS,&co2_bkcal_day);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
	if(err == 0x1102)
	{
		co2_bkcal_day = 7;
		save_uint16_to_flash(FLASH_CO2_CAL_DAYS, co2_bkcal_day);
	}
	if(co2_bkcal_day > 30)  co2_bkcal_day = 7;

	//co2_level =((uint16)AT24CXX_ReadOneByte(EEP_CO2_NATURE_LEVEL+1)<<8)|AT24CXX_ReadOneByte(EEP_CO2_NATURE_LEVEL);

	err = read_uint16_from_falsh(FLASH_CO2_NATURE_LEVEL,&co2_level);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
	if(err == 0x1102)
	{
		co2_level = 400;
		save_uint16_to_flash(FLASH_CO2_NATURE_LEVEL, co2_level);
	}
	if(co2_level > 500) co2_level = 400;								//default is 400ppm, minimum is 390, max is 500

	//value_keep_time = AT24CXX_ReadOneByte(EEP_CO2_LOWVALUE_REMAIN_TIME); 	//default 1 h
	err = read_uint8_from_falsh(FLASH_CO2_LOWVALUE_REMAIN_TIME,&value_keep_time);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
	if(err == 0x1102)
	{
		value_keep_time = 1;
		save_uint8_to_flash(FLASH_CO2_LOWVALUE_REMAIN_TIME, value_keep_time);
	}

	//min_co2_adj = AT24CXX_ReadOneByte(EEP_CO2_MIN_ADJ);         		 	//default is 1ppm
	err = read_uint8_from_falsh(FLASH_CO2_MIN_ADJ,&min_co2_adj);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
	if(err == 0x1102)
	{
		min_co2_adj = 1;
		save_uint8_to_flash(FLASH_CO2_MIN_ADJ, min_co2_adj);
	}
	if(min_co2_adj > 10 )  min_co2_adj = 1;							 	// default is 1ppm, max is 10 ppm, minimum is 1


	//co2_bkcal_onoff = AT24CXX_ReadOneByte(EEP_CO2_BKCAL_ONOFF);			//default is 0
	err = read_uint8_from_falsh(FLASH_CO2_BKCAL_ONOFF,&co2_bkcal_onoff);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
	if(err == 0x1102)
	{
		co2_bkcal_onoff = 0;
		save_uint8_to_flash(FLASH_CO2_BKCAL_ONOFF, co2_bkcal_onoff);
	}
	if(co2_bkcal_onoff > 1) co2_bkcal_onoff = 0;

	if(co2_bkcal_onoff == CO2_BKCAL_ON)
	{
		err = read_int16_from_falsh(FLASH_CO2_BKCAL_VALUE,&co2_bkcal_value);//read_eeprom(EEP_CO2_CAL_DAYS);					//default is 7 days. Max is 30 days. Minimum is 2 days
		if(err == 0x1102)
		{
			co2_level = 0;
			save_int16_to_flash(FLASH_CO2_BKCAL_VALUE, co2_bkcal_value);
		}
	}
		//co2_bkcal_value = (int16)(((uint16)AT24CXX_ReadOneByte(EEP_CO2_BKCAL_VALUE+1)<<8)|AT24CXX_ReadOneByte(EEP_CO2_BKCAL_VALUE));
	else
		co2_bkcal_value = 0;


	co2_bkcal_start();

}

static void save_14days_value(int16_t value)
{
	co2_bkcal_value = value;
	save_int16_to_flash(FLASH_CO2_BKCAL_VALUE, co2_bkcal_value);
	//write_eeprom(EEP_CO2_BKCAL_VALUE, value & 0xff);
	//write_eeprom(EEP_CO2_BKCAL_VALUE+1, (value>>8) & 0xff);
}

void co2_background_calibration(uint16_t current_co2)//need to call this function every one second for the software timer
{
	static uint8_t    i = 0;
	static uint16_t   co2_1h_counter=0;
	static uint16_t   repeat_counter = 0;
	static uint16_t   co2_bkcal_day_temp = 0;

//	if(co2_bkcal_onoff == CO2_BKCAL_ON)	//co2 back ground calibration on
//	{
		co2_1h_counter ++;
		if(co2_temp > current_co2)			//get minum co2 value
		{
			i++;
			if(i>10)
			{
				i=0;
//				co2_temp = current_co2;
				co2_temp -= min_co2_adj;
				co2_1h_counter = 0;
			}
		}

		if((current_co2 <= co2_temp + 10) && (current_co2 >= co2_temp - 10))//repeat the lowest value 1 hour
		{
			repeat_counter++;
			if(repeat_counter >= 3600 )
				co2_lowest_value = co2_temp;
		}
		if(co2_1h_counter >= 3600 ) // 1 hour
		{
			co2_1h_timer++;
			co2_1h_counter = 0;
			co2_bkcal_day_temp++;
			if(co2_1h_timer >= value_keep_time)
			{
				co2_1h_timer = 0;
//				if(co2_lowest_value > co2_level + 50)
//					co2_bkcal_value += min_co2_adj;
//				if(co2_lowest_value < co2_level - 50)
//					co2_bkcal_value -= min_co2_adj;
				//save_bkcal_onoff(CO2_BKCAL_OFF);
			}
		}
		if(co2_bkcal_day_temp >= co2_bkcal_day*24)//14 days, calibration finished
		{
			co2_bkcal_day_temp = 0;
		// 	co2_bkcal_onoff = CO2_BKCAL_OFF;
		//	save_bkcal_onoff(CO2_BKCAL_OFF);
			if((co2_lowest_value > co2_level + 10)||(co2_lowest_value < co2_level - 10))
			{
//				i = co2_lowest_value - co2_level;

				save_14days_value((int16_t)co2_lowest_value - co2_level);
				co2_bkcal_start();
			}
			//write_eeprom(EEP_CO2_BKCAL_ONOFF, CO2_BKCAL_OFF);
		}
//		}
//	}

}
