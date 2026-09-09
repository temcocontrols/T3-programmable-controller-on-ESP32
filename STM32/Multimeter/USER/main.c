#include <string.h>
#include "stm32f10x.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "dma.h"
#include "timerx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "modbus.h"
#include "inputs.h"
#include "switch.h"
#include "output.h"
#include "ES51932_Driver.h"
#include "hy3131_spi.h"

void vCOMMTask(void *pvParameters );
void vI2cTask(void *pvParameters );
void vINPUTSTask( void *pvParameters );
void vOutputTask(void *pvParameters );

void SHT4x_Initial(void);
void Refresh_SHT4x(void);

#define MAX_DEAD_TIMER 20//120
u8 flag_ISP;

extern u32 	comm_heartbeat;
extern u8  flag_Comm;
extern u16 count_comm;
void vStartKeyTasks( unsigned char uxPriority);
void vRefreshSensorTask(void *pvParameters );

void I2C_TX_buffer(void);
u8 flag_led;

//static void debug_config(void)
//{
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB /*| RCC_APB2Periph_GPIOA*/, ENABLE);

//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

//}
static void debug_config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;			
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  						
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);										
	GPIO_ResetBits(GPIOA, GPIO_Pin_13 | GPIO_Pin_14);
}


void watchdog_init(void)
{
		/* Enable write access to IWDG_PR and IWDG_RLR registers */ 
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
		/* IWDG counter clock: 40KHz(LSI) / 4 = 10 KHz */ 
		IWDG_SetPrescaler(IWDG_Prescaler_4); 
		/* Set counter reload value to 10000 = 1s */ 
		IWDG_SetReload(30000); 
		IWDG_ReloadCounter(); // reload the value
		IWDG_Enable();  			//enable the watchdog

}

#define I2C1_SLAVE_ADDRESS7    (0x74 << 1)

void I2C_Initial(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	vu8 ReceivedCommand = 0, PECValue = 0;
	
  I2C_Cmd(I2C1, ENABLE);

  /* I2C2 configuration: SMBus Device ----------------------------------------*/
  I2C_InitStructure.I2C_Mode = I2C_Mode_SMBusDevice;
	I2C_InitStructure.I2C_ClockSpeed = 100000;	
  I2C_InitStructure.I2C_OwnAddress1 = I2C1_SLAVE_ADDRESS7;	
	
  I2C_Init(I2C1, &I2C_InitStructure);

  /* Enable I2C1 ARP */
  I2C_ARPCmd(I2C1, ENABLE);

  /* Enable I2C1 PEC Transmission */
  I2C_CalculatePEC(I2C1, ENABLE);

  /* Get I2C1 SMBDEFAULT flag status */
  //Status = 
	I2C_GetFlagStatus(I2C1, I2C_FLAG_SMBDEFAULT); 

  /* Clear ADDR flag */
  I2C_ClearFlag(I2C1, I2C_FLAG_ADDR); 
  /* Wait for I2C2 received data */
  while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE)); 
  /* Store received data on I2C2 */
  ReceivedCommand = I2C_ReceiveData(I2C1);
	
	
  /* Enable Transfer PEC next for I2C1 */
  I2C_TransmitPEC(I2C1, ENABLE);
  /* Wait for I2C1 received data */
  while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));  
  /* Store received PEC on I2C2 */
  PECValue = I2C_ReceiveData(I2C1);
	
  /* Test on I2C1 EV4 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_STOP_DETECTED));
  /* Clear I2C1 STOPF flag */
  I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);


}
void I2C1_Slave_Init(void);

