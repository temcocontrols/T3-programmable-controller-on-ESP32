#include "controls.h"
#include "product.h"
#include "define.h"
#include "scan.h"
#include "commsub.h"

U8_T base_in;
U8_T base_out;
U8_T base_var;

#define MINI_CM5  0
#define MINI_BIG	 1
#define MINI_SMALL  2
#define MINI_TINY	 3			// ASIX CORE
#define MINI_NEW_TINY	 4  // ARM CORE


#define MINI_BIG_ARM	 	5
#define MINI_SMALL_ARM  6
#define MINI_TINY_ARM		7
#define MINI_ROUTER    8

#define MINI_TSTAT10 9

#define MINI_VAV	 10


#define STM_TINY_REV 7

#define HI_COMMON_CHANNEL  32

#define SUB_NO  254





U32_T far high_spd_counter[HI_COMMON_CHANNEL];
U32_T far high_spd_counter_tempbuf[HI_COMMON_CHANNEL];
U8_T far high_spd_en[HI_COMMON_CHANNEL];


U16_T Test[50];

U8_T base_in;
U8_T base_out;
U8_T base_var;

U8_T far sub_no;



U16_T far Test[50];
//U16_T far input_raw[MAX_INS];

U8_T change_value_by_range(U8_T channel)
{
	return 0;
}

U32_T get_rpm(U8_T point)
{
	return 0;
}

void Set_Input_Type(U8_T point)
{
	// maybe not need it

}

U16_T get_input_raw(U8_T point)
{
	return input_raw[point];
}

void set_output_raw(U8_T point,U16_T value)
{
	output_raw[point] = value;
}

U16_T get_output_raw(U8_T point)
{
	return output_raw[point];
}


U32_T conver_by_unit_5v(U32_T sample)
{

	if(Modbus.mini_type == MINI_BIG_ARM)
	{
		return  (5000L * sample ) >> 10;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM) // rev4  use input moudle
	{
		return  (5000L * sample ) >> 10;
	}
	else if(Modbus.mini_type == MINI_TINY_ARM)
	{
		return (5000L * sample ) >> 10;
	}
	else //if(Modbus.mini_type == MINI_CM5) // rev4  use input moudle
	{
		return ( 5000L *  sample) >> 10;
	}
}

U32_T conver_by_unit_10v(U32_T sample)
{

		if(Modbus.mini_type == MINI_BIG_ARM)
		{
			return (10000l * sample) >> 10;
		}
		else if(Modbus.mini_type == MINI_SMALL_ARM) // rev4  use input moudle
		{
			return (10000l * sample) >> 10;
		}
		else if(Modbus.mini_type == MINI_TINY_ARM)
		{
			return (10000L * sample ) >> 10;
		}
		else //if(Modbus.mini_type == MINI_CM5)
		{
			return  ( 10000L * sample) >> 10;
		}
}

U32_T conver_by_unit_custable(U8_T point,U32_T sample)
{
	if(input_type[point] == INPUT_V0_5)
	{
		if(Modbus.mini_type == MINI_BIG_ARM)
		{
			return  ( 5000L * sample) >> 10;
		}
		else if(Modbus.mini_type == MINI_SMALL_ARM) // rev4  use input moudle
		{
			return  ( 5000L * sample  ) >> 10;
		}
		else if(Modbus.mini_type == MINI_TINY_ARM)
		{
			return (5000L * sample ) >> 10;
		}
		else //if(Modbus.mini_type == MINI_CM5)
		{
			return  ( 5000L * sample ) >> 10;
		}

	}
	else if(input_type[point] == INPUT_I0_20ma)
	{
		return ( 20000L * sample ) >> 10;
	}
	else if(input_type[point] == INPUT_0_10V)
	{
		if(Modbus.mini_type == MINI_BIG_ARM)
		{
			return ( 10000l * sample) >> 10;
		}
		else if(Modbus.mini_type == MINI_SMALL_ARM) // rev4  use input moudle
		{
			return (10000l * sample ) >> 10;
		}
		else if(Modbus.mini_type == MINI_TINY_ARM)
		{
			return (10000L * sample ) >> 10;
		}
		else //if(Modbus.mini_type == MINI_CM5)
		{
			return  (10000L * sample) >> 10;
		}

	}
	else if(input_type[point] == INPUT_THERM || input_type[point] == INPUT_NOUSED)
	{
		return ( 3000L  * sample ) >> 10;//get_input_value_by_range( inputs[point].range, sample );
	}
	return 0;
}

U8_T get_max_input(void)
{
	return base_in;
}

U8_T get_max_output(void)
{
	return base_out;
}

