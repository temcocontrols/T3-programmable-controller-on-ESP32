/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.r
*
*********************************************************************/

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
/*#include "mstp.h" */
//#include "uart.h"
#include <fifo.h>
#include "define.h"
//uint8_t Rec_Mstp_Byte;
//uint8_t Rec_Mstp_Err;

/* This file has been customized for use with ATMEGA168 */
//#include "hardware.h"
#include "timer.h"
#include "esp_attr.h"

void uart_send_string(uint8_t *p, uint16_t length,uint8_t port);
#define Tturnaround  (40UL)
extern volatile uint16_t SilenceTime;
extern uint16_t Test[50];
/* Timers for turning off the TX,RX LED indications */
/*static uint8_t LED1_Off_Timer;
static uint8_t LED3_Off_Timer; */
extern STR_MODBUS Modbus;
/* baud rate */
//static uint32_t RS485_Baud = 9600;

/*unsigned char xdata gucReceiveCount;
unsigned char xdata gucReceive_Index;
unsigned char xdata gucTake_Index; */
uint8_t  gbReceiveError;
uint8_t MSTP_Transmit_Finished;
/* buffer for storing received bytes - size must be power of two */
//static uint8_t Receive_Buffer_Data0[512];
//static FIFO_BUFFER Receive_Buffer0;
//static uint8_t Receive_Buffer_Data1[512];
//static FIFO_BUFFER Receive_Buffer1;
//uint8_t Receive_Buffer_Data2[512];



extern EXT_RAM_ATTR FIFO_BUFFER Receive_Buffer2;
extern EXT_RAM_ATTR uint8_t Receive_Buffer_Data2[512];

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void Recievebuf_Initialize(uint8_t port)
{
//	if(port == 0)
//	{
//	FIFO_Init(&Receive_Buffer0, &Receive_Buffer_Data0[0],
//         sizeof(Receive_Buffer_Data0)); 	
//	}
//	else if(port == 1)
//	{
//	FIFO_Init(&Receive_Buffer1, &Receive_Buffer_Data1[0],
//         sizeof(Receive_Buffer_Data1));
//	}
//	else if(port == 2)
//	{
	FIFO_Init(&Receive_Buffer2, &Receive_Buffer_Data2[0],
         sizeof(Receive_Buffer_Data2));
//	}

    return;
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
//uint32_t RS485_Get_Baud_Rate(
//    void)
//{
//    return RS485_Baud;
//}

/****************************************************************************
* DESCRIPTION: Sets the baud rate for the chip USART
* RETURN:      true if valid baud rate
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
//bool RS485_Set_Baud_Rate(
//    uint32_t baud)
//{
//    bool valid = true;
//	RS485_Initialize();	
// #if 0
//    switch (baud) {
//        case 9600:
//        case 19200:
//        case 38400:
//        case 57600:
//        case 76800:
//        case 115200:
//            RS485_Baud = baud;
//            /* 2x speed mode */
//            BIT_SET(UCSR0A, U2X0);
//            /* configure baud rate */
//            UBRR0 = (F_CPU / (8UL * RS485_Baud)) - 1;
//            /* FIXME: store the baud rate */
//            break;
//        default:
//            valid = false;
//            break;
//    }
//#endif
//    return valid;
//}

/****************************************************************************
* DESCRIPTION: Enable or disable the transmitter
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Transmitter_Enable(
    bool enable)
{
// ESP dont need it


}

/****************************************************************************
* DESCRIPTION: Waits on the SilenceTimer for 40 bits.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Turnaround_Delay(
    void)
{
	uint16_t turnaround_time = 0;
//	U8_T i;
//	uint16_t RS485_Baud;
	/* delay after reception before trasmitting - per MS/TP spec */
	/* wait a minimum  40 bit times since reception */
	/* at least 1 ms for errors: rounding, clock tick */
	// if baud is 9600, turnaround_time is 4
	// if baud if 19200, turnaround time is 2
	//    turnaround_time = 2 + ((Tturnaround * 1000UL) / RS485_Baud);
//	if(Modbus.com_config[0] == MAIN_MSTP)
//	{
//		if(uart0_baudrate == UART_9600)
//		{
//			turnaround_time = 6;
//		}
//		else if(uart0_baudrate == UART_19200)
//		{
//		    turnaround_time = 4;

//		}
//	}
//	else 
	//if(Modbus.com_config[2] == MAIN_MSTP)
	/*{
		switch(uart2_baudrate)
		{
			case UART_9600:	 
			turnaround_time = 6;
			break;
			case UART_19200:	 
			turnaround_time = 4;
			break;
			case UART_38400:
			turnaround_time = 3;
			break;		
//			case UART_57600:	 
//			case UART_115200:	 
//			case UART_921600:			
			default: 
			turnaround_time = 2;
			break;
			
		}
	}*/

	while (Timer_Silence() < turnaround_time) 
	{
	/* do nothing - wait for timer to increment */
	};
}


