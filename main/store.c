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
signed int  old_reading[32];
uint16_t Filter(uint8_t channel,uint16_t input)
{
	// -------------FILTERING------------------
	int16  siDelta;
	int32_t siResult = 0;
	uint8_t I;
	Str_points_ptr ptr;
	signed int  siTemp;
	signed long  slTemp;
	I = channel;
  	siTemp = input;
  	ptr = put_io_buf(IN,I);
  	siDelta = siTemp - (signed int)old_reading[I] ;    //compare new reading and old reading

  	// If the difference in new reading and old reading is greater than 5 degrees, implement rough filtering.
  	if (( siDelta >= 100 ) || ( siDelta <= -100 ) ) // deg f
  	{
  		old_reading[I] = old_reading[I] + (siDelta >> 1);
  	}
  	// Otherwise, implement fine filtering.
  	else
  	{
  		slTemp = (signed long)ptr.pin->filter * old_reading[I];
  		slTemp += (signed long)siTemp;
  		if(ptr.pin->filter + 1 > 0)
  			old_reading[I] = (signed int)(slTemp/(ptr.pin->filter +1));
  		else
  			Test[20 + I] = ptr.pin->filter + 10;
  	}

  	siResult = old_reading[I];
  	return siResult;

}

