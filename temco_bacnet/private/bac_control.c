//#include "controls.h"
#include "bac_point.h"
#include "user_data.h"
#include "e2prom.h"
#include "bo.h"
#include "ao.h"
 
void control_input(void);
void control_output(void);
S16_T exec_program(S16_T current_prg, U8_T *prog_code);
void update_comport_health(void);
extern void check_output_priority_array(U8_T i,U8_T HOA);
extern void Set_AO_raw(uint8 i,float value);
void set_output_raw(uint8_t point,uint16_t value);
void sample_points( void );
//extern S16_T get_point_value( Point *point, S32_T *val_ptr );
extern U16_T Test[50];

uint8_t get_max_internal_output(void);
/*{
	return MAX_INS;
}*/

Con_aux				 			con_aux[MAX_CONS];

#define PID_SAMPLE_COUNT 20
#define PID_SAMPLE_TIME 10



void pid_controller( S16_T p_number )   // 10s 
{
 /* The setpoint and input point can only be local to the panel even
		though the structure would allow for points from the network segment
		to which the local panel is connected */
	S32_T op, oi, od, err, erp, out_sum;
/*  err = percent of error = ( input_value - setpoint_value ) * 100 /
					proportional_band	*/
//	U8_T sample_time = 10L;       /* seconds */
	Point *pt1 = NULL;
	S32_T value = 0;

	U16_T prop;
	S32_T l1;
	Str_controller_point *con;
	Con_aux *conx;
	static S32_T temp_input_value,temp_setpoint_value;
	
	controllers[p_number].sample_time = PID_SAMPLE_TIME;
	con = &controllers[p_number];
	

	if(con->auto_manual == 1)  // manual - 1 , 0 - auto
		return;

	conx = &con_aux[p_number];

#if 1
	//
	get_point_value( (Point*)&con->input, &con->input_value);
	get_point_value( (Point*)&con->setpoint, &con->setpoint_value );
	od = oi = op = 0;
//	con->proportional = 20;

	prop = con->prop_high;	
	prop <<= 8;
	prop += con->proportional;

	temp_input_value = (con->input_value);
	temp_setpoint_value = (con->setpoint_value); 

	err = temp_input_value - temp_setpoint_value;  /* absolute error */
		
	erp = 0L;

//	con->reset = 20;

/* proportional term*/
	if( prop > 0 )
		erp = 100L * err / prop;
	if( erp > 100000L ) erp = 100000L;
	if( con->action > 0)
		op = erp; /* + */
	else
		op = -erp; /* - */

	erp = 0L;
	
/* integral term	*/
	/* sample_time = 10s */
	l1 = ( conx->old_err + err ) * (con->sample_time / 2); /* 5 = sample_time / 2 */
	l1 += conx->error_area;
	if( conx->error_area >= 0 )
	{
		 if( l1 > 8388607L )
				l1 = 8388607L;
	}
	else
	{
		 if( l1 < -8388607L )
				l1 = -8388607L;
	}
	conx->error_area = l1;
	if( con->reset > 0 )
	{
		if( con->action > 0)  // fix iterm 
			oi = con->reset * conx->error_area;
		else
			oi -= con->reset * conx->error_area;

		if(con->repeats_per_min > 0)
			oi /= 60L;
		else
			oi /= 3600L;
	}
/* differential term	*/
	if( con->rate > 0)
	{
		od = conx->old_err * 100;
		od /= prop;
		od = erp - od;
		if(con->action > 0)
		{
/*			od = ( erp - conx->old_err * 100 / prop ) * con->rate / 600L;
			 600 = sample_time * 60  */
			od *= con->rate;
		}
		else
		{
/*			od = -con->rate * ( erp - conx->old_err
						* 100 / prop ) / 600L;  600 = sample_time * 60  */
			od *= ( -con->rate );
		}
		od /= (con->sample_time * 60); 	/* 600 = sample_time * 60  */
	}

	out_sum = op + con->bias + od / 100; //  od / 100 , because con->rate is 100x

	if( out_sum > 100000L ) out_sum = 100000L;
	if( out_sum < 0 ) out_sum = 0;
	if( con->reset > 0)
	{
		 out_sum += oi;
		 if( out_sum > 100000L )
		 {
				out_sum = 100000L;
		 }
		 if( out_sum < 0 )
		 {
			out_sum = 0;
		 }
	}
	conx->old_err = err;
	con->value = (out_sum);
#endif


}


