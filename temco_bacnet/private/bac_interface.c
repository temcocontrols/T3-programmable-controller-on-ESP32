//#include "main.h"
#include "bacnet.h"
#include "user_data.h"
#include "bactimevalue.h"
#include "bacdevobjpropref.h"
#include <string.h>
#include "datetime.h"
#include <stdlib.h>

typedef enum
{
	INPUT_NOUSED = 0,
	INPUT_I0_20ma,
	INPUT_V0_5,
	INPUT_0_10V,
	INPUT_THERM,// 10K
	INPUT_PT1K,
}E_IN_TYPE;


UN_Time Rtc;
U8_T flag_mstp_source;
char * itoa( int value, char *string, int radix );

/*U8_T Get_AOx_by_index(uint8_t index,uint8_t *ao_index);
U8_T Get_BOx_by_index(uint8_t index,uint8_t *bo_index);
U8_T Get_index_by_AVx(uint8_t av_index,uint8_t *var_index);
U8_T Get_index_by_BVx(uint8_t bv_index,uint8_t *var_index);
U8_T Get_index_by_AIx(uint8_t ai_index,uint8_t *in_index);
U8_T Get_index_by_BIx(uint8_t bi_index,uint8_t *in_index);
U8_T Get_index_by_AOx(uint8_t ao_index,uint8_t *out_index);
U8_T Get_index_by_BOx(uint8_t do_index,uint8_t *out_index);*/


//extern BACNET_DATE Local_Date;
//extern BACNET_TIME Local_Time;
extern uint8_t count_lcd_time_off_delay;
extern uint8_t count_hold_on_bip_to_mstp;
extern uint32_t net_health[4];
void Set_broadcast_bip_address(uint32_t net_address);
void save_TemcoAV_AIRALB(uint16_t index, uint16_t value);
uint16_t get_TemcoAVS_airlab(uint8_t index);


char get_current_mstp_port(void)
{
	return 0;
}
uint8_t get_max_internal_output(void);
extern void set_output_raw(uint8_t point,uint16_t value);
int save_point_info(uint8_t point_type);
void save_icon_config(uint8_t value);

U8_T Get_Mini_Type(void);
void Get_AVS(void);


U8_T base_in;
U8_T base_out;
U8_T base_var;
extern U8_T max_aos;
extern U8_T max_dos;
extern S16_T timezone;

//extern U16_T input_raw[64];
//extern U16_T output_raw[64];
//extern S8_T panelname[20];
//U16_T Test[50];
#if 1//BAC_COMMON 

char bacnet_vendor_name[20] = BACNET_VENDOR_TEMCO;
char bacnet_vendor_product[20] = BACNET_PRODUCT_TEMCO;
U16_T Bacnet_Vendor_ID = 148;

U8_T Get_AOx_by_index(uint8_t index,uint8_t *ao_index)
{
	U8_T i;
	
	int ret;
	
	ret = Get_Bacnet_Index_by_Number(OBJECT_ANALOG_OUTPUT,index);
	if( ret != -1)
	{
		*ao_index = ret;
		return 1;
	}
	else 
		return 0;

}

U8_T Get_BOx_by_index(uint8_t index,uint8_t *bo_index)
{
	U8_T i;
	int ret;
	
	ret = Get_Bacnet_Index_by_Number(OBJECT_BINARY_OUTPUT,index);
	if( ret != -1)
	{
		*bo_index = ret;
		return 1;
	}
	else 
		return 0;
	
}

U8_T Get_index_by_AVx(uint8_t av_index,uint8_t *var_index)
{
	U8_T i;
		
	int ret;
	ret = Get_Number_by_Bacnet_Index(OBJECT_ANALOG_VALUE,av_index);
	if(ret != -1)
	{
		*var_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}
}

U8_T Get_index_by_BVx(uint8_t bv_index,uint8_t *var_index)
{
	U8_T i;
		
	int ret;
	ret = Get_Number_by_Bacnet_Index(OBJECT_BINARY_VALUE,bv_index);
	if(ret != -1)
	{
		*var_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}
}

U8_T Get_index_by_AIx(uint8_t ai_index,uint8_t *in_index)
{
	U8_T i;
		
	int ret;
	
	ret = Get_Number_by_Bacnet_Index(OBJECT_ANALOG_INPUT,ai_index);
	if(ret != -1)
	{
		*in_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}
}

U8_T Get_index_by_BIx(uint8_t bi_index,uint8_t *in_index)
{
	U8_T i;
		
	int ret;
	
	ret = Get_Number_by_Bacnet_Index(OBJECT_BINARY_INPUT,bi_index);
	if(ret != -1)
	{
		*in_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}
}

U8_T Get_index_by_AOx(uint8_t ao_index,uint8_t *out_index)
{
	U8_T i;
		
	int ret;
	
	ret = Get_Number_by_Bacnet_Index(OBJECT_ANALOG_OUTPUT,ao_index);
	if(ret != -1)
	{
		*out_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}

}

U8_T Get_index_by_BOx(uint8_t do_index,uint8_t *out_index)
{
	U8_T i;
	int ret;
	
	ret = Get_Number_by_Bacnet_Index(OBJECT_BINARY_OUTPUT,do_index);
	if(ret != -1)
	{
		*out_index = ret;
		return 1;
	}
	else
	{
		return 0;
	}

}
// MSTP
void Set_TXEN(uint8_t dir)
{	
	// tbd:
}

char* get_description(uint8_t type,uint8_t num)
{
	U8_T io_index;	
	Str_points_ptr ptr;
#if BAC_TRENDLOG
	if(type == TRENDLOG)
	{
		if(num >= MAX_TREND_LOGS) return NULL;
		return (char*)"tendlog";
	}
#endif
	if(type == CALENDAR)
	{
		if(num >= MAX_CALENDARS) return NULL;
		return (char*)annual_routines[num].label;
	}
	if(type == SCHEDULE)
	{
		if(num >= MAX_SCHEDULES) return NULL;
		return (char*)weekly_routines[num].label;
	}
	if(type == AV)	
	{	
		Get_index_by_AVx(num,&io_index);
		if(io_index >= MAX_AVS) return NULL;
		ptr = put_io_buf(VAR,io_index);
		return (char*)ptr.pvar->label;
	}
	if(type == BV)	
	{	
		Get_index_by_BVx(num,&io_index);
		if(io_index >= MAX_BVS) return NULL;
		ptr = put_io_buf(VAR,io_index);
		return (char*)ptr.pvar->label;
	}
	if(type == AI)	
	{		
		Get_index_by_AIx(num,&io_index);
		if(io_index >= MAX_AIS) return NULL;
		ptr = put_io_buf(IN,io_index);
		return (char*)ptr.pin->label;
		
	}
	if(type == BI)	
	{		
		Get_index_by_BIx(num,&io_index);
		if(io_index >= MAX_AIS) return NULL;
		ptr = put_io_buf(IN,io_index);
		return (char*)ptr.pin->label;
	}
	if(type == AO)
	{		
		Get_index_by_AOx(num,&io_index);
		if(io_index >= MAX_AOS) return NULL;
		ptr = put_io_buf(OUT,io_index);
		return (char*)ptr.pout->label;
	}
	if(type == BO)
	{
		Get_index_by_BOx(num,&io_index);
		if(io_index >= MAX_BOS) return NULL;
		ptr = put_io_buf(OUT,io_index);
		return (char*)ptr.pout->label;
	}
	if(type == TEMCOAV)
	{
		if(Get_Mini_Type() == 9/*MINI_TSTAT10*/)
		{
			if(num == 0) return "panel number";
			/*else if(num == 1) return "dead master";
			else if(num == 2) return "lcd time off delay";
			else if(num == 3) return "disable icon";
			else if(num == 4) return "lcd display configure";
			else if(num == 5) return "icon configure";*/
			else if(num == 1) return "range_IN1";
			else if(num == 2) return "range_IN2";
			else if(num == 3) return "range_IN3";
			else if(num == 4) return "range_IN4";
			else if(num == 5) return "range_IN5";
			else if(num == 6) return "range_IN6";
			else if(num == 7) return "range_IN7";
			else if(num == 8) return "range_IN8";
			else return "reserved";
		}
		else
		{
			if(num == 0) return "device id";  // id
			if(num == 1) return "baudrate";
			if(num == 2) return "protocal";
			else return "reserved";
		}

	}

#if 1//ARM_TSTAT_WIFI
	if(type == MSV)
	{
		if(num == 0) return "FAN MODE";
		else return "reserved";
	}
#endif
	return NULL;
}