U8_T get_max_internal_input(void)
{
	if(Modbus.mini_type == MINI_BIG_ARM)
	{
		return BIG_MAX_AIS;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM)
	{
	  return SMALL_MAX_AIS;
	}
	else if(Modbus.mini_type == MINI_TINY_ARM)
	{
	  return NEW_TINY_MAX_AIS;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{
	  return TSTAT10_MAX_AIS;
	}
	return 0;
}

U8_T get_max_internal_output(void)
{
	if(Modbus.mini_type == MINI_BIG_ARM)
	{
		return BIG_MAX_AOS + BIG_MAX_DOS;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM)
	{
	  return SMALL_MAX_AOS + SMALL_MAX_DOS;
	}
	else if(Modbus.mini_type == MINI_TINY_ARM)
	{
	  return NEW_TINY_MAX_AOS + NEW_TINY_MAX_DOS;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{
		return TSTAT10_MAX_AOS + TSTAT10_MAX_DOS;
	}
	return 0;
}


U32_T get_high_spd_counter(U8_T point)
{
	//inputs[point].value = (int32_t)((high_spd_counter[point] + high_spd_counter_tempbuf[point]) * 1000);
	return 0;//(high_spd_counter[point] + high_spd_counter_tempbuf[point]) * 1000;
}

// old io.lib run it
void map_extern_output(U8_T point)
{
}

//void map_extern_output(U8_T point)
//{
//	U16_T reg;
//	U8_T sub_index;
//	U16_T value;

//
//#if  T3_MAP
////	if((outputs[point].sub_product == PM_T38AI8AO6DO) || (outputs[point].sub_product == PM_T322AI))
////		return;
////	outputs[point].switch_status = SW_AUTO;
//	if(point >= get_max_internal_output())
//	{
//		if( outputs[point].digital_analog == 0 )	 // DO
//		{
//
//			output_raw[point] = outputs[point].control ? 1000 : 0;
//			if(output_raw[point] != output_raw_back[point])
//			{
//				reg = count_output_reg(&sub_index,MAP_DO,point);
//				if(reg > 0)
//				{
////					if((sub_map[sub_index].type == PM_T38I13O) || (sub_map[sub_index].type == PM_T34AO)
////						|| (sub_map[sub_index].type == PM_T38AI8AO6DO))   // T3 1 byte for DO
//					{
//						if(output_raw[point] >= 512)
//						{	// tbd: set tstat_type, different tstat have different register list
//							// choose 0 for now
//							value = 1;
//							write_parameters_to_nodes(WRITE_VARIABLES,scan_db[sub_index].id,reg,(uint16*)&value,1);
//						}
//						else if(output_raw[point] == 0)
//						{
//							value = 0;
//							write_parameters_to_nodes(WRITE_VARIABLES,scan_db[sub_index].id,reg,(uint16*)&value,1);
//						}
//					}
//				}
//
//				output_raw_back[point] = output_raw[point];
//			}
//		}
//		else  // AO
//		{
//			output_raw[point] = (float)swap_double(outputs[point].value) / 10000 * 4095  ;
//			if(output_raw[point] != output_raw_back[point])
//			{
//				reg = count_output_reg(&sub_index,MAP_AO,point);
//				if(reg > 0)
//				{
//					if((sub_map[sub_index].type == PM_T34AO) || (sub_map[sub_index].type == PM_T38AI8AO6DO))
//					{
//						write_parameters_to_nodes(WRITE_VARIABLES,scan_db[sub_index].id,reg,(uint16*)&output_raw[point],1);
//					}

//				}
//				output_raw_back[point] = output_raw[point];
//			}
//		}
//	}
//#endif
//
//}


S8_T check_external_in_on_line(U8_T index)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		if((index < sub_map[i].ai_start + sub_map[i].ai_len) && (index >= sub_map[i].ai_start))
		{
			break;
		}
	}

	if(i == sub_no)
		return -1;
	else
	{
		return (current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8))) ? 1 : 0;
	}
}


S8_T check_external_out_on_line(U8_T index)
{
	U8_T i;
	if(index < get_max_internal_output())
	{
		return -1;
	}
	for(i = 0;i < sub_no;i++)
	{
		if((index < sub_map[i].ao_start + sub_map[i].ao_len) && (index >= sub_map[i].ao_start))
		{
			break;
		}
		if((index < sub_map[i].do_start + sub_map[i].do_len) && (index >= sub_map[i].do_start))
		{
			break;
		}
	}

	if(i == sub_no)
		return -1;
	else
	{
		return (current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8))) ? 1 : 0;
	}
}
