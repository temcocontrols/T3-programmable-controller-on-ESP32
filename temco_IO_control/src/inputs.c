#include "controls.h"



#ifdef INPUT

extern Str_table_point			far				 custom_tab[MAX_TBLS];
extern Str_in_point        far        inputs[MAX_INS];

uint8_t far input_type[32];
uint8_t far input_type1[32];

extern U16_T far chip_info[6];
extern U16_T PT1K_para;

#define SENSOR_DELAY 10
#define FILTER_ADJUST 4
#define NO_TABLE_RANGES 16
#define MIDDLE_RANGE     8

#define NO_TABLE_RANGES_PT 	50
#define MIDDLE_RANGE_PT     25



const S32_T tab_int[10] = { 11875, 21375, 10000, 18000, 10000, 18000,10000, 18000, 10000, 18000 };

const S32_T limit[10][2] = { { -40000L, 150000L }, { -40000L, 302000L },
							{ -40000L, 120000L }, { -40000L, 248000L },
							{ -40000L, 120000L }, { -40000L, 248000L },
							{ -40000L, 120000L }, { -40000L, 248000L },
							{ -200000L, 300000l }, { -200000L, 300000L }
						  };


const uint16_t def_tab_10bit[5][17] = {
 /* 3k termistor YSI44005 -40 to 150 Deg.C or -40 to 302 Deg.F */
	{ 233*4,  211*4, 179*4, 141*4, 103*4, 71*4, 48*4, 32*4,
		21*4, 14*4, 10*4, 7*4, 5*4, 4*4, 3*4, 2*4, 1*4 },

 /* 10k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */  // type2
	{ 988, 964, 924, 862, 778, 682, 572, 462,
	 364, 282, 214, 164, 128, 100, 76, 62, 48 },

 /* 3k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */
	{ 233*4, 215*4, 190*4, 160*4, 127*4, 96*4, 70*4, 50*4,
		35*4, 25*4, 18*4, 13*4, 9*4, 7*4, 5*4, 4*4, 3*4 },

 /* 10k termistor KM -40 to 120 Deg.C or -40 to 248 Deg.F */ // type3
	{ 976, 948, 906, 842, 764, 670, 566, 466,
		376, 296, 234, 180, 144, 114, 90, 76, 60 },

 /* 3k termistor AK -40 to 150 Deg.C or -40 to 302 Deg.F */
//	{ 246*4, 238*4, 227*4, 211*4, 191*4, 167*4, 141*4, 115*4,
//		92*4, 72*4, 55*4, 42*4, 33*4, 25*4, 19*4, 15*4, 12*4 }
		{
			//61,105,150,191,232,272,314,355,396,433,474,512,546,587,624,661,699
			109,122,164,206,247,288,328,368,407,446,485,523,562,600,638,676,694
		}
};

const uint16_t def_tab_12bit_PT1K[51] = 
{
	
//	318,378,434,// -200 -190 -180
//	492,550,606,// -170 -160 -150
//	661,718,772,// -140 -130 -120
//	827,883,938,// -110 -100 -90
//	992,1048,1100,// -80 -70 -60
//	1153,1208,1262,// -50 -40 -30
//	1314,1368,1420,// -20 -10 0
//	1472,1525,1579,//10 20 30
//	1629,1681,1734,// 40 50 60
//	1783,1834,1886,// 70 80 90
//	1927,1988,2037,// 100 110 120
//	2090,2139,2188,// 130 140 150
//	2240,2289,2338,// 160 170 180
//	2390,2438,2485,// 190 200 210
//	2535,2584,2691,// 220 230 240
//	2680,2726,2772,// 250 260 270
//	2815,2850,2878// 280 290 300
	461,537,609,// -200 -190 -180
	683,758,829,// -170 -160 -150
	900,971,1043,// -140 -130 -120
	1113,1183,1256,// -110 -100 -90
	1327,1392,1464,// -80 -70 -60
	1532,1601,1668,// -50 -40 -30
	1739,1805,1872,// -20 -10 0
	1940,2006,2076,//10 20 30
	2140,2209,2276,// 40 50 60
	2344,2407,2474,// 70 80 90
	2540,2607,2672,// 100 110 120
	2737,2804,2866,// 130 140 150
	2934,2997,3058,// 160 170 180
	3126,3188,3250,// 190 200 210
	3315,3382,3441,// 220 230 240
	3504,3566,3629,// 250 260 270
	3693,3752,3813// 280 290 300
};