char* get_label(uint8_t type,uint8_t num)
{
	U8_T io_index;	
	Str_points_ptr ptr;
	if(type == SCHEDULE)
	{
		if(num >= MAX_SCHEDULES) return NULL;
		return (char*)weekly_routines[num].description;
	}
	if(type == CALENDAR)
	{
		if(num >= MAX_CALENDARS) return NULL;
		return (char*)annual_routines[num].description;
	}
	if(type == AV)	
	{
		Get_index_by_AVx(num,&io_index);		
		if(io_index >= MAX_AVS) return NULL;
		ptr = put_io_buf(VAR,io_index);
		return (char*)ptr.pvar->description;
	}
	if(type == BV)	
	{
		Get_index_by_BVx(num,&io_index);		
		if(io_index >= MAX_BVS) return NULL;
		ptr = put_io_buf(VAR,io_index);
		return (char*)ptr.pvar->description;
	}
	if(type == AI)
	{	
		Get_index_by_AIx(num,&io_index);		
		if(io_index >= MAX_AIS) return NULL;
		ptr = put_io_buf(IN,io_index);
		return (char*)ptr.pin->description;
	}
	if(type == BI)
	{		
		Get_index_by_BIx(num,&io_index);
		if(io_index >= MAX_AIS) return NULL;
		ptr = put_io_buf(IN,io_index);
		return (char*)ptr.pin->description;
	}
	if(type == AO)
	{		
		Get_index_by_AOx(num,&io_index);
		if(io_index >= MAX_AOS) return NULL;
		ptr = put_io_buf(OUT,io_index);
		return (char*)ptr.pout->description;
	}
	if(type == BO)	
	{
		Get_index_by_BOx(num,&io_index);
		if(io_index >= MAX_BOS) return NULL;
		ptr = put_io_buf(OUT,io_index);
		return (char*)ptr.pout->description;
	}
#if 1//ARM_TSTAT_WIFI
	if(type == MSV)
	{
		if(num == 0) return "FAN MODE TYPE";
		else return "reserved";
	}
#endif	
	
#if BAC_PROPRIETARY
	if(type == TEMCOAV)
	{

		if(Get_Mini_Type() == 9/*MINI_TSTAT10*/)
		{
			if(num == 0) return "panel number";
			/*else if(num == 1) return "dead master";
			else if(num == 2) return "lcd time off delay";
			else if(num == 3) return "disable icon";
			else if(num == 4) return "lcd display configure";
			else if(num == 5) return "icon configure";*/
			else if(num == 1) return "range_IN1";
			else if(num == 2) return "range_IN2";
			else if(num == 3) return "range_IN3";
			else if(num == 4) return "range_IN4";
			else if(num == 5) return "range_IN5";
			else if(num == 6) return "range_IN6";
			else if(num == 7) return "range_IN7";
			else if(num == 8) return "range_IN8";
			else return "reserved";
		}
		else
		{
			if(num == 0) return "device id";  // id
			if(num == 1) return "baudrate";
			if(num == 2) return "protocal";
			else return "reserved";
		}

	}
#endif
	return NULL;
}

// tbd: add more range
char get_range(uint8_t type,uint8_t num)
{
	uint8_t io_index;	
	Str_points_ptr ptr;
	if(type == AV)	
	{
		Get_index_by_AVx(num,&io_index);
		ptr = put_io_buf(VAR,io_index);

		if(io_index >= MAX_AVS) return 0;
// add unit
		if(ptr.pvar->range == not_used_input)
			return UNITS_NO_UNITS;  
		if(ptr.pvar->digital_analog == 0)  // digital
		{
			return UNITS_NO_UNITS;	
		}
		else  // analog
		{		
			if(ptr.pvar->range == degC)  				return UNITS_DEGREES_CELSIUS;
			else if(ptr.pvar->range == degF)  			return UNITS_DEGREES_FAHRENHEIT;
			
			else if(ptr.pvar->range == Volts) 			return UNITS_VOLTS;
			else if(ptr.pvar->range == KV) 				return UNITS_KILOVOLTS;

			else if(ptr.pvar->range == Amps) 			return UNITS_MILLIAMPERES;
			else if(ptr.pvar->range == ma) 				return UNITS_AMPERES;
			
			else if(ptr.pvar->range == Sec) 			return UNITS_SECONDS;
			else if(ptr.pvar->range == Min) 			return UNITS_MINUTES;
			else if(ptr.pvar->range == Hours) 			return UNITS_HOURS;
			else if(ptr.pvar->range == Days) 			return UNITS_DAYS;
			
			else if(ptr.pvar->range == Pa) 				return UNITS_PASCALS;
			else if(ptr.pvar->range == KPa) 			return UNITS_KILOPASCALS;
			
			else if(ptr.pvar->range == Watts) 			return UNITS_WATTS;
			else if(ptr.pvar->range == KW) 				return UNITS_KILOWATTS;
			
			else if(ptr.pvar->range == in_w) 			return UNITS_LUMENS;
			
			else if(ptr.pvar->range == RH) 				return UNITS_PERCENT_RELATIVE_HUMIDITY;
			else if(ptr.pvar->range == ppm) 			return UNITS_PARTS_PER_MILLION;
			else if(ptr.pvar->range == procent) 		return UNITS_PERCENT;
			else
				return UNITS_NO_UNITS;	
		}
	}
	if(type == AI)	
	{
		Get_index_by_AIx(num,&io_index);		
		ptr = put_io_buf(IN,io_index);

		if(io_index >= MAX_AIS) return 0;
		if(ptr.pin->digital_analog == 0)  // digital
		{
			return UNITS_NO_UNITS;	
		}
		else  // analog
		{		
			if((ptr.pin->range == R10K_40_120DegC) || (ptr.pin->range == KM10K_40_120DegC)) 
				return UNITS_DEGREES_CELSIUS;
			else if((ptr.pin->range == R10K_40_250DegF) || (ptr.pin->range == KM10K_40_250DegF)) 
				return UNITS_DEGREES_FAHRENHEIT;
			else if(ptr.pin->range == I0_20ma) 
				return UNITS_MILLIAMPERES;
			else if((ptr.pin->range == V0_10_IN) || (ptr.pin->range == V0_5)) 
				return UNITS_VOLTS;
			else
				return UNITS_NO_UNITS;	
		}
	}
	if(type == AO)
	{	
		Get_index_by_AOx(num,&io_index);
		ptr = put_io_buf(OUT,io_index);
		if(ptr.pout->range == 0)
			return UNITS_NO_UNITS;	
		else	
		{		
			if(ptr.pout->range == V0_10)			
				return UNITS_VOLTS;
			else if((ptr.pout->range == P0_100_Open) || 
				(ptr.pout->range == P0_100_Close) ||
				(ptr.pout->range == P0_100))			
				return UNITS_PERCENT;
			else if(ptr.pout->range == I_0_20ma)			
				return UNITS_MILLIAMPERES;
			else
				return UNITS_NO_UNITS;	
		}
	}
	if(type == BO)	
	{
		if(num >= MAX_BOS) return 0;
		return UNITS_NO_UNITS;	
	}
	if(type == BI)	
	{
		if(num >= MAX_BIS) return 0;
		return UNITS_NO_UNITS;	
	}
	return 0;
}


char Get_Out_Of_Service(uint8_t type,uint8_t num)
{	
	
	uint8_t io_index;
	Str_points_ptr ptr;
	if(type == AO)
	{
		Get_index_by_AOx(num,&io_index);
		ptr = put_io_buf(OUT,io_index);
		return ptr.pout->out_of_service;
	}
	else if(type == BO)	
	{
		Get_index_by_BOx(num,&io_index);
		ptr = put_io_buf(OUT,io_index);
		return ptr.pout->out_of_service;
	}		
	else if(type == SCHEDULE)	
	{
		if(num >= MAX_SCHEDULES) return 0;
		return weekly_routines[num].auto_manual;
	}
	else if(type == CALENDAR)	
	{
		if(num >= MAX_CALENDARS) return 0;
		return annual_routines[num].auto_manual;
	}
	else if(type == AV)	
	{
		Get_index_by_AVx(num,&io_index);
		ptr = put_io_buf(VAR,io_index);
		if(io_index >= MAX_AVS) return 0;
		return ptr.pvar->auto_manual;
	}
	else if(type == BV)	
	{
		Get_index_by_BVx(num,&io_index);
		ptr = put_io_buf(VAR,io_index);
		if(io_index >= MAX_BVS) return 0;
		return ptr.pvar->auto_manual;
	}
	else
		return 0;
}


