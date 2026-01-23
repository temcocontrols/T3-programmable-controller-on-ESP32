
#ifndef __MENU_H__

#define	__MENU_H__
#include "types.h"

#include "menuIdle.h"
#include "menuMain.h"
#include "menuSet.h"
#include "menuDaySet.h"
#include <stdbool.h>

#define MENU_BLOCK_SECONDS_DEFAULT	    60 //
#define BACKLIGHT_KEEP_SECONDS_DEFAULT	30
#define SWTIMER_COUNT_SECOND	 configTICK_RATE_HZ



#define MAX_MENU_NUM 4 // add sch


typedef struct
{
	uint8 first_line[10];
	uint8 second_line[10];
	int16 max;
	int16 min;
}MENUNAME;

struct _MENU_STATE_
{
	uint8 Index;
	void (*KeyCope)(U16_T);
	void (*InitAction)(void);
	void (*DisplayPeriod)(void);
	uint8 BlockTime;
};

extern struct _MENU_STATE_ CurrentState;

extern uint8 menu_system_start;
extern uint8 in_sub_menu;
extern uint8 value_change;
extern uint8 previous_internal_co2_exist;
extern uint8 pre_db_ctr;

#define MAX_PASSWORD_DIGITALS	4
extern uint8 menu_password;
extern uint8 use_password;
extern uint8 password_index;
extern uint8 password_buffer[4];
extern uint8 user_password[4];

extern uint8 text[50];
extern uint8 int_text[21];
//extern uint16 set_value;

extern uint8 const   internal_text[];
extern uint8 const   external_text[];
extern uint8 const   ppm_text[];
//extern uint8 const code co2_text[];
extern uint8 const   int_space[];
//extern uint8 const code online_text[];
//extern uint8 const code offline_text[];
extern uint8 const   warming_text[];
extern MENUNAME const menu[MAX_MENU_NUM];


extern uint8 menu_block_seconds;
extern uint8 backlight_keep_seconds;
//extern xQueueHandle xMutex,IicMutex;
extern uint8 blink_count;
 enum
{
	MenuIdle = 0,
	MenuMain,
	MenuSet,
		MenuDaySet, // set day
			MenuTimeSet, // select time
				MenuTimeChange, // change time
	MenuEnd,
};

void Check_identify_tstat10(void);
void show_system_info(void);
void update_menu_state(uint8 MenuId);
void exit_request_password(void);
void vStartMenuTask(unsigned char uxPriority);
void start_menu(void);
void menu_init(void);
void DisplayHomeScreen( bool isHomeScreen );
//void vStartScrollingTask(unsigned char uxPriority);
void print(char *p);

#endif

