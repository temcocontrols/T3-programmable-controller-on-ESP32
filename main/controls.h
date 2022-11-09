#ifndef	CONTROLS_H
#define	CONTROLS_H




//#define MANUAL_JUMPER	
#define AUTO_JUMPER



#define far 
#define xdata 
#define code 


#pragma pack(1) 



#include "user_data.h"


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
extern U8_T far control_auto[MAX_OUTS];
extern U8_T far switch_status_back[MAX_OUTS];
void control_input(void);
void control_output(void);
U32_T get_input_value_by_range(U8_T range, U16_T raw);


extern U16_T far Test[50];

uint16_t crc16(uint8_t *p, uint8_t length);

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