// i is box, aox,avx ...
float Get_bacnet_value_from_buf(uint8_t type,uint8_t priority,uint8_t i)
{	
	uint8_t io_index;
	Str_points_ptr ptr;
	switch(type)
	{
		case AV:
			Get_index_by_AVx(i,&io_index);
			ptr = put_io_buf(VAR,io_index);
			if(Get_Mini_Type() == 15/*PROJECT_AIRLAB*/)
			{
				Get_AVS();
				return ptr.pvar->value;
			}
			
			if(ptr.pvar->range > 0)
			{
				if(ptr.pvar->digital_analog == 1)  // av
				{
					return (float)(ptr.pvar->value) / 1000;
				}
			}
			return 0;
			
		case BV:
			Get_index_by_BVx(i,&io_index);
			ptr = put_io_buf(VAR,io_index);
			if(ptr.pvar->range > 0)
			{
				if(ptr.pvar->digital_analog == 0)  // bv
				{
					if((ptr.pvar->range >= ON_OFF  && ptr.pvar->range <= HIGH_LOW)
					||(ptr.pvar->range >= custom_digital1 // customer digital unit
					&& ptr.pvar->range <= custom_digital8
					&& digi_units[ptr.pvar->range - custom_digital1].direct == 1))
					{  // inverse logic
						if(ptr.pvar->control == 1)
						{
							return 0;
						}
						else
						{
							return 1;
						}
					}	
					else
					{
						if(ptr.pvar->control == 1)
						{
							return 1;
						}
						else
						{
							return 0;
						}
					}	
				}
			}
			return 0;
		case BI:
			Get_index_by_BIx(i,&io_index);
			ptr = put_io_buf(IN,io_index);
			if(ptr.pin->range > 0)
			{
				if(ptr.pin->digital_analog == 0)  // digital
				{
					if((ptr.pin->range >= ON_OFF  && ptr.pin->range <= HIGH_LOW)
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
					{  // inverse logic
						if(ptr.pin->control == 1)
						{
							return 0;
						}
						else
						{
							return 1;
						}
					}	
					else
					{
						if(ptr.pin->control == 1)
						{
							return 1;
						}
						else
						{
							return 0;
						}
					}	
				}
			}
			else
				return input_raw[io_index];
		//break;
		case AI:
			Get_index_by_AIx(i,&io_index);
			ptr = put_io_buf(IN,io_index);
			if(ptr.pin->range > 0)
			{
				if(ptr.pin->digital_analog == 1)  // analog
					return (float) (ptr.pin->value) / 1000;
			}
			else
				return input_raw[io_index];
		//break;
		case AO:
		// find output index by AOx
		{
				uint8 temp; 
				Get_index_by_AOx(i,&io_index);
				ptr = put_io_buf(OUT,io_index);
				if(io_index >= max_dos + max_aos)
				{
					if(priority == 15)
						return (float) (ptr.pout->value) / 1000;
					else
						return 0xff;
				}
			
				if(output_priority[io_index][priority] == 0xff)
					return 0xff;
				
				if(ptr.pout->digital_analog == 0)
				{					
					temp = output_priority[io_index][priority] ? 1 : 0;
					if((ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW)
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
					{  // inverse logic
						
						if(temp == 1)
						{
							return 0;
						}
						else
						{
							return 1;
						}
					}	
					else
					{
						
						if(temp == 1)
						{
							return 1;
						}
						else
						{
							return 0;
						}
					}
				}
				else	
				{
					if(io_index >= get_max_internal_output())
						output_raw[io_index] = output_priority[io_index][priority] * 1000;
					return output_priority[io_index][priority];
				}

			}	
			
		case BO:	
		{
			uint8 temp; 
			// priority			
			Get_index_by_BOx(i,&io_index);
			ptr = put_io_buf(IN,io_index);
			if(io_index >= max_dos + max_aos)
			{
				if(priority == 15)
					return ptr.pout->value ? 1 : 0;
				else
					return 0xff;
			}
			if(output_priority[io_index][priority] == 0xff)
				return 0xff;	

			if(ptr.pout->digital_analog == 0)
			{  // digital
				temp = output_priority[io_index][priority] ? 1 : 0;
				if((ptr.pout->range >= ON_OFF  && ptr.pout->range <= HIGH_LOW)
				||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{  // inverse logic
					if(temp == 1)
					{
						return 0;
					}
					else
					{
						return 1;
					}
				}	
				else
				{
					if(temp == 1)
					{
						return 1;
					}
					else
					{
						return 0;
					}
				}
			}
			else
			{ // range is analog
				return output_priority[io_index][priority];
			}
		}
		case SCHEDULE:	
			return weekly_routines[i].value ? 1 : 0;
		//break;
		case CALENDAR:			
			return annual_routines[i].value ? 1 : 0;
		//break;	
//#if BAC_PROPRIETARY
		case TEMCOAV:
		{
			uint32 value = 0;
			if(Get_Mini_Type() == 9/*MINI_TSTAT10*/)
			{
				switch(i)
				{
					case 0: value = panel_number; 				
						break;				
/*					case 1:
						if(Modbus.dead_master & 0x80)  // bit7 is tha flag whether enable deadmaster
							value = Modbus.dead_master & 0x7f;
						else  // disable deadmaster
							value = 0;
						break;
					case 2:	value = Modbus.LCD_time_off_delay;					break;
					case 3:	value = Modbus.disable_tstat10_display;			break;
	//					case 4:		value = Modbus.display_lcd;		break;
					case 5:	value = Modbus.icon_config;				break;*/
					case 1: value = (inputs[0].digital_analog << 8) + inputs[0].range;					break;
					case 2: value = (inputs[1].digital_analog << 8) + inputs[1].range;					break;
					case 3: value = (inputs[2].digital_analog << 8) + inputs[2].range;					break;
					case 4: value = (inputs[3].digital_analog << 8) + inputs[3].range;					break;
					case 5: value = (inputs[4].digital_analog << 8) + inputs[4].range;					break;
					case 6: value = (inputs[5].digital_analog << 8) + inputs[5].range;					break;
					case 7: value = (inputs[6].digital_analog << 8) + inputs[6].range;					break;
					case 8: value = (inputs[7].digital_analog << 8) + inputs[7].range;					break;
					default:
						value = 0;
					break;
				}
			}
			else
			{
				value = get_TemcoAVS_airlab(i);
			}			
			return value;
		}
//#endif
		
#if ARM_TSTAT_WIFI
				case MSV:
				{
					switch(i)
					{
						case 0:
							return vars[7].value / 1000;
						default:
							break;

					}
				}
#else
//				case MSV:
//				{
//					switch(i)
//					{
//						case 0:
//							return msv_data[i][].status
//						default:
//							break;

//					}
//				}
				return 0;
#endif
				
		default:
			break;
				
	}	
	return 0;
}

char* Get_Object_Name(void)
{
	return panelname;
}

U8_T Get_WR_ON_OFF(uint8_t object_index,uint8_t day,uint8_t i)
{
	return wr_time_on_off[object_index][day][i];
}
//Wr_one_day					 		wr_times[MAX_WR][MAX_SCHEDULES_PER_WEEK];
// get time & value from weekly roution
BACNET_TIME_VALUE Get_Time_Value(uint8_t object_index,uint8_t day,uint8_t i)
{
	BACNET_TIME_VALUE  array;
	array.Time.hour = wr_times[object_index][day].time[i].hours;
	array.Time.min = wr_times[object_index][day].time[i].minutes;
	array.Time.sec = 0;
	array.Time.hundredths = 0;
	array.Value.type.Enumerated = wr_time_on_off[object_index][day][i];//(i + 1) % 2;
	array.Value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
	return array;

}

uint8_t Get_TV_count(uint8_t object_index,uint8_t day)
{
	uint8_t count;
	uint8_t i = 0;
	count = 0;
	for(i = 0;i < 8;i++)
	{
		if(wr_time_on_off[object_index][day][i] != 0xff)
			count++;
		//if((wr_times[object_index][day].time[i].hours == 0) && (wr_times[object_index][day].time[i].minutes == 0))
			//return i;
	}
	return count;
}




void Check_wr_time_on_off(uint8_t i,uint8_t j,uint8_t mode)
{
	U8_T k;
	U8_T k1,k2;
	U16_T tmp1,tmp2,tmp;
	U8_T tmp_onoff;
//	Time_on_off tmp;

	
	/*ChangeFlash = 1;
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)	
	write_page_en[WR_TIME] = 1;
	write_page_en[24] = 1;
#endif*/
	for(k = 0;k < 8;k++)
	{
		if(mode == 0)  // T3000 write weekly roution
		{
			if(wr_times[i][j].time[k].hours == 0 && wr_times[i][j].time[k].minutes == 0 )
				wr_time_on_off[i][j][k] = 0xff;
			else
				wr_time_on_off[i][j][k] = (k + 1) % 2;
		}
		
		if(wr_time_on_off[i][j][k] == 0xff)
		{
			if(wr_times[i][j].time[k].hours == 0 && wr_times[i][j].time[k].minutes == 0 )
			{
				if(k == 0)  // first one is 0
				{
					if(wr_times[i][j].time[k + 1].hours == 0 && wr_times[i][j].time[k + 1].minutes == 0	)
						wr_time_on_off[i][j][k] = 0xff;
					else
						wr_time_on_off[i][j][k] = 1;
				}
				else
					wr_time_on_off[i][j][k] = 0xff;
			}
			else
				wr_time_on_off[i][j][k] = (k + 1) % 2;
		}
		
	}
	
	// sort
	for(k1 = 0;k1 < 8;k1++)
	{
		if(wr_time_on_off[i][j][k1] == 0xff)
			continue;

		tmp1 = 60L * wr_times[i][j].time[k1].hours + wr_times[i][j].time[k1].minutes;

		for(k2 = k1 + 1;k2 < 8;k2++)
		{			
			if(wr_time_on_off[i][j][k2] == 0xff)
				continue;

			tmp2 = 60L *  wr_times[i][j].time[k2].hours + wr_times[i][j].time[k2].minutes;

			if(tmp1 > tmp2)
			{				
				tmp = tmp1;
				tmp1 = tmp2;
				tmp2 = tmp;
				
				wr_times[i][j].time[k1].hours = tmp1 / 60;
				wr_times[i][j].time[k1].minutes = tmp1 % 60;
				
				wr_times[i][j].time[k2].hours = tmp2 / 60;
				wr_times[i][j].time[k2].minutes = tmp2 % 60;
				
				
				tmp_onoff = wr_time_on_off[i][j][k1];
				wr_time_on_off[i][j][k1] = wr_time_on_off[i][j][k2]; 
				wr_time_on_off[i][j][k2] = tmp_onoff;
				
			}
		}
	}
	
	
}

void Check_All_WR(void)
{
	U8_T i,j;
	for(i = 0;i < MAX_WR;i++)
		for(j = 0;j < 9;j++)	
			Check_wr_time_on_off(i,j,1);
	
}

BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * Get_Object_Property_References(uint8_t i)
{
//	BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE obj;
//	memset(&obj,0,sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
//	obj.arrayIndex = -1;
//	
//	obj.objectIdentifier.instance = 0;
//	obj.objectIdentifier.type = 0;

//	obj.propertyIdentifier = 85;

////	obj.arrayIndex = 0x5678;

//	obj.deviceIndentifier.instance = 0;
//	obj.deviceIndentifier.type = 0;
//	return &obj;
	return NULL;//&obj;
}


uint8_t Get_CALENDAR_count(uint8_t object_index)
{
	uint16_t i,j;
	uint16_t count;
	count = 0;
	
	for(i = 0;i < AR_DATES_SIZE;i++)
	{
		for(j = 0;j < 8;j++)
		{
			if(ar_dates[object_index][i] & (0x01 << j)) 
			{
				count++;
			}
		}
	}
	
	return count;
}

void clear_calendar_data(uint8 index)
{
	memset(&ar_dates[index],0,AR_DATES_SIZE);
}

void get_bacnet_date_by_dayofyear(uint16_t dayofyear,BACNET_DATE * array)
{
	uint16_t  day;
	uint16_t  i;//,j,k;

	day = dayofyear + 1;
	
	if( ( Rtc.Clk.year & '\x03' ) == '\x0' )
		month_length[1] = 29;
	else
		month_length[1] = 28;
	
	for(i = 0;i < 12;i++)
	{
		if(day > month_length[i])
		{
			day -= month_length[i];
		}
		else
		{
			array->month = i + 1;
			array->day = day;
//			if(array->day == month_length[i] )
//			{
//				array->month++;
//				array->day = 1;
//			}
			i = 12;
		}
	}
	
	if(Rtc.Clk.day_of_year > dayofyear)
	{
		day = Rtc.Clk.week + 7 - (Rtc.Clk.day_of_year - dayofyear) % 7;
	}
	else
		day = Rtc.Clk.week + (dayofyear - Rtc.Clk.day_of_year) % 7;
	
	array->wday = day % 7 + 1;
	array->year = 2000 + Rtc.Clk.year;
	

	
}


BACNET_DATE Get_Calendar_Date(uint8_t object_index,uint8_t k)
{
	BACNET_DATE  array;
	
	uint16_t i,j;
	uint16_t dayofyear;
	uint16_t count;
	count = 0;


	for(i = 0;i < AR_DATES_SIZE;i++)
	{	
		for(j = 0;j < 8;j++)
		{
			if(ar_dates[object_index][i] & (0x01 << j)) 
			{
				if(k == count) // find current count
				{
					dayofyear = i * 8 + j;
					// map to BACNET_DATE
					get_bacnet_date_by_dayofyear(dayofyear,&array);
				}
				count++;	
			}
					
		}
	}
	return array;
}




void write_bacnet_unit_to_buf(uint8_t type,uint8_t priority,uint8_t i,uint8_t unit)
{
	U8_T temp;
	uint8_t io_index;
	Str_points_ptr ptr;

		switch(type)
		{
			case AI:
				Get_index_by_AIx(i,&io_index);
				if(io_index >=  MAX_AIS) break;
				ptr = put_io_buf(IN,io_index);
				if(unit == UNITS_NO_UNITS)
				{
					ptr.pin->range = not_used_input;			
				}
				if(unit == UNITS_DEGREES_CELSIUS)
				{
					ptr.pin->range = R10K_40_120DegC;
				}
				if(unit == UNITS_DEGREES_FAHRENHEIT)
				{
					ptr.pin->range = R10K_40_250DegF;	
				}				
				if(unit == UNITS_AMPERES)
				{
						ptr.pin->range = I0_20ma;
					// software jumper 
					temp = ptr.pin->decom;
					temp &= 0x0f;
					temp |= (INPUT_I0_20ma << 4);
					ptr.pin->decom = temp;
				}
				if(unit == UNITS_VOLTS)
				{
						ptr.pin->range = V0_10_IN;
					// software jumper 
					temp = ptr.pin->decom;
					temp &= 0x0f;
					temp |= (INPUT_0_10V << 4);
					ptr.pin->decom = temp;
				}
				
				if( (unit != UNITS_VOLTS) && (unit != UNITS_AMPERES) )
				{
						// software jumper 
					temp = ptr.pin->decom;
					temp &= 0x0f;
					temp |= (INPUT_THERM << 4);
					ptr.pin->decom = temp;
				}

				push_expansion_in_stack(ptr.pin);
				break;
			case BO:
				Get_index_by_BOx(i,&io_index);
				if(io_index >= MAX_BOS) break;
				ptr = put_io_buf(OUT,io_index);
				ptr.pout->digital_analog = 0;
				if(unit == UNITS_NO_UNITS)
					ptr.pout->range = 0;
				else
					ptr.pout->range = OFF_ON;

				push_expansion_out_stack(ptr.pout,io_index,1);

				break;
			case AO:
				Get_index_by_AOx(i,&io_index);

				if(io_index >= MAX_AOS) break;
				ptr = put_io_buf(OUT,io_index);
			
				ptr.pout->digital_analog = 1;
				if(unit == UNITS_NO_UNITS)
					ptr.pout->range = 0;
				else
					ptr.pout->range = V0_10;
		
				push_expansion_out_stack(ptr.pout,io_index,1);	
			
				break;
	
			default:
			break;
		}
}

void write_bacnet_description_to_buf(uint8_t type,uint8_t priority,uint8_t i,char* str)
{
	uint8_t io_index;
	Str_points_ptr ptr;

		switch(type)
		{
			case AI:
				Get_index_by_AIx(i,&io_index);
				if(io_index >= MAX_AIS) break;
				ptr = put_io_buf(IN,io_index);
				memcpy(ptr.pin->label,str,8);

				push_expansion_in_stack(ptr.pin);

				break;
			case BI:
				Get_index_by_BIx(i,&io_index);
				if(io_index >= MAX_AIS) break;
				ptr = put_io_buf(IN,io_index);
				memcpy(ptr.pin->label,str,8);

				push_expansion_in_stack(ptr.pin);

				break;
			case BO:
				Get_index_by_BOx(i,&io_index);
				if(io_index >= MAX_BOS) break;
				ptr = put_io_buf(OUT,io_index);
				memcpy(ptr.pout->label,str,8);

				push_expansion_out_stack(ptr.pout,io_index,1);

				break;
			case AO:
				Get_index_by_AOx(i,&io_index);
				if(io_index >= MAX_AOS) break;
				ptr = put_io_buf(OUT,io_index);
				memcpy(ptr.pout->label,str,8);

				push_expansion_out_stack(ptr.pout,io_index,1);

				break;
			case AV:
				Get_index_by_AVx(i,&io_index);
				if(io_index >= MAX_AVS) break;
				ptr = put_io_buf(VAR,io_index);
				memcpy(ptr.pvar->label,str,8);
				break;
			case BV:
				Get_index_by_BVx(i,&io_index);
				if(io_index >= MAX_BVS) break;
				ptr = put_io_buf(VAR,io_index);
				memcpy(ptr.pvar->label,str,8);
				break;
			case SCHEDULE:
				if(i >= MAX_SCHEDULES) break;

				memcpy(weekly_routines[i].label,str,8);
				break;
			case CALENDAR:
				if(i >= MAX_CALENDARS) break;

				memcpy(annual_routines[i].label,str,8);
				break;

			default:
			break;
		}
			
}


void wirte_bacnet_value_to_buf(uint8_t type,uint8_t priority,uint8_t i,float value)
{
	uint8_t io_index;
	Str_points_ptr ptr;

		switch(type)
		{
			case AV:
			Get_index_by_AVx(i,&io_index);
			ptr = put_io_buf(VAR,io_index);
			if(io_index >= MAX_AVS) break;
			if(ptr.pvar->digital_analog == 1)  // analog
			{	
				if(ptr.pvar->range == 0)
					ptr.pvar->range = R10K_40_120DegC;
				ptr.pvar->value =  (value * 1000) ;
			}
			ptr.pvar->value =  (value * 1000);
				break;
			case BV:
			Get_index_by_BVx(i,&io_index);			
			if(io_index >= MAX_BVS) break;
			ptr = put_io_buf(VAR,io_index);
			if(ptr.pvar->digital_analog == 0)  // digital
			{
				if(ptr.pvar->range == 0)
					ptr.pvar->range = OFF_ON;
				ptr.pvar->value =  (value * 1000);	

				if(( ptr.pvar->range >= ON_OFF && ptr.pvar->range <= HIGH_LOW )
				||(ptr.pvar->range >= custom_digital1 // customer digital unit
					&& ptr.pvar->range <= custom_digital8
					&& digi_units[ptr.pvar->range - custom_digital1].direct == 1))
				{ // inverse
					ptr.pvar->control = value ? 0 : 1;		
				}
				else
				{
					ptr.pvar->control = value ? 1 : 0;	
				}
			}

				break;
			case AI:
				Get_index_by_AIx(i,&io_index);
				if(io_index >= MAX_AIS) break;
				ptr = put_io_buf(IN,io_index);
				if(ptr.pin->digital_analog == 1)  // digital
				{	
					if(ptr.pin->range == 0)
						ptr.pin->range = R10K_40_120DegC;
					ptr.pin->value =  (value * 1000) ;
				}

				push_expansion_in_stack(ptr.pin);

				break;
				
			case BI:
				Get_index_by_BIx(i,&io_index);
				if(io_index >= MAX_AIS) break;
				ptr = put_io_buf(IN,io_index);
				if(ptr.pin->digital_analog == 0)  // digital
				{
					if(ptr.pin->range == 0)
						ptr.pin->range = OFF_ON;
					ptr.pin->value =  (value * 1000);	
//					inputs[i].control = value ? 1 : 0;	

					if(( ptr.pin->range >= ON_OFF && ptr.pin->range <= HIGH_LOW )
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
					{
						ptr.pin->control = value ? 0 : 1;		
					}	
					else
					{
						ptr.pin->control = value ? 1 : 0;							
					}	
				}
				else
				{	
					if(ptr.pin->range == 0)
						ptr.pin->range = R10K_40_120DegC;
					ptr.pin->value =  (value * 1000) ;
				}

				push_expansion_in_stack(ptr.pin);

				break;
			case BO:
				Get_index_by_BOx(i,&io_index);
				if(io_index >= MAX_BOS) break;
				ptr = put_io_buf(OUT,io_index);
				if(value == 0xff)	
					output_priority[io_index][priority] = 0xff;	
				else
				{
					if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
					{ // inverse
						if(io_index < max_dos + max_aos)
						{
							output_priority[io_index][priority] = value ? 0 : 1;
							ptr.pout->control = Binary_Output_Present_Value(io_index)/*value*/ ? 0 : 1;		
						}
						else
						{
							ptr.pout->control = value ? 0 : 1;
							output_priority[io_index][15] = value ? 0 : 1;
							if(priority != 15)
								output_priority[io_index][priority] = 0xff;							
						}						
					}
					else
					{						
						if(io_index < max_dos + max_aos)
						{
							output_priority[io_index][priority] = value ? 1 : 0;						
							ptr.pout->control = Binary_Output_Present_Value(io_index)/*value*/ ? 1 : 0;	
						}
						else
						{
							ptr.pout->control = value ? 1 : 0;	
							if(priority != 15)
								output_priority[io_index][priority] = 0xff;
							output_priority[io_index][15] = value ? 1 : 0;
						}					
					}	
			
				}
				
				ptr.pout->digital_analog = 0;
				check_output_priority_array_without_AM(io_index);
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif				
				if(io_index >= max_dos + max_aos)
				{
					ptr.pout->auto_manual = 1;
				}

				if( ptr.pout->range == 0)
					ptr.pout->range = OFF_ON;			
					
				push_expansion_out_stack(ptr.pout,io_index,0);

				break;
			case AO:	
				Get_index_by_AOx(i,&io_index);
				if(io_index >= MAX_AOS) break;
				ptr = put_io_buf(OUT,io_index);
				
				if(ptr.pout->range == 0)
					ptr.pout->range = V0_10;
				ptr.pout->digital_analog = 1;

				if(io_index < max_aos + max_dos)
				{
					output_priority[io_index][priority] = value;	
					ptr.pout->value =  (Analog_Output_Present_Value(io_index) * 1000);// (value * 1000);
				}

				if(io_index >= max_dos + max_aos)
				{
					ptr.pout->auto_manual = 1;
					output_priority[io_index][15] = value;
					if(priority != 15)
						output_priority[io_index][priority] = 0xff;
					ptr.pout->value =  (value * 1000);
					//outputs[out_index].value =  (Analog_Output_Present_Value(out_index) * 1000);// (value * 1000);
					output_raw[io_index] =  (value * 1000);
				}
//				output_priority[out_index][priority] = value;	
//				outputs[out_index].value =  (Analog_Output_Present_Value(out_index) * 1000);// (value * 1000);
				check_output_priority_array_without_AM(io_index);
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif
				

				push_expansion_out_stack(ptr.pout,io_index,0);

				break;
		 case SCHEDULE:
				//if(weekly_routines[i].auto_manual == 1)
				{
					weekly_routines[i].value = (value * 1000);
				}
				break;
			case CALENDAR:

				//if(annual_routines[i].auto_manual == 1)
				{
					annual_routines[i].value = (value * 1000);
				}
				break;

			case TEMCOAV:
			if(Get_Mini_Type() == 9/*MINI_TSTAT10*/)
			{
				/*if(i == 1)
				{
					//Modbus.dead_master = value;				
					//E2prom_Write_Byte(EEP_DEAD_MASTER,Modbus.dead_master);
					//clear_dead_master();
				}
				else if(i == 2)
				{
					//Modbus.LCD_time_off_delay = value;				
					//E2prom_Write_Byte(EEP_LCD_TIME_OFF_DELAY,value);
					//count_lcd_time_off_delay = 0;
				}
				else if(i == 3)
				{
					//Modbus.disable_tstat10_display = value;
					//E2prom_Write_Byte(EEP_DISABLE_T10_DIS,value);
				}
				else if(i == 5)
				{
					save_icon_config(value);
				}
				else*/ if(i >= 1 && i<= 8)
				{				
					inputs[i - 1].digital_analog = (U16_T)value >> 8;
					inputs[i - 1].range = (U8_T)value;
					//write_page_en[IN] = 1;	
					//ChangeFlash = 1;
					save_point_info(0);
				}
			}
			else
			{
				save_TemcoAV_AIRALB(i,  value);
				
			}
			break;


			default:
			break;
		}			
}


void write_Out_Of_Service(uint8_t type,uint8_t i,uint8_t am)
{
	uint8_t io_index;
	Str_points_ptr ptr;

	if(type == BO)
	{
		Get_index_by_BOx(i,&io_index);
		ptr = put_io_buf(OUT,io_index);
		ptr.pout->auto_manual = am;

		push_expansion_out_stack(ptr.pout,io_index,1);

	}
	if(type == AO)
	{
		Get_index_by_AOx(i,&io_index);
		ptr = put_io_buf(OUT,io_index);
		ptr.pout->auto_manual = am;

		push_expansion_out_stack(ptr.pout,io_index,1);

	}
	if(type == BV)
	{
		Get_index_by_BVx(i,&io_index);
		ptr = put_io_buf(VAR,io_index);
		ptr.pvar->auto_manual = am;
	}
	if(type == AV)
	{
		Get_index_by_AVx(i,&io_index);
		ptr = put_io_buf(VAR,io_index);
		ptr.pvar->auto_manual = am;
	}
}

void write_bacnet_name_to_buf(uint8_t type,uint8_t priority,uint8_t i,char* str)
{
	uint8_t io_index;
	Str_points_ptr ptr;

	switch(type)
	{
		case AI:
			Get_index_by_AIx(i,&io_index);
			if(io_index >= MAX_AVS) break;
			ptr = put_io_buf(IN,io_index);
			memcpy(ptr.pin->description,str,21);
			push_expansion_in_stack(ptr.pin);
			break;
		case BI:
			Get_index_by_BIx(i,&io_index);
			if(io_index >= MAX_AVS) break;
			ptr = put_io_buf(IN,io_index);
			memcpy(ptr.pin->description,str,21);
			push_expansion_in_stack(ptr.pin);
			break;
		case BO:
			Get_index_by_BOx(i,&io_index);
			if(io_index >= MAX_BOS) break;
			ptr = put_io_buf(OUT,io_index);
			memcpy(ptr.pout->description,str,19);
			push_expansion_out_stack(ptr.pout,io_index,1);
			break;
		case AO:
			Get_index_by_AOx(i,&io_index);
			if(io_index >= MAX_AOS) break;
			ptr = put_io_buf(OUT,io_index);
			memcpy(ptr.pout->description,str,19);
			push_expansion_out_stack(ptr.pout,io_index,1);

			break;
		case AV:
			Get_index_by_AVx(i,&io_index);
			if(io_index >= MAX_AVS) break;
			ptr = put_io_buf(VAR,io_index);
			memcpy(ptr.pvar->description,str,21);
			break;
		case BV:
			Get_index_by_BVx(i,&io_index);
			if(io_index >= MAX_AVS) break;
			ptr = put_io_buf(VAR,io_index);
			memcpy(ptr.pvar->description,str,21);
			break;
		case SCHEDULE:
			if(i >= MAX_SCHEDULES) break;

			memcpy(weekly_routines[i].description,str,21);
			break;
		case CALENDAR:
			if(i >= MAX_CALENDARS) break;

			memcpy(annual_routines[i].description,str,21);
			break;
		default:
		break;
	}

}





void Clear_Time_Value(uint8_t index,uint8_t day)
{
	char i = 0;
	for(i = 0;i < 8;i++)
	{
	/*	wr_times[index][day].time[i].hours = 0;
		wr_times[index][day].time[i].minutes = 0;
		wr_time_on_off[index][day][i] = 0xff;*/
	}
}



void write_Time_Value(uint8_t index,uint8_t day,uint8_t i,uint8_t hour,uint8_t min/*BACNET_TIME_VALUE time_value*/,uint8_t value)
{	
	/*ChangeFlash = 1;
	//write_page_en[WR_TIME] = 1;*/
	wr_times[index][day].time[i].hours = hour;//time_value.Time.hour;
	wr_times[index][day].time[i].minutes = min;//time_value.Time.min;
	wr_time_on_off[index][day][i] = value;	
}



void write_annual_date(uint8_t index,BACNET_DATE date)
{
	uint16_t day = 0;
	uint8_t j = 0;

	//ChangeFlash = 1;

	if(date.year != 2000 + Rtc.Clk.year) return;
	
	if( ( Rtc.Clk.year & '\x03' ) == '\x0' )
		month_length[1] = 29;
	else
		month_length[1] = 28;
	
	day = 0;
	if(date.month >= 1 && date.month <= 12)
	{
		for(j = 0;j < date.month - 1;j++)
			day += month_length[j];
	}
	day += date.day;
	ar_dates[index][(day - 1) / 8] |= (0x01 << ((day - 1) % 8));

}


#endif


U16_T Get_Vendor_ID(void)
{
	/*switch(Bacnet_Vendor_ID)
	{// ����0 1 2 255 65535 �⼸������id�������ϵ�����
		case 1: //netixcontrols
		case BACNET_VENDOR_ID_NETIX:
			memcpy(bacnet_vendor_name,BACNET_VENDOR_NETIX,20);
			memcpy(bacnet_vendor_product,BACNET_PRODUCT_NETIX,20);
//				bacnet_vendor_name = BACNET_VENDOR_NETIX;
//				bacnet_vendor_product = BACNET_PRODUCT_NETIX;
				return BACNET_VENDOR_ID_NETIX;
		
		case 2: // jet controls
		case BACNET_VENDOR_ID_JET:
//				bacnet_vendor_name = BACNET_VENDOR_JET;
//				bacnet_vendor_product = BACNET_PRODUCT_JET;
		memcpy(bacnet_vendor_name,BACNET_VENDOR_JET,20);
		memcpy(bacnet_vendor_product,BACNET_PRODUCT_JET,20);
				return BACNET_VENDOR_ID_JET;
		case 3: // NEWRON
		case BACNET_VENDOR_ID_NEWRON:
//				bacnet_vendor_name = BACNET_VENDOR_JET;
//				bacnet_vendor_product = BACNET_PRODUCT_JET;
		memcpy(bacnet_vendor_name,BACNET_VENDOR_NEWRON,20);
		memcpy(bacnet_vendor_product,BACNET_PRODUCT_NEWRON,20);
		return BACNET_VENDOR_ID_JET;
		
		case BACNET_VENDOR_ID_TEMCO:
		case 255:
		case 65535:
		case 0:
//			bacnet_vendor_name = BACNET_VENDOR_TEMCO;
//			bacnet_vendor_product = BACNET_PRODUCT_TEMCO;
		memcpy(bacnet_vendor_name,BACNET_VENDOR_TEMCO,20);
		memcpy(bacnet_vendor_product,BACNET_PRODUCT_TEMCO,20);
			return BACNET_VENDOR_ID_TEMCO;
		
		default:
//			bacnet_vendor_name = BACNET_VENDOR_TEMCO;
//			bacnet_vendor_product = BACNET_PRODUCT_TEMCO;
			return Bacnet_Vendor_ID;
	 }*/
	 return Bacnet_Vendor_ID;
}



const char*  Get_Vendor_Name(void)
{
	return bacnet_vendor_name;
}

const char*  Get_Vendor_Product(void)
{

	return bacnet_vendor_product;
}

// T3-IO�����м�����ĺ������ÿͻ��Լ��༭��T3-Controller��û�м�
void Set_Vendor_Name(char* name)
{
	//write_page_en[25] = 1;
	memcpy(bacnet_vendor_name,name,20);
	//Flash_Write_Mass();
}

void Set_Vendor_Product(char* product)
{
	//write_page_en[25] = 1;
	memcpy(bacnet_vendor_product,product,20);
	//Flash_Write_Mass();
}

void Set_Vendor_ID(uint16_t vendor_id)
{
//	if(vendor_id == 1)
//	{
//		//Bacnet_Vendor_ID = BACNET_VENDOR_ID_NETIX;
//	}
//	if(vendor_id == 2)
//	{
//		//Bacnet_Vendor_ID = 
//	}
	//AT24CXX_WriteOneByte(EEP_BAC_VENDOR_ID_LO ,vendor_id);
	//AT24CXX_WriteOneByte(EEP_BAC_VENDOR_ID_HI ,vendor_id >> 8);
	if((vendor_id == 0) || (vendor_id == 255) || (vendor_id == 65535) // temco
		|| (vendor_id == 1)  // netIX
		|| (vendor_id == 2))  // JET
	{
		//Bacnet_Vendor_ID = 148;		
		return;
	}
	else
		Bacnet_Vendor_ID = vendor_id;
	
}

void Set_Daylight_Saving_Status(bool status)
{
	Rtc.Clk.is_dst = status;
}
	
bool Get_Daylight_Savings_Status(void)
{
	return Rtc.Clk.is_dst;
}




#if 1//BAC_TIMESYNC
bool write_Local_Date(BACNET_DATE* array)
{
	memcpy(&Local_Date,array,sizeof(BACNET_DATE));
	Rtc.Clk.day = array->day;
	Rtc.Clk.mon = array->month;
	Rtc.Clk.week = array->wday;
	Rtc.Clk.year = (array->year - 60) & 0xff;
#if (ASIX_MINI || ASIX_CM5)
	Rtc_Set(Rtc.Clk.year,Rtc.Clk.mon,Rtc.Clk.day,Rtc.Clk.hour,Rtc.Clk.min,Rtc.Clk.sec,0);
	//Updata_Clock(0);
#endif
	
	Local_Date.year = Rtc.Clk.year + 2000;

	flag_Updata_Clock = 1;
	return 1;

}


bool write_Local_Time(BACNET_TIME* array)
{
	memcpy(&Local_Time,array,sizeof(BACNET_TIME));
	Rtc.Clk.hour = array->hour;
	Rtc.Clk.min = array->min;
	Rtc.Clk.sec = array->sec;
#if (ASIX_MINI || ASIX_CM5)
	Rtc_Set(Rtc.Clk.year,Rtc.Clk.mon,Rtc.Clk.day,Rtc.Clk.hour,Rtc.Clk.min,Rtc.Clk.sec,0);
#endif


	flag_Updata_Clock = 1;
	return 1;
}


#endif

S16_T Get_UTC_Offset(void)
{
	return timezone;
}


void Set_UTC_OFFset(void/*S16_T tz*/)  //??????????????????
{
//	timezone = tz;
}


void Send_TimeSync_Broadcast(uint8_t protocal)
{
	BACNET_DATE bdate;
	BACNET_TIME btime;
	
	memcpy(&bdate,/*Get_Local_Date()*/&Local_Date,sizeof(BACNET_DATE));
	memcpy(&btime,/*Get_Local_Time()*/&Local_Time,sizeof(BACNET_TIME));


  if(protocal == BAC_IP_CLIENT)
	{
		Set_broadcast_bip_address(0xffffffff);
		bip_set_broadcast_addr(0xffffffff);
		Send_TimeSync(&bdate,&btime,protocal);
	}
	else if(protocal == BAC_MSTP)
	{
		Send_Time_Sync = 1;
	}

}

//void store_output_relinguish(U8_T i, U32_T value)
//{
//	AT24CXX_WriteOneByte(EEP_OUTPUT_RELINQUISH_START + i * 4,(U32_T)value >> 24);
//	AT24CXX_WriteOneByte(EEP_OUTPUT_RELINQUISH_START + i * 4 + 1,(U32_T)value >> 16);
//	AT24CXX_WriteOneByte(EEP_OUTPUT_RELINQUISH_START + i * 4 + 2,(U32_T)value >> 8);
//	AT24CXX_WriteOneByte(EEP_OUTPUT_RELINQUISH_START + i * 4 + 3,(U8_T)value);
//}


void write_Output_Relinguish(uint8_t type,uint8_t i,float value)
{
	Str_points_ptr ptr;
	ptr = put_io_buf(OUT,i);
	if(type == BO)
	{
		if(i < max_dos)
		{			
			output_relinquish[i] = value;	
			if(ptr.pout->digital_analog == 0)
			{				
				if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{// inverse
					output_relinquish[i] = value ? 0 : 1;			
					ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;	
				}		
				else
				{
					output_relinquish[i] = value ? 1 : 0;	
					ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;	
				}
				
				if(ptr.pout->control) 
					set_output_raw(i,1000);
				else 
					set_output_raw(i,0);
			}
			else
				output_relinquish[i] = value;
			
			check_output_priority_array(i,0);
		}
	}
	else if(type == AO)
	{
		output_relinquish[i + max_dos] = value;
		output_raw[i + max_dos] = value;
		// set output_raw
		check_output_priority_array(i + max_dos,0);
	}
}

float Get_Output_Relinguish(uint8_t type,uint8_t i)
{
	if(type == BO)
	{
		if(i < max_dos)
			return (float)output_relinquish[i];		
		else
			return 0;
	}
	else
		return (float)output_relinquish[i + max_dos];
}
#if BAC_TRENDLOG
void adjust_trend_log(void)
{
	int i;
	Str_points_ptr ptr;
	TRENDLOGS = 0;

	for(i = 0;i < base_in;i++)
	{
		ptr = put_io_buf(IN,i);
		if(ptr.pin->digital_analog == 1)
		{			
			add_Trend_Log(OBJECT_ANALOG_INPUT,i + 1);
		}
		else
		{
			add_Trend_Log(OBJECT_BINARY_INPUT,i + 1);
		}
	}
	
	for(i = 0;i < base_out;i++)
	{
		ptr = put_io_buf(OUT,i);
		if(ptr.pout->digital_analog == 1)
		{			
			add_Trend_Log(OBJECT_ANALOG_OUTPUT,i + 1);
		}
		else
		{
			add_Trend_Log(OBJECT_ANALOG_OUTPUT,i + 1);
		}
	}
	
	for(i = 0;i < MAX_VARS;i++)
	{
		ptr = put_io_buf(VAR,i);
		if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 1))
		{
			add_Trend_Log(OBJECT_ANALOG_VALUE,i + 1);
		}
		if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 0))
		{			
			add_Trend_Log(OBJECT_BINARY_VALUE,i + 1);
		}
	}

}
#endif
uint8_t AI_Index_To_Instance[MAX_AIS];
uint8_t BI_Index_To_Instance[MAX_BIS];
uint8_t AO_Index_To_Instance[MAX_AOS];
uint8_t BO_Index_To_Instance[MAX_AOS];
uint8_t AV_Index_To_Instance[MAX_AVS];
uint8_t BV_Index_To_Instance[MAX_AVS];

