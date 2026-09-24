#include "output.h"
#include "stm32f10x_adc.h"
#include "led.h"


u16 relay_value;



#define T10_REALY1	PBout(3)
#define T10_REALY2	PBout(4)
#define T10_REALY3	PBout(5)
#define T10_REALY4	PBout(8)
#define T10_REALY5	PBout(9)

void Output_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;	

	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 ;  //REALY 1 - 5 ,AO1, AO2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 );
	
	
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

//	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
//	GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);
	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;  
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	TIM_TimeBaseStructure.TIM_Period = 1000;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);  // AO1 PB10 	TIM2_CH3
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);  // AO2 PB11 	TIM2_CH4
	
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
	
	TIM_Cmd(TIM2, ENABLE);

}

extern u16 test[];
void output_control(uint8_t * output)
{	
	if(output[0] == 1)	
		T10_REALY1 = 1; 	
	else
		T10_REALY1 = 0;	
	
	if(output[1] == 1)	
		T10_REALY2 = 1;	
	else
		T10_REALY2 = 0; 
	
	if(output[2] == 1)	
		T10_REALY3 = 1;	
	else
		T10_REALY3 = 0;	
	
	
	if(output[3] == 1)	
		T10_REALY4 = 1;	
	else
		T10_REALY4 = 0;	
	
	if(output[4] == 1)	
		T10_REALY5 = 1;
	else
		T10_REALY5 = 0;

	TIM_SetCompare3(TIM2, output[5] * 4);
	TIM_SetCompare4(TIM2, output[6] * 4);
	
}