void check_weekly_routines(void)
{
 /* Override points can only be local to the panel even though the
		structure would allow for points from the network segment to which
		the local panel is connected */
	S8_T w, i, j;
	S32_T value;
	Str_weekly_routine_point *pw = NULL;
	Time_on_off *pt = NULL;
#if 0
	pw = &weekly_routines[0];
	for( i=0; i< MAX_WR; pw++, i++ )
	{
		w = Rtc.Clk.week - 1;
		if( w < 0 ) w = 6;
		if( pw->auto_manual == 0 )  // auto
		{		
			if( pw->override_2.point_type )
			{				
				get_point_value( (Point*)&pw->override_2, &value );
				
				pw->override_2_value = value?1:0;
				if( value )		w = 8;
			}
			else
			 	pw->override_2_value = 0;
		
			if(pw->override_1.point_type)
			{				
				get_point_value( (Point*)&pw->override_1, &value );
				pw->override_1_value = value?1:0;
				if( value )
					w = 7;
			}
			else
			 	pw->override_1_value = 0;
		
		
			pt = &wr_times[i][w].time[2*MAX_INTERVALS_PER_DAY-1];

			j = 2 * MAX_INTERVALS_PER_DAY - 1;
		/*		for( j=2*MAX_INTERVALS_PER_DAY-1; j>=0; j-- )*/
		/*	do
			{				
				pt->hours = 10 + j;
				pt->minutes = 25 + j;
				pt--;
				
			}
			while( --j >= 0 );
			pt = &wr_times[i][w].time[2*MAX_INTERVALS_PER_DAY-1];
			j = 2 * MAX_INTERVALS_PER_DAY - 1; 
		 */

			do
			{				
				//if( pt->hours || pt->minutes )
				if(wr_time_on_off[i][w][j] != 0xff)
				{	
					if( Rtc.Clk.hour > pt->hours ) break;
					if( Rtc.Clk.hour == pt->hours )
						if( Rtc.Clk.min >= pt->minutes )
							break;
				}
				pt--;
				
			}
			while( --j >= 0 );

			if(wr_time_on_off[i][w][j] == 0xff)
				pw->value = 0;
			else
				pw->value = wr_time_on_off[i][w][j];
			
#if 0			
			if( j < 0)		
				pw->value = 0;
			else
			{ 				
				if( j & 1 ) /* j % 2 */
				{
					pw->value = 0;//SetByteBit(&pw->flag,0,weekly_value,1);
				}
				else
				{
					pw->value = 1;//SetByteBit(&pw->flag,1,weekly_value,1);
				}    
			}
#endif
		}
	}
#endif
}


void check_annual_routines( void )
{
	S8_T i;
	S8_T mask;
	S8_T octet_index;
	Str_annual_routine_point *pr;

	pr = &annual_routines[0];
#if 0
	for( i=0; i<MAX_AR; i++, pr++ )
	{
   	if( pr->auto_manual == 0 )
	 	{
			mask = 0x01;
			/* Assume bit0 from octet0 = Jan 1st */
			/* octet_index = ora_current.day_of_year / 8;*/
			octet_index = (Rtc.Clk.day_of_year) >> 3;
			/* bit_index = ora_current.day_of_year % 8;*/    // ????????????????????????????
	/*		bit_index = ora_current.day_of_year & 0x07;*/
			mask = mask << ((Rtc.Clk.day_of_year) & 0x07 );
			
			if( ar_dates[i][octet_index] & mask )
			{
				pr->value = 1;
			}
			else
				pr->value = 0;
	   	}
	}
  //	misc_flags.check_ar=0;
#endif

}



