 /*================================================================================
 * Module Name : key.c
 * Purpose     : key dirver and deal with back light and buzzer
 * Author      : Chelsea
 * Date        : 2008/11/10
 * Notes       : 
 * Revision	   : 
 *	rev1.0
 *================================================================================
 */


#define KEY
#ifdef KEY

#include "FreeRTOS.h"
#include "stm32f10x.h"
#include "task.h"
#include "queue.h"
#include "key.h"
#include "delay.h"

/* CONSTANT DECLARATIONS */

#define C_MAX_TIME       200
#define C_TRUE_TIME      1
#define C_HOLD_TIME_S    15
#define C_HOLD_TIME_M    30
#define C_HOLD_TIME_L 	 50



#define C_BEEP			 1        	/* last time when the buzzer is enabled */
#define	C_BACK		     1200		/* last time when the backlit is enabled */

#define 	KEY_QUEUE_SIZE 		3


/* GLOBAL VARIABLE DECLARATIONS */
U16_T w_Key_Count;
U8_T by_Key_Buffer;
U8_T b_PressKey;
U16_T w_Beep_Count;
U16_T w_Backlit_Count;
U8_T by_Key;			/* the value of button */
static U8_T by_shake_Count = 0;

extern u16 test[];
xTaskHandle xKeyTask;



void KEY_IO_config(void)
{	

	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

}




/*
 *--------------------------------------------------------------------------------
 * void Key_Inital(void)
 * Purpose : initial I/O port and datas of Key
 * Params  : none
 * Returns : none
 * Note    : 
 *--------------------------------------------------------------------------------
 */

// <summary>
//  Key_Inital: Fuction for initialing I/O port and datas of Key
//  </summary>
void Key_Inital(void)
{
	w_Key_Count = 0;
}


/*
 *--------------------------------------------------------------------------------
 * void vStartKeyTasks( unsigned char uxPriority)
 * Purpose : start the KEY_TASK and create a queue for key event
 * Params  : none
 * Returns : none
 * Note    : 
 *--------------------------------------------------------------------------------
 */

// <summary>
//  vStartKeyTasks : Function for starting the KEY_TASK and creating a queue for key event
//  </summary>
//  <param name="uxPriority"> the priority of this KEY_TASK </param>

void vStartKeyTasks( unsigned char uxPriority)
{	
	Key_Inital();
	xTaskCreate(Key_Process, (const signed portCHAR * const)"key_task",100, NULL, uxPriority, (xTaskHandle *)&xKeyTask);
}


u16 global_key = K_NONE;
u16 pre_key = K_NONE;



u8 KEY_Scan(void)
{	 
	u16 key_1st, key_2nd;
	
	u16 key_val = K_NONE;
	test[0]++;
	key_1st = ~GPIO_ReadInputData(GPIOA) & 0x7800; // PA11-14
	delay_ms(10);IWDG_ReloadCounter();		
	key_2nd = ~GPIO_ReadInputData(GPIOA) & 0x7800; // PA11-14
	
	if(key_1st & key_2nd & K_DOWN){test[22]++;
		key_val |= K_DOWN;}
	
	if(key_1st & key_2nd & K_UP){test[23]++;
		key_val |= K_UP;}
	
	if(key_1st & key_2nd & K_LEFT){test[24]++;
		key_val |= K_LEFT;}
	
	if(key_1st & key_2nd & K_RIGHT){test[25]++;
		key_val |= K_RIGHT;}
	
	return  (key_val >> 11);
}
 
extern void watchdog(void);
uint8_t g_key;
void I2C_TX_buffer(void);
xQueueHandle qKey;
void Key_Process(void ) 
{
	u16 key_temp;
	u16 count;
	static U8_T long_press_key_start = 0;
	qKey = xQueueCreate(5, 2);

 	KEY_IO_config();
//	print("Key Task\r\n");
	//delay_ms(100);
	for( ;; )
	{
		if((key_temp = KEY_Scan()) != pre_key)
		{test[30]++;
			if(pre_key == 0) // 避免单键和组合键粘连
			{test[31]++;
				//xQueueSend(qKey, &key_temp, 0);
				g_key = key_temp;
				I2C_TX_buffer();
			}
			pre_key = key_temp;
			long_press_key_start = 0;
		}
		else
		{
			if(key_temp != K_NONE)
			{	test[32]++;	
				if(long_press_key_start >= LONG_PRESS_TIMER_SPEED_10)
					key_temp |= KEY_SPEED_10;
				else if(long_press_key_start >= LONG_PRESS_TIMER_SPEED_1)
					key_temp |= KEY_SPEED_1;

				if(long_press_key_start >= LONG_PRESS_TIMER_SPEED_1)
				{test[33]++;
					g_key = key_temp;//
					//xQueueSend(qKey, &key_temp, 0);
					I2C_TX_buffer();
				}

				if(long_press_key_start < LONG_PRESS_TIMER_SPEED_100)
					long_press_key_start++;				
			}
			else
			{
				//g_key = 0;
				test[34]++;
				//if(count++ % 3 == 0){test[35]++;
						g_key = 0;//xQueueSend(qKey, &key_temp, 0);
				I2C_TX_buffer();
					//}
			}
		} 
		//vTaskDelay(10 / portTICK_RATE_MS);
  }
}

#endif






