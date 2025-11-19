#include "controls.h"
#include "product.h"
#include "define.h"
#include "scan.h"
#include "commsub.h"

U8_T base_in;
U8_T base_out;
U8_T base_var;



#define STM_TINY_REV 7



U16_T Test[50];

U8_T base_in;
U8_T base_out;
U8_T base_var;

U8_T far sub_no;



//U16_T far input_raw[MAX_INS];
extern uint8_t InputLed[32];
extern uint8 flag_internal_temperature;
U8_T change_value_by_range(U8_T channel)
{
	/*if(Modbus.mini_type == MINI_T10P)
	{
		// if no internal temperature sensor, use temperature of humidity
		if(input_raw[HI_COMMON_CHANNEL] > 1000)
		{
			if(channel < HI_COMMON_CHANNEL)  // 12
				return 1;
			else
				return 0;
		}
		else
		{
			if(channel <= HI_COMMON_CHANNEL)  // 12
				return 1;
			else
				return 0;
		}
	}
	else if(Modbus.mini_type == MINI_T3OEM_12I)
	{
		// if no internal temperature sensor, use temperature of humidity
		if(input_raw[HI_COMMON_CHANNEL2] > 1000)
		{
			if(channel < HI_COMMON_CHANNEL2)  // 16
				return 1;
			else
				return 0;
		}
		else
		{
			if(channel <= HI_COMMON_CHANNEL2)  // 16
				return 1;
			else
				return 0;
		}
	}
	else */if(Modbus.mini_type == MINI_TSTAT10)
	{
		// if no internal temperature sensor, use temperature of humidity
		if(flag_internal_temperature == 0)
		{
			if(channel < 8/*COMMON_CHANNEL*/)  // 8
				return 1;
			else
				return 0;
		}
		else
		{
			if(channel <= 8/*COMMON_CHANNEL*/)  // 8
				return 1;
			else
				return 0;
		}
	}
	else
		return 1;
}

/*U32_T get_rpm(U8_T point)
{
	return 0;
}*/

void Set_Input_Type(U8_T point)
{
	// maybe not need it
#if 1//I2C_TASK
	if((Modbus.mini_type == MINI_BIG_ARM) || (Modbus.mini_type == MINI_SMALL_ARM) || (Modbus.mini_type == PROJECT_RMC1216) || (Modbus.mini_type == PROJECT_NG2_NEW))
	{
		InputLed[point] &= 0x0f;
		if(input_type[point] >= 1)
			InputLed[point] |= ((input_type[point] - 1) << 4);
		else
			InputLed[point] |= (input_type[point] << 4);

	}
#endif
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
	else if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW) // rev4  use input moudle
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
	else if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW) // rev4  use input moudle
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
		else if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW) // rev4  use input moudle
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
		else if(Modbus.mini_type == MINI_SMALL_ARM  || Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW) // rev4  use input moudle
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

