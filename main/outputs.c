#include "controls.h"
#include "user_data.h"


#ifdef OUTPUT

#if NEW_IO
extern uint8 max_outputs;
extern Str_out_point 		*new_outputs;
#else
extern Str_out_point 	outputs[MAX_OUTS];
#endif

//U8_T far control_auto[MAX_OUTS];
//U8_T far switch_status_back[MAX_OUTS];

void control_output(void)
{
//	Str_out_point *outs;
	Str_points_ptr ptr;
	U8_T point = 0;
	U32_T val;
//	U8_T loop;
	U32_T value;
	point = 0;
	

	while( point < MAX_OUTS )
	{	
#if NEW_IO
	if(point < max_outputs)
		ptr.pout = new_outputs + point;
#else
	ptr.pout = &outputs[point];
#endif
		if(point < get_max_output())
		{			
			if(point < get_max_internal_output())
			{		
				ptr.pout->sub_id = 0;
				ptr.pout->sub_product = 0;
				ptr.pout->sub_number = 0;
				ptr.pout->decom = 0;
			}
			if( ptr.pout->range == not_used_output )
			{
				ptr.pout->value = 0L;
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
				ptr.pout->sub_id = 0;
				ptr.pout->sub_product = 0;
				ptr.pout->sub_number = 0;
				
			  if(point >= get_max_internal_output())
					ptr.pout->decom = 1;  // no used
			}

		}
		else
		{
			ptr.pout->sub_id = 0;
			ptr.pout->sub_product = 0;
			ptr.pout->sub_number = 0;			

			ptr.pout->decom = 1;  // no used
		}
		point++;
		ptr.pout++;
				
	}	

}

#endif

