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
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

//uint8_t Rec_Mstp_Byte;
//uint8_t Rec_Mstp_Err;

/* This file has been customized for use with ATMEGA168 */
#include "bacnet.h"
#include "timer.h"


uint16_t Timer_Silence(void);
void Timer_Silence_Reset( void);
#define Tturnaround  (40UL)

/* Timers for turning off the TX,RX LED indications */
/*static uint8_t LED1_Off_Timer;
static uint8_t LED3_Off_Timer; */

/* baud rate */
//static uint32_t RS485_Baud = 9600;

/*unsigned char xdata gucReceiveCount;
unsigned char xdata gucReceive_Index;
unsigned char xdata gucTake_Index; */
bool  gbReceiveError;
bool MSTP_Transmit_Finished;
/* buffer for storing received bytes - size must be power of two */
uint8_t  Receive_Buffer_Data0[512];
FIFO_BUFFER Receive_Buffer0;
/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void Recievebuf_Initialize(uint8_t port)
{
	FIFO_Init(&Receive_Buffer0, &Receive_Buffer_Data0[0], sizeof(Receive_Buffer_Data0)); 
#if 0	
	timer_elapsed_start(&Silence_Timer);
#endif
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
		if(enable)
			Set_TXEN(1);//TXEN = 1;//SEND;
		else
			Set_TXEN(0);//TXEN = 0;//RECEIVE;		
		
		

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
	uint16_t turnaround_time;
	uint8_t baudrate = RS485_Get_Baudrate();
//if(modbus.protocal == BAC_MSTP)
	{
		if(baudrate == 5) //UART_9600
		{
			turnaround_time = 6;
		}
		else if(baudrate == 6/*UART_19200*/)
		{
		    turnaround_time = 4;
		}
		else if(baudrate == 7/*UART_38400*/)
		{
		    turnaround_time = 3;
		}
		else
			turnaround_time = 2;
	}
	while (Timer_Silence() < turnaround_time) 
	{
	/* do nothing - wait for timer to increment */
	};
}




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
#if ARM
	if(get_current_mstp_port() != -1)
	{
		uart_send_string(buffer,nbytes,get_current_mstp_port());	
	}
#endif

		
#if ASIX
	uart_send_string(buffer,nbytes,2);		
#endif
		
		
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
 //   bool ReceiveError = false;
 
    return false;
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
	    if (!FIFO_Empty(&Receive_Buffer0)) {
	        if (data_register) {
	            *data_register = FIFO_Get(&Receive_Buffer0);
#if ASIX
			hsurRxCount --;				
#endif
	        }
	     //   timer_elapsed_start(&Silence_Timer);
	       DataAvailable = true;
	    }

		
    return DataAvailable;

}


volatile uint16_t SilenceTime;

/* Configure the Timer */
void Timer_Initialize(void)
{
  
}


/* Public access to the Silence Timer */
uint16_t Timer_Silence(void)
{ 
    return SilenceTime;
}

/* Public reset of the Silence Timer */
void Timer_Silence_Reset( void)
{
	SilenceTime = 0;
}						  