uint8_t AI_Instance_To_Index[MAX_AIS];
uint8_t BI_Instance_To_Index[MAX_BIS];
uint8_t AO_Instance_To_Index[MAX_AOS];
uint8_t BO_Instance_To_Index[MAX_AOS];
uint8_t AV_Instance_To_Index[MAX_AVS];
uint8_t BV_Instance_To_Index[MAX_AVS];

void Count_IN_Object_Number(void)
{
	U8_T count1,count2,i;
	Str_points_ptr ptr;
	count1 = 0;
	count2 = 0;
	for(i = 0;i < base_in;i++)
	{
		ptr = put_io_buf(IN,i);
		if(ptr.pin->digital_analog == 1)
		{
			AI_Index_To_Instance[count1] = i;
			AI_Instance_To_Index[i] = count1;
			count1++;
			
		}
		else
		{
			BI_Index_To_Instance[count2] = i;
			BI_Instance_To_Index[i] = count2;
			count2++;
			
		}
	}
	AIS = count1;
	BIS = count2;
#if BAC_TRENDLOG
	adjust_trend_log();  // adjust trend log if inputs are changed
#endif
}


void Count_OUT_Object_Number(void)
{
	U8_T count1,count2,i;
	Str_points_ptr ptr;
	count1 = 0;
	count2 = 0;
	
	for(i = 0;i < base_out;i++)
	{
		ptr = put_io_buf(OUT,i);
		if(ptr.pout->digital_analog == 1)
		{
			AO_Index_To_Instance[count1] = i;
			AO_Instance_To_Index[i] = count1;
			count1++;
			
		}
		else
		{
			BO_Index_To_Instance[count2] = i;
			BO_Instance_To_Index[i] = count2;
			count2++;
			
		}
	}
	AOS = count1;
	BOS = count2;
#if BAC_TRENDLOG
	adjust_trend_log(); // adjust trend log if outputs are changed
#endif
}