extern uint8_t uart0_config;

/****************************************************************************
* DESCRIPTION: Send some data and wait until it is sent
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Send_Data(
    uint8_t * buffer,   /* data to send */
    uint16_t nbytes)
{       /* number of bytes of data */
    /* send all the bytes */
	uint16_t count = 0;
	
//	if(Modbus.com_config[0] == MAIN_MSTP)
//	{
//	  while (nbytes) 
//		{
//			SBUF0 = *buffer;	
//	    MSTP_Transmit_Finished = 0;
//			count = 0;
//	    while (!MSTP_Transmit_Finished && count < 2500) {
//			count++;
//	            /* do nothing - wait until Tx buffer is empty */
//	        }
//			gbReceiveError = false;
//			if(count >= 2500)
//			{		
//				
//				gbReceiveError = true;
//			}
//				
//	        buffer++;
//	        nbytes--;
//	    }
//	}
	if(uart0_config == 1 || uart0_config == 9)  // for bootloader
	{
		uart_send_string(buffer,nbytes,0);
	}
	if(Modbus.com_config[2] == 9 || Modbus.com_config[2] == 1)
	{		
		uart_send_string(buffer,nbytes,2);
	}
	else if(Modbus.com_config[0] == 9 || Modbus.com_config[0] == 1)
	{
		uart_send_string(buffer,nbytes,0);
	}
    /* per MSTP spec */
	
  Timer_Silence_Reset();
}

/****************************************************************************
* DESCRIPTION: Return true if a framing or overrun error is present
* RETURN:      true if error
* ALGORITHM:   autobaud - if there are a lot of errors, switch baud rate
* NOTES:       Clears any error flags.
*****************************************************************************/
bool RS485_ReceiveError(
    void)
{
   // bool ReceiveError = false;
 
    return false;//gbReceiveError;
}

/****************************************************************************
* DESCRIPTION: Return true if data is available
* RETURN:      true if data is available, with the data in the parameter set
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool RS485_DataAvailable(uint8_t * data_register,uint8_t port)
{
    bool DataAvailable = false;
//   	if(port == 0)
//	{
//	    if (!FIFO_Empty(&Receive_Buffer0)) {
//	        if (data_register) {
//	            *data_register = FIFO_Get(&Receive_Buffer0);
//	        }
//	     //   timer_elapsed_start(&Silence_Timer);
//	       DataAvailable = true;
//	    }
//	}
//	else if(port == 1)
//	{
//		if (!FIFO_Empty(&Receive_Buffer1)) {
//	        if (data_register) {
//	            *data_register = FIFO_Get(&Receive_Buffer1);
//	        }
//	     //   timer_elapsed_start(&Silence_Timer);
//	       DataAvailable = true;
//	    }
//	}
//	else if(port == 2)
	{
		if (!FIFO_Empty(&Receive_Buffer2)) {
	        if (data_register) {
	            *data_register = FIFO_Get(&Receive_Buffer2);
				//hsurRxCount --;
	        }
	     //   timer_elapsed_start(&Silence_Timer);
	       DataAvailable = true;
	    }
	}
    return DataAvailable;
}



//}


//#ifdef TEST_RS485
//int main(
//    void)
//{
//    unsigned i = 0;
//    uint8_t DataRegister;
//
//    RS485_Set_Baud_Rate(38400);
//    RS485_Initialize();
//    /* receive task */
//    for (;;) {
//        if (RS485_ReceiveError()) {
//            fprintf(stderr, "ERROR ");
//        } else if (RS485_DataAvailable(&DataRegister)) {
//            fprintf(stderr, "%02X ", DataRegister);
//        }
//    }
//}
//#endif /* TEST_RS485 */


/* Public access to the Silence Timer */
/*uint16_t Timer_Silence(void)
{ 
    return SilenceTime;
}*/

/* Public reset of the Silence Timer */
/*void Timer_Silence_Reset( void)
{
	SilenceTime = 0;
}	*/
