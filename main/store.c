#include "flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_task.h"
#include "store.h"
#include "user_data.h"
#include "define.h"
#include "controls.h"



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

/*
 * RMC1232 + old ARM rev23: 0-20mA glitch is one ARM sample, but ESP rereads
 * the same I2C word many times (~100ms) before ARM rescans that channel (~800ms).
 * Confirm only on a new ARM raw, or after one full mux period.
 */
#define AI_MA_GLITCH_TH		120
#define AI_MA_BIG_TH		400
#define AI_MA_CONFIRM_TH	100
#define AI_MA_HOLD_MS		2500
static uint16_t ma_last_raw[32];
static int16_t ma_cand[32];
static uint8_t ma_holding[32];
static uint8_t ma_seeded[32];
static uint8_t ma_hits[32];
static TickType_t ma_hold_tick[32];

static int16_t ma_abs_delta(signed int a, signed int b)
{
	signed int d = a - b;
	if(d < 0)
		d = -d;
	return (int16_t)d;
}

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

	if((Modbus.mini_type == PROJECT_RMC1232) && (I < 32))
	{
		uint8_t is_ma;
		int16_t jump;
		uint8_t new_arm;
		TickType_t now;

		is_ma = ((ptr.pin->range == I0_20ma) || (ptr.pin->range == P0_100_4_20ma) ||
		         (((ptr.pin->decom >> 4) & 0x0f) == INPUT_I0_20ma));

		if(ma_seeded[I] == 0)
		{
			ma_seeded[I] = 1;
			ma_last_raw[I] = input;
			old_reading[I] = siTemp;
			return (uint16_t)old_reading[I];
		}

		jump = ma_abs_delta(siTemp, old_reading[I]);
		new_arm = (input != ma_last_raw[I]);
		now = xTaskGetTickCount();

		if((is_ma && (jump >= AI_MA_GLITCH_TH)) || (jump >= AI_MA_BIG_TH))
		{
			if(ma_holding[I] == 0)
			{
				ma_holding[I] = 1;
				ma_hits[I] = 1;
				ma_cand[I] = (int16_t)siTemp;
				ma_hold_tick[I] = now;
				ma_last_raw[I] = input;
				return (uint16_t)old_reading[I];
			}

			if(ma_abs_delta(siTemp, ma_cand[I]) <= AI_MA_CONFIRM_TH)
			{
				if(new_arm)
					ma_hits[I]++;
				/* need a second ARM sample, or ~one mux period of the same raw */
				if((ma_hits[I] < 2) &&
				   ((now - ma_hold_tick[I]) < pdMS_TO_TICKS(AI_MA_HOLD_MS)))
				{
					ma_last_raw[I] = input;
					return (uint16_t)old_reading[I];
				}
				ma_holding[I] = 0;
				ma_last_raw[I] = input;
			}
			else
			{
				ma_hits[I] = 1;
				ma_cand[I] = (int16_t)siTemp;
				ma_hold_tick[I] = now;
				ma_last_raw[I] = input;
				return (uint16_t)old_reading[I];
			}
		}
		else
		{
			ma_holding[I] = 0;
			ma_last_raw[I] = input;
		}
	}

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

  	}

  	siResult = old_reading[I];
  	return siResult;

}

