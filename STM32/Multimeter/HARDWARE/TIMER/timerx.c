#include "timerx.h"
#include "led.h"
#include "inputs.h"

extern u32 	comm_heartbeat;
extern u8 flag_ISP;
extern u8 LED_status[67];
//extern u8 flag_Start;
extern u8  flag_Comm;
extern u16 count_comm;


//定时器3中断服务程序	 
void TIM3_IRQHandler(void)
{ 		    		  			    
//	if(TIM_GetFlagStatus(TIM3, TIM_IT_Update) == SET)
//	{
//		LED1 = !LED1;
//	}
//	FGM3_ISR();
	TIM_ClearFlag(TIM3, TIM_IT_Update);	
}

//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler = psc; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	//Timer3 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);								//根据指定的参数初始化NVIC寄存器
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, DISABLE);
}
 	 

/////////////////////////////////////////////////////////////////////////////////////////
u8 DelayTime = 0;
u8 scroll_led = 0;
extern u8 flag_led;
void TIM6_IRQHandler(void)
{
	static u16 count1 = 0;
	u8 i;
	u8 j;
	if(TIM_GetFlagStatus(TIM6, TIM_IT_Update) == SET)
	{		
		
	}
	count1++;

	if(flag_ISP == IN_ISP)
	{ 
		if(count1 == 20)		LED_HEART = 0;
		if(count1 == 40)		LED_HEART = 1;
		if(count1 == 60)		LED_HEART = 0;
		if(count1 == 80)		LED_HEART = 1;
		if(count1 == 200)		count1 = 0;
		
	}
	else if(flag_ISP == IN_NORMAL)
	{
		if(count1 % 500 == 0)
			Refresh_LED();
	}	
	
	TIM_ClearFlag(TIM6, TIM_IT_Update);		
}


//基本定时器6中断初始化					  
//arr：自动重装值。		
//psc：时钟预分频数		 
//Tout= ((arr+1)*(psc+1))/Tclk；
void TIM6_Int_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	
	TIM_TimeBaseStructure.TIM_Period = arr;
	TIM_TimeBaseStructure.TIM_Prescaler = psc; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
	
	//Timer3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;			//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);								//根据指定的参数初始化NVIC寄存器
	
	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM6, ENABLE);
	
//	RCC->APB1ENR |= 1 << 4;					//TIM6时钟使能    
// 	TIM6->ARR = arr;  						//设定计数器自动重装值 
//	TIM6->PSC = psc;  			 			//设置预分频器.
// 	TIM6->DIER |= 1 << 0;   				//允许更新中断				
// 	TIM6->CR1 |= 0x01;    					//使能定时器6
//	MY_NVIC_Init(0, 0, TIM6_IRQn, 2);		//抢占1，子优先级2，组2		
}
