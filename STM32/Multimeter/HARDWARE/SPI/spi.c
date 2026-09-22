#include "spi.h"
#include <string.h>
#include "inputs.h"


#if NEW_BB

#define HW_REV 28 

#else

#define HW_REV 27 

#endif

#define SW_REV 48  // ARM board, start from 30, less than 30 is C8051 board

typedef  enum 
{
	/* 	send	*/
	C_INITIAL = 0,	

	S_OUTPUT_LED = 0x10, 	/* 0x10 + 24 bytes */
	S_INPUT_LED = 0x11, 	/* 0x11 + 32 bytes */
	S_HI_SP_FLAG = 0x12,  	/* 0x12 + 6 bytes  */	
	S_COMM_LED	= 0x013,	/* 0x13 + 6 bytes  */
	S_ALL = 0x14,		   //  64
	S_ALL_NEW = 0x15,		   //  64
	
	G_SWTICH_STATUS = 0x20,	/* 0x20 + 24 bytes */
	G_INPUT_VALUE = 0x21,	/* 0x21 + 64 bytes */
	G_TOP_CHIP_INFO	= 0x23, /* 0x21 + 12 bytes */
	G_SPEED_COUNTER = 0x30,	 // 112
	G_ALL = 0x24,
	G_ALL_NEW = 0x25,

	C_MINITYPE = 0x80,
	C_ASIX_ISP = 0X81,
	C_END = 255

};




u8 command;
u8 state;
u8 array_index;
u32 comm_heartbeat = 0;
u8  flag_Comm = 0;
u16 count_comm;
u8 flag_ISP; // 0 - intial 1 - isp 2 - normal
u8 Mini_Type;
//u8 flag_Start;


u8 high_speed_flag[HI_COMMON_CHANNEL];  // 0 - clear  1 - start  2 - stop
UN_HIGH_COUNT high_speed_counter[HI_COMMON_CHANNEL];

extern u16 test[200];
extern u8 Switch_Status[24];
extern vu16 AD_Value[32];
u8  RX_SPI_BUF[56];
extern u8 range[32];
extern u8 LED_status[67];
extern u16 relay_value;

//**** SPI1 ****************
void SPI1_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);	//PORTA?SPI1???? 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;			//PA5-SCK, PA6-MISO, PA7-MOSI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  							//PA5/6/7?????? 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);										//???GPIOA
//	GPIO_SetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);  				//PA5/6/7??

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;			//??SPI???????????:SPI??????????
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;								//??SPI????:????SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;							//??SPI?????:SPI????8????
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;									//???????????????
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;								//?????????????(?????)?????
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;									//NSS?????(NSS??)????(??SSI?)??:??NSS???SSI???
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;		//??????????:????????256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;							//???????MSB???LSB???:?????MSB???
	SPI_InitStructure.SPI_CRCPolynomial = 7;									//CRC???????
	SPI_Init(SPI1, &SPI_InitStructure);											//??SPI_InitStruct???????????SPIx???
 
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);  //????????,????NVIC?????,1??1bit?????
 
  NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; 
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
    
	/* Enable SPI1 RXNE interrupt */    
	SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_RXNE,ENABLE); 

  //Enable SPI1 
  SPI_Cmd(SPI1, ENABLE); //????SPI,?????????????
 
//	flag_Start = 0;

	memset(high_speed_flag,0,HI_COMMON_CHANNEL);
	memset(high_speed_counter,0,sizeof(UN_HIGH_COUNT)*HI_COMMON_CHANNEL);
	
}

//SPI1??????
//SPI_BaudRatePrescaler_2   2??   
//SPI_BaudRatePrescaler_8   8??   
//SPI_BaudRatePrescaler_16  16??  
//SPI_BaudRatePrescaler_256 256?? 
//void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
//{
//	assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));		//????
//	SPI1->CR1 &= 0XFFC7; 
//	SPI1->CR1 |= SPI_BaudRatePrescaler;									//??SPI1??  
//	SPI_Cmd(SPI1, ENABLE);												//SPI1????	  
//}

