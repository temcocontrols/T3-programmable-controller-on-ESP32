#include "define.h"
#include "LCD_TSTAT.h"
#include "menu.h"
#include "bacnet.h"
#include "user_data.h"
#include "modbus.h"
#include <string.h>
#include <stdio.h>
#include "flash.h"

const char c_strBaudate[UART_BAUDRATE_MAX][7] =
{
    "1200  ",   //0
    "2400  ",   //1
    "3600  ",   //2
    "4800  ",   //3
    "7200  ",   //4
    "9600  ",   //5
    "19200 ",  //6
    "38400 ",  //7
    "76800 ",  //8
    "115200", //9
    "921600", //10
    "57600 "   //11
};

#define MAX_MENU_NUM 3
MENUNAME const menu[MAX_MENU_NUM] = {
{ "Modbus"  ,"Address", 254 , 0 },
{ "Baudrate","Select ", 9   , 0 },		// Baudrate
{ "Protocol","Select" , 1   , 0 }
};

uint8 item_to_adjust;
static uint16 set_value = 0;
void Count_com_config(void);

void start_menu(void)
{
	// kept for compatibility; no separate menu screen now
}

static uint8 current_protocol_setting(void)
{
	if ((Setting_Info.reg.com_config[0] == BACNET_SLAVE) ||
        (Setting_Info.reg.com_config[0] == BACNET_MASTER))
		return 0; // BACNET
	else
		return 1; // MODBUS
}

static uint8 prev_item = 0xFF;
static uint8 prev_addr = 0xFF;
static uint8 prev_baud = 0xFF;
static uint8 prev_proto = 0xFF;
static bool boxes_drawn = false;

