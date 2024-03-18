
#include "define.h"
#include "LCD_TSTAT.h"
#include "menu.h"
#include "Constcode.h"

#if 1//SET_SCH

str_my_point sch_on_off[8] =
{
	{ TSTAT10_SCH_ON1_X ,TSTAT10_SCH_ON1_Y },
	{ TSTAT10_SCH_OFF1_X ,TSTAT10_SCH_OFF1_Y },
	{ TSTAT10_SCH_ON2_X ,TSTAT10_SCH_ON2_Y },
	{ TSTAT10_SCH_OFF2_X ,TSTAT10_SCH_OFF2_Y },
	{ TSTAT10_SCH_ON3_X ,TSTAT10_SCH_ON3_Y },
	{ TSTAT10_SCH_OFF3_X ,TSTAT10_SCH_OFF3_Y },
	{ TSTAT10_SCH_ON4_X ,TSTAT10_SCH_ON4_Y },
	{ TSTAT10_SCH_OFF4_X ,TSTAT10_SCH_OFF4_Y }
};

#define TSTAT10_SCH_ON1_TIME_X   100
#define TSTAT10_SCH_ON1_TIME_Y   TSTAT10_SCH_ON1_Y
#define TSTAT10_SCH_OFF1_TIME_X   100
#define TSTAT10_SCH_OFF1_TIME_Y   TSTAT10_SCH_OFF1_Y

#define TSTAT10_SCH_ON2_TIME_X   100
#define TSTAT10_SCH_ON2_TIME_Y   TSTAT10_SCH_ON2_Y
#define TSTAT10_SCH_OFF2_TIME_X   100
#define TSTAT10_SCH_OFF2_TIME_Y   TSTAT10_SCH_OFF2_Y

#define TSTAT10_SCH_ON3_TIME_X   100
#define TSTAT10_SCH_ON3_TIME_Y   TSTAT10_SCH_ON3_Y
#define TSTAT10_SCH_OFF3_TIME_X   100
#define TSTAT10_SCH_OFF3_TIME_Y   TSTAT10_SCH_OFF3_Y

#define TSTAT10_SCH_ON4_TIME_X   100
#define TSTAT10_SCH_ON4_TIME_Y   TSTAT10_SCH_ON4_Y
#define TSTAT10_SCH_OFF4_TIME_X   100
#define TSTAT10_SCH_OFF4_TIME_Y   TSTAT10_SCH_OFF4_Y

str_my_point  sch_time[8] = 
{
	{ TSTAT10_SCH_ON1_TIME_X , TSTAT10_SCH_ON1_TIME_Y },
	{ TSTAT10_SCH_OFF1_TIME_X, TSTAT10_SCH_OFF1_TIME_Y },
	{ TSTAT10_SCH_ON2_TIME_X , TSTAT10_SCH_ON2_TIME_Y },
	{ TSTAT10_SCH_OFF2_TIME_X, TSTAT10_SCH_OFF2_TIME_Y },
	{ TSTAT10_SCH_ON3_TIME_X , TSTAT10_SCH_ON3_TIME_Y },
	{ TSTAT10_SCH_OFF3_TIME_X, TSTAT10_SCH_OFF3_TIME_Y },
	{ TSTAT10_SCH_ON4_TIME_X , TSTAT10_SCH_ON4_TIME_Y },
	{ TSTAT10_SCH_OFF4_TIME_X, TSTAT10_SCH_OFF4_TIME_Y }
};

//uint8_t item_schedule;
uint8_t item_to_day = 0;
uint8_t itme_to_time = 0;
uint8_t change_time = 0;
extern uint8_t current_schedule;
uint8_t current_day = 0;

