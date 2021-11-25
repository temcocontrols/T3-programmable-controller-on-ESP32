#include "controls.h"


#ifdef OUTPUT

extern Str_out_point far	outputs[MAX_OUTS];

//U8_T far control_auto[MAX_OUTS];
//U8_T far switch_status_back[MAX_OUTS];

void control_output(void)
{
	Str_out_point *outs;
	U8_T point = 0;
	U32_T val;
//	U8_T loop;
	U32_T value;
	point = 0;
	outs = outputs;


	while( point < MAX_OUTS )
	{	
		if(point < get_max_output())
		{			
			if(point < get_max_internal_output())
			{		
				outs->sub_id = 0;
				outs->sub_product = 0;
				outs->sub_number = 0;
				outs->decom = 0;
			}
			if( outs->range == not_used_output )
			{
				outs->value = 0L;
				val = 0;
			}
//			else
//			{	
//					if( outs->digital_analog == 1 ) //  analog
//					{	
//						if(point >= get_max_internal_output())  // // external ouput 					
//						{  // range is 0-10v
//							if(outs->sub_product != 44/*PM_T38AI8AO6DO*/)
//							{
//								if(outs->read_remote == 1)
//								{
//									val = 10000000 / 4095;
//									val = val * get_output_raw(point) / 1000;
//									outs->read_remote = 0;
//								}
//								else
//								{		
//									val = swap_double(outs->value);	
//	//								val = val * 4095 / 10000;
//									// ?????????
//									if(val < get_output_raw(point))
//									{
//										if(get_output_raw(point) - val == 1)
//											set_output_raw(point,val + 1);//output_raw[point] = val + 1;
//										if(get_output_raw(point) - val == 2)
//											set_output_raw(point,val + 2);//output_raw[point] = val + 2;
//									}
//									else
//										set_output_raw(point,val);//output_raw[point] = val;
//									
//								}
//							}
//						}
////					}
//				}
//			}

			if(check_external_out_on_line(point) == 1)
			{	
				map_extern_output(point);
			}
			else
			{
				outs->sub_id = 0;
				outs->sub_product = 0;
				outs->sub_number = 0;
				
			  if(point >= get_max_internal_output())
					outs->decom = 1;  // no used
			}

		}
		else
		{
			outs->sub_id = 0;
			outs->sub_product = 0;
			outs->sub_number = 0;
			

			outs->decom = 1;  // no used
		}
		point++;
		outs++;
				
	}	

}

#endif