int main(void)
{
//	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
//	debug_config();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	delay_init(72);	

	flag_ISP = IN_NORMAL;//0;
	//LED_Init();	
	//Output_Init();
	TIM6_Int_Init(5, 7199); // 500 us	
	I2C1_Slave_Init();
	watchdog_init();
//	USART_Configuration();
//	GPIO_Configuration();
	ES51932_Init();
 	xTaskCreate( vCOMMTask, ( signed portCHAR * ) "COMM", configMINIMAL_STACK_SIZE + 500, NULL, tskIDLE_PRIORITY, NULL );
  	// create HY3131 measurement task (SPI sensor)
 // 	xTaskCreate( HY3131_Task, ( signed portCHAR * ) "HY3131", configMINIMAL_STACK_SIZE + 400, NULL, tskIDLE_PRIORITY + 2, NULL );
//	xTaskCreate( vINPUTSTask, ( signed portCHAR * ) "INPUT", configMINIMAL_STACK_SIZE + 200, NULL, tskIDLE_PRIORITY + 2, NULL );
	// output
	//xTaskCreate( vI2cTask, ( signed portCHAR * ) "Output", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 6, NULL );
	
	vStartKeyTasks(tskIDLE_PRIORITY + 6);
//	xTaskCreate( vRefreshSensorTask, ( signed portCHAR * ) "RefreshSensor", configMINIMAL_STACK_SIZE + 500, NULL, tskIDLE_PRIORITY + 1, NULL );
	/* Start the scheduler. */
	vTaskStartScheduler();
}

extern uint8_t i2c_rcv[];

void multiMeterTask(void *pvParamters)
{
	for(;;)
	{
		if(i2c_rcv[0] == 0x71)
		{
			GPIO_SetBits(GPIOE, GPIO_Pin_0);
			GPIO_ResetBits(GPIOB, GPIO_Pin_1);
			GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
		}
		if(i2c_rcv[1] == 0x03)
		{
			GPIO_ResetBits(GPIOE, GPIO_Pin_2);
			GPIO_SetBits(GPIOB, GPIO_Pin_0);
			GPIO_ResetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
		}
		delay_ms(20);
		IWDG_ReloadCounter();	
	}
}

void Refresh_SCD40(void);
void Check_Voc(void);
void VOC_Init(void);
void SCD40_Initial(void);
void vRefreshSensorTask(void *pvParameters )
{	
	SHT4x_Initial();
	VOC_Init();
	SCD40_Initial();
	for( ;; )
	{
		test[20]++;		
		Refresh_SCD40();
		delay_ms(50);	IWDG_ReloadCounter();			
		Refresh_SHT4x();
		delay_ms(50) ;IWDG_ReloadCounter();		
		Check_Voc();
		delay_ms(50) ;IWDG_ReloadCounter();		
	}	
}

void vCOMMTask(void *pvParameters )
{	
	uart1_init(115200);
	modbus_init();
	for( ;; )
	{
		test[1]++;
		if (dealwithTag)
		{  
		 dealwithTag--;
		  if(dealwithTag == 1)//&& !Serial_Master )	
			dealwithData();
		}
		if(serial_receive_timeout_count>0)  
		{
			serial_receive_timeout_count -- ; 
			if(serial_receive_timeout_count == 0)
			{
				serial_restart();
			}
		}
		
		//retboot it 
		IWDG_ReloadCounter();		
		delay_ms(5) ;
	}
	
}


// i2c communication with ESP chip
void vI2cTask(void *pvParameters )
{		
	I2C1_Slave_Init();
	for( ;; )
	{
		// I2C roution.
		GPIO_SetBits(GPIOB, GPIO_Pin_3);
		GPIO_ResetBits(GPIOB, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9);
		GPIO_ResetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8);
		
		I2C_TX_buffer();
		delay_ms(50) ;
	}
	
}


extern uint8_t hum_exists;
int16_t sht4x_probe(void);
void vINPUTSTask( void *pvParameters )
{
	u8 i ;
	uint16_t count_1s = 0;
	for(i = 0; i < MAX_AI_CHANNEL; i++)
	{
		AD_Value[i] = 9 ;	
	}
	
	inputs_init();
//	SHT4x_Initial();
	
	for( ;; )
	{
		delay_ms(20);
		inpust_scan();		
		test[29] = hum_exists;
		//I2C_TX_buffer();	
		IWDG_ReloadCounter();
	}	
}
