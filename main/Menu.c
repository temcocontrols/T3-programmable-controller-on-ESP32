#include "define.h"
#include "menu.h"
#include <string.h>
#include "lcd.h"
#include "airlab.h"
#include "driver/uart.h"
#include "LcdTheme.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


#define	MenuSTACK_SIZE		portMINIMAL_STACK_SIZE//1024
#define	ScrollingSTACK_SIZE	portMINIMAL_STACK_SIZE//1024
#define	CursorSTACK_SIZE	portMINIMAL_STACK_SIZE//512

//xTaskHandle Handle_Menu, Handle_Scrolling, Handle_Cursor;

U8_T show_identify;
U8_T count_show_id;
QueueHandle_t qKey = 0;

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
uint32 menu_refresh_timer_start, menu_refresh_timer_end;
uint8 menu_block_seconds = MENU_BLOCK_SECONDS_DEFAULT;
//uint8 backlight_keep_seconds = BACKLIGHT_KEEP_SECONDS_DEFAULT;
struct _MENU_STATE_ CurrentState;

typedef enum
{
	LCD_IDEAL = 0,
	LCD_INIT,
	LCD_WAIT,
	LCD_MENU_INIT,
	LCD_RUN,
}eLcd_state_t;

eLcd_state_t Lcd_state = LCD_INIT;
uint16_t count_lcd_time_off_delay;

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

void LCD_IO_Init(void);
void lcd_back_set(uint8_t status);
void Display_DeviceName(void);
void LcdMenuRunning(void);

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

void Check_LCD_time_off(void)
{
//	if(Modbus.backlight == 1)
	{
		if((Modbus.LCD_time_off_delay != 0) && (Modbus.LCD_time_off_delay != 255))
		{
			count_lcd_time_off_delay++;
			if(count_lcd_time_off_delay > Modbus.LCD_time_off_delay)
			{
				count_lcd_time_off_delay = 0;
				lcd_back_set(0);
			}
		}
		else if(Modbus.LCD_time_off_delay == 255)
		{
			count_lcd_time_off_delay = 0;
			lcd_back_set(1);
		}
		else if(Modbus.LCD_time_off_delay == 0)
		{
			count_lcd_time_off_delay++;
			if(count_lcd_time_off_delay > 5)
			{
				count_lcd_time_off_delay = 0;
				lcd_back_set(0);
			}
		}
	}
}

void MenuTask(void *pvParameters)
{
	while(1)
	{
		switch(Lcd_state)
		{
			case LCD_IDEAL:
				// Do nothing, wait for initialization
				break;

			case LCD_INIT:
				// Initialize LCD
				LcdThemeInit();
				LCD_IO_Init();
				Display_DeviceName();
				uart_write_bytes(UART_NUM_0, "\r\nLCD Init\r\n", 12);
				Lcd_state = LCD_WAIT;
				break;

			case LCD_WAIT:
				// Wait for some condition if needed
				if(Get_Device_Stage() == DEVICE_STAGE_RUNNING)
				{
					Lcd_state = LCD_MENU_INIT;
				}
				break;

			case LCD_MENU_INIT:
			{
				// Initialize Menu
				menu_init();
				delay_ms(100);
				if(Modbus.mini_type == PROJECT_AIRLAB)
				{
					// disp_str(FORM15X30, SCH_XPOS, PM25_W_POS, "PM  :", 0xffff - aqi_background_color, aqi_background_color);
					// disp_str(FORM15X30, SCH_XPOS,  PM25_W_POS,  "PM", 0xffff - aqi_background_color, aqi_background_color);
					// disp_special_str(FORM15X30, SCH_XPOS + 24 * 2 - 1, PM25_W_POS + 12, "2.5", 0xffff - aqi_background_color, aqi_background_color);
					// disp_str(FORM15X30, SCH_XPOS + 24 * 4 - 3, PM25_W_POS , ":", 0xffff - aqi_background_color, aqi_background_color);
					// disp_str(FORM15X30, SCH_XPOS,  PM25_N_POS, "PM10:", 0xffff - aqi_background_color, aqi_background_color);
				}
				if(qKey == NULL)
					qKey = xQueueCreate(2, 2);

				Lcd_state = LCD_RUN;
			}
			break;

			case LCD_RUN:
				LcdThemeLoop();
				LcdMenuRunning();
				// Normal operation
				break;

			default:
				Lcd_state = LCD_IDEAL;
				break;
		}
	}
	vTaskDelay(1 / portTICK_PERIOD_MS);
}

void LcdMenuRunning(void)
{
	u16 temp_key = 0;

	Check_LCD_time_off();

	if(xQueueReceive(qKey, &temp_key, 5) == pdTRUE)
	{
		uart_write_bytes(UART_NUM_0, "key Received\r\n", strlen("key Send\r\n"));
		Test[4]++;
		CurrentState.KeyCope(temp_key);
		if(CurrentState.BlockTime)
		{
			menu_block_timer_start = xTaskGetTickCount();
		}
		CurrentState.DisplayPeriod();
	}
	else
	{
		if(menu_system_start == TRUE)
		{
			menu_refresh_timer_end = xTaskGetTickCount();
			if((menu_refresh_timer_end - menu_refresh_timer_start) >= (100 / portTICK_PERIOD_MS))
			{
				Test[17]++;
				menu_refresh_timer_start = xTaskGetTickCount();
				CurrentState.DisplayPeriod();
			}

			if(CurrentState.BlockTime)
			{
				Test[18]++;
				menu_block_timer_end = xTaskGetTickCount();
				Test[7] = menu_block_timer_end / 1000;
				Test[8] = menu_block_timer_start / 1000;
				Test[9] = CurrentState.BlockTime;
				if((menu_block_timer_end - menu_block_timer_start) >= (CurrentState.BlockTime * SWTIMER_COUNT_SECOND))
				{
					update_menu_state(MenuIdle);
				}
			}
		}
	}
}

void exit_request_password(void)
{
//	cursor_off();
	menu_password = FALSE;
}






