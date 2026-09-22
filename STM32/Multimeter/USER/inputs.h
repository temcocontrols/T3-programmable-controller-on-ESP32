#ifndef __INPUTS_H
#define __INPUTS_H
#include "bitmap.h"

#include "spi.h"
//typedef union
//{
//	u8 byte[4];
//	u32 longbyte;

//}UN_HIGH_COUNT;


//typedef enum	
//{
//	NO_ISP,IN_ISP,IN_NORMAL
//}ISP_MODE;


#define RISE 0
#define FALL 1

#define HI_COMMON_CHANNEL  6
#define COMMON_CHANNEL 			8  // TSTAT10 no pulse counter

extern u8 high_speed_flag[HI_COMMON_CHANNEL];
extern UN_HIGH_COUNT high_speed_counter[HI_COMMON_CHANNEL]; 
extern u8 flag_ISP;

//#define MAX_AI_CHANNEL 32
extern vu8 MAX_AI_CHANNEL;
extern vu16 AD_Value[32]; 
extern vu16 AO_Feedback[4];



extern u16 test[];

#define I0_20ma     0
#define V0_5        1
#define V0_10       2
#define Thermistor  3

#define SEL1_IN   	PCout(10)	
#define SEL2_IN   	PCout(9)
#define SEL3_IN   	PCout(8)

#define RANGE_SET0			PCout(7)
#define RANGE_SET1			PCout(6)


typedef struct
{
	u8 enable; // whether enalbe adding module by hand
	u32 flag; 
	// bit 0: wifi
	// bit1. 10K ?????
	// bit2. HUM ?????
	// bit3. OCC ???
	// bit4. CO2 ????
	// bit5. Pressure ??
	// bit6. TVOC ???????
	// bit7. Light ????
	// bit8. ?????
	// bit9. Zigbee ??
	// bit10. PM2.5 ??
	// bit11. AI ????1KPT 
	// bit13. 
	
}STR_EX_MODULE;


typedef enum
{
	E_FLAG_WIFI,// bit 0: wifi
	E_FLAG_10K,// bit1. 10K ?????
	E_FLAG_HUM,// bit2. HUM ?????
	E_FLAG_OCC,// bit3. OCC ???
	E_FLAG_CO2,// bit4. CO2 ????
	E_FLAG_PRESS,// bit5. Pressure ??
	E_FLAG_TVOC,// bit6. TVOC ???????
	E_FLAG_LIGHT,// bit7. Light ????
	E_FLAG_VOICE,// bit8. voice
	E_FLAG_ZIGBEE,// bit9. Zigbee ??
	E_FLAG_PM25,// bit10. PM2.5 ??
	E_FLAG_PT1K,// bit11. AI ????1KPT 
	//E_FLAG_10K,// bit13. 
}E_EX_FLAG;


void Input_IO_Init(void);
void inputs_init(void) ;
void inpust_scan(void) ;
u16 ADC_getChannal(ADC_TypeDef* ADCx, u8 channal,u8 rank);



#endif


