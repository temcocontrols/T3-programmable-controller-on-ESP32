/*
 * lcd.h
 *
 *  Created on: 2022��11��2��
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

#define SCH_XPOS  10

#define SCH_COLOR  0xffff//0XB73F
#define SCH_BACK_COLOR  0x3bef//0x43f2//0x14E9

#define THERM_METER_POS										5
#define CH_HEIGHT													36

#define SETPOINT_POS											108
#define FAN_MODE_POS											SETPOINT_POS+CH_HEIGHT+7
#define SYS_MODE_POS											FAN_MODE_POS+CH_HEIGHT+7
#define PM25_W_POS 												SYS_MODE_POS+CH_HEIGHT+7
#define PM25_N_POS												PM25_W_POS+CH_HEIGHT+7

#define ITEM_NONE 20
#define ITEM_TEMP 0
#define ITEM_HUM 1
#define ITEM_CO2 2
#define ITEM_VOC 3
#define ITEM_LIGHT 4


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