//SPI1 ??????
//TxData:??????
//???:??????
u8 SPI1_ReadWriteByte(u8 TxData)
{		
	u16 retry = 0;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
	{
		retry++;
		if(retry > 0xffff) return 0;
	};
	SPI_I2S_SendData(SPI1, TxData);
	
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
	{
		retry++;
		if(retry > 0xffff) return 0;
	};
	return SPI_I2S_ReceiveData(SPI1);
}

void hearbeat_spi_reset(void)
{
	comm_heartbeat = 0;
	flag_Comm = 1;
	count_comm = 0;
}

u8 spi_rx[100];
u8 spi_tx[200];

void SPI_TX_buffer(void)
{
#if (BB || NEW_BB)
	u8 index;
	vu16 crc;
	vu16 count = 0;
	u8 spi_tx_temp[200];
	count = 0;
		
	index = 0;
	do
	{
		if(Mini_Type == BIG || Mini_Type == SMALL)
		{	
			if(index < 24)   // switch
			{
				spi_tx_temp[index] = Switch_Status[index];
			}
			else if(index < 88)   // input value
			{	
				if((index - 24) % 2 == 0)
				{
					spi_tx_temp[index] = (u8)(AD_Value[(index - 24) / 2] >> 8);
				}
				else 
				{
					spi_tx_temp[index] = (u8)(AD_Value[(index - 24) / 2]);
				}
			}
			else // high_speed_counter
			{
				spi_tx_temp[index] = high_speed_counter[(index - 88) / 4].byte[(index - 88) % 4];
			}
		}
		else   // if dont get mini type, return 0X55
			spi_tx_temp[index] = 0x55;
	}while(index++ < 112);
	
	crc = crc16(spi_tx_temp,112);
	spi_tx_temp[112] = crc / 256;
	spi_tx_temp[113] = crc % 256;
	
	memcpy(spi_tx,spi_tx_temp,114);
#endif	

}
extern u8 flag_PT;
void SPI1_IRQHandler(void) 
{ 
    // ????
		u8 i;
		// receive correct command
		u16 crc;
   if(state == 0)
   {		  
	    command = SPI_I2S_ReceiveData(SPI1);			
			if((command == 0x55) || (command == 0xaa))
			{test[2] = command;
				test[3]++;
				hearbeat_spi_reset();
				flag_ISP = IN_ISP;	
			}
			else
			{
				flag_ISP = IN_NORMAL;
			}
		 
			if(command == G_TOP_CHIP_INFO)	 {	state = 1;  }
			else if(command == C_MINITYPE)		{state = 1; test[46]++;}
			else if((command == S_ALL) || (command == S_ALL_NEW))			{state = 1; test[47]++;}
			else if((command == G_ALL) ||	(command == G_ALL_NEW))			{state = 1;test[48]++;
		  }
			
			array_index = 0;
	 }
	 else if(state == 1)
	 {
#if (BB || NEW_BB)
		 
	 	if((command == G_ALL_NEW) || (command == G_ALL))
		{
			if(array_index >= 113) 
			{	
				if(command == G_ALL_NEW)
					SPI1_ReadWriteByte(spi_tx[array_index]);
				else
					SPI1_ReadWriteByte(0xaa);
				array_index = 0; 
				state = 0;
				command = 0;
				
				// receive correct command
				hearbeat_spi_reset();
			}
			else 
			{
				if((command == G_ALL) && (array_index == 112))
				{
					SPI1_ReadWriteByte(0x55);
				}
				else
				{
					SPI1_ReadWriteByte(spi_tx[array_index]);
				}
				array_index++;
				state = 1;
			}
		}
		if((command == S_ALL_NEW) || (command == S_ALL))  /* input led status */	
		{
			vu16 crc;			
			spi_rx[array_index]	= SPI_I2S_ReceiveData(SPI1);
			
			if(array_index >= 65) 	
			{
				if(command == S_ALL_NEW)
				{
					crc = crc16(spi_rx,64);
				}
				else
					crc = 0x55aa;  // old command
				
				array_index = 0; 
				state = 0;
				command = 0;  
				
				test[90]++;
				test[91] = crc;
				test[92] = spi_rx[64];
				test[93] = spi_rx[65];
				if((spi_rx[64] == (crc / 256)) && (spi_rx[65] == (crc % 256)))  // crc ok
				//if(spi_rx[64] == 0x55 && spi_rx[65] == 0xaa)  // crc ok
				{					
					//memcpy(&comm_led[0],&spi_rx[0],2);
					// LED status of COMM					
					if(Mini_Type == BIG)
					{
						
					for(i = 0;i < 8;i++)
						LED_status[56 + i] = (spi_rx[0] & (0x01 << i)) ? 5 : 0;
					for(i = 0;i < 3;i++)
						LED_status[64 + i] = (spi_rx[1] & (0x01 << i)) ? 5 : 0;
			// 2 + 24 + 32 + 6  = 64
			// 2  - communication led
			// 24 - led of outputs status 
			// 32 - inputs status, types and led
			// -> first 4 bit for inputs type, last 4 bits for input led status,added in new hardware with INPUT moudle
			// 6  - flag of high speed inputs
						
						memcpy(&RX_SPI_BUF[0],&spi_rx[26],32);
						memcpy(&RX_SPI_BUF[32],&spi_rx[2],24);
						for(i = 0;i < 32;i++)
						{
							range[i] = (RX_SPI_BUF[i] >> 4) & 0x0f;
						}
						for(i = 0;i < 56;i++)
						{
							LED_status[i] = RX_SPI_BUF[i] & 0x0f;
						}
						for(i = 0;i < HI_COMMON_CHANNEL;i++)  // HI_COMMON_CHANNEL 6
						{	
							if(spi_rx[58 + i]  != high_speed_flag[i])
							{
								high_speed_flag[i] = spi_rx[58 + i];
								if(high_speed_flag[i] == 1)  // start counter
								{
									if(range[26 + i] == Thermistor) // thermsitor  
									{
										pulse_set(i,RISE);									
									}
									else if(range[26 + i] == V0_5)// 0-5v, 
									{
										pulse_set(i,FALL);
									}
								}
								if(high_speed_flag[i] == 2)	
									high_speed_counter[i].longbyte = 0;
							}
						}
						
					}
					else if(Mini_Type == SMALL)
					{
						LED_status[11] = (spi_rx[0] & (0x01 << 0)) ? 5 : 0;
						LED_status[12] = (spi_rx[0] & (0x01 << 1)) ? 5 : 0;
						
						LED_status[13] = (spi_rx[0] & (0x01 << 2)) ? 5 : 0;
						LED_status[14] = (spi_rx[0] & (0x01 << 3)) ? 5 : 0;
						
						LED_status[32] = (spi_rx[0] & (0x01 << 4)) ? 5 : 0;
						LED_status[33] = (spi_rx[0] & (0x01 << 5)) ? 5 : 0;
						
						LED_status[36] = (spi_rx[0] & (0x01 << 6)) ? 5 : 0;
						LED_status[37] = (spi_rx[0] & (0x01 << 7)) ? 5 : 0;
						
						LED_status[34] = (spi_rx[1] & (0x01 << 0)) ? 5 : 0;
						LED_status[35] = (spi_rx[1] & (0x01 << 1)) ? 5 : 0;						
						
			// 2 + 10 + 16 + 6  = 64
			// 2  - communication led
			// 10 - led of outputs status 
			// 16 - inputs status, types and led
//						-> first 4 bit for inputs type, last 4 bits for input led status,added in new hardware with INPUT moudle
			// 6  - flag of high speed inputs
						memcpy(&RX_SPI_BUF[0],&spi_rx[2],10);
						memcpy(&RX_SPI_BUF[10],&spi_rx[26],16); 
						for(i = 0;i < 16;i++)
							range[i] = ((RX_SPI_BUF[10 + i] >> 4) & 0x0f);
							
						for(i = 0;i < 10;i++)
						{  // output led
							LED_status[1 + i] = RX_SPI_BUF[i] & 0x0f;
						}
						for(i = 0;i < 16;i++)
						{  // input led
							LED_status[16 + i] = RX_SPI_BUF[10 + i] & 0x0f;
						}
						
						for(i = 0;i < HI_COMMON_CHANNEL;i++)  // HI_COMMON_CHANNEL 6
						{	
							if(spi_rx[58 + i]  != high_speed_flag[i])
							{
								high_speed_flag[i] = spi_rx[58 + i];
								if(high_speed_flag[i] == 1)  // start counter
								{
									if(range[10 + i] == Thermistor) // thermsitor  
									{
										pulse_set(i,RISE);									
									}
									else if(range[10 + i] == V0_5)// 0-5v, 
									{
										pulse_set(i,FALL);
									}
								}
								if(high_speed_flag[i] == 2)	
									high_speed_counter[i].longbyte = 0;
							}
						}
					}		
					// receive correct command
					hearbeat_spi_reset();
				} 
			}
			else 
			{
				array_index++;
				state = 1;
			}
		}
		if(command == G_TOP_CHIP_INFO)  /*send input value */	
		{		
			if(Mini_Type == 0)	   // do not get mini type
			{
				
				if(array_index < 12)
				{
				   	SPI1_ReadWriteByte(0x55);
				}
			}
			else
			{
				if(array_index < 12)
				{		
					if(array_index == 1)
					  SPI1_ReadWriteByte(HW_REV);
					else if(array_index == 3)
					  SPI1_ReadWriteByte(SW_REV);
					else if(array_index == 5)
						SPI1_ReadWriteByte(flag_PT);
					else 
						SPI1_ReadWriteByte(0);
				}
			}
			
			if(array_index == 12)
				SPI1_ReadWriteByte(0x55);		
			if(array_index == 13)
				SPI1_ReadWriteByte(0xaa);
			
			if(array_index >= 13) 
			{	
				test[19]++;
				array_index = 0; 
				state = 0;
				command = 0;
				// receive correct command
				hearbeat_spi_reset();
			}
			else 
			{  				 
				array_index++;
				state = 1;
			}					 
		}		
#endif
		
		
#if TB
	 	if(command == G_ALL)
		{
//			flag_Start = 1;
			if(array_index < 62)   // 8 + 22 + 24 + 8
			{
				if(array_index < 8)   // switch
				{
					SPI1_ReadWriteByte(Switch_Status[array_index]);
				}
				else if(array_index < 30)   // input value
				{	 
					if((array_index - 8) % 2 == 0)
						SPI1_ReadWriteByte((u8)(AD_Value[(array_index - 8) / 2] >> 10));
					else 
						SPI1_ReadWriteByte((u8)(AD_Value[(array_index - 8) / 2] >> 2));		
				}
				else if(array_index < 54) // high_speed_counter
				{
					SPI1_ReadWriteByte(high_speed_counter[(array_index - 30) / 4].byte[(array_index - 30) % 4]);
				}
				else// if(array_index < 62)
				{
					if((array_index - 54) % 2 == 0)
						SPI1_ReadWriteByte((u8)(AO_Feedback[(array_index - 54) / 2] >> 10));
					else 
						SPI1_ReadWriteByte((u8)(AO_Feedback[(array_index - 54) / 2] >> 2));	
				}
			}
			else if(array_index == 62)
			   	SPI1_ReadWriteByte(0x55);
			else if(array_index == 63)
			   	SPI1_ReadWriteByte(0xaa);
			
			if(array_index >= 63) 
			{		
				array_index = 0; 
				state = 0;
				command = 0;
				// receive correct command
				hearbeat_spi_reset();
			}
			else 
			{
				array_index++;
				state = 1;
			}
		}
		if(command == S_ALL)  /* input led status */	
		{		
			spi_rx[array_index]	= SPI_I2S_ReceiveData(SPI1);
			if(array_index >= 34) 	
			{
				array_index = 0; 
				state = 0;
				command = 0;  
				if(spi_rx[33] == 0x55 && spi_rx[34] == 0xaa)  // crc ok
				{
					LED_status[8] = spi_rx[21]; //  main 485
					LED_status[9] = spi_rx[22];
					
					LED_status[10] = spi_rx[23]; // ethernet
					LED_status[11] = spi_rx[24];
					
					LED_status[24] = spi_rx[19];  //  sub 485
					LED_status[25] = spi_rx[20];

					for(i = 0;i < 8;i++)  // output status
					{
						LED_status[i] = spi_rx[i] & 0x0f;
					}
					// input led status ,high 4 bits is range
					for(i = 0;i < 11;i++)  
					{
						LED_status[13 + i] = spi_rx[8 + i] & 0x0f;
						range[i] = ((spi_rx[8 + i] >> 4) & 0x0f);	
					}	
					
					for(i = 0;i < HI_COMMON_CHANNEL;i++)  // HI_COMMON_CHANNEL 6
					{	
							
							if(spi_rx[25 + i]  != high_speed_flag[i])
							{
								high_speed_flag[i] = spi_rx[25 + i];
								if(high_speed_flag[i] == 1)  // start counter
								{
									if(range[5 + i] == Thermistor) // thermsitor  
									{
										pulse_set(i,RISE);									
									}
									else if(range[5 + i] == V0_5)// 0-5v, 
									{
										pulse_set(i,FALL);
									}
								}
								if(high_speed_flag[i] == 2)
										high_speed_counter[i].longbyte = 0;
								
								//test[20 + i] = high_speed_flag[i];
							}
						}
								// receive correct command
					hearbeat_spi_reset();			
				}
//				memcpy(&high_speed_flag[0],&spi_rx[25],6);
//				for(i = 0;i < 6;i++)  // tiny have 6 hsp
//					if(high_speed_flag[i] == 2)	high_speed_counter[i].longbyte = 0;
				
				

					
				
				
				
				relay_value = (spi_rx[32] * 256 + spi_rx[31]) & 0x3f;
			}
			else 
			{
				array_index++;
				state = 1;
			}
		}		
		if(command == G_TOP_CHIP_INFO)  /*send input value */	
		{		
			if(Mini_Type == 0)	   // do not get mini type
			{
				if(array_index < 12)
				{
				   	SPI1_ReadWriteByte(0x55);
				}
			}
			else
			{
				if(array_index < 12)
				{		
					if(array_index == 0)
						SPI1_ReadWriteByte(HW_REV);
					else if(array_index == 1)
					  SPI1_ReadWriteByte(SW_REV);					
					else
						SPI1_ReadWriteByte(0);
				}
			}
			
			if(array_index == 12)
				SPI1_ReadWriteByte(0x55);		
			if(array_index == 13)
				SPI1_ReadWriteByte(0xaa);

			if(array_index >= 13) 
			{	
				array_index = 0; 
				state = 0;
				command = 0;
				// receive correct command
				hearbeat_spi_reset();
			}
			else 
			{  				 
				array_index++;
				state = 1;
			}					 
		}		
#endif
		

		if(command == C_MINITYPE)	   // MINI TYPE
		{	
//			Mini_Type = SPI_I2S_ReceiveData(SPI1);
//			if(Mini_Type == BIG)
//				MAX_AI_CHANNEL = 32;
//			else
//			{
//				MAX_AI_CHANNEL = 16;
//				GPIO_ResetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11);
//			}
//			flag_Start = 1;
			state = 0;
		}
		
	 }  
	
} 