void MenuDaySet_init(void)
{
	char temp_buffer[5];
	ClearScreen(TSTAT8_BACK_COLOR);
	item_to_day = current_day;
	itme_to_time = 0;
	// show schedule
	temp_buffer[0] = 'S';
	temp_buffer[1] = 'C';
	temp_buffer[2] = 'H';
	temp_buffer[3] = current_schedule + '1';
	temp_buffer[4] = ' ';
	disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, 0, temp_buffer, TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	
	
	// show day
	if (item_to_day == 0)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Mon", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 1)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tus", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 2)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Wed", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 3)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tur", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 4)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Fri", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 5)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sat", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 6)			
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sun", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 7)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex1", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 8)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex2", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);


	// show ON/OFF
	disp_str_16_24(FORM15X30, TSTAT10_SCH_ON1_X, TSTAT10_SCH_ON1_Y, "ON", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, TSTAT10_SCH_OFF1_X, TSTAT10_SCH_OFF1_Y, "OFF", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR1);
	
	disp_str_16_24(FORM15X30, TSTAT10_SCH_ON2_X, TSTAT10_SCH_ON2_Y, "ON", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, TSTAT10_SCH_OFF2_X, TSTAT10_SCH_OFF2_Y, "OFF", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR1);

	disp_str_16_24(FORM15X30, TSTAT10_SCH_ON3_X, TSTAT10_SCH_ON3_Y, "ON", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, TSTAT10_SCH_OFF3_X, TSTAT10_SCH_OFF3_Y, "OFF", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR1);

	disp_str_16_24(FORM15X30, TSTAT10_SCH_ON4_X, TSTAT10_SCH_ON4_Y, "ON", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, TSTAT10_SCH_OFF4_X, TSTAT10_SCH_OFF4_Y, "OFF", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR1);

}


void MenuDaySet_display(void)
{
	char temp_buffer[6];
	uint8_t hour,min;
	char j;
	memset(temp_buffer, 0, 0);
	blink_count = (blink_count ++) % 2;
	
	if(blink_count % 2  == 0)
	{
		// show day
	if (item_to_day == 0)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Mon", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 1)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tus", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 2)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Wed", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 3)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tur", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 4)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Fri", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 5)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sat", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 6)			
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sun", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 7)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex1", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 8)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex2", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	}
	else
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "             ", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	for (j = 0; j < 8;j++)
	{
		min = wr_times[current_schedule][item_to_day].time[j].minutes;
		hour = wr_times[current_schedule][item_to_day].time[j].hours;
		
		if(hour < 10)
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0' + hour;
		}
		else if(hour <= 12)
		{
			temp_buffer[0] = '0' + hour / 10;
			temp_buffer[1] = '0' + hour % 10;
		}
		else
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0';
		}
		
		temp_buffer[2] = ':';
		if(min < 10)
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0' + min;
		}
		else if(min <= 59)
		{
			temp_buffer[3] = '0' + min / 10;
			temp_buffer[4] = '0' + min % 10;
		}
		else
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0';
		}
		temp_buffer[5] = '\0';
		disp_str_16_24(FORM15X30, sch_time[j].x_pos, sch_on_off[j].y_pos, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
}
	
	
	
	
void MenuDaySet_keycope(uint16 key_value)
{	
	switch(key_value& KEY_SPEED_MASK)
	{
		case 0:
			// do nothing
			break;
		case KEY_UP_MASK:
			// do nothing
			if(item_to_day < 8)	item_to_day++; 
			else
				item_to_day = 0;
				break;
		case KEY_DOWN_MASK:
			// do nothing
			if(item_to_day > 0)	item_to_day--;
			else
				item_to_day = 8;
				break;
		case KEY_LEFT_MASK:
			ClearScreen(TSTAT8_BACK_COLOR);
			current_day = 0;
			update_menu_state(MenuSet);
				break;
		case KEY_RIGHT_MASK:			
			//Save_Sch_Parmeter(item_schedule);
			current_day = item_to_day;
		  itme_to_time = 0;
			update_menu_state(MenuTimeSet);				
			break;
		default:
			break;
	}	
}


void MenuTimeSet_init(void)
{
	
//	ClearScreen(TSTAT8_BACK_COLOR);
//	itme_to_time = 0;
	// show ON/OFF
	// show day
	if (item_to_day == 0)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Mon", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 1)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tus", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 2)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Wed", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 3)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Tur", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 4)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Fri", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 5)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sat", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 6)			
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Sun", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 7)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex1", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	else if (item_to_day == 8)
		disp_str_16_24(FORM15X30, TSTAT10_SCH_DAY_X, TSTAT10_SCH_DAY_Y, "Ex2", TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);

}


