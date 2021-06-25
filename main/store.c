//#include "deviceparams.h"
#include "flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_task.h"
#include "store.h"
#include "co2_cal.h"
#include "modbus.h"
//#include "ud_str.h"
//#include "controls.h"
//#include "driver/uart.h"


Str_in_point   inputs[MAX_AIS];
//Str_variable_point	vars[MAX_VARS + 12];
int16_t pre_mul_analog_input[10]; //used to filter  readings
int16_t mul_analog_in_buffer[10];
int16_t mul_analog_input[10];

trigger_t light_trigger;
trigger_t sound_trigger;
trigger_t co2_trigger;
trigger_t occ_trigger;

//Str_Setting_Info    Setting_Info;
const uint8_t Inputs_label[MAX_AIS][9] = {
 	"TEM",
	"HUM",
	"CO2",
	"TVOC",
	"PM1.0DEN",//PM1.0_w",
	"PM2.5DEN",//_w",
	"PM4.0DEN",//_w",
	"PM10DEN",//_w",
	"PM0.5C",//_n",
	"PM1.0C",//_n",
	"PM2.5C",//_n",
	"PM4.0C",//_n",
	"PM10C",//_n",
	"P_size",
	"SOUND",
	"LIGHT",
	"OCC",
	"AMBIENT",
	"OBJECT"
};
const uint8_t Inputs_Description[MAX_AIS][21] = {

	"TEMPERATURE",//"Temperatrue",
	"HUMIDITY",
	"CO2",
//	"VOC minipid2",
	"TVOC Sensor",//VOC sensirion",
	"PM1.0 DENSITY",//PM1.0 in ug/m3",
	"PM2.5 DENSITY",//in ug/m3",
	"PM4.0 DENSITY",//in ug/m3",
	"PM10 DENSITY",//in ug/m3",
	"PM0.5 COUNT",//particle",
	"PM1.0 COUNT",//particle",
	"PM2.5 COUNT",//particle",
	"PM4.0 COUNT",//particle",
	"PM10 COUNT",//particle",
	"TypicalParticleSize",
	"Sound Level",
	"Light Strength",
	"Occupancy",
	"Ambient temperature",
	"Object temperature"
};

void mass_flash_init(void)
{
	uint16_t temp=0;
	uint16_t loop , j ;
	uint16_t len = 0;
	uint8_t  tempbuf[INPUT_PAGE_LENTH];
	read_uint16_from_falsh(FLASH_INPUT_FLAG, &temp);
	if(temp != 10000)
	{
		for(loop=0; loop<MAX_AIS; loop++ )
		{
			memcpy(inputs[loop].description, Inputs_Description[loop], 21);
			memcpy(inputs[loop].label, Inputs_label[loop], 9);
			inputs[loop].value = 0;
			inputs[loop].filter = 5 ;
			inputs[loop].decom = 0 ;
			inputs[loop].sub_id = 0 ;
			inputs[loop].sub_product = 0 ;
			inputs[loop].control = 0 ;
			inputs[loop].auto_manual = 0 ;
			inputs[loop].digital_analog = 1 ;
			inputs[loop].calibration_sign = 0 ;
			inputs[loop].sub_number = 0 ;
			inputs[loop].calibration_hi = 0;//(500>>8)&0xff ;
			inputs[loop].calibration_lo = 0;//500 &0xff ;
			inputs[loop].range = not_used_input ;
		}
		inputs[1].range = R10K_40_120DegC;
		inputs[1].range = Humidty;
		inputs[2].range = CO2_PPM;
		inputs[3].range = not_used_input;
		inputs[4].range = not_used_input;
		inputs[5].range = not_used_input;
		inputs[6].range = not_used_input;
		inputs[7].range = not_used_input;
		inputs[8].range = not_used_input;
		inputs[9].range = not_used_input;
		inputs[10].range = not_used_input;
		inputs[11].range = not_used_input;
		inputs[12].range = not_used_input;
		inputs[14].range = DB;
		inputs[15].range = LUX;
		inputs[17].range = R10K_40_120DegC;
		inputs[18].range = R10K_40_120DegC;

		len = MAX_AIS * sizeof(Str_in_point) ;
		memcpy(tempbuf,(void*)&inputs[0], len);
		//iap_write_appbin(IN_PAGE,(uint8_t*)tempbuf, len);
		save_blob_info(FLASH_INPUT_INFO, (const void*)tempbuf, len);
		save_uint16_to_flash(FLASH_INPUT_FLAG, 10000);
		//STMFLASH_WriteHalfWord(IN_PAGE_FLAG, 10000) ;
		//STMFLASH_Lock();
	}
	else
	{
		len = MAX_AIS * sizeof(Str_in_point) ;
		//STMFLASH_MUL_Read(IN_PAGE,(void *)&inputs[0].description[0], len );
		read_blob_info(FLASH_INPUT_INFO, (const void *)&inputs[0].description[0], len );
	}
}

