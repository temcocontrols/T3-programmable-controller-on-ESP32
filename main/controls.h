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

//#define MAX_INS     	64
//#define MAX_OUTS        64

#pragma pack(1) 

#endif


#include "ud_str.h"



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

extern uint8_t far input_type[32];
extern Str_in_point  far  inputs[MAX_INS];
extern Str_out_point far	outputs[MAX_OUTS];
extern U8_T far control_auto[MAX_OUTS];
extern U8_T far switch_status_back[MAX_OUTS];
void control_input(void);
void control_output(void);
int32_t swap_double( int32_t dat );
int16_t swap_word( int16_t dat );
uint32_t get_input_value_by_range(uint8_t range, uint16_t raw);
Str_points_ptr put_io_buf(Point_type_equate type, uint8 point);


extern U16_T far Test[50];

void initial_HSP(void);
void Store_Pulse_Counter(uint8 flag);
void Check_Pulse_Counter(void);

// do it in own code
extern void Set_Input_Type(uint8_t point);  
extern uint16_t get_input_raw(uint8_t point);
extern void set_output_raw(uint8_t point,uint16_t value);
extern uint16_t get_output_raw(uint8_t point);
extern uint16_t Filter(uint8_t channel,uint16_t input);
extern uint8_t get_max_output(void);
extern uint8_t get_max_input(void);
extern uint32 conver_by_unit_5v(uint32 sample);
extern uint32 conver_by_unit_10v(uint32 sample);
extern uint32 conver_by_unit_custable(uint8_t point,uint32 sample);
extern uint32_t get_high_spd_counter(uint8_t point);
extern uint32_t get_rpm(uint8_t point);

void map_extern_output(uint8_t point);
extern uint8_t get_max_internal_input(void);
extern uint8_t get_max_internal_output(void);
S8_T check_external_in_on_line(U8_T index);
S8_T check_external_out_on_line(U8_T index);
extern uint8_t change_value_by_range(U8_T channel);

#endif

