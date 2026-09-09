#include "delay.h"
#include "modbus.h"
#include "ES51932_Driver.h"

#define ID 254
#define SOFTVER_VER 3

#if 1
u16 update_flash_status = 0; 

u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
u8 uart_send[USART_SEND_LEN] ;
vu8 transmit_finished = 0 ; 
vu8 revce_count = 0 ;
vu8 rece_size = 0 ;
vu8 serial_receive_timeout_count ;
u8 SERIAL_RECEIVE_TIMEOUT ;
u8 dealwithTag ;
u16 sendbyte_num = 0 ;
u16 uart_num = 0 ;
u8  Station_NUM= 12;
u8 tx_count;
u8 rx_count;
 
u16 test[300];


void USART1_IRQHandler(void)                	//串口1中断服务程序
{		
	u8 receive_buf ;
	static u16 send_count = 0 ;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)	//接收中断
	{

	if(revce_count < 250)
		USART_RX_BUF[revce_count++] = USART_ReceiveData(USART1);//(USART1->DR);		//读取接收到的数据
		else
			 serial_restart();
		if(revce_count == 1)
		{
			// This starts a timer that will reset communication.  If you do not
			// receive the full packet, it insures that the next receive will be fresh.
			// The timeout is roughly 7.5ms.  (3 ticks of the hearbeat)
			rece_size = 250;
			serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;
		}
		else if(revce_count == 4)
		{
			//check if it is a scan command
			if((((vu16)(USART_RX_BUF[2] << 8) + USART_RX_BUF[3]) == 0x0a) && (USART_RX_BUF[1] == WRITE_VARIABLES))
			{
				rece_size = DATABUFLEN_SCAN;
				serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;	
			}
		}
		else if(revce_count == 7)
		{
			if((USART_RX_BUF[1] == READ_VARIABLES) || (USART_RX_BUF[1] == WRITE_VARIABLES))
			{
				rece_size = 8;
				//dealwithTag = 1;
			}
			else if(USART_RX_BUF[1] == MULTIPLE_WRITE)
			{
				rece_size = USART_RX_BUF[6] + 9;
				serial_receive_timeout_count = USART_RX_BUF[6] + 8;
			}
			else
			{
				rece_size = 250;
			}
		}
		else if(revce_count == rece_size)		
		{
			// full packet received - turn off serial timeout
			serial_receive_timeout_count = 0;
			dealwithTag = 2;		// making this number big to increase delay
			rx_count = 2 ;
		}

	}
	else  if( USART_GetITStatus(USART1, USART_IT_TXE) == SET  )
	{		
		if( send_count > sendbyte_num)
		{
			 USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			 send_count = 0 ;
//			 Timer_Silence_Reset();
			 serial_restart();
		}
		else
		{
			USART_SendData(USART1, uart_send[send_count++] );
//			Timer_Silence_Reset();
		}
	}
}

void serial_restart(void)
{
	TXEN = RECEIVE;
	revce_count = 0;
	dealwithTag = 0;
} 

//it is ready to send data by serial port . 
static void initSend_COM(void)
{
	TXEN = SEND;
}

void send_byte(u8 ch, u8 crc)
{	
	USART_ClearFlag(USART1, USART_FLAG_TC); 
	USART_SendData(USART1,  ch);
	tx_count = 2 ;
	if(crc)
	{
		crc16_byte(ch);
	}
}

 void USART_SendDataString( u8 num )
 {
	 tx_count = 2 ;
	 sendbyte_num = num;
	 uart_num = 0 ;
   USART_ITConfig(USART1, USART_IT_TXE, ENABLE);//
 }
void modbus_init(void)
{
	//uart1_init(19200);
	serial_restart();
	SERIAL_RECEIVE_TIMEOUT = 3;
	serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;

//	laddress = read_eeprom(EEP_ADDRESS);
//	if((laddress == 255) || (laddress == 0))
//	{
//		laddress = 254;
//	}
//	laddress = 254;
//	update_flash = 0;
}

void SoftReset(void)
{
	__set_FAULTMASK(1);      // 关闭所有中断
	NVIC_SystemReset();      // 复位
}

