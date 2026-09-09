 /*================================================================================
 * Module Name : key.h
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

#include "types.h"
/* CONSTANT DECLARATIONS */

/*************************************************************************/
/*  KEY FUCTION DECLARTION */
/*************************************************************************/
#define BEEP_ON 	0
#define BEEP_OFF 	1

#define BACK_ON 	1
#define BACK_OFF 	0

#define K_NONE      0

#define K_LEFT 0X4000
#define K_DOWN 0X2000
#define K_UP	 0X1000
#define K_RIGHT 0X0800
#define K_LEFT_RIGHT 0X4800



#define	LONG_PRESS_TIMER_SPEED_100	200
#define	LONG_PRESS_TIMER_SPEED_50	100
#define	LONG_PRESS_TIMER_SPEED_10	30
#define	LONG_PRESS_TIMER_SPEED_1	10


#define KEY_SPEED_1			(0x0000)
#define KEY_SPEED_10		(0x0100)
#define KEY_SPEED_50		(0x0200)
#define KEY_SPEED_100		(0x0300)
#define KEY_SPEED_MASK		(0x00ff)
#define KEY_FUNCTION_MASK	(0xff00)

#define	KEY_UP_MASK			2//(1 << 1)
#define	KEY_DOWN_MASK		4//(1 << 2)
#define	KEY_LEFT_MASK		8//(1 << 3)
#define	KEY_RIGHT_MASK		1//(1 << 0)
#define	KEY_LEFT_RIGHT_MASK		9//(1 << 0)


/*enum
{
  KEY1 = 1,KEY2,KEY3,KEY4,LCD
};
*/
extern U8_T by_Key;
extern U8_T flag_Send;



void Key_Inital(void);
void Key_Process(void) ;   // 2ms
void vStartKeyTasks( unsigned char uxPriority);

/******END KEY***********************************************************/



#endif




