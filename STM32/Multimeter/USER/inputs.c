#include "stm32f10x.h"
#include "sht3x.h"
#include "math.h" 
#include "inputs.h"
#include "stm32f10x_adc.h"

// only for tstat-wifi
/*
-> output
-> input 

*/
STR_EX_MODULE  ex_moudle;

extern uint8_t internal_co2_module_type;
extern uint8_t hum_exists;


#define LIGHT_COE    63
#define LIGHT_R1     1

uint16 co2_asc;
uint16 co2_frc;
u8 range[32];
vu8 MAX_AI_CHANNEL;
vu16 AD_Value[32]; 
vu16 AO_Feedback[4];

// for MSV
uint8 FAN_MODE[8][9];
U8_T flag_count_in[HI_COMMON_CHANNEL + 4];
void signal_dealwith(u8 i);
uint8 Check_sensor_exist(uint8 type);

void SCD40_Initial(void);
void Refresh_SCD40(void);
/*
从tstat10_rev4开始硬件引脚变动 */


// Tstat10P 新增HSP count和 两路 AO
uint8 Check_sensor_exist(uint8 type);
 /*  PE11 - PG14
 PE11------------>	HSP_INPUT1
 PE12------------>	HSP_INPUT2
 PE13------------>	HSP_INPUT3
 PE14------------>	HSP_INPUT4
*/
#define READ_PULSE1 GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_11)
#define READ_PULSE2 GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_12)
#define READ_PULSE3 GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_13)
#define READ_PULSE4 GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_14)

#define INPUT1 PEin(11)
#define INPUT2 PEin(12)
#define INPUT3 PEin(13)
#define INPUT4 PEin(14)

#define FILTER 7

#define RISE 0
#define FALL 1

U8_T high_spd_flag[HI_COMMON_CHANNEL];

void pulse_set(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	// PC6 7 8 9
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource14);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource11);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource12);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource13);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line14 | EXTI_Line11 | EXTI_Line12 | EXTI_Line13; 
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	

//	high_spd_counter_tempbuf[COMMON_CHANNEL] = 0;
//	high_spd_counter_tempbuf[COMMON_CHANNEL + 1] = 0;
//	high_spd_counter_tempbuf[COMMON_CHANNEL + 2] = 0;
//	high_spd_counter_tempbuf[COMMON_CHANNEL + 3] = 0;

}


 
U8_T flag_ready_to_scan = 0;
U8_T table_pwm[8];
U8_T slop[10];

U8_T hum_sensor_type;
float tem_org = 0;
float hum_org = 0;

int16_t HumSensor_dew_pt;
int16_t HumSensor_dew_pt_F;
uint16_t HumSensor_Pws;
uint16_t HumSensor_Mix_Ratio;
uint16_t HumSensor_Enthalpy;


void range_set_func(u8 range)
{
	if(range == V0_5)
	{
		RANGE_SET0 = 1 ;
		RANGE_SET1 = 0 ;
	}
	else if(range == V0_10)
	{		
		RANGE_SET0 = 0 ;
		RANGE_SET1 = 1 ;
	}
	else if(range == I0_20ma)
	{
		RANGE_SET0 = 0 ;
		RANGE_SET1 = 0 ;
	}
	else
	{		
		RANGE_SET0 = 1 ;
		RANGE_SET1 = 1 ;
	} 

}

// PC1 - ADC123_IN11
// PC5 - internal temperature sensor  ADC12_IN15
// PB0 - ADC12_IN8
// PB1 - ADC12_IN9
// PC0 - ADC123_IN10
void inputs_init(void)
{
	ADC_InitTypeDef ADC_InitStructure;
  char i;
	Input_IO_Init();
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);		
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 5;
  ADC_Init(ADC1, &ADC_InitStructure);	
		
 	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 2, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 3, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 4, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 5, ADC_SampleTime_55Cycles5);

  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
     
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	
//	initial_tstat10_range();
}


