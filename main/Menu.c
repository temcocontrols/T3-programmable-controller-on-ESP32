#include "define.h"
#include "menu.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define	MenuSTACK_SIZE		portMINIMAL_STACK_SIZE//1024
#define	ScrollingSTACK_SIZE	portMINIMAL_STACK_SIZE//1024
#define	CursorSTACK_SIZE	portMINIMAL_STACK_SIZE//512

//xTaskHandle Handle_Menu, Handle_Scrolling, Handle_Cursor;

U8_T show_identify;
U8_T count_show_id;
//extern xQueueHandle qKey;

uint8 text[50]; 
uint8 int_text[21];
//uint16 set_value;

uint8 menu_system_start = FALSE;
uint8 in_sub_menu = FALSE;
uint8 value_change = FALSE;
uint8 previous_internal_co2_exist;
uint8 pre_db_ctr = 0;


uint8 menu_password = FALSE;
uint8 use_password = FALSE;
uint8 password_buffer[4];
uint8 user_password[4] = {'1', '2', '3', '4'};
uint8 password_index;

uint32 menu_block_timer_start, menu_block_timer_end;
uint8 menu_block_seconds = MENU_BLOCK_SECONDS_DEFAULT;
//uint8 backlight_keep_seconds = BACKLIGHT_KEEP_SECONDS_DEFAULT;
struct _MENU_STATE_ CurrentState;


struct _MENU_STATE_ const StateArray[MenuEnd] =
{
	{
		MenuIdle,
		MenuIdle_keycope,
		MenuIdle_init,
		MenuIdle_display,
		0,
	},
	{
		MenuMain,
		MenuMain_keycope,
		MenuMain_init,
		MenuMain_display,
		MENU_BLOCK_SECONDS_DEFAULT,
	},
	{
		MenuSet,
		MenuSet_keycope,
		MenuSet_init,
		MenuSet_display,
		MENU_BLOCK_SECONDS_DEFAULT,
	},
#if 0//SET_SCH
	{
		MenuDaySet,
		MenuDaySet_keycope,
		MenuDaySet_init,
		MenuDaySet_display,
		MENU_BLOCK_SECONDS_DEFAULT,
	},

	{
		MenuTimeSet,
		MenuTimeSet_keycope,
		MenuTimeSet_init,
		MenuTimeSet_display,
		MENU_BLOCK_SECONDS_DEFAULT,
	},
	{
		MenuTimeChange,
		MenuTimeChange_keycope,
		MenuTimeChange_init,
		MenuTimeChange_display,
		MENU_BLOCK_SECONDS_DEFAULT,
	},
#endif
};

void update_menu_state(uint8 MenuId)
{

	memcpy((void *)&CurrentState, (void *)&StateArray[MenuId], sizeof(struct _MENU_STATE_));
	 
	CurrentState.InitAction();
}

void show_system_info(void)
{
	// ip address
//	sprintf((char *)text, "IP:   %u.%u.%u.%u", (uint16)modbus.ip_addr[0], (uint16)modbus.ip_addr[1], (uint16)modbus.ip_addr[2], (uint16)modbus.ip_addr[3]);
//	Lcd_Show_String(0, 0, DISP_NOR, text);
//	// subnet mask address
//	sprintf((char *)text, "MASK: %u.%u.%u.%u", (uint16)modbus.mask_addr[0], (uint16)modbus.mask_addr[1], (uint16)modbus.mask_addr[2], (uint16)modbus.mask_addr[3]);
//	Lcd_Show_String(1, 0, DISP_NOR, text);
//	// tcp port
//	sprintf((char *)text, "GATE: %u.%u.%u.%u", (uint16)modbus.gate_addr[0], (uint16)modbus.gate_addr[1], (uint16)modbus.gate_addr[2], (uint16)modbus.gate_addr[3]);
//	Lcd_Show_String(2, 0, DISP_NOR, text);
//	// tcp port
//	sprintf((char *)text, "PORT: %u", ((uint16)modbus.listen_port));
//	Lcd_Show_String(3, 0, DISP_NOR, text);
//	// MAC address
//	sprintf((char *)text, "MAC:%02X:%02X:%02X:%02X:%02X:%02X", (uint16)modbus.mac_addr[0], (uint16)modbus.mac_addr[1], (uint16)modbus.mac_addr[2], (uint16)modbus.mac_addr[3], (uint16)modbus.mac_addr[4], (uint16)modbus.mac_addr[5]);
//	Lcd_Show_String(4, 0, DISP_NOR, text);
}

void menu_init(void)
{
	menu_system_start = TRUE;
	update_menu_state(MenuIdle);
}




extern void watchdog(void);
U16_T qKey = 0;
void LCD_IO_Init(void);
void MenuTask(void *pvParameters)
{
	static U8_T refresh_screen_timer = 0;
	
	U16_T key_temp = 0;
	portTickType xDelayPeriod = (portTickType)50 / portTICK_RATE_MS;
//	U8_T i;
	LCD_IO_Init();
	menu_init();
	delay_ms(100);

	while(1)
	{
		//if(xQueueReceive(qKey, &key_temp, 20) == pdTRUE)
		//if(key_temp = qKey)


		if(qKey != 0)
		{Test[12]++;
			CurrentState.KeyCope(qKey);
			if(CurrentState.BlockTime)
				menu_block_timer_start = xTaskGetTickCount();
			CurrentState.DisplayPeriod();
			qKey = 0;
		}

		if(menu_system_start == TRUE)
		{
			refresh_screen_timer++;
			if (refresh_screen_timer >= 10)
			{
				CurrentState.DisplayPeriod();
				refresh_screen_timer = 0;
			}
			
			if(CurrentState.BlockTime)
			{
				menu_block_timer_end = xTaskGetTickCount();
				if((menu_block_timer_end - menu_block_timer_start) >= (CurrentState.BlockTime * SWTIMER_COUNT_SECOND))
					update_menu_state(MenuIdle);
			}
		}

		vTaskDelay(50);//50ms
//		stack_detect(&test[9]);
	}	
}

void exit_request_password(void)
{
//	cursor_off();
	menu_password = FALSE;
}

/*void Check_identify_tstat10(void)
{
	if(show_identify == 1)
	{			
		if(count_show_id++ % 2 == 0)										
			BACKLIT = 1;
		else
			BACKLIT = 0;
		
		if(count_show_id > 50)
			show_identify = 0;
	}
}*/