void MenuMain_init(void)
{
	item_to_adjust = 0;
	prev_item = 0xFF;
	prev_addr = 0xFF;
	prev_baud = 0xFF;
	prev_proto = 0xFF;
	boxes_drawn = false;
    ClearScreen(TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, SCH_XPOS+20,  IDLE_LINE1_POS+20, (uint8 *)"Communication", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, SCH_XPOS+50,  IDLE_LINE2_POS, (uint8 *)"Settings", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

	disp_str_16_24(FORM15X30, 20, ICON_POS - 20, (uint8 *)"  +  Edit  -  ", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	disp_str_16_24(FORM15X30, 20, ICON_POS, (uint8 *)"< Back   Next >", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
}

uint32 get_display_value(uint8 item)
{
	if(item == 0)
        return Modbus.address;
    else if (item == 1)
        return  Setting_Info.reg.com_baudrate[0];
    else if (item == 2)
        return Modbus.protocal;
	else
		return 0;
}

static void render_labels(uint8 selected_index)
{
    uint16 bg0 = (selected_index == 0) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR;
    uint16 bg1 = (selected_index == 1) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR;
    uint16 bg2 = (selected_index == 2) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR;

	disp_str_16_24(FORM15X30, SCH_XPOS,  SETPOINT_POS, (uint8 *)"Modbus", SCH_COLOR, bg0);
	disp_str_16_24(FORM15X30, SCH_XPOS,  FAN_MODE_POS-5, (uint8 *)"Baud", SCH_COLOR, bg1);
	disp_str_16_24(FORM15X30, SCH_XPOS,  SYS_MODE_POS-10, (uint8 *)"Mode", SCH_COLOR, bg2);
}

static void render_value_row(uint8 row, uint16 value)
{
    uint16 ypos = (row == 0) ? SETPOINT_POS : (row == 1 ? FAN_MODE_POS-5 : SYS_MODE_POS-10);
	char temp_buffer[16] = {0};
	memcpy(temp_buffer, "      ", strlen("      "));
	if(row == 0)
	{
		sprintf(temp_buffer, "%u      ", value);
	}
	else if(row == 1)
	{
		if(value < UART_BAUDRATE_MAX)
			memcpy(temp_buffer, c_strBaudate[value], 6);
		else
			sprintf(temp_buffer, "val    %u", value);
	}
	else if(row == 2)
	{
		if(value == 0) memcpy(temp_buffer, "BACNET", 6);
		else memcpy(temp_buffer, "MODBUS", 6);
	}
	temp_buffer[6] = '\0';
	disp_str_16_24(FORM15X30, SCH_XPOS + 118, ypos, (uint8 *)temp_buffer, SCH_COLOR, TSTAT8_MENU_COLOR);
}

static void Save_Comm_Parameter(uint8 item)
{
	switch (item)
    {
    case 0: // address
        Modbus.address = set_value;
        save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
        Station_NUM = Modbus.address;
        Setting_Info.reg.MSTP_ID = Station_NUM;
        Setting_Info.reg.modbus_id = Station_NUM;
        if (Modbus.protocal == 0)
            dlmstp_init(NULL);
        break;
    case 1:  // baudrate
    {
    	Modbus.baudrate[0] = set_value;
        Setting_Info.reg.com_baudrate[0] = Modbus.baudrate[0];
        uart_init(0);
        save_uint8_to_flash(FLASH_BAUD_RATE, Modbus.baudrate[0]);
		Count_com_config();
    }
    break;
    case 2:  // protocol
    {
        if (set_value == 0)
            set_value = BACNET_MASTER;
        else
            set_value = MODBUS_SLAVE;

        Modbus.com_config[0] = set_value;
        Setting_Info.reg.com_config[0] = set_value;
		Modbus.protocal = (set_value == BACNET_MASTER) ? 0 : 1;

        if (Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
            dlmstp_init(NULL);
        save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
		uart_init(0);
		Count_com_config();
    }
        break;
    default:
        break;
    }
}

void MenuMain_display(void)
{
	uint8 current_val_0 = (uint8)get_display_value(0);
	uint8 current_val_1 = (uint8)get_display_value(1);
	uint8 current_val_2 = current_protocol_setting();

	if(!boxes_drawn)
	{
		draw_tangle(125, SETPOINT_POS - 3);
		draw_tangle(125, FAN_MODE_POS - 8);
		draw_tangle(125, SYS_MODE_POS - 13);
		boxes_drawn = true;
	}

	if(prev_item != item_to_adjust)
	{
		render_labels(item_to_adjust);
		prev_item = item_to_adjust;
	}

	if(prev_addr != current_val_0)
	{
		render_value_row(0, current_val_0);
		prev_addr = current_val_0;
	}
	if(prev_baud != current_val_1)
	{
		render_value_row(1, current_val_1);
		prev_baud = current_val_1;
	}
	if(prev_proto != current_val_2)
	{
		render_value_row(2, current_val_2);
		prev_proto = current_val_2;
	}
}

void MenuMain_keycope(uint16 key_value)
{
	switch(key_value & KEY_SPEED_MASK)
	{
		case 0:
			break;
		case KEY_UP_MASK:
			set_value = (uint16)get_display_value(item_to_adjust);
			if(set_value < menu[item_to_adjust].max)
			{
				set_value++;
				Save_Comm_Parameter(item_to_adjust);
			}
			break;
		case KEY_DOWN_MASK:
			set_value = (uint16)get_display_value(item_to_adjust);
			if(set_value > menu[item_to_adjust].min)
			{
				set_value--;
				Save_Comm_Parameter(item_to_adjust);
			}
			break;
		case KEY_LEFT_MASK:
			update_menu_state(MenuIdle);
			break;
		case KEY_RIGHT_MASK:
			item_to_adjust++;
			if(item_to_adjust >= MAX_MENU_NUM)
			{
				item_to_adjust = 0;
				update_menu_state(MenuIdle);
			}
			break;
		default:
			break;
	}
}

void draw_tangle(uint8 xpos, uint16 ypos)
{
	uint8_t xEndPos = 113;
	uint8_t yEndPos = 34;
	if(xpos > 110)	{xEndPos = 94; yEndPos = 25;}
    disp_edge     (8, 8, leftup, 	xpos,	ypos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
    disp_null_icon(xEndPos, 2, 0, xpos+6,ypos+2,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(3, yEndPos-6, 0, xpos+2,ypos+8,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_edge     (8, 8, leftdown, 	xpos,	ypos+yEndPos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
    disp_null_icon(xEndPos, 2, 0, xpos+6,ypos+yEndPos+5,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_edge     (8, 8, rightdown, 	xpos+xEndPos+2,	ypos+yEndPos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
    disp_edge     (8, 8, rightup, 	xpos+xEndPos+2,	ypos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

    disp_null_icon(2, yEndPos-6, 0, xpos+xEndPos+7,ypos+8,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    disp_null_icon(xEndPos+2, 2, 0, xpos+5,ypos+yEndPos+6,TSTAT8_CH_COLOR, TANGLE_COLOR);
    disp_null_icon(xEndPos+2, 2, 0, xpos+5,ypos,TSTAT8_CH_COLOR, TANGLE_COLOR);
    disp_null_icon(2, yEndPos-2, 0, xpos,ypos+6,TSTAT8_CH_COLOR, TANGLE_COLOR);
    disp_null_icon(2, yEndPos-2, 0, xpos+xEndPos+8,ypos+6,TSTAT8_CH_COLOR, TANGLE_COLOR);
}

/* End of MenuMain.c */