void MenuTimeSet_display(void)
{
	char temp_buffer[6];
	uint8_t hour,min;
	char j;
	memset(temp_buffer, 0, 0);
	blink_count = (blink_count++) % 2;

	if(blink_count %2  == 0)
	{
		for (j = 0; j < 8;j++)
		{
			min = wr_times[current_schedule][item_to_day].time[j].minutes;
			hour = wr_times[current_schedule][item_to_day].time[j].hours;
			
			if(hour < 10)
			{
				temp_buffer[0] = '0';
				temp_buffer[1] = '0' + hour;
			}
			else if(hour <= 23)
			{
				temp_buffer[0] = '0' + hour / 10;
				temp_buffer[1] = '0' + hour % 10;
			}
			else
			{
				temp_buffer[0] = '0';
				temp_buffer[1] = '0';
			}
			
			temp_buffer[2] = ':';
			if(min < 10)
			{
				temp_buffer[3] = '0';
				temp_buffer[4] = '0' + min;
			}
			else if(min <= 59)
			{
				temp_buffer[3] = '0' + min / 10;
				temp_buffer[4] = '0' + min % 10;
			}
			else
			{
				temp_buffer[3] = '0';
				temp_buffer[4] = '0';
			}
			temp_buffer[5] = '\0';
			disp_str_16_24(FORM15X30, sch_time[j].x_pos, sch_on_off[j].y_pos, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
	}
	else
	{
		min = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].minutes;
		hour = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].hours;
		
		if(hour < 10)
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0' + hour;
		}
		else if(hour <= 23)
		{
			temp_buffer[0] = '0' + hour / 10;
			temp_buffer[1] = '0' + hour % 10;
		}
		else
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0';
		}
		
		temp_buffer[2] = ':';
		if(min < 10)
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0' + min;
		}
		else if(min <= 59)
		{
			temp_buffer[3] = '0' + min / 10;
			temp_buffer[4] = '0' + min % 10;
		}
		else
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0';
		}
		temp_buffer[5] = '\0';
			
		if(itme_to_time % 2 == 0)
		{
			temp_buffer[0] = ' ';
			temp_buffer[1] = ' ';
		}
		else
		{
			temp_buffer[3] = ' ';
			temp_buffer[4] = ' ';
		}

		disp_str_16_24(FORM15X30, sch_time[itme_to_time / 2].x_pos, sch_on_off[itme_to_time / 2].y_pos, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
}
	
	
	
	
void MenuTimeSet_keycope(uint16 key_value)
{	
	switch(key_value& KEY_SPEED_MASK)
	{
		case 0:
			// do nothing
			break;
//		case KEY_UP_MASK:
//			// do nothing
//			if(itme_to_time < 15)	itme_to_time++;
//			else
//				itme_to_time = 0;
//				break;
//		case KEY_DOWN_MASK:
//			// do nothing
//			if(itme_to_time > 0)	itme_to_time--; 
//			else
//				itme_to_time = 15;
//				break;
		case KEY_LEFT_MASK:
			update_menu_state(MenuDaySet);
				break;
		case KEY_UP_MASK:
		case KEY_DOWN_MASK:
		case KEY_RIGHT_MASK:			
			//Save_Sch_Parmeter(item_schedule);	
			update_menu_state(MenuTimeChange);				
			break;
		default:
			break;
	}	
}



void MenuTimeChange_init(void)
{
	if(itme_to_time % 2 == 0) // change hour
	{
		change_time = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].hours;		
	}
	else // change minute
	{
		change_time = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].minutes;		
	}


}


