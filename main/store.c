#include "flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_task.h"
#include "store.h"
#include "user_data.h"
#include "driver/gpio.h"
#include "define.h"
#include "controls.h"

//Str_in_point   inputs[MAX_AIS];
//Str_variable_point	vars[MAX_VARS + 12];
int16_t pre_mul_analog_input[10]; //used to filter  readings
int16_t mul_analog_in_buffer[10];
int16_t mul_analog_input[10];

trigger_t occ_trigger = {
	.trigger = 1,
	.timer = 300,
	.alarmOn = 0,
	.count_down = 0,
};

/* ESP32-S3 schematic net IO4. Occupancy PIR on Tstat11. */
#define TSTAT11_OCC_GPIO		    GPIO_NUM_4
#define TSTAT11_OCC_IN_INDEX	    4
#define TSTAT11_OCC_DEFAULT_TIMER	30

static void tstat11_occ_write_in4(uint8_t occupied)
{
	Str_points_ptr ptr = put_io_buf(IN, TSTAT11_OCC_IN_INDEX);

	if(ptr.pin == NULL)
	{
		return;
	}
	if(ptr.pin->auto_manual != 0)
	{
		return;
	}

	ptr.pin->digital_analog = 0;
	if(ptr.pin->range == 0)
	{
		ptr.pin->range = UNOCCUPIED_OCCUPIED;
	}
	ptr.pin->control = occupied ? 1 : 0;
	ptr.pin->value = occupied ? 1000 : 0;
}

void tstat11_occ_init(void)
{
	gpio_config_t io_conf;

	if(Modbus.mini_type != MINI_TSTAT11)
	{
		return;
	}

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = (1ULL << TSTAT11_OCC_GPIO);
	io_conf.pull_down_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

	if(occ_trigger.timer == 0)
	{
		occ_trigger.timer = TSTAT11_OCC_DEFAULT_TIMER;
	}
	g_sensors.occ = 0;
	tstat11_occ_write_in4(0);
}

void tstat11_occ_update(void)
{
	static uint8_t inited = 0;
	static uint16_t ms_acc = 0;
	static uint8_t last_pin = 0;
	uint8_t occupied_pin;
	uint16_t occupied_level;

	if(Modbus.mini_type != MINI_TSTAT11)
	{
		return;
	}

	if(inited == 0)
	{
		tstat11_occ_init();
		inited = 1;
	}

	occupied_level = (occ_trigger.trigger != 0) ? 1 : 0;
	occupied_pin = (gpio_get_level(TSTAT11_OCC_GPIO) != 0) ? 1 : 0;

	/* Reset hold timer on a new trigger, then let it count down each second. */
	if((occupied_pin == occupied_level) && (last_pin != occupied_level))
	{
		if(occ_trigger.timer == 0)
		{
			occ_trigger.timer = TSTAT11_OCC_DEFAULT_TIMER;
		}
		occ_trigger.alarmOn = 1;
		occ_trigger.count_down = occ_trigger.timer;
		g_sensors.occ = 1;
		tstat11_occ_write_in4(1);
	}
	last_pin = occupied_pin;

	ms_acc += 10;
	if(ms_acc < 1000)
	{
		return;
	}
	ms_acc = 0;

	if(occ_trigger.count_down > 0)
	{
		occ_trigger.count_down--;
		if(occ_trigger.count_down == 0)
		{
			occ_trigger.alarmOn = 0;
			g_sensors.occ = 0;
			tstat11_occ_write_in4(0);
		}
		else
		{
			g_sensors.occ = 1;
			tstat11_occ_write_in4(1);
		}
	}
}

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

  	}

  	siResult = old_reading[I];
  	return siResult;

}