void check_graphic_element(void)
{
	U8_T i;
	for(i = 0;i < MAX_GRPS;i++)
	{
		control_groups[i].element_count = 0;
	}
	
	for(i = 0;i < MAX_ELEMENTS;i++)
	{		
		if(group_data[i].reg.label_status == 0)
				break;
		
		if(group_data[i].reg.label_status == 1)  // current element is valid
		{
			if(group_data[i].reg.nScreen_index < MAX_GRPS)
			{
				control_groups[group_data[i].reg.nScreen_index].element_count++;
			}
		}			
	}
}

//1a 00 74 00 00 13 00 0c 00 01 0a 00 09 9c 02 12 9d a0 86 01 00 fe 00
//10  VAR1 = 100  TST1-SETPOINT = 100
//const U8_T code test_prg_code[30] = {
//0x1d,0x00,0x74,0x00,0x00,0x16,0x00,0x0f,0x00,0x01,
//0x0a,0x00,0x09,0x9E,0x55,0xaa,0x0f,0x02,0x12,0x9d,0xa0,0x86,0x01,0x00,0xfe};


//24 00 74 00 00 1d 00 16 00 01 0a 00 09 9c 00 03 9d a0 86 01 00 01 14 00 09 9c 01 03 9c 02 03 fe 00
// 10  VAR1 = 100
//20  VAR2 = VAR3
//const U8_T test_prg_code[30] = {
//0x1a,0x00,0x74,0x00,0x00,0x13,0x00,0x0c,0x00,0x01,
//0x0a,0x00,0x09,0x9c,0x02,0x12,0x9d,0xa0,0x86,0x01,0x00,0xfe};
//const U8_T test_prg_code[50] = {
//0x24,0x00 ,0x74 ,0x00 ,0x00 ,0x1d ,0x00 ,0x16 ,0x00 ,0x01 ,0x0a ,0x00 ,0x09 ,0x9c ,0x02, 
//0x12 ,0x9d ,0xa0 ,0x86 ,0x01 ,0x00 ,0x01 ,0x14 ,0x00 ,0x09 ,0x9c ,0x01 ,0x03 ,0x9c ,0x05,0x12 ,0xfe ,0x00
//};


void Ethernet_Debug_Task();
U8_T count_10s = 0;
U8_T count_reset_zigbee = 0;
U16_T last_reset_zigbee;
U8_T count_1min = 0;	
U8_T count_3s = 0;
U8_T count_1s = 0;
U8_T current_day;
//U8_T count_pid[MAX_CONS] = 0;
//uint8_t flag_load_prg;
//uint8_t count_load_prg;
void Check_All_WR(void);
U8_T check_whehter_running_code(void);
extern U8_T flag_writing_code;
extern U8_T count_wring_code;
extern U8_T max_dos;
extern U8_T max_aos;
extern U16_T output_raw[MAX_OUTS];

#define		SW_OFF 	 0
#define 	SW_HAND	 2
#define		SW_AUTO	 1
void vTaskDelay( const uint32_t xTicksToDelay );	


void check_trendlog_1s(unsigned char count)
{
	//static U16_T count_wait_sample = 0;
	static U8_T count_1s = 0;
	// wait 5 second, after input value is ready

	//if(count_wait_sample >= 100)
	{
		//count_wait_sample = 0;

		//if(count_1s++ % count == 0)
		{
			count_1s = 0;
			// tbd: sample

		sample_points();	
#if BAC_TRENDLOG
		trend_log_timer(0);
#endif
		//update_comport_health();
		}
		
	}
	//else
	//	count_wait_sample++;
}

void Set_AO_raw(uint8 i,float value)
{	
	Str_points_ptr ptr;
	ptr = put_io_buf(OUT,i);
	switch( ptr.pout->range )
	{
		case V0_10:				
			set_output_raw(i,value / 10);
			break;
		case P0_100_Open:
		case P0_100_Close:
		case P0_100:
		case P0_100_PWM:	
			set_output_raw(i,value / 100);
			break;
		case P0_20psi:
		case I_0_20ma:	
			set_output_raw(i,value / 20);
			break;
		case P0_100_2_10V:
			set_output_raw(i,200 + value * 8 / 1000);
			break;
		default:
			set_output_raw(i,value / 1000);
			break; 				 			
		
	}	
}