void MenuTimeChange_display(void)
{
	char temp_buffer[6];
	uint8_t hour,min;
	char j;
	memset(temp_buffer, 0, 0);
	blink_count = (blink_count++) % 2;

	if(blink_count % 2 == 0)
	{
		for (j = 0; j < 8;j++)
		{
			min = wr_times[current_schedule][item_to_day].time[j].minutes;
			hour = wr_times[current_schedule][item_to_day].time[j].hours;
			
			if(itme_to_time % 2 == 0) // change hour
			{
				if(itme_to_time / 2 == j)
					hour = change_time;
			}
			else // change minute
			{
				if(itme_to_time / 2 == j)
					min = change_time;
			}
			
			
			if(hour < 10)
			{
				temp_buffer[0] = '0';
				temp_buffer[1] = '0' + hour;
			}
			else if(hour <= 23)
			{
				temp_buffer[0] = '0' + hour / 10;
				temp_buffer[1] = '0' + hour % 10;
			}
			else
			{
				temp_buffer[0] = '0';
				temp_buffer[1] = '0';
			}
			
			temp_buffer[2] = ':';
			if(min < 10)
			{
				temp_buffer[3] = '0';
				temp_buffer[4] = '0' + min;
			}
			else if(min <= 59)
			{
				temp_buffer[3] = '0' + min / 10;
				temp_buffer[4] = '0' + min % 10;
			}
			else
			{
				temp_buffer[3] = '0';
				temp_buffer[4] = '0';
			}
			
			
			temp_buffer[5] = '\0';
			disp_str_16_24(FORM15X30, sch_time[j].x_pos, sch_on_off[j].y_pos, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
	}
	else
	{
		
		min = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].minutes;
		hour = wr_times[current_schedule][item_to_day].time[itme_to_time / 2].hours;
		
		if(hour < 10)
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0' + hour;
		}
		else if(hour <= 23)
		{
			temp_buffer[0] = '0' + hour / 10;
			temp_buffer[1] = '0' + hour % 10;
		}
		else
		{
			temp_buffer[0] = '0';
			temp_buffer[1] = '0';
		}
		
		temp_buffer[2] = ':';
		if(min < 10)
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0' + min;
		}
		else if(min <= 59)
		{
			temp_buffer[3] = '0' + min / 10;
			temp_buffer[4] = '0' + min % 10;
		}
		else
		{
			temp_buffer[3] = '0';
			temp_buffer[4] = '0';
		}
		temp_buffer[5] = '\0';
			
		if(itme_to_time % 2 == 0)
		{
			temp_buffer[0] = ' ';
			temp_buffer[1] = ' ';
		}
		else
		{
			temp_buffer[3] = ' ';
			temp_buffer[4] = ' ';
		}

		disp_str_16_24(FORM15X30, sch_time[itme_to_time / 2].x_pos, sch_on_off[itme_to_time / 2].y_pos, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
}
	
	
	
	
void MenuTimeChange_keycope(uint16 key_value)
{	
	switch(key_value& KEY_SPEED_MASK)
	{
		case 0:
			// do nothing
			break;
		case KEY_UP_MASK:
			// do nothing
			if(itme_to_time % 2 == 0)
			{
				if(change_time < 23)	change_time++;
				else 
					change_time = 0;
			}	
			else
			{
				if(change_time < 59)	change_time++;
				else 
					change_time = 0;
			}
			break;
		case KEY_DOWN_MASK:
			// do nothing
			if(itme_to_time % 2 == 0)
			{
				if(change_time > 0)	change_time--; 
				else
					change_time = 23;
			}
			else
			{
				if(change_time > 0)	change_time--; 
				else
					change_time = 59;
			}
				break;
		case KEY_LEFT_MASK:// cancle
			update_menu_state(MenuTimeSet);
				break;
		case KEY_RIGHT_MASK: // save			
			if(itme_to_time % 2 == 0)
				wr_times[current_schedule][item_to_day].time[itme_to_time / 2].hours = change_time;
			else
				wr_times[current_schedule][item_to_day].time[itme_to_time / 2].minutes = change_time;
			
			if(itme_to_time < 15)	itme_to_time++;
			else
				itme_to_time = 0;
			
			// save it to flash memory
			ChangeFlash = 1;
			write_page_en[WR_TIME] = 1;	
			vTaskResume(Handle_Menu);			
			//update_menu_state(MenuTimeSet);			
				
			break;
		default:
			break;
	}	
}

#endif

