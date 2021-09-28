#ifndef	CONTROLS_H
#define	CONTROLS_H


//#define ASIX_CON
#define ARM_CON 



//#define MANUAL_JUMPER	
#define AUTO_JUMPER


#ifdef ARM_CON

#define far 
#define xdata 
#define code 

#define MAX_INS     	64	  	
#define MAX_OUTS        64  

#pragma pack(1) 

#endif


#include "ud_str.h"

#ifdef ASIX_CON
#define MAX_INS     	64	  	
#define MAX_OUTS        64  
#endif

#define INPUT
#define OUTPUT

typedef enum
{
	INPUT_NOUSED = 0,
	INPUT_I0_20ma,
	INPUT_V0_5,
	INPUT_0_10V,
	INPUT_THERM,
	INPUT_1KPT,
}E_IN_TYPE;

extern U8_T far input_type[32];
extern Str_in_point  far  inputs[MAX_INS];
extern Str_out_point far	outputs[MAX_OUTS];
extern U8_T far control_auto[MAX_OUTS];
extern U8_T far switch_status_back[MAX_OUTS];
void control_input(void);
void control_output(void);
int32_t swap_double( int32_t dat );
int16_t swap_word( int16_t dat );
U32_T get_input_value_by_range(U8_T range, U16_T raw);


extern U16_T far Test[50];



// do it in own code
extern void Set_Input_Type(U8_T point);  
extern U16_T get_input_raw(U8_T point);
extern void set_output_raw(U8_T point,U16_T value);
extern U16_T get_output_raw(U8_T point);
extern U16_T Filter(U8_T channel,U16_T input);
extern U8_T get_max_output(void);
extern U8_T get_max_input(void);
extern U32_T conver_by_unit_5v(U32_T sample);
extern U32_T conver_by_unit_10v(U32_T sample);
extern U32_T conver_by_unit_custable(U8_T point,U32_T sample);
extern U32_T get_high_spd_counter(U8_T point);
extern U32_T get_rpm(U8_T point);

void map_extern_output(U8_T point);
extern U8_T get_max_internal_input(void);
extern U8_T get_max_internal_output(void);
S8_T check_external_in_on_line(U8_T index);
S8_T check_external_out_on_line(U8_T index);
extern U8_T change_value_by_range(U8_T channel);

#endif