void check_output_priority_HOA(U8_T i)
{	
	Str_points_ptr ptr;
	ptr = put_io_buf(OUT,i);
	// check swtich status
		if(ptr.pout->switch_status == SW_OFF)
		{	
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif						
			if(ptr.pout->digital_analog == 0)
			{									
				if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
				||(ptr.pout->range >= custom_digital1 // customer digital unit
				&& ptr.pout->range <= custom_digital8
				&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{ // inverse
					output_priority[i][6] = 1;	

					if(i < max_dos)
						ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;
					else
						ptr.pout->control = Analog_Output_Present_Value(i) ? 0 : 1;
				}
				else
				{
					output_priority[i][6] = 0;	
					if(i < max_dos)
						ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;
					else
						ptr.pout->control = Analog_Output_Present_Value(i) ? 1 : 0;
					
				}				
				if(i < max_dos)
				{
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000);					
				}
				else
				{
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);					
				}
				
				if(ptr.pout->control) 
					set_output_raw(i,1000);
				else 
					set_output_raw(i,0);	
			}
			else
			{
				output_priority[i][6] = 0;
				ptr.pout->control = 0;
				
				if(i < max_dos)
				{
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000);					
					set_output_raw(i,Binary_Output_Present_Value(i) * 1000);
				}
				else
				{
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);					
					Set_AO_raw(i,(ptr.pout->value));			
				}
			}
		}
		else if(ptr.pout->switch_status == SW_HAND)
		{
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif			
			if(ptr.pout->digital_analog == 0)
			{
				if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{// inverse
					output_priority[i][6] = 0;	
					if(i < max_dos)
						ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;	
					else
						ptr.pout->control = Analog_Output_Present_Value(i) ? 0 : 1;
				}
				else
				{
					output_priority[i][6] = 1;
					if(i < max_dos)					
						ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;	
					else
						ptr.pout->control = Analog_Output_Present_Value(i) ? 1 : 0;	
				}	
				
				if(i < max_dos)
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000);
				else
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);

				if(ptr.pout->control) 
					set_output_raw(i,1000);
				else 
					set_output_raw(i,0);	
			
			}
			else
			{
				switch( ptr.pout->range )
				{
					case V0_10:	
						output_priority[i][6] = 10;
						break;
					case P0_100_Open:
					case P0_100_Close:
					case P0_100:
					case P0_100_PWM:	
					case P0_100_2_10V:
						output_priority[i][6] = 100;
						break;
					case P0_20psi:
					case I_0_20ma:	
						output_priority[i][6] = 20;
						break;
					default:
						output_priority[i][6] = (ptr.pout->value) / 1000;
						break; 				 			
					
				}						
				
				if(i < max_dos)
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000);
				else
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);

				Set_AO_raw(i,(ptr.pout->value));					
			}
		}
		else if(ptr.pout->switch_status == SW_AUTO)// auto
		{
			output_priority[i][6] = 0xff;	
			check_output_priority_array(i,1);
		}
		else
		{
			// error status

		}

}



