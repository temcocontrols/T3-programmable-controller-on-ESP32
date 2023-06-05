/*
 * lcd.h
 *
 *  Created on: 2022Äê11ÔÂ2ÈÕ
 *      Author: Administrator
 */

#ifndef MAIN_LCD_H_
#define MAIN_LCD_H_

#include "types.h"
#include "esp_attr.h"

typedef struct
{
	uint8_t unit;
	uint8_t setpoint;
	uint8_t fan;
	uint8_t sysmode;
	uint8_t occ_unocc;
	uint8_t heatcool;
	uint8_t fanspeed;
	uint8_t cmnct_send;
	uint8_t cmnct_rcv;
} DISP_CHANGE;

#define FORM32X64 		0
#define FORM15X30		1


#ifdef COLOR_TEST
#define TSTAT8_BACK_COLOR1  0x9df5//0xc67b// 0x7557 //0x2589
extern uint32 TSTAT8_BACK_COLOR;   //
extern uint32 TSTAT8_MENU_COLOR2;
extern uint32 TANGLE_COLOR;
#else
#define TSTAT8_BACK_COLOR1  0x7E19
#define TSTAT8_BACK_COLOR   0x7E19//
#define TSTAT8_MENU_COLOR2  0x7e17
#define TANGLE_COLOR        0xbe9c
#endif

#endif /* MAIN_LCD_H_ */