uint16_t Filter(uint8_t channel,uint16_t input)
{
	// -------------FILTERING------------------
//	int16 xdata siDelta;
	int32_t siResult;
	uint8_t I;
  int32_t siTemp;
	I = channel;
	siTemp = input;
  /*if(I == 10)
	{
	if(power_up_timer < 5)
    	old_temperature = siTemp;
	siResult = (old_temperature * EEP_Filter + siTemp) / (EEP_Filter + 1);
	old_temperature = siResult;

	}
	else*/
	{
			siResult = (pre_mul_analog_input[I] * inputs[I].filter + siTemp) *10 / (inputs[I].filter + 1);
			if(siResult%10 >= 5)
				siResult += 10;
			pre_mul_analog_input[I] = siResult/10;// + InputFilter(I);
			siResult /= 10;
	}
	return siResult;

}

#if 1
void control_input(void)
{
	Str_in_point *ins;// = new Str_in_point;
	uint8_t point=0;
	uint8_t temp;
	uint32_t sample=0;
	ins = inputs;

	while (point<MAX_INS)
	{
		if(ins->digital_analog ==1)  //analog
		{
			temp = ins->decom;
			temp &= 0xf0;
			temp |= IN_NORMAL;
			ins->decom = temp;

			if(point == 0)
			{
				sample = g_sensors.temperature*100;
			}
			else if(point == 1)
			{
				sample = g_sensors.humidity*100;
			}
			else if(point == 2)
			{
				sample = g_sensors.co2*1000;
			}
			else if(point == 3)
			{
				sample = g_sensors.voc_value*1000;//minipid2_value*1000;
			}
//				else if(point == 4)
//				{
//					sample = voc_value*1000;
//				}
			else if(point == 4)
			{
				sample = 0;//pm25_weight_10*1000;
			}
			else if(point == 5)
			{
				sample = 0;//bac_input[0]*1000;
			}
			else if(point == 6)
			{
				sample = 0;//pm25_weight_40*1000;
			}
			else if(point == 7)
			{
				sample = 0;//pm25_weight_100*1000;
			}
			else if(point == 8)
			{
				sample = 0;//pm25_number_05*1000;
			}
			else if(point == 9)
			{
				sample = 0;//pm25_number_10*1000;
			}
			else if(point == 10)
			{
				sample = 0;//pm25_number_25*1000;
			}
			else if(point == 11)
			{
				sample = 0;//pm25_number_40*1000;
			}
			else if(point == 12)
			{
				sample = 0;//pm25_number_100*1000;
			}
			else if(point == 13)
			{
				sample = 0;//typical_partical_size*1000;
			}
			else if(point == 14)
			{
				sample = g_sensors.sound*1000;
			}
			else if(point == 15)
			{
				sample = g_sensors.light_value*1000;//light_sensor*1000;
			}
			else if(point == 16)
			{
				sample = g_sensors.occ*1000;
			}
			else if(point == 17)
			{
				sample = g_sensors.ambient*100;
			}
			else if(point == 18)
			{
				sample = g_sensors.object*100;
			}
#if 0
			if(!ins->calibration_sign)
				sample += 100 * (ins->calibration_hi * 256 + ins->calibration_lo);
			else
				sample += -100 * (ins->calibration_hi * 256 + ins->calibration_lo);
#endif
			ins->value = sample;//swap_double(sample);
			//inputs[point].value = sample;
		}
		point++;
		ins++;
	}
}
#endif

void deal_input_trigger(void)
{
	if(((g_sensors.light_value-light_trigger.trigger)>0)&&(light_trigger.count_down == 0))
	{
		light_trigger.alarmOn = 1;
		light_trigger.count_down = light_trigger.timer*60;
	}
	if(g_sensors.occ == 1)
	{
		occ_trigger.alarmOn = 1;
		occ_trigger.count_down = occ_trigger.timer*60;
	}
	if(((g_sensors.co2-co2_trigger.trigger)>0)&&(co2_trigger.count_down == 0))
	{
		co2_trigger.alarmOn = 1;
		co2_trigger.count_down = light_trigger.timer*60;
	}
	if(((g_sensors.sound-sound_trigger.trigger)>0)&&(sound_trigger.count_down == 0))
	{
		sound_trigger.alarmOn = 1;
		sound_trigger.count_down = light_trigger.timer*60;
	}
}

void deal_trigger_timer(void)
{
	if(sound_trigger.count_down>0)
		sound_trigger.count_down--;
	else
		sound_trigger.alarmOn = 0;
	if(light_trigger.count_down>0)
		light_trigger.count_down--;
	else
		light_trigger.alarmOn = 0;
	if(co2_trigger.count_down >0)
		co2_trigger.count_down--;
	else
		co2_trigger.alarmOn = 0;
	if(occ_trigger.count_down >0)
		occ_trigger.count_down--;
	else
		occ_trigger.alarmOn = 0;
}

void input_task(void *arg)
{
	while(1)
	{
		control_input();
		deal_input_trigger();
		deal_trigger_timer();
		//uart_write_bytes(UART_NUM_1, (const char *)holding_reg_params.testBuf, 20);
		if(co2_bkcal_onoff == CO2_BKCAL_ON)
			co2_background_calibration(g_sensors.co2);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