const uint16_t def_tab_12bit[5][17] = {
 /* 3k termistor YSI44005 -40 to 150 Deg.C or -40 to 302 Deg.F */
	{ 233*16,  211*16, 179*16, 141*16, 103*16, 71*164, 48*164, 32*16,
		21*16, 14*16, 10*16, 7*16, 5*16, 4*16, 3*16, 2*16, 1*16 },

 /* 10k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */  // type2
	{ 3975, 3879, 3720, 3476, 3110, 2747, 2303, 1864,
	 1464, 1140, 866, 660, 516, 393, 312, 250,203},
//		{
//			3979,3879,3718,3476,3112,2747,2302,1859,1456,1132,854,649,503,382,299,236,191
//		},

 /* 3k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */
	{ 233*16, 215*16, 190*16, 160*16, 127*16, 96*16, 70*16, 50*16,
		35*16, 25*16, 18*16, 13*16, 9*16, 7*16, 5*16, 4*16, 3*16 },

 /* 10k termistor KM -40 to 120 Deg.C or -40 to 248 Deg.F */ // type3
	{ 3932, 3816, 3640, 3394, 3075, 2700,2289, 1881,
		1512, 1196, 950, 738, 577, 456, 365, 306, 248 },
//		{
//			3934,3822,3642,3393,3075,2699,2286,1880,1506,1189,940,729,566,444,353,290,236
//		},

 /* 3k termistor AK -40 to 150 Deg.C or -40 to 280 Deg.F */
//	{ 246*4, 238*4, 227*4, 211*4, 191*4, 167*4, 141*4, 115*4,
//		92*4, 72*4, 55*4, 42*4, 33*4, 25*4, 19*4, 15*4, 12*4 }
		{
			//61,105,150,191,232,272,314,355,396,433,474,512,546,587,624,661,699
			//109,122,164,206,247,288,328,368,407,446,485,523,562,600,638,676,694
			318,492,662,828,993,1157,1318,1477,1634,1789,1942,2098,2246,2398,2540,2686,2820// 2883(300)
		}
};


int32_t swap_double( int32_t dat ) 	 //swap_double
{ 
	
	return dat;
}


int16_t swap_word( int16_t dat ) 	//	  swap_word
{ 

	return dat;
}