// 用于内部input，range可以调整
U8_T get_max_input(void)
{
	if(Modbus.mini_type == MINI_TSTAT10)
	{// base in is 14，包含6路外部senseor
		if(flag_internal_temperature == 1)
			return 9;
		else
			return 8;
	}
	else
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
	else if(Modbus.mini_type == MINI_SMALL_ARM )
	{
	  return SMALL_MAX_AIS;
	}
	else if(Modbus.mini_type == MINI_TINY_ARM)
	{
	  return NEW_TINY_MAX_AIS;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{
	  if(flag_internal_temperature == 1)
		  return TSTAT10_MAX_AIS + 1;
	  else
		  return TSTAT10_MAX_AIS;
	}
	else if(Modbus.mini_type == PROJECT_RMC1216 )
	{
	  return NG2_MAX_AIS;
	}
	else if(Modbus.mini_type == PROJECT_NG2_NEW)
	{
	  return NEWNG2_MAX_AIS;
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
	else if(Modbus.mini_type == PROJECT_RMC1216)
	{
	  return NG2_MAX_AOS + NG2_MAX_DOS;
	}
	else if(Modbus.mini_type == PROJECT_NG2_NEW)
	{
	  return NEWNG2_MAX_AOS + NEWNG2_MAX_DOS;
	}
	else if(Modbus.mini_type == MINI_NANO)
	{
		return NANO_MAX_AOS + NANO_MAX_DOS;
	}
	else if(Modbus.mini_type == PROJECT_LIGHT_PWM)
	{
		return LPWM_MAX_AOS + LPWM_MAX_DOS;
	}
	return 0;
}

// fro HSP counter

extern uint32_t run_time;
#define HI_COMMON_CHANNEL  32

void Save_SPD_CNT(void);

uint32_t  Input_RPM[HI_COMMON_CHANNEL];
//uint32_t  high_spd_counter[HI_COMMON_CHANNEL];
uint32_t  high_spd_counter_tempbuf[HI_COMMON_CHANNEL];
uint8_t  high_spd_en[HI_COMMON_CHANNEL];
uint8_t  high_spd_flag[HI_COMMON_CHANNEL];
uint8_t  clear_high_spd[HI_COMMON_CHANNEL];
uint16_t  count_clear_hsp[HI_COMMON_CHANNEL];
uint8_t Level_Spd[HI_COMMON_CHANNEL];

uint32_t get_high_spd_counter(uint8_t point)
{

	inputs[point].value = ((/*high_spd_counter[point] +*/ high_spd_counter_tempbuf[point]) * 1000);
	return (/*high_spd_counter[point] +*/ high_spd_counter_tempbuf[point]) * 1000;
}


void Store_Pulse_Counter(uint8 flag)
{
	uint16 save_time;
	static u32 old_pulse[HI_COMMON_CHANNEL];

	uint8_t i;

	save_time = 300;

	if((run_time % save_time == 0) || (flag == 1))
	{
		for(i = 0;i < HI_COMMON_CHANNEL;i++)
		{
			if((inputs[i].range == 15/*HI_spd_count*/) || (inputs[i].range == 25/*N0_2_32counts*/)
					|| (inputs[i].range == 29/*RPM*/))
			{
				if(old_pulse[i] != (/*high_spd_counter[i] +*/ high_spd_counter_tempbuf[i]))
				{
					old_pulse[i] = (/*high_spd_counter[i] +*/ high_spd_counter_tempbuf[i]);
					// store it into flash memory
					//write_page_en[1] = 1;
					//ChangeFlash = 2;
					Save_SPD_CNT();
				}
			}
		}
	}
}

void initial_HSP(void)
{
	uint8_t i;

//	flag_high_spd_changed = 0;

	//memset(&high_spd_counter,0,4 * HI_COMMON_CHANNEL);
	//memset(&high_spd_counter_tempbuf,0,4*HI_COMMON_CHANNEL);

	for(i = 0;i < HI_COMMON_CHANNEL;i++)
	{
		//high_spd_counter[i] = 0;
		//high_spd_counter_tempbuf[i] = 0;
		if((inputs[i].range == 15/*HI_spd_count*/) || (inputs[i].range == 25/*N0_2_32counts*/)
			|| (inputs[i].range == 29/*RPM*/)
		)
		{
			//high_spd_counter[i] = (inputs[i].value) / 1000;
			high_spd_counter_tempbuf[i] = 0;
		}
		count_clear_hsp[i] = 0;
		high_spd_en[i] = 0;
		high_spd_flag[i] = 0;
		Level_Spd[i] = 1;
	}

}


void clear_pulse_counter(uint8_t i)
{
	if((inputs[i].range == 15/*HI_spd_count*/) || (inputs[i].range == 25/*N0_2_32counts*/)
		|| (inputs[i].range == 29/*RPM*/)	)
	{
		if(inputs[i].value == 0)
		{
			//high_spd_counter[i] = 0; // clear high spd count

			high_spd_counter_tempbuf[i] = 0;

			Input_RPM[i] = 0;
			clear_high_spd[i] = 1;
			Save_SPD_CNT();

		}
	}
}


// 放在读取input_raw的函数里执行，通过判断input_raw计算低速pulse counter
void Check_Pulse_Counter(void)
{
	uint8_t loop;

	for(loop = 0;loop < HI_COMMON_CHANNEL;loop++)
	{
		if(high_spd_flag[loop] == 1) // start
		{

			if(input_raw[loop] > 3000)
			{
				if(Level_Spd[loop] == 0)
				{
					high_spd_counter_tempbuf[loop]++;
					Level_Spd[loop] = 1;
				}

			}
			if(input_raw[loop] < 1000)
			{
				if(Level_Spd[loop] == 1)
				{
					//high_spd_counter_tempbuf[loop]++;
					Level_Spd[loop] = 0;
				}

			}

		}

		if(high_spd_flag[loop] == 2) // clear
		{
			high_spd_counter_tempbuf[loop] = 0;
			inputs[loop].value = 0;
		}


	}


	for(loop = 0;loop < HI_COMMON_CHANNEL;loop++)
	{
	// high_spd_flag 0: disable high speed counter  1:  start count 2: clear counter
		if(clear_high_spd[loop] == 1) // write 0 to clear it by T3000.
		{
			high_spd_flag[loop] = 2;  // clear
			count_clear_hsp[loop]++;
			if(count_clear_hsp[loop] > 20)
			{
				count_clear_hsp[loop] = 0;
				clear_high_spd[loop] = 0;
			}
		}
		else
		{
			if((inputs[loop].range == 15/*HI_spd_count*/) || (inputs[loop].range == 25/*N0_2_32counts*/)
				|| (inputs[loop].range == 29/*RPM*/)

			)
			{
				//high_spd_flag[HI_COMMON_CHANNEL - loop - 1] = high_spd_en[HI_COMMON_CHANNEL - loop - 1] + 1;
				high_spd_flag[loop] = 1; // start
			}
			else
			{
				high_spd_flag[loop] = 0; // stop
			}
		}
	}
}

// 10s执行一次
void calculate_RPM(void)
{
	static u32 old_count[HI_COMMON_CHANNEL];
	static u32 runtime[HI_COMMON_CHANNEL];
	static u16 count[HI_COMMON_CHANNEL];

	uint8_t i;
//	char channel;

	for(i = 0;i < HI_COMMON_CHANNEL;i++)
	{
//		if((inputs[i].range == HI_spd_count) || (inputs[i].range == N0_2_32counts)
//			|| (inputs[i].range == RPM))
		if(inputs[i].range == RPM)
		{
			if(old_count[i] <= high_spd_counter_tempbuf[i])
			{
				Input_RPM[i] = (high_spd_counter_tempbuf[i] - old_count[i]) * 60L / (run_time - runtime[i]);

				if(count[i]++ >= 5)  // 1·ÖÖÓÖØÐÂ¼ÆËã
				{
					runtime[i] = run_time;
					old_count[i] = high_spd_counter_tempbuf[i];
					count[i] = 0;
				}
			}
			else
			{
				count[i] = 0;
				runtime[i] = run_time;
				old_count[i] = high_spd_counter_tempbuf[i];
			}
		}
	}
}




uint32_t get_rpm(uint8_t point)
{
	return Input_RPM[point] * 1000;
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