void Input_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);
	
	// PC0 -> SOUND  PC4-COMMON AI   PC5-TEMPERATURE	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_4 | GPIO_Pin_5; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
// light sensor & occupied sensor 
	// PB0 & PB1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11; 	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);	
	
	
	GPIO_SetBits(GPIOC, GPIO_Pin_11); // MOD_SET HIGH
	
	
	
}


u16 ADC_getChannal(ADC_TypeDef* ADCx, u8 channal,uint8 rank)
{
	uint16_t tem = 0;
	ADC_ClearFlag(ADCx, ADC_FLAG_EOC);
	ADC_RegularChannelConfig(ADCx, channal, rank, ADC_SampleTime_55Cycles5);
	ADC_SoftwareStartConvCmd(ADCx, ENABLE);
	
	while(ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);
	tem = ADC_GetConversionValue(ADCx);
	return tem;        
}

// 10ms 一次
// 载波的高低电压0.6~0.8
#define MIC_CARRIER_HI 	1090
#define MIC_CARRIER_LO	820
uint16 voice_table[10][2] = 
{ {60,9},{63,13},{67,36},{70,55},{72,95},
	{74,170},{76,220},{78,270},{80,340},{83,500}
};

u8 check_voice_table(uint16 adc)
{
	char i;
	if(adc < voice_table[0][1]) 
		return 50;
	
	if(adc > voice_table[9][1]) 
		return 90;
	
	for(i = 0;i < 9;i++)
	{		
		if((adc >= voice_table[i][1]) && (adc < voice_table[i + 1][1]))
		{
			return voice_table[i][0] + 
				(voice_table[i + 1][0] - voice_table[i][0]) * (adc - voice_table[i][1]) / (voice_table[i + 1][1] - voice_table[i][1]);
		}
	}
	return 0;
}




u16 voltage_occ;
void inpust_scan(void)
{
	static u8 channel_count = 0;
	u8 i;
	float light_temp;
	u32 temp1,temp2,temp3,temp4,temp5;
	u32 mic_sum;
	u8 count;
//	static u8 count_no_mic = 0;
	mic_sum = 0;
	
	temp1 = 0;  // AI0-AI7
	temp2 = 0; // temperature
	temp3 = 0;  // light
	temp4 = 0;  // occ
	temp5 = 0; // mic
	count = 0;
	
	for(i = 0;i < 100;i++)
	{
		temp1 += ADC_getChannal(ADC1,ADC_Channel_14,1);
		temp2 += ADC_getChannal(ADC1,ADC_Channel_15,1);
		temp3 += ADC_getChannal(ADC1,ADC_Channel_8,1);
		temp4 += ADC_getChannal(ADC1,ADC_Channel_9,1);		
		temp5 = ADC_getChannal(ADC1,ADC_Channel_10,1);
		
		if(temp5 > MIC_CARRIER_HI)
		{
			count++;
			temp5 = temp5 - MIC_CARRIER_HI;
		}
		else if(temp5 < MIC_CARRIER_LO)
		{
			count++;
			temp5 = MIC_CARRIER_LO - temp5;
		}
		else
		{
			temp5 = 0;
		}
		
		mic_sum += temp5;// * temp5;
		delay_us(5);
		
	}
	
	temp1 = temp1 / 100;	
	temp2 = temp2 / 100;
	temp3 = temp3 / 100;
	temp4 = temp4 / 100;
	
	
	if(Check_sensor_exist(E_FLAG_VOICE))
	{
		if(count >= 0)
		{
			temp5 = check_voice_table(mic_sum /count);			
			AD_Value[COMMON_CHANNEL + 6] = temp5 * 1000;
		}
		else
		{
			AD_Value[COMMON_CHANNEL + 6] = 40000;
		}
	}
	else
	{
		AD_Value[COMMON_CHANNEL + 6] = 0;
	}
	
	AD_Value[channel_count] = temp1;	
	test[40 + channel_count] = temp1;
	
	// if no sht4x, no co2
	if((internal_co2_module_type == 0) && (hum_exists == 0))
	{
		AD_Value[COMMON_CHANNEL] = temp2;		// temperatue
		
	}
	test[40 + COMMON_CHANNEL] = temp2;
	channel_count++;
	
	
	channel_count %= 8;
	
	if(Check_sensor_exist(E_FLAG_LIGHT))
	{
		light_temp = 3000 * temp3 / 4095;
		AD_Value[COMMON_CHANNEL + 5] = light_temp * 100 * 1000/(LIGHT_COE * LIGHT_R1);
	}	
	
	 // detect occ senseor
	if(Check_sensor_exist(E_FLAG_OCC))
	{
		voltage_occ = temp4 * 3000 / 40 / 1023;
		AD_Value[COMMON_CHANNEL + 3] = voltage_occ;
	}
	
	
	switch(channel_count)
	{
		case 0: 
			SEL3_IN = 0;	SEL1_IN = 0;	SEL2_IN = 0;	
			break;
		case 1: 
			SEL3_IN = 0;	SEL1_IN = 1;	SEL2_IN = 0;
			break;
		case 2: 
			SEL3_IN = 0;	SEL1_IN = 0;	SEL2_IN = 1;			
			break;
		case 3: 
			SEL3_IN = 0;	SEL1_IN = 1;	SEL2_IN = 1;
			break;		
		case 4: 
			SEL3_IN = 1;	SEL1_IN = 0;	SEL2_IN = 0;
			break;	
		case 5: 
			SEL3_IN = 1;	SEL1_IN = 1;	SEL2_IN = 0;
			break;		
		case 6: 
			SEL3_IN = 1;	SEL1_IN = 0;	SEL2_IN = 1;
			break;		
		case 7: 
			SEL3_IN = 1;	SEL1_IN = 1;	SEL2_IN = 1;
			break;			
		default:
			break;	
	}
	range_set_func(range[channel_count]);

}

