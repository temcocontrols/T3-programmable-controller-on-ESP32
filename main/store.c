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

