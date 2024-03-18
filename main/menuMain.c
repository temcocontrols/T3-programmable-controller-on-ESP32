#include "define.h"
#include "LCD_TSTAT.h"
#include "menu.h"
#include "bacnet.h"
#include "user_data.h"
#include "modbus.h"


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

#define MAX_MENU_NUM 4
MENUNAME const menu[MAX_MENU_NUM] = {//Added setpoint and fan mode to menu
{	"Modbus","Address", 254 , 0},
{ "Baudrate","Select ",9,0 },		//08 Baudrate
{ "Protocol","Select",1,0 },
{ "Set     ","Schedule",8,0 }
};
/*const char c_strSch[MAX_WR][7] =
{
    "1     ",   //0
    "2     ",   //1
    "3     ",   //2
    "4     ",   //3
    "5     ",   //4
    "6     ",   //5
    "7     ",  //6
    "8     ",  //7
};
*/
uint8 item_to_adjust;
//extern uint8_t current_schedule;
void start_menu(void)
{
	uint8 i;
	uint8 menu_buf1[10];
	uint8 menu_buf2[10];
//	clear_lines();

	for(i=0;i<10;i++)
	{
		menu_buf1[i] = menu[item_to_adjust].first_line[i];//
		menu_buf2[i] = menu[item_to_adjust].second_line[i];//
	}

	display_menu(menu_buf1,menu_buf2);
}

void MenuMain_init(void)
{
	item_to_adjust = 0;
//	clear_line(1);
//	clear_line(0);
	clear_lines();
	start_menu();
}
 
uint32 get_display_value(uint8 item)
{

	if(item == 0) 		
        return Modbus.address;
    else if (item == 1)
        return  Setting_Info.reg.com_baudrate[0];//      Modbus.baudrate;
    else if (item == 2)
        return Modbus.protocal;
	else	//if(item == 3)			
		return 0;
	
}


void show_parameter(void)
{
    char temp_buffer[10];
    memset(temp_buffer, 0, 0);
    switch (item_to_adjust)
    {
    case 0:
        display_value(0, get_display_value(item_to_adjust), ' ');
        break;
    case 1:
    {
        //vars[108].value = Setting_Info.reg.com_baudrate[0];
        switch (Setting_Info.reg.com_baudrate[0])
        {
            //case UART_1200:	    //Fandu  ��һ����������ʱ��������
            //case UART_2400:	   
            //case UART_3600:	   
            //case UART_4800:	   
            //case UART_7200:	   
            //case UART_921600:	

        case UART_9600:
        case UART_19200:
        case UART_38400:
        case UART_57600:
        case UART_76800:
        case UART_115200:
            memcpy(temp_buffer, c_strBaudate[Setting_Info.reg.com_baudrate[0]], 7);
            break;
        default:
            sprintf(temp_buffer, "val %d", Setting_Info.reg.com_baudrate[0]);
            break;

        }
        disp_str(FORM15X30, 0, MENU_VALUE_POS, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
    }
    break;
    case 2:
    {
        if ((Setting_Info.reg.com_config[0] == BACNET_SLAVE) ||
            (Setting_Info.reg.com_config[0] == BACNET_MASTER))
            disp_str(FORM15X30, 0, MENU_VALUE_POS, "BACNET", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
        else
            disp_str(FORM15X30, 0, MENU_VALUE_POS, "MODBUS", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
    }
        //if (Setting_Info.reg.com_config[0] == 0)
        //    disp_str(FORM15X30, 0, MENU_VALUE_POS, "BACNET", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
        //else
        //    disp_str(FORM15X30, 0, MENU_VALUE_POS, "MODBUS", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
        break;
		/* case 3:
        memcpy(temp_buffer, c_strSch[current_schedule], 7);						
        disp_str(FORM15X30, 0, MENU_VALUE_POS, temp_buffer, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
  
		 break;*/
    default:
        break;
    }
}

void MenuMain_display(void)
{
//	if(item_to_adjust == 1)
//	{
//		char text[20];
//		sprintf(text, "%u.%u.%u.%u", (uint16)Modbus.ip_addr[0], (uint16)Modbus.ip_addr[1], (uint16)Modbus.ip_addr[2], (uint16)Modbus.ip_addr[3]);
//		disp_str(FORM15X30, 0, SYS_MODE_POS,  text,  TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
//	}
//	else	
	{		
        show_parameter();
		//display_value(0,get_display_value(item_to_adjust), ' '); 	
	}
}

void MenuMain_keycope(uint16 key_value)
{
	switch(key_value & KEY_SPEED_MASK)
	{
		case 0:
			// do nothing
			break;
		case KEY_UP_MASK:
			if(item_to_adjust < MAX_MENU_NUM - 1)	
				item_to_adjust++; 
			start_menu();
			break;
		case KEY_DOWN_MASK:
			if(item_to_adjust > 0)	
				item_to_adjust--; 
			start_menu();
			break;
		case KEY_LEFT_MASK:
			update_menu_state(MenuIdle);
			break;
		case KEY_RIGHT_MASK:
			// go into main menu
			// change value
			update_menu_state(MenuSet);
			break;
	}
}

void display_menu (uint8 *item1, uint8 *item2)
{
	clear_line(1);
	disp_str(FORM15X30, 0,SETPOINT_POS,(char *)item1,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	disp_str(FORM15X30, 0,FAN_MODE_POS,(char *)item2,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
}

void draw_tangle(uint8 xpos, uint16 ypos)
{
		disp_icon(8, 8, leftup, 	xpos,	ypos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

		disp_null_icon(113, 1, 0, xpos+6,ypos+2,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

		disp_null_icon(2, 28, 0, xpos+2,ypos+8,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

		disp_icon(8, 8, leftdown, 	xpos,	ypos+34, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

		disp_null_icon(113, 1, 0, xpos+6,ypos+39,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

		disp_icon(8, 8, rightdown, 	xpos+115,	ypos+34, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

		disp_icon(8, 8, rightup, 	xpos+115,	ypos, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);


		disp_null_icon(1, 28, 0, xpos+120,ypos+8,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

		disp_null_icon(115, 2, 0, xpos+5,ypos+40,TSTAT8_CH_COLOR, TANGLE_COLOR);
	  disp_null_icon(115, 2, 0, xpos+5,ypos,TSTAT8_CH_COLOR, TANGLE_COLOR);
	  disp_null_icon(2, 32, 0, xpos,ypos+6,TSTAT8_CH_COLOR, TANGLE_COLOR);
		disp_null_icon(2, 32, 0, xpos+121,ypos+6,TSTAT8_CH_COLOR, TANGLE_COLOR);

}