void internalDeal(u16 start_address)
{

	u8 address_temp ;

	if(USART_RX_BUF[1] == WRITE_VARIABLES)
	{
		if(start_address  <= 1000  )
		{		
			if(start_address == 16 )
			{  
				update_flash_status = USART_RX_BUF[5]; 
			}	
			else  // write test
			{
				test[start_address] = USART_RX_BUF[5] + USART_RX_BUF[4] * 256;
			}
			// If writing to Serial number Low word, set the Serial number Low flag			
//			if(USART_RX_BUF[3] <= MODBUS_SERIALNUMBER_LOWORD+1)
//			{	
//				modbus.serial_Num[0] = USART_RX_BUF[5] ;
//				modbus.serial_Num[1] = USART_RX_BUF[4] ;
//				modbus.SNWriteflag |= 0x01;
//			}
//			// If writing to Serial number High word, set the Serial number High flag
//			else if(USART_RX_BUF[3] <= MODBUS_SERIALNUMBER_HIWORD+1)
//			{		
//				modbus.serial_Num[2] = USART_RX_BUF[5] ;
//				modbus.serial_Num[3] = USART_RX_BUF[4] ;
//				modbus.SNWriteflag |= 0x02;
//			}
//			else if(USART_RX_BUF[3] <= MODBUS_VERSION_NUMBER_LO+1)
//			{	
//				modbus.software = (USART_RX_BUF[5]<<8) ;
//				modbus.software |= USART_RX_BUF[4] ;				
//			}
//			else if(USART_RX_BUF[3] == MODBUS_ADDRESS )
//			{
//				modbus.address	= USART_RX_BUF[5] ;
//			}
		}
	}
	if (update_flash_status == 0x7f)//0X7F
	{
		 SoftReset();	
	}
}
static void responseData(u16 start_address)
{
	u8 num, i, temp1, temp2;
	u16 send_cout = 0 ;
	if(USART_RX_BUF[1] == WRITE_VARIABLES)
	{
		for(i = 0; i < rece_size; i++)
		{
			uart_send[send_cout++] = USART_RX_BUF[i] ;
		}
		USART_SendDataString(send_cout);
	}
	else if(USART_RX_BUF[1] == MULTIPLE_WRITE)
	{
		for(i = 0; i < 6; i++)
		{
			 uart_send[send_cout++] = USART_RX_BUF[i] ;
			crc16_byte(USART_RX_BUF[i]);
		}
		uart_send[send_cout++] = CRChi ;
		uart_send[send_cout++] = CRClo ;
		USART_SendDataString(send_cout);		
	}
	else if(USART_RX_BUF[1] == READ_VARIABLES)
	{
		u16 address;
		u16 address_temp ;
		num = USART_RX_BUF[5];		
		uart_send[send_cout++] = USART_RX_BUF[0] ;
		uart_send[send_cout++] = USART_RX_BUF[1] ;
		uart_send[send_cout++] = (USART_RX_BUF[5]<<1) ;
		crc16_byte(USART_RX_BUF[0]);
		crc16_byte(USART_RX_BUF[1]);
		crc16_byte((USART_RX_BUF[5]<<1));
		for(i = 0; i < num; i++)
		{
			address = start_address + i;
			if(address == 16)
			{
				temp1 = 0 ;
				temp2 = update_flash_status; 
				uart_send[send_cout++] = temp1 ;
				uart_send[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == 4)
			{
				uart_send[send_cout++] = 0 ;
				uart_send[send_cout++] = SOFTVER_VER;
				crc16_byte(0);
				crc16_byte(SOFTVER_VER);
			}
			else if(address == 6)
			{				
				uart_send[send_cout++] = 0 ;
				uart_send[send_cout++] = ID;
				crc16_byte(0);
				crc16_byte(ID);
			}
			else if(address == 7)
			{				
				uart_send[send_cout++] = 0 ;
				uart_send[send_cout++] = 101;
				crc16_byte(0);
				crc16_byte(101);
			}
			else
			{
				temp1 = test[address] >> 8 ;
				temp2 = test[address]; 
				uart_send[send_cout++] = temp1 ;
				uart_send[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}

		}//end of number
		temp1 = CRChi ;
		temp2 =  CRClo; 
		uart_send[send_cout++] = temp1 ;
		uart_send[send_cout++] = temp2 ;
		USART_SendDataString(send_cout);
	}
	else if(USART_RX_BUF[1] == CHECKONLINE)
	{

	}
}


u8 checkData(u16 address)
{
	//static unsigned char xdata rand_read_ten_count = 0 ;
	u16 crc_val;
	//u8 minaddr,maxaddr, variable_delay;
	//u8 i;
	// check if packet completely received
	if(revce_count != rece_size)
		return 0;

	// check if talking to correct device ID
	if(USART_RX_BUF[0] != 255 && USART_RX_BUF[0] != ID && USART_RX_BUF[0] != 0)
		return 0;	

	//  --- code to verify what is on the network ---------------------------------------------------
//	if( USART_RX_BUF[1] == CHECKONLINE)
//	{
//		crc_val = crc16(USART_RX_BUF,4) ;
//		if(crc_val != (USART_RX_BUF[4]<<8) + USART_RX_BUF[5] )
//		{
//			return FALSE;
//		}
//		minaddr = (USART_RX_BUF[2] >= USART_RX_BUF[3] ) ? USART_RX_BUF[3] : USART_RX_BUF[2] ;	
//		maxaddr = (USART_RX_BUF[2] >= USART_RX_BUF[3] ) ? USART_RX_BUF[2] : USART_RX_BUF[3] ;	
//		if(info[6] < minaddr || info[6] > maxaddr)
//			return FALSE;
//		else
//		{	// in the TRUE case, we add a random delay such that the Interface can pick up the packets
//			srand(heart_beat);
//			variable_delay = rand() % 20;
//			for ( i=0; i<variable_delay; i++)
//				delay_us(100);
//	
//			return TRUE;
//		}

	//}
	// ------------------------------------------------------------------------------------------------------



	// check that message is one of the following
	if( (USART_RX_BUF[1]!=READ_VARIABLES) && (USART_RX_BUF[1]!=WRITE_VARIABLES) && (USART_RX_BUF[1]!=MULTIPLE_WRITE) )
		return 0;
	// ------------------------------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------------------------------
		
//	if(USART_RX_BUF[2]*256 + USART_RX_BUF[3] ==  FLASH_ADDRESS_PLUG_N_PLAY)
//	{
//		if(USART_RX_BUF[1] == WRITE_VARIABLES)
//		{
//			if(USART_RX_BUF[6] != info[0]) 
//			return FALSE;
//			if(USART_RX_BUF[7] != info[1]) 
//			return FALSE;
//			if(USART_RX_BUF[8] != info[2])  
//			return FALSE;
//			if(USART_RX_BUF[9] != info[3]) 
//			return FALSE;
//		}
//		if (data_buffer[1] == READ_VARIABLES)
//		{
//			randval = rand() % 10 / 2 ;
//		}
//		if(randval != RESPONSERANDVALUE)
//		{
////mhf:12-29-05 if more than 5 times does not response read register 10,reponse manuly.
//			rand_read_ten_count++;
//			if(rand_read_ten_count%5 == 0)
//			{
//				rand_read_ten_count = 0;
//				randval = RESPONSERANDVALUE;
//				variable_delay = rand() % 10;
//				for ( i=0; i<variable_delay; i++)
//					delay_us(75);
//			}
//			else
//				return FALSE;
//		}
//		else
//		{		
//			// in the TRUE case, we add a random delay such that the Interface can pick up the packets
//			rand_read_ten_count = 0;
//			variable_delay = rand() % 10;
//			for ( i=0; i<variable_delay; i++)
//				delay_us(75);				
//		}
//		
//	}

	// if trying to write the Serial number, first check to see if it has been already written
	// note this does not take count of multiple-write, thus if try to write into those reg with multiple-write, command will accept
//	if( (USART_RX_BUF[1]==WRITE_VARIABLES)  && (address<= FLASH_HARDWARE_REV) )
//	{
//		// Return false if trying to write SN Low word that has already been written
//		if(data_buffer[3] < 2)
//		{
//			if(SNWriteflag & 0x01)                // low byte of SN writed
//				return FALSE;
//		}
//		// Return false if trying to write SN High word that has already been written
//		else if (data_buffer[3] < 4)
//		{
//			if(SNWriteflag & 0x02)                 // high byte of SN writed
//				return FALSE;
//		}
//		else if (data_buffer[3] ==  FLASH_HARDWARE_REV)
//		{
//			if(SNWriteflag & 0x04)                 // hardware byte writed
//				return FALSE;
//		}

//	}


	crc_val = crc16(USART_RX_BUF, rece_size-2);

	if(crc_val == (USART_RX_BUF[rece_size-2]<<8) + USART_RX_BUF[rece_size-1] )
	{
		return 1;
	}
	else
	{
		return 0;
	}
	//return TRUE;

 }

 
 
 void dealwithData(void)
{	
	u16 address;
	// given this is used in multiple places, decided to put it as an argument
	address = (u16)(USART_RX_BUF[2]<<8) + USART_RX_BUF[3];
	if (checkData(address))
	{		
//		// Initialize tranmission
		initSend_COM();	
		// Initialize CRC
		init_crc16();		
//		// Store any data being written
		internalDeal(address);
//		// Respond with any data requested
		responseData(address);


	}
	else
	{
		serial_restart();
	}
}


#endif