#define TSTAT_REALY1	PEout(0)
#define TSTAT_REALY2	PEout(1)
#define TSTAT_REALY3	PBout(4)
#define TSTAT_REALY4	PBout(5)
#define TSTAT_REALY5	PEout(6)

#define TSTAT_REALY6	PBout(9)
#define TSTAT_REALY7	PGout(7)


extern U16_T test_adc;
extern U8_T test_adc_flag;



uint8 Check_sensor_exist(uint8 type)
{
	if((ex_moudle.enable >= 0x55) && (ex_moudle.enable <= 0x65))
	{
		if(type == E_FLAG_HUM)
		{
			if(ex_moudle.flag & 0x04)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_OCC)
		{
			if(ex_moudle.flag & 0x08)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_CO2)
		{
			if(ex_moudle.flag & 0x10)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_PRESS)
		{
			if(ex_moudle.flag & 0x20)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_TVOC)
		{
			if(ex_moudle.flag & 0x40)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_LIGHT)
		{
			if(ex_moudle.flag & 0x80)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_VOICE)
		{
			if(ex_moudle.flag & 0x100)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_ZIGBEE)
		{
			if(ex_moudle.flag & 0x0200)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_PM25)
		{
			if(ex_moudle.flag & 0x0400)
				return 1;
			else
				return 0;
		}
		if(type == E_FLAG_PT1K)
		{
			if(ex_moudle.flag & 0x800)
				return 1;
			else
				return 0;
		}
	}
	else
		return 1;
}

extern uint8_t hum_exists;
// for SCD40
void SCD40_get_value(uint16_t co2,int32_t temperaute, int32_t humidity)
{
	// TBD:
	// read co2,temperaute,humitdy from scd40	
	tem_org = temperaute / 100;
	hum_org = humidity / 100;
	test[10] = tem_org;
	test[11] = hum_org;
	test[12] = co2;
	test[13]++;
	if(hum_exists == 0)
	{
		AD_Value[COMMON_CHANNEL] = temperaute;
		AD_Value[COMMON_CHANNEL + 2] = humidity;
		
	}
	AD_Value[COMMON_CHANNEL + 4] = co2 * 1000;	
	
}



