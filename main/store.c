#include "flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_task.h"
#include "store.h"
#include "user_data.h"



//Str_in_point   inputs[MAX_AIS];
//Str_variable_point	vars[MAX_VARS + 12];
int16_t pre_mul_analog_input[10]; //used to filter  readings
int16_t mul_analog_in_buffer[10];
int16_t mul_analog_input[10];

/*trigger_t light_trigger;
trigger_t sound_trigger;
trigger_t co2_trigger;
trigger_t occ_trigger;*/

//Str_Setting_Info    Setting_Info;

void mass_flash_init(void)
{
/*	uint16_t temp=0;
	uint16_t loop , j ;
	uint16_t len = 0;
	uint8_t  tempbuf[INPUT_PAGE_LENTH];
	read_uint16_from_falsh(FLASH_INPUT_FLAG, &temp);
	if(temp != 10000)
	{		for(loop=0; loop<MAX_AIS; loop++ )
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
		//read_blob_info(FLASH_INPUT_INFO, (const void *)&inputs[0].description[0], len );
	}*/
}

uint16_t Filter(uint8_t channel,uint16_t input)
{
	// -------------FILTERING------------------
//	int16 xdata siDelta;
	int32_t siResult = 0;
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
			//siResult = (pre_mul_analog_input[I] * inputs[I].filter + siTemp) *10 / (inputs[I].filter + 1);
			if(siResult%10 >= 5)
				siResult += 10;
			pre_mul_analog_input[I] = siResult/10;// + InputFilter(I);
			siResult /= 10;
	}
	return siResult;

}