uint32_t get_input_value_by_range( uint8_t range, uint16_t raw )
{
	uint16_t index = 0;
	S32_T val = 0;
	S32_T work_var = 0;
	S32_T ran_in = 0;
	S32_T step = 0;
	S32_T delta = MIDDLE_RANGE;
	uint16_t *def_tbl;
	Byte end = 0;
	range--;
	ran_in = range;
	range >>= 1;
	end = 0;
	chip_info[1] = 42;
	//raw = raw * PT1K_para / 10000;
	if(chip_info[1] >= 42)  // firmware rev is higher than 42, it is 12bit adc for arm chip
	{
			
	if(range <= 3) // 0,1,2,3 -> old type
	{step = 100;
		delta = MIDDLE_RANGE;
		def_tbl = ( uint16_t * )&def_tab_12bit[range];

		
		if( raw <= def_tbl[NO_TABLE_RANGES] )
			return limit[ran_in][1];
		if( raw >= def_tbl[0] )
			return limit[ran_in][0];
		index = MIDDLE_RANGE;

		
		while( !end )
		{
			if( ( raw >= def_tbl[index] ) && ( raw <= def_tbl[index-1] ) )
			{
				index--;
				delta = def_tbl[index] - def_tbl[index+1];
				if( delta )
				{
					work_var = (int)( ( def_tbl[index] - raw ) * step );
					work_var /= delta;
					work_var += ( index * step );
					val = tab_int[ran_in];
					val *= work_var;
					val /= step;
					val += limit[ran_in][0];
				}
				return val;
			}
			else
			{
				if( !delta )
					end = 1;
				delta /= 2;
				if( raw < def_tbl[index] )
					index += delta;
				else
					index -= delta;
				if( index <= 0 )
					return limit[ran_in][0];
				if( index >= NO_TABLE_RANGES )
					return limit[ran_in][1];
			}
		}
		
	}
	else // 4 -> new PK sensor
	{
		step = 100;		
		delta = MIDDLE_RANGE_PT;

		def_tbl = /*( uint16_t * )&def_tab_12bit[range];*/( uint16_t * )&def_tab_12bit_PT1K;
		if( raw <= def_tbl[0] )
		{
			return limit[ran_in][0];
		}
		if( raw >= def_tbl[NO_TABLE_RANGES_PT] )
		{
			return limit[ran_in][1];
		}
		
		index = MIDDLE_RANGE_PT;
		
		while( !end )
		{		
			uint8_t i;
			if(raw > def_tbl[MIDDLE_RANGE_PT])
			{
				for(i = MIDDLE_RANGE_PT; i < NO_TABLE_RANGES_PT;i++)
				{
					if( ( raw >= def_tbl[i] ) && ( raw <= def_tbl[i+1] ) )
					{
						delta = def_tbl[i + 1] - def_tbl[i];
						if( delta )
						{
							work_var = (int)( ( raw - def_tbl[i]) * step );
							work_var /= delta;
							work_var += ( i * step );
							val = tab_int[ran_in];
							val *= work_var;
							val /= step;
							val += limit[ran_in][0];
						}
						Test[8] = val / 1000;
						return val;
					}
				}
			}
			else
			{
				for(i = 0; i < MIDDLE_RANGE_PT;i++)
				{
					if( ( raw >= def_tbl[i] ) && ( raw <= def_tbl[i+1] ) )
					{
						delta = def_tbl[i + 1] - def_tbl[i];
						if( delta )
						{
							work_var = (int)( ( raw - def_tbl[i]) * step );
							work_var /= delta;
							work_var += ( i * step );
							val = tab_int[ran_in];
							val *= work_var;
							val /= step;
							val += limit[ran_in][0];
						}
						Test[9] = val / 1000;
						return val;
					}
				}
				
			}
			
		}
	}	
		
	}
	else
	{step = 100;
		def_tbl = ( uint16_t * )&def_tab_10bit[range];
		delta = MIDDLE_RANGE;
			if( raw <= def_tbl[NO_TABLE_RANGES] )
			return limit[ran_in][1];
			if( raw >= def_tbl[0] )
				return limit[ran_in][0];
			index = MIDDLE_RANGE;

			
			while( !end )
			{
				if( ( raw >= def_tbl[index] ) && ( raw <= def_tbl[index-1] ) )
				{
					index--;
					delta = def_tbl[index] - def_tbl[index+1];
					if( delta )
					{
						work_var = (int)( ( def_tbl[index] - raw ) * step );
						work_var /= delta;
						work_var += ( index * step );
						val = tab_int[ran_in];
						val *= work_var;
						val /= step;
						val += limit[ran_in][0];
					}
					return val;
				}
				else
				{
					if( !delta )
						end = 1;
					delta /= 2;
					if( raw < def_tbl[index] )
						index += delta;
					else
						index -= delta;
					if( index <= 0 )
						return limit[ran_in][0];
					if( index >= NO_TABLE_RANGES )
						return limit[ran_in][1];
				}
			}			
	}	
	
	return 0;
}


float test_match_custom( S16_T range, S16_T raw )
{   /* custom tables */

	Tbl_point *table_point;
	S16_T index = 1;
	float val = 0;
	float diff = 0;
	range -= table1;

	do
	{
		 table_point = &custom_tab[range].dat[index];
		 if(raw == swap_word(table_point->value))
		 {
				return swap_double(table_point->unit);
		 }
		 if(raw == swap_word((table_point-1)->value))
		 {
				return swap_double((table_point-1)->unit);
		 }
		 if( ( raw < swap_word(table_point->value) ) &&
				( raw > swap_word((table_point-1)->value) ) )
			{ index--; break; }
		 else
			index++;
	}
	while( index <= 14 );

	table_point = &custom_tab[range].dat[index];
	index = swap_word((table_point+1)->value) - swap_word(table_point->value);
	if( index )
	{
/*		val = ( raw - table_point->value ) * 1000 /
			( (table_point+1)->value - table_point->value );*/
		val = ( raw - swap_word(table_point->value) );
		val *= 1000;
		val /= index;
	}
	diff = swap_double((table_point+1)->unit) - swap_double(table_point->unit);

	if( diff )
	{
/*		val = table_point->unit + val *
			( (table_point+1)->unit - table_point->unit );*/
		val *= diff;
		val /= 1000;
		val += swap_double(table_point->unit);
	}
	return val;

	

}