void Count_VAR_Object_Number(uint8_t base_var)
{
	U8_T count1,count2,i;
	Str_points_ptr ptr;
	count1 = 0;
	count2 = 0;

	for(i = 0;i < max_vars;i++)
	{
		ptr = put_io_buf(VAR,i);
		if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 1))
		{
			AV_Index_To_Instance[count1] = i;
			AV_Instance_To_Index[i] = count1;
			count1++;			
		}
		if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 0))
		{
			BV_Index_To_Instance[count2] = i;
			BV_Instance_To_Index[i] = count2;
			count2++;			
		}
	}
	AVS = count1;
	BVS = count2;
#if BAC_TRENDLOG
	adjust_trend_log(); // adjust trend log if variables are changed
#endif
//#if ARM_TSTAT_WIFI 
//	
//	AVS = 20;
//	BVS = 0;
//	for(i = 0;i < AVS;i++)
//	{
//		AV_Index_To_Instance[i] = i;
//		AV_Instance_To_Index[i] = i;
//	}
//#endif
}


int Get_Bacnet_Index_by_Number(U8_T type,U8_T number)
{
	int count,i;
	Str_points_ptr ptr;
	count = 0;
	switch(type)
	{
		case OBJECT_ANALOG_INPUT:
			for(i = 0;i < base_in;i++)
			{
				ptr = put_io_buf(IN,i);
				if(ptr.pin->digital_analog == 1)
				{					
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_INPUT:
			for(i = 0;i < base_in;i++)
			{
				ptr = put_io_buf(IN,i);
				if(ptr.pin->digital_analog == 0)
				{
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_OUTPUT:
			for(i = 0;i < base_out;i++)
			{
				ptr = put_io_buf(OUT,i);
				if(ptr.pout->digital_analog == 0)
				{
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		case OBJECT_ANALOG_OUTPUT:
			for(i = 0;i < base_out;i++)
			{
				ptr = put_io_buf(OUT,i);
				if(ptr.pout->digital_analog == 1)
				{
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		case OBJECT_ANALOG_VALUE:
			for(i = 0;i < MAX_VARS;i++)
			{
				ptr = put_io_buf(VAR,i);
				if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 1))
				{					
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_VALUE:
			for(i = 0;i < MAX_INS;i++)
			{
				ptr = put_io_buf(VAR,i);
				if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 0))
				{
					if(i == number)
						return count;
					count++;
				}
			}
			break;
		default:
			break;
	}	
	
	return -1;
}

int Get_Number_by_Bacnet_Index(U8_T type,U8_T index)
{
	int count,i;
	Str_points_ptr ptr;
	count = 0;
	switch(type)
	{
		case OBJECT_ANALOG_INPUT:
			for(i = 0;i < base_in;i++)
			{
				ptr = put_io_buf(IN,i);
				if(ptr.pin->digital_analog == 1)
				{
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_INPUT:
			for(i = 0;i < base_in;i++)
			{
				ptr = put_io_buf(IN,i);
				if(ptr.pin->digital_analog == 0)
				{					
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_OUTPUT:
			for(i = 0;i < base_out;i++)
			{
				ptr = put_io_buf(OUT,i);
				if(ptr.pout->digital_analog == 0)
				{					
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		case OBJECT_ANALOG_OUTPUT:
			for(i = 0;i < base_out;i++)
			{
				ptr = put_io_buf(OUT,i);
				if(ptr.pout->digital_analog == 1)
				{					
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		case OBJECT_ANALOG_VALUE:
			for(i = 0;i < MAX_VARS;i++)
			{
				ptr = put_io_buf(VAR,i);
				if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 1))
				{
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		case OBJECT_BINARY_VALUE:
			for(i = 0;i < MAX_VARS;i++)
			{
				ptr = put_io_buf(VAR,i);
				if((ptr.pvar->range != 0) &&(ptr.pvar->digital_analog == 0))
				{					
					if(count == index)
						return i;
					count++;
				}
			}
			break;
		default:
			break;
	}
		
	return -1;
}


BACNET_POLARITY Binary_Input_Polarity(
    uint32_t object_instance)
{
	uint8_t index;
	Str_points_ptr ptr;
	index = BI_Index_To_Instance[object_instance];
	ptr = put_io_buf(IN,index);
	if (index < MAX_BIS) {
			
		if(ptr.pin->range >= OFF_ON && ptr.pin->range <= LOW_HIGH)
			return POLARITY_NORMAL;
		else
			return POLARITY_REVERSE;	
		
	}
	
	return POLARITY_NORMAL;
}

bool Binary_Input_Polarity_Set(
    uint32_t object_instance,
    BACNET_POLARITY polarity)
{
		bool status = false;
		//uint8_t index;
		Str_points_ptr ptr;
		//index = BI_Instance_To_Index[object_instance];

		ptr = put_io_buf(IN,object_instance - OBJECT_BASE);
		if(object_instance < MAX_BIS) 
		{
			if(polarity == POLARITY_NORMAL)
			{
				if(ptr.pin->range >= ON_OFF && ptr.pin->range <= HIGH_LOW)
				{ 
					ptr.pin->range = ptr.pin->range - 11;
					return true;
				}
			}
			else 
			{	
				if(ptr.pin->range >= OFF_ON && ptr.pin->range <= LOW_HIGH)
				{
					ptr.pin->range = ptr.pin->range + 11;
					return true;
				}
			}
		}

		return status;
}



BACNET_POLARITY Binary_Output_Polarity(
    uint32_t object_instance)
{
	uint8_t index;
	Str_points_ptr ptr;
	index = BO_Index_To_Instance[object_instance];
	ptr = put_io_buf(OUT,index);
	if (index < MAX_BOS) {
			
		if(ptr.pout->range >= OFF_ON && ptr.pout->range <= LOW_HIGH)
			return POLARITY_NORMAL;
		else
			return POLARITY_REVERSE;	
		
	}
	return POLARITY_NORMAL;
}

bool Binary_Output_Polarity_Set(
    uint32_t object_instance,
    BACNET_POLARITY polarity)
{
		bool status = false;
		//uint8_t index;
		Str_points_ptr ptr;
		//index = BO_Instance_To_Index[object_instance];

		ptr = put_io_buf(OUT,object_instance - OBJECT_BASE);
		if(object_instance < MAX_BOS) 
		{
			if(polarity == POLARITY_NORMAL)
			{
				if(ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW)
				{ 
					ptr.pout->range = ptr.pout->range - 11;
					return true;
				}
			}
			else 
			{	
				if(ptr.pout->range >= OFF_ON && ptr.pout->range <= LOW_HIGH)
				{
					ptr.pout->range = ptr.pout->range + 11;
					return true;
				}
			}
		}

		return status;
}


BACNET_POLARITY Binary_Value_Polarity(
    uint32_t object_instance)
{
	uint8_t index;
	Str_points_ptr ptr;
	index = BV_Index_To_Instance[object_instance];
	ptr = put_io_buf(VAR,index);
	if (index < MAX_BVS) {
			
		if(ptr.pvar->range >= OFF_ON && ptr.pvar->range <= LOW_HIGH)
			return POLARITY_NORMAL;
		else
			return POLARITY_REVERSE;	
		
	}
	return POLARITY_NORMAL;
}

bool Binary_Value_Polarity_Set(
    uint32_t object_instance,
    BACNET_POLARITY polarity)
{
		bool status = false;
		//uint8_t index;
		Str_points_ptr ptr;
		//index = BV_Instance_To_Index[object_instance];

		ptr = put_io_buf(VAR,object_instance - OBJECT_BASE);
		if(object_instance < MAX_BVS) 
		{
			if(polarity == POLARITY_NORMAL)
			{
				if(ptr.pvar->range >= ON_OFF && ptr.pvar->range <= HIGH_LOW)
				{ 
					ptr.pvar->range = ptr.pvar->range - 11;
					return true;
				}
			}
			else 
			{	
				if(ptr.pvar->range >= OFF_ON && ptr.pvar->range <= LOW_HIGH)
				{
					ptr.pvar->range = ptr.pvar->range + 11;
					return true;
				}
			}
		}

		return status;
}



int32_t backup_IN_value[MAX_INS];
bool Analog_Input_Change_Of_Value(unsigned int object_instance)
{
	unsigned object_index;
	unsigned int err = 0;
	object_index = AI_Instance_To_Index[object_instance];

	// 检查当前值与备份值的差值是否合理
	// 如果range是温湿度或者电压电流，0.5的改变需要报告change
	if(inputs[object_index].range <= 49 || inputs[object_index].range == 57 ) // temprature or humdity
		err = 500; // 0.5
	else if(inputs[object_index].range == 58) // co2
		err = 10000;
	else
		err = 5000;
	
    if (abs(inputs[object_index].value - backup_IN_value[object_index]) > err) {
        // 更新备份值
        backup_IN_value[object_index] = inputs[object_index].value;
        return true; // 值变化超过 10
    } else {
        return false; // 值变化未超过 10
    }
}
bool Binary_Input_Change_Of_Value(uint32_t object_instance)
{
	unsigned object_index;
	object_index = BI_Instance_To_Index[object_instance];
			
	if(inputs[object_index].control != backup_IN_value[object_index])
	{
		backup_IN_value[object_index] = inputs[object_index].control;
		return true;
	}
	else
	{
		return false;
	}
}

int32_t backup_VAR_value[MAX_VARS];
bool Analog_Value_Change_Of_Value(unsigned int object_instance)
{
	unsigned object_index;
	bool status = false;
	object_index = AV_Instance_To_Index[object_instance];

	if(vars[object_index].value != backup_VAR_value[object_index])
	{	
		backup_VAR_value[object_index] = vars[object_index].value;
		status = true;
	}
	else
	{	
		status = false;
	}
	
	return status;
}

bool Binary_Value_Change_Of_Value(unsigned int object_instance)
{
	unsigned object_index;
	bool status = false;
	object_index = BV_Instance_To_Index[object_instance];

	if(vars[object_index].control != backup_VAR_value[object_index])
	{	
		backup_VAR_value[object_index] = vars[object_index].control;
		status = true;
	}
	else
	{	
		status = false;
	}
	
	return status;
}


int32_t backup_OUT_value[MAX_OUTS];
bool Analog_Output_Change_Of_Value(unsigned int object_instance)
{
	unsigned object_index;
	object_index = AO_Instance_To_Index[object_instance];
			
	if(outputs[object_index].value != backup_OUT_value[object_index])
	{
		backup_OUT_value[object_index] = outputs[object_index].value;
		return true;
	}
	else
	{
		return false;
	}
}
bool Binary_Output_Change_Of_Value(unsigned int object_instance)
{
	unsigned object_index;
	object_index = BO_Instance_To_Index[object_instance];
			
	if(outputs[object_index].control != backup_OUT_value[object_index])
	{
		backup_OUT_value[object_index] = outputs[object_index].control;
		return true;
	}
	else
	{
		return false;
	}
}




// tbd:
//void Set_Vendor_ID(uint16_t vendor_id)
//{}
//void Set_Vendor_Name(char* name)
//{}
//void Set_Vendor_Product(char* product)
//{}




uint8_t flag_suspend_mstp;
uint16_t count_suspend_mstp;

void check_whether_suspend_mstp(void)
{
	if(flag_suspend_mstp)
	{
		if(count_suspend_mstp > 0)	
			count_suspend_mstp--;
		else
			flag_suspend_mstp = 0;
	}
}



void Store_MASTER_To_Eeprom(uint8_t master)
{
	
	//E2prom_Write_Byte(EEP_MAX_MASTER,master);
}


char* Get_temcovars_string_from_buf(uint8_t number)
{
	char type,num;
	char str[20];
	if(number == 4)
	{
		memset(str,'\0',20);
		type = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type;
		num = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number;
		
		if(type == IN)
		{
			memcpy(&str,"IN",2);
			itoa(num,&str[2],10);
			return (char *)(str);
		}
		else if(type == VAR)
		{
			memcpy(&str,"VAR",3);
			itoa(num,&str[3],10);
			return (char *)(str);
		}
		else
			return "null";
	}
	else
		return "error";
}


char Write_temcovars_string_to_buf(uint8_t number,char * str)
{
	char type,num = 0;
	if(number == 4)
	{
		if(str[0] == 'I' && str[1] == 'N')
		{
			Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type = IN;
			
			if((str[2] >= '0' && str[2] <= '9') && (str[3] >= '0' && str[3] <= '9'))
				num = str[3] - '0' + 10 * (str[2] - '0');
			else if(str[2] >= '0' && str[2] <= '9')
				num = str[2] - '0';
			
			if(num >= 1)
				Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number = num;
			//memcpy(Modbus.display_lcd.lcddisplay,Setting_Info.reg.display_lcd.lcddisplay,sizeof(lcdconfig));
			//write_page_en[25] = 1;	
			//Flash_Write_Mass();
		}
		else if(str[0] == 'V' && str[1] == 'A' && str[2] == 'R')
		{
			Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type = VAR;
			
			if((str[3] >= '0' && str[3] <= '9') && (str[4] >= '0' && str[4] <= '9'))
				num = str[4] - '0' + 10 * (str[3] - '0');
			else if(str[3] >= '0' && str[3] <= '9')
				num = str[3] - '0';
			
			if(num >= 1)
				Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number = num;
			//memcpy(Modbus.display_lcd.lcddisplay,Setting_Info.reg.display_lcd.lcddisplay,sizeof(lcdconfig));
			//write_page_en[25] = 1;	
			//Flash_Write_Mass();
		}
		return 1;
	}
	return 0;
}

#include <string.h>

extern uint8_t header_len;
extern uint16_t transfer_len;
void Send_UserList_Broadcast(U8_T start,U8_T end)
{
	BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
	BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
	uint8_t  test_value[480] = { 0 };
	bool status = false;
	int private_data_len = 0;	
	BACNET_ADDRESS dest;
	uint8_t  temp[500];
	
	private_data.vendorID = Get_Vendor_ID();//BACNET_VENDOR_ID;
	private_data.serviceNumber = 1;	
	
	temp[0] = (USER_DATA_HEADER_LEN + sizeof(Password_point)) & 0x00FF;
	temp[1] = ((USER_DATA_HEADER_LEN + sizeof(Password_point)) & 0xff00 ) >> 8;
	temp[2] = WRITEUSER_T3000;
	temp[3] = start;
	temp[4] = end;
	temp[5] = (sizeof(Password_point)) & 0x00ff; 
	temp[6] = (sizeof(Password_point) & 0xff00) >> 8;
	
	header_len = USER_DATA_HEADER_LEN;	
	transfer_len = sizeof(Password_point) * (end - start + 1);
	memcpy(&temp[7],(char *)&passwords[0],transfer_len);
	status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,(char *)&temp, &data_value);
	private_data_len = bacapp_encode_application_data(&test_value[0], &data_value);
	private_data.serviceParameters = &test_value[0];
	private_data.serviceParametersLen = private_data_len;
	
		
//	Send_bip_Flag = 1;	
//	count_send_bip = 0;
//	Send_bip_count = MAX_RETRY_SEND_BIP;	
//	Set_broadcast_bip_address(0xffffffff);
//	bip_get_broadcast_address(&dest);
	Send_ConfirmedPrivateTransfer(&dest,&private_data,BAC_IP_CLIENT);
	
}

void uart_send_string(U8_T *p, U16_T length,U8_T port);
int Send_private_scan(U8_T index)
{
	BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
	BACNET_APPLICATION_DATA_VALUE data_value = { 0 };
	//uint8_t  test_value[480] = { 0 };
	bool status = false;
	int private_data_len = 0;
	BACNET_ADDRESS dest;
	uint8_t  temp[500];
	unsigned max_apdu = 0;

	uint8_t* test_value = (uint8_t*)malloc(512);
	//uint8_t* temp = (uint8_t*)malloc(512);
	private_data.vendorID = Get_Vendor_ID();
	private_data.serviceNumber = 1;

	temp[0] = 0x07;
	temp[1] = 0x00;
	temp[2] = READ_BACNET_TO_MDOBUS;
	temp[3] = 0;
	temp[4] = 0; // start addr is 0
	temp[5] = 10;
	temp[6] = 0;  // len is 10

	header_len = USER_DATA_HEADER_LEN;
	transfer_len = 0;
	status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,(char *)&temp, &data_value);
	
	private_data_len = bacapp_encode_application_data(&test_value[0], &data_value);
	private_data.serviceParameters = &test_value[0];
	private_data.serviceParametersLen = private_data_len;

	if(count_hold_on_bip_to_mstp > 0)
	{
		free(test_value);
		//free(temp);
		return -1;
	}
	if(remote_panel_db[index].sub_id != 0)
	{
		flag_mstp_source = 2;   // T3-CONTROLLER
		Send_Private_Flag = 3;   // send normal bacnet packet
		TransmitPacket_panel = remote_panel_db[index].sub_id;
		
		status = address_get_by_device(remote_panel_db[index].device_id, &max_apdu, &dest);
		if(status)
		{
			if((TransmitPacket_panel < 255) && (TransmitPacket_panel > 0))
			{
				invokeid_mstp = Send_ConfirmedPrivateTransfer(&dest,&private_data,BAC_MSTP);
			}
		}
	}
	
	
	
	free(test_value);
	//free(temp);
	return invokeid_mstp;
}




