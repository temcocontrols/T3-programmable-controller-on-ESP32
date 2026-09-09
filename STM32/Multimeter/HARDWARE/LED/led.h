#ifndef __LED_H
#define __LED_H
 
#include "bitmap.h"

#define LED_HEART 		PEout(6)
#define LED_UART1_TX 	PEout(7)
#define LED_UART1_RX 	PEout(8)
#define LED_UART2_TX 	PEout(9)
#define LED_UART2_RX 	PEout(10)



#define LED_IN1				PEout(0)
#define LED_IN2				PEout(1)
#define LED_IN3				PEout(4)
#define LED_IN4				PEout(5)
#define LED_IN5				PEout(11)
#define LED_IN6				PEout(12)
#define LED_IN7				PEout(13)
#define LED_IN8				PEout(14)
#define LED_IN9				PEout(15)
#define LED_IN10			PFout(0)
#define LED_IN11			PFout(1)
#define LED_IN12			PFout(2)
#define LED_IN13			PFout(3)
#define LED_IN14			PFout(4)
#define LED_IN15			PFout(5)
#define LED_IN16			PFout(6)


#define LED_OUT1				PFout(12)
#define LED_OUT2				PFout(13)
#define LED_OUT3				PFout(14)
#define LED_OUT4				PFout(15)
#define LED_OUT5				PGout(0)
#define LED_OUT6				PGout(1)
#define LED_ERR					PGout(3)

#define LED_EN					PGout(2)

extern unsigned char flag_led_in[16];

void LED_Init(void);
void Refresh_LED(void);
		 				    
#endif

