void control_input(void)
{
	Str_in_point *ins;
//	In_aux *inx;
	uint8_t point = 0;
	U32_T sample;
//	U8_T max_input;
  U8_T temp;	
	U8_T shift = 1;
	ins = inputs;
//	inx = in_aux;
	chip_info[1] = 42;
	if(chip_info[1] >= 42)
		shift = 4;
	else
 
		shift = 1;
	
	while( point < MAX_INS )
	{		
		if(change_value_by_range(point))
		{
		if(point < get_max_input())
		{
#ifdef MANUAL_JUMPER			
			input_type[point] = ins->decom >> 4;
#endif


			if((ins->range >= table1) && (ins->range <= table5))
			{
				if(ins->decom >> 4 == 0)
					input_type[point] = INPUT_THERM;
				else
					input_type[point] = ins->decom >> 4;
				
			}		
			else 
			{
				if( ins -> digital_analog == 0)
				{
					input_type[point] = INPUT_THERM;
				}
				else
				{
					if((ins->range == V0_5) || (ins->range == P0_100_0_5V)) // 0-_5v
					{
						input_type[point] = INPUT_V0_5;				
					}
					else if((ins->range == V0_10_IN) || (ins->range == P0_100_0_10V))   // 0-10v
					{
						input_type[point] = INPUT_0_10V;
					}
					else if((ins->range == I0_20ma) || (ins->range == P0_100_4_20ma))
					{
						input_type[point] = INPUT_I0_20ma;
					}
					else if((ins->range == PT1000_200_300DegC) || (ins->range == PT1000_200_570DegF)) // 0-_5v
					{
						input_type[point] = INPUT_1KPT;				
					}
					else
						input_type[point] = INPUT_THERM;
				}		

			}	
			temp = ins->decom;
			temp &= 0x0f;
			temp |= (input_type[point] << 4);
			ins->decom = temp;	
		

		
			if(point < get_max_internal_input())
			{
				ins->sub_id = 0;
				ins->sub_product = 0;
				ins->sub_number = 0;
				
				if(input_type[point] != input_type1[point])
				{	
					input_type1[point] = input_type[point];
					// set input type					
					Set_Input_Type(point);					
				}		

				
				if(ins->auto_manual == 0)  // auto			 
				{ 			
					 // raw value			
					if(ins->range != not_used_input)
					{				
						sample = get_input_raw(point);//input_raw[point];
						
						if( ins -> digital_analog == 0)  // digital
						{						
							temp = ins->decom;
							temp &= 0xf0;
							temp |= IN_NORMAL;
							ins->decom = temp;
							
							if( ins->range >= ON_OFF  && ins->range <= HIGH_LOW )  // control 0=OFF 1=ON
							{
								ins->control = (sample > (512 * shift) ) ? 1 : 0;
							}
							else
							{
								ins->control = (sample < (512 * shift) ) ? 0 : 1;					
							}
							
							if( ins->range >= custom_digital1 && ins->range <= custom_digital8 )
							{
								ins->control = (sample < (512 * shift) ) ? 0 : 1;	
							}
							
							ins->value = swap_double(ins->control ? 1000L : 0);
								
							sample = ins->control ? 1000L : 0;
						}
						else if(ins -> digital_analog == 1)	// analog
						{						
							
							temp = ins->decom;
							temp &= 0xf0;
							temp |= IN_NORMAL;
							ins->decom = temp;
							// add filter 
							//sample = Filter(point,sample);	
								
							switch(ins->range)
							{
							case Y3K_40_150DegC:
							case Y3K_40_300DegF:
							case R3K_40_150DegC:
							case R3K_40_300DegF:
							case R10K_40_120DegC:
							case R10K_40_250DegF:
							case KM10K_40_120DegC:
							case KM10K_40_250DegF:

								if(get_input_raw(point) > 1000 * shift)   
								{
									temp = ins->decom;
									temp &= 0xf0;
									temp |= IN_OPEN;
									ins->decom = temp;
								}
								else if(get_input_raw(point) < 20)  
								{ 
									temp = ins->decom;
									temp &= 0xf0;
									temp |= IN_SHORT;
									ins->decom = temp;
								}				
								
								sample = get_input_value_by_range( ins->range, sample );
								break;
							case PT1000_200_300DegC:
							case PT1000_200_570DegF:
								if(get_input_raw(point)  > 4000)    
								{
									temp = ins->decom;
									temp &= 0xf0;
									temp |= IN_OPEN;
									ins->decom = temp;
								}
								else if(get_input_raw(point) < 50 * shift) 
								{ 
									temp = ins->decom;
									temp &= 0xf0;
									temp |= IN_SHORT;
									ins->decom = temp;
								}				
								sample = get_input_value_by_range( ins->range, sample );
								break;
							case V0_5:			
								sample = conver_by_unit_5v(sample / shift);


								break;
							case V0_10_IN:
							
								sample = conver_by_unit_10v(sample / shift);

								break;
							case I0_100Amps:
								sample = ( 100000L * sample / shift ) >> 10;
								break;
							case I0_20ma:
								sample = ( 20000L * sample / shift ) >> 10;
				
								break;
							case I0_20psi:
								sample = ( 20000L * sample / shift ) >> 10;
								break;
//							case N0_3000FPM_0_10V:
//								sample = ( 2700000L * sample ) >> 10;
//								break;
							case P0_100_0_10V:
							case P0_100_0_5V:
								sample = ( 100000L * sample / shift ) >> 10;
								break;
							case P0_100_4_20ma:
								if(sample / shift  < 204) 
									sample = 0;
								else
									sample = 100000L * ( sample / shift - 204 ) / 816;
								break;
							case table1:
							case table2:
							case table3:
							case table4:
							case table5:
								sample = conver_by_unit_custable(point,sample / shift) / 100;
								sample = 1000l * test_match_custom((int)ins->range, (int)sample);	
								//sample = 1000l * sample;
								break;
							case N0_2_32counts:
							case HI_spd_count:	
								sample =  get_high_spd_counter(point);//(high_spd_counter[point] + high_spd_counter_tempbuf[point] ) * 1000;
								break;
							case RPM:
								sample =  get_rpm(point);
								break;
							default:
								//	sample = sample * 1000;	
								break;
							}
					
						//	if( ins->calibration_increment ) 
							{
								if( !ins->calibration_sign )
									sample += 100L * (ins->calibration_hi * 256 + ins->calibration_lo);
								else
									sample += -100L * (ins->calibration_hi * 256 + ins->calibration_lo);
							}
						}
						ins->value = swap_double(sample);
					
					}
					else  // not_used_input
					{
						// if range is 0, show raw value
							temp = ins->decom;
							temp &= 0xf0;
							temp |= IN_NORMAL;
							ins->decom = temp;
							// if internal input

							if(point < get_max_internal_input())
								ins->value = swap_double((U32_T)get_input_raw(point) / shift * 3000 / 1023);
							else

								ins->value = swap_double((U32_T)get_input_raw(point));
					}	
				}
				else if(ins->auto_manual == 1)	// manual
				{		
					temp = ins->decom;
					temp &= 0xf0;
					temp |= IN_NORMAL;
					ins->decom = temp;
				}	
			}
			else
			{ // external input
				
				// check whether currect id is on line or not
				if(check_external_in_on_line(point) == 0)  // off-ine
				{
					ins->sub_id = 0;
					ins->sub_product = 0;
					ins->sub_number = 0;
					
					temp = ins->decom;
					temp &= 0xf0;
					temp |= IN_NORMAL;
					ins->decom = temp;
				}
					
			}
			
		
	  } 
		else
		{
			ins->sub_id = 0;
			ins->sub_product = 0;
			ins->sub_number = 0;
			
			temp = ins->decom;
			temp &= 0xf0;
			temp |= IN_NORMAL;
			ins->decom = temp;
			
		}
	}
		point++;
	  ins++;
		
	}
}

#endif