// CHECK A/M
// HOA: 1 -> �ı�HOA�����level7ֵ����
//			0 -> modbus����bancent���ֱ�Ӹı� leve7 ֵ�ı�
void check_output_priority_array(U8_T i,U8_T HOA)
{	
	Str_points_ptr ptr;
	ptr = put_io_buf(OUT,i);

	if(i >= max_dos + max_aos)
	{
		output_priority[i][7] = 0xff;	
	}
	else
	{
		if(ptr.pout->auto_manual == 0)  // auto , pirotry_array level 8 is NULL
		{
			output_priority[i][7] = 0xff;	
			if(ptr.pout->digital_analog == 0)
			{	// digital
				//if(i < max_dos)
				{
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000); 

					if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
					{// inverse
						ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;	
					}	
					else
					{
						ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;
					}
				}
				
				// when OUT13-OUT24 are used for DO
				if(ptr.pout->control) 
				{
					set_output_raw(i,1000);
				}
				else 
				{
					set_output_raw(i,0);	
				}
			}
			else
			{
				if(i < get_max_internal_output())
				{

					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);
					// set output_raw

					Set_AO_raw(i,(ptr.pout->value));
				}
				else
				{
					output_raw[i] = (ptr.pout->value);
				}					
			}

		} // else manual
		else
		{
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif			
			if(ptr.pout->digital_analog == 0)
			{	// digital
				if(HOA == 0)
				{
					if(ptr.pout->control == 1)
						output_priority[i][7] = 1;
					else
						output_priority[i][7] = 0;
				}
				if(i < max_dos)
					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000); 
				else
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);
				
				if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{// inverse
					ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;	
				}	
				else
				{
					ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;
				}
				
				if(ptr.pout->control) 
					set_output_raw(i,1000);
				else 
					set_output_raw(i,0);	
			}		
			else
			{  // analog
//				if(i < max_dos)
//				{
//					if(HOA == 0)
//						output_priority[i][7] = (float)(ptr.pout->value) / 1000;
//					ptr.pout->value = (Binary_Output_Present_Value(i) * 1000); 
//					set_output_raw(i,Binary_Output_Present_Value(i) * 1000);
//				}
//				else
				{
					if(HOA == 0)
						output_priority[i][7] = (float)(ptr.pout->value) / 1000;

					if(i < get_max_internal_output())
					{
						ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);
						// set output_raw
						Set_AO_raw(i,(ptr.pout->value));
					}
					else
					{
						output_raw[i] = (ptr.pout->value);
					}
				}					
			}
		}	
	}	
	
}


// dont check A/M
void check_output_priority_array_without_AM(U8_T i)
{	
	Str_points_ptr ptr;
	ptr = put_io_buf(OUT,i);
	if(i >= max_dos + max_aos)
	{
		output_priority[i][7] = 0xff;	
	}
	else
	{		
		if(ptr.pout->digital_analog == 0)
		{	// digital
			if(i < max_dos)
			{
				ptr.pout->value = (Binary_Output_Present_Value(i) * 1000);
			}				
			else
			{
				ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);
			}
				
			if(ptr.pout->control) 
				set_output_raw(i,1000);
			else 
				set_output_raw(i,0);	
		}		
		else
		{  // analog
			if(i < max_dos)
			{
				ptr.pout->value = (Binary_Output_Present_Value(i) * 1000); 
				set_output_raw(i,Binary_Output_Present_Value(i) * 1000);
			}
			else
			{
				if(i < get_max_internal_output())
				{
					ptr.pout->value = (Analog_Output_Present_Value(i) * 1000);
					// set output_raw
					Set_AO_raw(i,(ptr.pout->value));
				}
				else
				{
					output_raw[i] = (ptr.pout->value);
				}
			}	
			
		}

	}	
	
}

U8_T   output_pri_live[MAX_OUTS];


void Check_Program_Output_Pri_valid(void)
{
	U8_T i;
	for(i = 0;i < MAX_OUTS;i++)
	{
		if(output_priority[i][9] != 0xff)
		{
			if(output_pri_live[i] > 0)
				output_pri_live[i]--;
			else
				output_priority[i][9] = 0xff;
		}
	}
	
}



#if OUTPUT_DEATMASTER
U32_T   count_dead_master = 0;	

void clear_dead_master(void)
{
	count_dead_master = 0;	
}


void output_dead_master(void)
{
	U8_T i,j;
	if(Modbus.dead_master != 0)  
	{
		if(count_dead_master++ > Modbus.dead_master  * 60)
		{
			// go to relinquish
			count_dead_master = 0;
			for(i = 0;i < MAX_OUTS;i++)
			{
				for(j = 0;j < 16;j++)
					output_priority[i][j] = 0xff;
				check_output_priority_array(i,0);
			}
		}
			
	}
}

#endif


