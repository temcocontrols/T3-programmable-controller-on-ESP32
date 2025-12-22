#include "define.h"
#include "LCD_TSTAT.h"
#include "menu.h"
#include "wifi.h"
#include "bacnet.h"
#include "user_data.h"
#include <string.h>
#include "rtc.h"
#include "driver/uart.h"
#include "LcdTheme.h"
#include "airlab.h"

#define	NODES_POLL_PERIOD	30

#define DISPLAY_VAL_LENTH   5

char UI_DIS_LINE1[4];  // Corresponds to setpoint, fan, and system
char UI_DIS_LINE2[4];
char UI_DIS_LINE3[4];
char UI_DIS_TOP[9];

uint8 *scroll;
uint8 scroll_buf[SCROLL_WINDOW + 1];
uint8 scroll_ram[5][MAX_SCOROLL];
uint8 scroll_fan = 0;
uint16_t scroll_index = 0;

static uint8 display_around_time_ctr = NODES_POLL_PERIOD;
static uint8 disp_index = 0;
static uint8 set_msv = 0;
static uint8 warming_state = TRUE;
static uint8 force_refresh = TRUE;
uint8 flag_left_key = 0;
uint8	count_left_key = 0;
uint8 flag_digital_top_area = 0;
uint8 digital_top_area_type = 0;
uint8 digital_top_area_num = 0;
uint8 digital_top_area_changed = 0;
void set_output_raw(uint8_t point,uint16_t value);
extern uint16_t count_suspend_mstp;
extern uint8_t ChangeFlash;
static bool IsHomeScreen = false;

void MenuIdle_init(void)
{
	uint8 i,j;
	Str_points_ptr ptr;
	//LCDtest();
	ClearScreen(TSTAT8_BACK_COLOR);
	flag_digital_top_area = 0;
	digital_top_area_type = 0;
	digital_top_area_num = 0;
	digital_top_area_changed = 0;
	if(Modbus.mini_type == PROJECT_AIRLAB)
	{
		Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type = IN;
		Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number  = 1;

		Modbus.disable_tstat10_display = 3;
	}
	digital_top_area_type = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type;
	digital_top_area_num = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number - 1;

	memset(UI_DIS_TOP,0,9);
	digital_top_area_changed = 0;

	IsHomeScreen = false;

	if(IsHomeScreen == false)
	{
		DisplayHomeScreen(IsHomeScreen);
		IsHomeScreen = true;
	}
	else
	{
		disp_str(FORM15X30, SCH_XPOS,  0, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
		disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "            ",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
		disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
		disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT * 2 - 7, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);

		if(digital_top_area_type == IN)
		{
			ptr = put_io_buf(IN,digital_top_area_num);
			memcpy(UI_DIS_TOP, ptr.pin->label, 9);
		}
		else if(digital_top_area_type == OUT)
		{
			ptr = put_io_buf(OUT,digital_top_area_num);
			memcpy(UI_DIS_TOP, ptr.pout->label, 9);
		}
		else if(digital_top_area_type == VAR)
		{
			ptr = put_io_buf(VAR,digital_top_area_num);
			memcpy(UI_DIS_TOP, ptr.pvar->label, 9);
		}

		disp_null_icon(240, 29, 0, 0,TIME_POS,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

		ptr = put_io_buf(VAR,0);
		memcpy(UI_DIS_LINE1, ptr.pvar->label, 3);UI_DIS_LINE1[3] = 0;
		if(ptr.pvar->range != 0)
		{
			draw_tangle(102,112);
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
		}
		ptr = put_io_buf(VAR,1);
		memcpy(UI_DIS_LINE2, ptr.pvar->label, 3);UI_DIS_LINE2[3] = 0;
		if(ptr.pvar->range != 0)
		{
			draw_tangle(102,155);
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
		}
		ptr = put_io_buf(VAR,2);
		memcpy(UI_DIS_LINE3, ptr.pvar->label, 3);UI_DIS_LINE3[3] = 0;
		if(ptr.pvar->range != 0)
		{
			draw_tangle(102,198);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);
		}
	}
	//msv_data[MAX_MSV][STR_MSV_MULTIPLE_COUNT]
	for (i = 0;i < MAX_MSV;i++)
		for (j = 0; j < STR_MSV_MULTIPLE_COUNT;j++)
		{
			if(msv_data[i][j].status == 255)
			{
				msv_data[i][j].status = 0;
			}
		}
}


void get_data_format(u8 loc,float num,char *s)
{
	u8 i,s_len,s_start,buf_start;

	if(loc == 0)
		sprintf(s,"%9.0f",num);
	else if(loc == 1)
		sprintf(s,"%9.1f",num);
	else if(loc == 2)
		sprintf(s,"%9.2f",num);
	else if(loc == 3)
		sprintf(s,"%9.3f",num);
	else if(loc == 4)
		sprintf(s,"%9.4f",num);
	else if(loc == 5)
		sprintf(s,"%9.5f",num);
	else if(loc == 6)
		sprintf(s,"%9.6f",num);
	else
		sprintf(s,"%f",num);

	for(i=0;i<9;i++)
	{
		if(s[i]!= 0x20) break;
	}
	s_len = 9 - i;   					//���ݳ���
	s_start = i;     					//������ʼλ��
	buf_start = i - i / 2; 				//�������ú����ʼλ��

	for(i=0;i<s_len;i++) 				//��������
	{
		s[buf_start + i] = s[s_start + i];
	}
	for(i=buf_start + s_len;i<9;i++ ) 	//��" "
	{
		s[i] = 0x20;
	}
}

void DisplayMenuScreen(void)
{
   	static u8 count_tx = 0;
	static u8 count_rx = 0;
	Str_points_ptr ptr;
	ptr = put_io_buf(VAR,0);
	if(memcmp(UI_DIS_LINE1,ptr.pvar->label,3) || ptr.pvar->range == 0)
	{
		disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, "           ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE1,'\0',4);
		memcpy(UI_DIS_LINE1, ptr.pvar->label, 3);
		//disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, "    ",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	if(ptr.pvar->range != 0)
	{

		display_screen_value( 1);
		draw_tangle(102,112);
		// Display the three lines with different background colors based on disp_index
		disp_str(FORM15X30, SCH_XPOS, SETPOINT_POS, UI_DIS_LINE1, SCH_COLOR,(disp_index == 1) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR);
	}

	ptr = put_io_buf(VAR,1);
	if(memcmp(UI_DIS_LINE2,ptr.pvar->label,3) || ptr.pvar->range == 0)
	{
		disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, "           ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE2,'\0',4);
		memcpy(UI_DIS_LINE2, ptr.pvar->label, 3);
		//disp_str(FORM15X30, SCH_XPOS, FAN_MODE_POS, "    ", SCH_COLOR, TSTAT8_BACK_COLOR);
	}
	if(ptr.pvar->range != 0)
	{
		display_screen_value( 2);
		draw_tangle(102,155);
		// Display the three lines with different background colors based on disp_index
		disp_str(FORM15X30, SCH_XPOS, FAN_MODE_POS, UI_DIS_LINE2, SCH_COLOR, (disp_index == 2) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR);
	}


	ptr = put_io_buf(VAR,2);
	if(memcmp(UI_DIS_LINE3,ptr.pvar->label,3) || ptr.pvar->range == 0)
	{
		disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, "           ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE3,'\0',4);
		memcpy(UI_DIS_LINE3, ptr.pvar->label, 3);
		//disp_str(FORM15X30, SCH_XPOS, SYS_MODE_POS, "    ", SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	if(ptr.pvar->range != 0)
	{
		display_screen_value( 3);
		draw_tangle(102,198);
		// Display the three lines with different background colors based on disp_index
		disp_str(FORM15X30, SCH_XPOS, SYS_MODE_POS, UI_DIS_LINE3, SCH_COLOR,(disp_index == 3) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR);
	}


	if(Modbus.disable_tstat10_display == 0)
	{
		display_scroll();
		display_icon();
		display_fan();
	}
	else if(Modbus.disable_tstat10_display == 1)
	{
		disp_str(FORM15X30, 0,TIME_POS,"            ",TSTAT8_CH_COLOR,TSTAT8_MENU_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, FIRST_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, SECOND_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, THIRD_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, FOURTH_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
	}
	else if(Modbus.disable_tstat10_display == 2)
	{
		display_scroll();
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, FIRST_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, SECOND_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, THIRD_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, FOURTH_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
	}
	else if(Modbus.disable_tstat10_display == 3)  // for AL_BTN, show PM2.5
	{
		if(Modbus.mini_type == PROJECT_AIRLAB)
		{
			if(flag_pm25 == 0)
			{
				disp_pm25_weight_25 = 0;
				disp_pm25_number_25 = 0;
			}
			display_pm25w( disp_pm25_weight_25);//disp_pm25_weight_25
			display_pm25n( disp_pm25_number_25);

		}
	}
		{
			uint8 type,num;
			//if(Setting_Info.reg.display_lcd.lcddisplay[0] == 1) // modbus
			if(Modbus.mini_type == MINI_TSTAT10)
			{
				if(digital_top_area_type != Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type)
				{
					digital_top_area_type = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type;
					digital_top_area_changed = 1;
				}

				if(digital_top_area_num != Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number - 1)
				{
					digital_top_area_num = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number - 1;
					digital_top_area_changed = 1;
				}
			}

				type = digital_top_area_type;
				num = digital_top_area_num;

				if(digital_top_area_changed)
				{
					digital_top_area_changed = 0;
					disp_str(FORM15X30, SCH_XPOS,  0, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
					disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "            ",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
					disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
					disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT * 2 - 7, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
				}

				if(type == IN)
				{
					ptr = put_io_buf(IN,num);
					if(ptr.pin->digital_analog == 1)
					{
						flag_digital_top_area = 0;
						if(ptr.pin->range == 4/* R10K_40_250DegF*/)
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pin->value / 100, TOP_AREA_DISP_UNIT_F);
						else if(ptr.pin->range == 3/*R10K_40_120DegC*/)
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pin->value / 100, TOP_AREA_DISP_UNIT_C);
						else if(ptr.pin->range == 27)  // humidity
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pin->value / 100, TOP_AREA_DISP_UNIT_RH);
						else
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pin->value / 1000, TOP_AREA_DISP_UNIT_NONE);
					}
					else
					{
						flag_digital_top_area = 1;
						ptr = put_io_buf(IN,digital_top_area_num);
						memcpy(UI_DIS_TOP, ptr.pin->label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						ptr = put_io_buf(IN,num);
						if(ptr.pin->control)
						{
							if(ptr.pin->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STRAT",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(ptr.pin->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pin->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE1_POS, "LOW",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);
						}

					}
				}
				else if(type == OUT)
				{
					ptr = put_io_buf(OUT,num);
					if(ptr.pout->digital_analog == 1)
					{
						flag_digital_top_area = 0;
					}
					else
					{
						flag_digital_top_area = 1;
						ptr = put_io_buf(OUT,digital_top_area_num);
						memcpy(UI_DIS_TOP, ptr.pout->label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						if(ptr.pout->control)
						{
							if(ptr.pout->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "START",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(ptr.pout->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pout->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE1_POS, "LOW",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);
						}

					}
				}
				else if(type == VAR)
				{
					ptr = put_io_buf(VAR,num);
					if(ptr.pvar->digital_analog == 1)
					{
						flag_digital_top_area = 0;
						if(ptr.pvar->range == degF) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 100, TOP_AREA_DISP_UNIT_F);
						}
						else	if(ptr.pvar->range == degC) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 100, TOP_AREA_DISP_UNIT_C);
						}
						else	if(ptr.pvar->range == KPa) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 1000, TOP_AREA_DISP_UNIT_kPa);
						}
						else	if(ptr.pvar->range == Pa) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 1000, TOP_AREA_DISP_UNIT_Pa);
						}
						else	if(ptr.pvar->range == RH)
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 1000, TOP_AREA_DISP_UNIT_RH);
						}
						else
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, ptr.pvar->value / 1000, TOP_AREA_DISP_UNIT_NONE);
					}
					else
					{
						flag_digital_top_area = 1;
						ptr = put_io_buf(VAR,digital_top_area_num);
						memcpy(UI_DIS_TOP, ptr.pvar->label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						ptr = put_io_buf(VAR,num);
						if(ptr.pvar->control)
						{
							if(ptr.pvar->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "START",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(ptr.pvar->range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(ptr.pvar->range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE1_POS, "LOW",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);
						}

					}
//					else
//					{
//						Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].control, TOP_AREA_DISP_UNIT_NONE);
//					}
//					else
//					{// uint is not
//						// tbd: add more
//
//					}
				}
				// ..... tbd: add more type

// 			if(Setting_Info.reg.display_lcd.lcddisplay[0] == 1) // bacnet
//			{
//			}

		}

		if(count_left_key > 5)
			disp_index = 0;
		else
			count_left_key++;

		if (flag_digital_top_area == 1)
		{
		    disp_str_16_24(FORM15X30, SCH_XPOS + 20, IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP, SCH_COLOR,
		                   (disp_index == 4) ? TSTAT8_BACK_COLOR1 : TSTAT8_BACK_COLOR);
		}



	if(SSID_Info.IP_Wifi_Status == WIFI_NORMAL || SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)//����Ļ���Ͻ���ʾwifi��״̬
	{
		if(SSID_Info.rssi < -80)
			disp_edge(26, 26, wifi_1, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else if(SSID_Info.rssi < -70)
			disp_edge(26, 26, wifi_2, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else if(SSID_Info.rssi < -60)
			disp_edge(26, 26, wifi_3, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_edge(26, 26, wifi_4, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

	}
	else if((SSID_Info.IP_Wifi_Status == WIFI_NO_CONNECT) ||
			(SSID_Info.IP_Wifi_Status == WIFI_SSID_FAIL) ||
			(SSID_Info.IP_Wifi_Status == WIFI_DISCONNECTED)	)
			disp_edge(26, 26, wifi_0, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		// if WIFI_NONE, do not show wifi flag
	else //if((SSID_Info.IP_Wifi_Status == WIFI_NO_WIFI)
		disp_edge(26, 26, wifi_none, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

	// show TX,RX

	if(flagLED_sub_tx > 0)
	{
		if(count_tx++ % 2 == 0)
			disp_edge(13, 26, cmnct_send, 	0,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_null_icon(13, 26, 0, 0,0,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
	else
	{
		count_tx = 0;
		disp_null_icon(13, 26, 0, 0,0,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);//(26, 26, cmnct_icon, 	0,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}


	if(flagLED_sub_rx > 0)
	{
		// if TX on, then RX off
		if(count_tx % 2 == 1)
			count_rx = 0;
		if(count_rx++ % 2 == 1)
			disp_edge(13, 26, cmnct_rcv, 	13,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_null_icon(13, 26, 0, 13,0,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
	else
	{
		count_rx = 0;
		disp_null_icon(13, 26, 0, 13,0,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);//(26, 26, cmnct_icon, 	0,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}

	if(flagLED_sub_tx > 0)
		flagLED_sub_tx = 0;
	if(flagLED_sub_rx > 0)
		flagLED_sub_rx = 0;

}

void MenuIdle_display(void)
{
	if(disp_index)
	{
		if(IsHomeScreen)
		{
			ClearScreen(TSTAT8_BACK_COLOR);
			IsHomeScreen = false;
		}
		DisplayMenuScreen();
	}
	else
	{
		DisplayHomeScreen(IsHomeScreen);
		if(!IsHomeScreen)
		{
			IsHomeScreen = true;
		}
	}
}

uint8_t check_msv_data_len(uint8_t index)
{
	uint8_t j;
	uint8_t len;
	len = 0;

	for(j = 0; j < STR_MSV_MULTIPLE_COUNT;j++)
	{
		if(msv_data[index][j].status != 0)
		{
			len++;
		}
		else
		{
			return len;
		}
	}
	return len;
}


void MenuIdle_keycope(uint16 key_value)
{
    uint8 i;
    uint8 temp_value = 0;
    Str_points_ptr ptr;
	switch(key_value /*& KEY_SPEED_MASK*/)
	{
		case 0:
			break;
		case KEY_UP_MASK:
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				ptr = put_io_buf(VAR,disp_index - 1);
				if ((ptr.pvar->range >= 101) && (ptr.pvar->range <= 103))  // 101 102 103 	MSV range
				{
//					if (vars[disp_index - 1].range == 101)  //�ж�range �ǲ��Ƕ�̬���ǵĻ� ������̬��ֵ;
					{
						uint8 len;
						len = check_msv_data_len(disp_index - 1);
						for (i = 0; i < len; i++)
						{
							if (ptr.pvar->value / 1000 == msv_data[disp_index - 1][i].msv_value)
							{
								temp_value = i;
								break;
							}
						}

						for (i = temp_value; i < 7; i++)
						{
							if(strlen(msv_data[disp_index - 1][i + 1].msv_name) != 0
								&& msv_data[disp_index - 1][i + 1].msv_name[0] != 0xff)
							{
								ptr.pvar->value = msv_data[disp_index - 1][i + 1].msv_value * 1000;
								break;
							}
						}
					}
//					else
//					{
//						if(vars[disp_index - 1].value < STR_MSV_MULTIPLE_COUNT * 1000)
//							vars[disp_index - 1].value = vars[disp_index - 1].value + 1000;
//						else
//							vars[disp_index - 1].value = 0;
//					}
				}
				else
				{
					if(ptr.pvar->digital_analog == 0)
					{
						if(ptr.pvar->control == 0)
							ptr.pvar->control = 1;
						else
							ptr.pvar->control = 0;
					}
					else
					{Test[27]++;
						if(ptr.pvar->value < 999 * 1000)
							ptr.pvar->value = ptr.pvar->value + 1000;
						else
							ptr.pvar->value = 0;
					}
				}
			}
			else // disp_index == 4
			{
				if(digital_top_area_type == IN)
				{
					ptr = put_io_buf(IN,digital_top_area_num);
					if(ptr.pin->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pin->control = ((ptr.pin->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == VAR)
				{
					ptr = put_io_buf(VAR,digital_top_area_num);
					if(ptr.pvar->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pvar->control = ((ptr.pvar->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == OUT)
				{
					ptr = put_io_buf(OUT,digital_top_area_num);
					if(ptr.pout->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pout->control = ((ptr.pout->control) == 0) ? 1 : 0;
						if(ptr.pout->control)
							set_output_raw(digital_top_area_num,1000);
						else
							set_output_raw(digital_top_area_num,0);
					}
				}
			}

			ChangeFlash = 1;
			break;
		case KEY_SPEED_10 | KEY_UP_MASK:
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				ptr = put_io_buf(VAR,disp_index - 1);
				if ((ptr.pvar->range >= 101) && (ptr.pvar->range <= 103))  // 101 102 103 	MSV range
				{
					char len;
					len = check_msv_data_len(disp_index - 1);
					for (i = 0; i < len; i++)
					{
						if (ptr.pvar->value / 1000 == msv_data[disp_index - 1][i].msv_value)
						{
							temp_value = i;
							break;
						}
					}

					for (i = temp_value; i < 7; i++)
					{
						if(strlen(msv_data[disp_index - 1][i + 1].msv_name) != 0
							&& msv_data[disp_index - 1][i + 1].msv_name[0] != 0xff)
						{
							ptr.pvar->value = msv_data[disp_index - 1][i + 1].msv_value * 1000;
							break;
						}
					}
				}
				else
				{
					if(ptr.pvar->digital_analog == 0)
					{
						if(ptr.pvar->control == 0)
							ptr.pvar->control = 1;
						else
							ptr.pvar->control = 0;
					}
					else
					{
						if(ptr.pvar->value < 999 * 1000)
							ptr.pvar->value = ptr.pvar->value + 10000;
						else
							ptr.pvar->value = 0;
					}
				}
			}
			else // disp_index == 4
			{

				if(digital_top_area_type == IN)
				{
					ptr = put_io_buf(IN,digital_top_area_num);
					if(ptr.pin->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pin->control = ((ptr.pin->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == VAR)
				{
					ptr = put_io_buf(VAR,digital_top_area_num);
					if(ptr.pvar->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pvar->control = ((ptr.pvar->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == OUT)
				{
					ptr = put_io_buf(OUT,digital_top_area_num);
					if(ptr.pout->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pout->control = ((ptr.pout->control) == 0) ? 1 : 0;
						if(ptr.pout->control)
							set_output_raw(digital_top_area_num,1000);
						else
							set_output_raw(digital_top_area_num,0);
					}
				}
			}

			ChangeFlash = 1;
			break;

		case KEY_DOWN_MASK:
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				ptr = put_io_buf(VAR,disp_index - 1);
				if ((ptr.pvar->range >= 101) && (ptr.pvar->range <= 103))  // 101 102 103 	MSV range
				{
					//if(vars[disp_index - 1].range == 101)  //�ж�range �ǲ��Ƕ�̬���ǵĻ� ������̬��ֵ;
					{
						// check the lenght of msv_data
						char len;
						len = check_msv_data_len(disp_index - 1);
							for (i = 0; i < len; i++)
							{
									if (ptr.pvar->value / 1000 == msv_data[disp_index - 1][i].msv_value)
									{
											temp_value = i;
											break;
									}
							}

							for (i = temp_value; i > 0; i--)
							{
									if (strlen(msv_data[disp_index - 1][i - 1].msv_name) != 0)
									{
										ptr.pvar->value = msv_data[disp_index - 1][i - 1].msv_value * 1000;
										break;
									}
							}
					}
//					else
//					{
//						if(vars[disp_index - 1].value > 1000)
//							vars[disp_index - 1].value = vars[disp_index - 1].value - 1000;
//						else
//							vars[disp_index - 1].value = STR_MSV_MULTIPLE_COUNT * 1000;
//					}
				}
				else
				{
					if(ptr.pvar->digital_analog == 0)
					{
						if(ptr.pvar->control == 0)
							ptr.pvar->control = 1;
						else
							ptr.pvar->control = 0;
					}
					else
					{
//					if(vars[disp_index - 1].value > 1000)
						ptr.pvar->value = ptr.pvar->value - 1000;
//						else
//							vars[disp_index - 1].value = 99 * 1000;
					}
				}
			}
			else // disp_index == 4
			{

				if(digital_top_area_type == IN)
				{
					ptr = put_io_buf(IN,digital_top_area_num);
					if(ptr.pin->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pin->control = ((ptr.pin->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == VAR)
				{
					ptr = put_io_buf(VAR,digital_top_area_num);
					if(ptr.pvar->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pvar->control = ((ptr.pvar->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == OUT)
				{
					ptr = put_io_buf(OUT,digital_top_area_num);
					if(ptr.pout->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pout->control = ((ptr.pout->control) == 0) ? 1 : 0;
						if(ptr.pout->control)
							set_output_raw(digital_top_area_num,1000);
						else
							set_output_raw(digital_top_area_num,0);
					}
				}
			}


			ChangeFlash = 1;
			break;
		case KEY_SPEED_10 | KEY_DOWN_MASK:
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				ptr = put_io_buf(VAR,disp_index - 1);
				if ((ptr.pvar->range >= 101) && (ptr.pvar->range <= 103))  // 101 102 103 	MSV range
				{
					char len;
					len = check_msv_data_len(disp_index - 1);
					for (i = 0; i < len; i++)
					{
						if (ptr.pvar->value / 1000 == msv_data[disp_index - 1][i].msv_value)
						{
							temp_value = i;
							break;
						}
					}

					for (i = temp_value; i > 0; i--)
					{
						if (strlen(msv_data[disp_index - 1][i - 1].msv_name) != 0)
						{
							ptr.pvar->value = msv_data[disp_index - 1][i - 1].msv_value * 1000;
							break;
						}
					}
				}
				else
				{
					if(ptr.pvar->digital_analog == 0)
					{
						if(ptr.pvar->control == 0)
							ptr.pvar->control = 1;
						else
							ptr.pvar->control = 0;
					}
					else
					{
						ptr.pvar->value =ptr.pvar->value - 10000;
					}
				}
			}
			else // disp_index == 4
			{

				if(digital_top_area_type == IN)
				{
					ptr = put_io_buf(IN,digital_top_area_num);
					if(ptr.pin->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pin->control = ((ptr.pin->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == VAR)
				{
					ptr = put_io_buf(VAR,digital_top_area_num);
					if(ptr.pvar->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pvar->control = ((ptr.pvar->control) == 0) ? 1 : 0;
					}
				}
				else if(digital_top_area_type == OUT)
				{
					ptr = put_io_buf(OUT,digital_top_area_num);
					if(ptr.pout->digital_analog == 0)
					{
						digital_top_area_changed = 1;
						ptr.pout->control = ((ptr.pout->control) == 0) ? 1 : 0;
						if(ptr.pout->control)
							set_output_raw(digital_top_area_num,1000);
						else
							set_output_raw(digital_top_area_num,0);
					}
				}
			}

			ChangeFlash = 1;
			break;

		case KEY_LEFT_MASK:
			// change SETP, FAN , SYS
			if(flag_digital_top_area == 1)
			{
				if(disp_index < 4) disp_index++;
				else
					disp_index = 1;
			}
			else
			{
				if(disp_index < 3) disp_index++;
				else
					disp_index = 1;
			}
			flag_left_key = 1;
			count_left_key = 0;
			break;
		case KEY_RIGHT_MASK:
			// go into main menu
			//vars[19].value += 1000;
			break;
		case KEY_LEFT_RIGHT_MASK:
			update_menu_state(MenuMain);
			break;
		default:
			break;
	}
}

//ÔÚÔ­ÏÈ setpoint  fan ºÍsysÎ»ÖÃ ÏÔÊ¾ in out var Öµ
void display_screen_value(uint8 type)
{
	Str_points_ptr ptr;
    int i = 0;
    uint8 spbuf[20];
    float show_value = 0;
    uint8 str_length = 0;

    memcpy(spbuf, "     ", strlen("     "));
    spbuf[5] = 0; //³õÊ¼»¯5¸ö×Ö½ÚÎª¿Õ¸ñ ±ÜÃâ µÚÒ»´ÎÏÔÊ¾12345 Öµ±äÎªABCµÄÊ±ºò   »áÏÔÊ¾ABC45
    ptr = put_io_buf(VAR,type - 1);
    if (type >= 1 && type <= 3)  // ÏÔÊ¾ÔÚLCDµÄÊý¾Ý¹Ì¶¨ÎªVAR1-VAR3
    {// ONLY var1-3 support MSV
		if ((ptr.pvar->range >= 101) && (ptr.pvar->range <= 103))  // 101 102 103 	MSV range
		{
			//sprintf(spbuf, "%d", msv_data[0][0].msv_value);
			for ( i = 0; i < 8; i++)
			{
				if ((ptr.pvar->value/1000) == msv_data[ptr.pvar->range - 101][i].msv_value)
				{
						str_length = strlen(msv_data[ptr.pvar->range - 101][i].msv_name);
						if (str_length >= 5)
							str_length = 5;
						memcpy(spbuf, msv_data[ptr.pvar->range - 101][i].msv_name, str_length);
						break;
				}
			}
		}
		else
		{
			show_value = ((float)ptr.pvar->value) / 1000;
			if(ptr.pvar->value / 1000 >= 10000)
				show_value = 9999;

			if(ptr.pvar->digital_analog == 0)
			{
				show_value = ptr.pvar->control ? 1 : 0;

				switch(ptr.pvar->range)
				{
					case OFF_ON:
						if(show_value == 0)			memcpy(spbuf, "OFF  ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "ON   ", DISPLAY_VAL_LENTH);
						break;
					case CLOSED_OPEN:
						if(show_value == 0)			memcpy(spbuf, "CLOSE", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "OPEN ", DISPLAY_VAL_LENTH);
						break;
					case STOP_START:
						if(show_value == 0)			memcpy(spbuf, "STOP ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "START", DISPLAY_VAL_LENTH);
						break;
					case DISABLED_ENABLED:
						if(show_value == 0)			memcpy(spbuf, "DISAB", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "ENABL", DISPLAY_VAL_LENTH);
						break;
					case NORMAL_ALARM:
						if(show_value == 0)			memcpy(spbuf, "NAORM", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "ALARM", DISPLAY_VAL_LENTH);
						break;
					case NORMAL_HIGH:
						if(show_value == 0)			memcpy(spbuf, "NAORM", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "HIGH ", DISPLAY_VAL_LENTH);
						break;
					case NORMAL_LOW:
						if(show_value == 0)			memcpy(spbuf, "NAORM", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "LOW  ", DISPLAY_VAL_LENTH);
						break;
					case NO_YES:
						if(show_value == 0)			memcpy(spbuf, "NO   ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "YES  ", DISPLAY_VAL_LENTH);
						break;
					case COOL_HEAT:
						if(show_value == 0)			memcpy(spbuf, "COOL ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "HEAT ", DISPLAY_VAL_LENTH);
						break;
					case UNOCCUPIED_OCCUPIED:
						if(show_value == 0)			memcpy(spbuf, "UNOCC", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "OCC  ", DISPLAY_VAL_LENTH);
						break;
					case LOW_HIGH:
						if(show_value == 0)			memcpy(spbuf, "LOW  ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "HIGH ", DISPLAY_VAL_LENTH);
						break;
					case ON_OFF:
						if(show_value == 0)			memcpy(spbuf, "ON   ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "OFF  ", DISPLAY_VAL_LENTH);
						break;
					case OPEN_CLOSED:
						if(show_value == 0)			memcpy(spbuf, "OPEN ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "CLOSE", DISPLAY_VAL_LENTH);
						break;
					case START_STOP:
						if(show_value == 0)			memcpy(spbuf, "START", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "STOP ", DISPLAY_VAL_LENTH);
						break;
					case ENABLED_DISABLED:
						if(show_value == 0)			memcpy(spbuf, "ENABL", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "DISAB", DISPLAY_VAL_LENTH);
						break;
					case ALARM_NORMAL:
						if(show_value == 0)			memcpy(spbuf, "ALARM", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "NORMA", DISPLAY_VAL_LENTH);
						break;
					case HIGH_NORMAL:
						if(show_value == 0)			memcpy(spbuf, "HIGH ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "NORMA", DISPLAY_VAL_LENTH);
						break;
					case LOW_NORMAL:
						if(show_value == 0)			memcpy(spbuf, "LOW  ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "NORMA", DISPLAY_VAL_LENTH);
						break;
					case YES_NO:
						if(show_value == 0)			memcpy(spbuf, "YES  ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "NO   ", DISPLAY_VAL_LENTH);
						break;
					case HEAT_COOL:
						if(show_value == 0)			memcpy(spbuf, "HEAT ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "COOL ", DISPLAY_VAL_LENTH);
						break;
					case OCCUPIED_UNOCCUPIED:
						if(show_value == 0)			memcpy(spbuf, "OCC  ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "UNOCC", DISPLAY_VAL_LENTH);
						break;
					case HIGH_LOW:
						if(show_value == 0)			memcpy(spbuf, "HIGH ", DISPLAY_VAL_LENTH);
						else						memcpy(spbuf, "LOW  ", DISPLAY_VAL_LENTH);
						break;
					default:
						break;
				}
			}
			else
			{
				if(ptr.pvar->range == degC || ptr.pvar->range == degF || ptr.pvar->range == Volts || ptr.pvar->range == RH)
				{// 带小数点
					if(show_value >= 1000)
					{
						sprintf((char *)spbuf, "%-5d",(int)show_value);
					}
					else if(ptr.pvar->value / 1000 >= 10)
						sprintf((char *)spbuf, "%.1f", show_value);
					else
					{
						if(ptr.pvar->value > 0)
						{
							sprintf((char *)spbuf, "%.2f", show_value);
						}
						else
						{  // '-' occupies the first position
							sprintf((char *)spbuf, "%.1f", show_value);
						}
					}
					// Add padding to make width = 5
					int len = strlen((char*)spbuf);
					if (len < DISPLAY_VAL_LENTH)
					{
						// Shift text to right and fill beginning with spaces
						memmove(spbuf + (DISPLAY_VAL_LENTH - len), spbuf, len + 1); // +1 for '\0'
						memset(spbuf, ' ', DISPLAY_VAL_LENTH - len);
					}
				}
				else
					sprintf((char *)spbuf, "%-5d",(int)show_value);
			}
		}
    }

	if(type == 1)
		disp_str(FORM15X30, SCH_XPOS + 96, SETPOINT_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR);
    else if (type == 2)
    {
    	disp_str(FORM15X30, SCH_XPOS + 96, FAN_MODE_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR);
    }
    else if (type == 3)
    {
    	disp_str(FORM15X30, SCH_XPOS + 96, SYS_MODE_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR);
    }
}

void display_SP(int16 disp_setpoint)
{
	uint8 spbuf[7];
	uint8 unit = 0;

	unit = 'C';

	if(disp_setpoint >= 0)
	{
		if(disp_setpoint >= 1000)
		{
			spbuf[0] = disp_setpoint/1000 + 0x30;
			spbuf[1] = (disp_setpoint%1000)/100 + 0x30;
			spbuf[2] = (disp_setpoint%100)/10 + 0x30;
			spbuf[3] = '.';
			spbuf[4] = (disp_setpoint%10) + 0x30;
			spbuf[5] =  '\0';
		}
		else if(disp_setpoint >= 100)
		{
			spbuf[0] = disp_setpoint/100 + 0x30;
			spbuf[1] = (disp_setpoint%100)/10 + 0x30;
			spbuf[2] = '.';
			spbuf[3] = (disp_setpoint%10) + 0x30;
			spbuf[4] = unit;
			spbuf[5] = '\0';
		}
		else if(disp_setpoint >= 10)
		{
			spbuf[0] = disp_setpoint/10 + 0x30;
			spbuf[1] = '.';
			spbuf[2] = (disp_setpoint%10) + 0x30;
			spbuf[4] = unit;
			spbuf[5] = '\0';
		}
		else
		{
			spbuf[0] = 0x30;

			spbuf[1] = '.';
			spbuf[2] = (disp_setpoint%10) + 0x30;
			spbuf[3] = unit;
			spbuf[4] = '\0';
			spbuf[5] = ' ';
		}
	//spbuf[6] = ' ';
	}
	else
	{
		if(disp_setpoint <= -1000)
		{
			spbuf[0] = '-';
			spbuf[1] = -disp_setpoint/1000 + 0x30;
			spbuf[2] = (-disp_setpoint%1000)/100 + 0x30;
			spbuf[3] = (-disp_setpoint%100)/10 + 0x30;
			spbuf[4] = '\0';
			//spbuf[5] = (-disp_setpoint%10) + 0x30;
			//spbuf[6] =  unit;
		}
		else if(disp_setpoint <= -100)
		{
			spbuf[0] = '-';
			spbuf[1] = -disp_setpoint/100 + 0x30;
			spbuf[2] = (-disp_setpoint%100)/10 + 0x30;
			spbuf[3] = unit;
			spbuf[4] = '\0';
		}
		else if(disp_setpoint <= -10)
		{
			spbuf[0] = '-';
			spbuf[1] = -disp_setpoint/10 + 0x30;
			spbuf[2] = '.';
			spbuf[3] = (-disp_setpoint%10) + 0x30;
			spbuf[4] = unit;
			spbuf[5] = '\0';
			spbuf[6] = ' ';
		}
		else
		{
			spbuf[0] = '-';
			spbuf[1] = 0x30;
			spbuf[2] = '.';
			spbuf[3] = (-disp_setpoint%10) + 0x30;
			spbuf[4] = unit;
			spbuf[5] = '\0';
			spbuf[6] = ' ';
		}
	}

	disp_str(FORM15X30, SCH_XPOS+96,SETPOINT_POS, (char *)spbuf,  SCH_COLOR,TSTAT8_MENU_COLOR);

}

uint8 lcd_rotate_max;
uint8 lcd_rotate_index;
void Refresh_scroll(void)
{
    memset(&scroll_ram[0][0],0x00, MAX_SCOROLL);

    if (SSID_Info.IP_Wifi_Status <= 1 || SSID_Info.IP_Wifi_Status == 3 || SSID_Info.IP_Wifi_Status == 4)
    {
        // Format time: YYYY-MM-DD HH:MM:SS
        uint8_t j = 0;
		scroll_ram[0][j++] = '2';
		scroll_ram[0][j++] = '0';
		scroll_ram[0][j++] = (rtc_date.year - 2000) / 10 + '0';
		scroll_ram[0][j++] = (rtc_date.year - 2000) % 10 + '0';
		scroll_ram[0][j++] = '-';
		scroll_ram[0][j++] = rtc_date.month / 10 + '0';
		scroll_ram[0][j++] = rtc_date.month % 10 + '0';
		scroll_ram[0][j++] = '-';
		scroll_ram[0][j++] = rtc_date.day / 10 + '0';
		scroll_ram[0][j++] = rtc_date.day % 10 + '0';
		scroll_ram[0][j++] = ' ';
		scroll_ram[0][j++] = rtc_date.hour / 10 + '0';
		scroll_ram[0][j++] = rtc_date.hour % 10 + '0';
		scroll_ram[0][j++] = ':';
		scroll_ram[0][j++] = rtc_date.minute / 10 + '0';
		scroll_ram[0][j++] = rtc_date.minute % 10 + '0';
		scroll_ram[0][j++] = ':';
		scroll_ram[0][j++] = rtc_date.second / 10 + '0';
		scroll_ram[0][j++] = rtc_date.second % 10 + '0';
		scroll_ram[0][j++] = ' ';
    }
    else if (SSID_Info.IP_Wifi_Status == 2)
    {
        sprintf((char*)&scroll_ram[0][0],"%d.%d.%d.%d",
				(uint8)SSID_Info.ip_addr[0],
				(uint8)SSID_Info.ip_addr[1],
				(uint8)SSID_Info.ip_addr[2],
				(uint8)SSID_Info.ip_addr[3]);

		uint8 len = strlen(&scroll_ram[0]);
		scroll_ram[0][len] = ' ';
    }
	else if(SSID_Info.IP_Wifi_Status == 5)
	{
		memcpy(&scroll_ram[0][0],"wifi configure     ",MAX_SCOROLL);
	}
    // Append some trailing spaces for smooth looping
    uint8_t len = strlen((char *)&scroll_ram[0][0]);
    memset(&scroll_ram[0][len],' ', SCROLL_WINDOW);
    scroll_ram[0][len] = '\0';
}

void display_scroll(void)
{
	uint8 cnt;
	uint8 i;

	Refresh_scroll();
	if(scroll == NULL)
		scroll = &scroll_ram[0][0];

	if(scroll >= &scroll_ram[0][1])
	{
		for(i = 0;i < (&scroll_ram[0][MAX_SCOROLL - 1] - scroll + 1);i++)
		{
			if(i < SCROLL_WINDOW + 1)
				scroll_buf[i] = *(scroll+i);
			else
				;//Test[20]++;
		}
	}
	else
	{
		for(i=0;i< SCROLL_WINDOW; i++)
			scroll_buf[i] = *(scroll+i);
	}

	if(scroll == &scroll_ram[0][MAX_SCOROLL - 1])
		scroll = &scroll_ram[0][0];
	else
		scroll++;

	scroll_buf[SCROLL_WINDOW] = '\0';
	disp_str_16_24(FORM15X30, 10,TIME_POS,(unsigned char *)scroll_buf,TSTAT8_CH_COLOR,TSTAT8_MENU_COLOR);

}

uint8_t fanspeedbuf;
uint8_t fan_speed_user;
uint8_t pre_mode;
uint8_t icon_flag[9];

void display_fan(void)
{
		uint8 stage_buf = 0;
		uint16 const *fannum;

		if(Modbus.icon_config == 0xff)
		{// low 6 bits are xx111111, dont show all icons
			disp_null_icon(FANBLADE_XDOTS, FANBLADE_YDOTS, 0, FOURTH_ICON_POS,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
			disp_null_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, 0, FIFTH_ICON_POS,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		}
		else
		{
			fanspeedbuf = FAN_AUTO;//FAN_OFF;
			stage_buf = Modbus.icon_config >> 6;

			if(stage_buf > 0)
			{
				scroll_fan = !scroll_fan;
			}

		//	stage_buf = get_cureent_stage();
			if((fanspeedbuf == FAN_OFF)||(stage_buf == FAN_COST))
			{
				fannum = fanbladeA;
				disp_icon(FANBLADE_XDOTS, FANBLADE_YDOTS, fannum, FOURTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			}
			else
			{
				if(scroll_fan == 1)
				{
					fannum = fanbladeA;
					disp_icon(FANBLADE_XDOTS, FANBLADE_YDOTS, fannum, FOURTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
				}
				else
				{
					fannum = fanbladeB;
					disp_icon(FANBLADE_XDOTS, FANBLADE_YDOTS, fannum, FOURTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
				}
			}


			if((stage_buf == FAN_COOL1) || (stage_buf == FAN_HEAT1))
			{
				fannum = fanspeed1a;
				disp_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, fannum, FIFTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			}
			else if((stage_buf == FAN_COOL2) || (stage_buf == FAN_HEAT2))
			{
				fannum = fanspeed2a;
				disp_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, fannum, FIFTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			}
			else if((stage_buf == FAN_COOL3) || (stage_buf == FAN_HEAT3))
			{
				fannum = fanspeed3a;
				disp_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, fannum, FIFTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			}
			else if(stage_buf == FAN_COST)
			{
				fannum = fanspeed0a;
				disp_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, fannum, FIFTH_ICON_POS,ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
				//disp_null_icon(FANSPEED_XDOTS, FANSPEED_YDOTS, 0, FIFTH_ICON_POS,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
			}
		}
}




void display_icon(void)
{

	icon_flag[0] = Modbus.icon_config & 0x01;
	icon_flag[1] = (Modbus.icon_config & 0x02) ? 1 : 0;
	icon_flag[2] = (Modbus.icon_config & 0x04) ? 1 : 0;
	icon_flag[3] = (Modbus.icon_config & 0x08) ? 1 : 0;
	icon_flag[4] = (Modbus.icon_config & 0x10) ? 1 : 0;
	icon_flag[5] = (Modbus.icon_config & 0x20) ? 1 : 0;

	//--------------day and night icon----------------
		if((icon_flag[0] == 1) && (icon_flag[1] == 1))
			disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, FIRST_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		else if(icon_flag[0] == 1)
			disp_icon(ICON_XDOTS, ICON_YDOTS, sunicon, FIRST_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_icon(ICON_XDOTS, ICON_YDOTS, moonicon, FIRST_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);


	//--------------occ and unocc icon----------------
		if((icon_flag[2] == 1)&&(icon_flag[3] == 1))
			disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, SECOND_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		else if(icon_flag[2] == 1)
			disp_icon(ICON_XDOTS, ICON_YDOTS, athome, SECOND_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_icon(ICON_XDOTS, ICON_YDOTS, offhome, SECOND_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

	//---------------heat and cool-----------------
		if((icon_flag[4] == 1)&&(icon_flag[5] == 1))
			disp_null_icon(ICON_XDOTS, ICON_YDOTS, 0, THIRD_ICON_POS ,ICON_POS,TSTAT8_BACK_COLOR, TSTAT8_BACK_COLOR);
		else if(icon_flag[4] == 1)
			disp_icon(ICON_XDOTS, ICON_YDOTS, heaticon, THIRD_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_icon(ICON_XDOTS, ICON_YDOTS, coolicon, THIRD_ICON_POS,	ICON_POS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

}

void clear_line(uint8 linenum)
{
	if(linenum == 1)
	{
		disp_str(FORM15X30, 0, MENU_ITEM1,  "                ",  TSTAT8_BACK_COLOR,TSTAT8_BACK_COLOR);
		disp_str(FORM15X30, 0, MENU_ITEM2,  "                ",  TSTAT8_BACK_COLOR,TSTAT8_BACK_COLOR);
	}
	else
		disp_str(FORM15X30, 0, MENU_VALUE_POS,  "                ",  TSTAT8_BACK_COLOR,TSTAT8_BACK_COLOR);
}

void clear_lines(void)
{
	disp_null_icon(239, 45, 0, 0,103,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	disp_null_icon(239, 45, 0, 0,103+BTN_OFFSET,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
 	disp_null_icon(239, 45, 0, 0,103+BTN_OFFSET+BTN_OFFSET,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
}

void display_value(uint16 pos, int16 disp_value, uint8 disp_unit)
{
	uint8 spbuf[8];
	uint8 unit = 0;
	unit = disp_unit;
	if(disp_unit == ' ')//show the value directly
	{
		if(disp_value >= 1000)
		{
			spbuf[0] = disp_value/1000 + 0x30;
			spbuf[1] = (disp_value%1000)/100 + 0x30;
			spbuf[2] = (disp_value%100)/10 + 0x30;
			spbuf[3] = (disp_value%10) + 0x30;
		}
		else if(disp_value >= 100)
		{
			spbuf[0] = (disp_value%1000)/100 + 0x30;
			spbuf[1] = (disp_value%100)/10 + 0x30;
			spbuf[2] = (disp_value%10) + 0x30;
			spbuf[3] = ' ';
		}
		else if(disp_value >= 10)
		{
			spbuf[0] = ' ';
			spbuf[1] = (disp_value%100)/10 + 0x30;
			spbuf[2] = (disp_value%10) + 0x30;
			spbuf[3] = ' ';
		}
		else if(disp_value >= 0)
		{
			spbuf[0] = ' ';
			spbuf[1] = (disp_value%10) + 0x30;
			spbuf[2] = ' ';
			spbuf[3] = ' ';
			}
			spbuf[4] = ' ';
			spbuf[5] = ' ';
			spbuf[6] = '\0';
		}
		else
		{
			if(disp_value >= 0)
			{
				if(disp_value >= 1000)
				{
					spbuf[0] = disp_value/1000 + 0x30;
					spbuf[1] = (disp_value%1000)/100 + 0x30;
					spbuf[2] = (disp_value%100)/10 + 0x30;
					spbuf[3] = '.';
					spbuf[4] = (disp_value%10) + 0x30;
					spbuf[5] =  unit;
				}
				else if(disp_value >= 100)
				{
					spbuf[0] = disp_value/100 + 0x30;
					spbuf[1] = (disp_value%100)/10 + 0x30;
					spbuf[2] = '.';
					spbuf[3] = (disp_value%10) + 0x30;
					spbuf[4] = unit;
					spbuf[5] = ' ';
				}
				else if(disp_value >= 10)
				{
					spbuf[0] = disp_value/10 + 0x30;
					spbuf[1] = '.';
					spbuf[2] = (disp_value%10) + 0x30;
					spbuf[4] = unit;
					spbuf[5] = ' ';
				}
				else
				{
					spbuf[0] = 0x30;
					spbuf[1] = '.';
					spbuf[2] = (disp_value%10) + 0x30;
					spbuf[3] = unit;
					spbuf[4] = ' ';
					spbuf[5] = ' ';
				}
				spbuf[6] = '\0';
			}
			else
			{
				if(disp_value <= -1000)
				{
					spbuf[0] = '-';
					spbuf[1] = -disp_value/1000 + 0x30;
					spbuf[2] = (-disp_value%1000)/100 + 0x30;
					spbuf[3] = (-disp_value%100)/10 + 0x30;
					spbuf[4] = '.';
					spbuf[5] = (-disp_value%10) + 0x30;
					spbuf[6] =  unit;
				}
				else if(disp_value <= -100)
				{
					spbuf[0] = '-';
					spbuf[1] = -disp_value/100 + 0x30;
					spbuf[2] = (-disp_value%100)/10 + 0x30;
					spbuf[3] = '.';
					spbuf[4] = (-disp_value%10) + 0x30;
					spbuf[5] = unit;
					spbuf[6] = ' ';
				}
				else if(disp_value >= 10)
				{
					spbuf[0] = '-';
					spbuf[1] = -disp_value/10 + 0x30;
					spbuf[2] = '.';
					spbuf[3] = (-disp_value%10) + 0x30;
					spbuf[4] = unit;
					spbuf[5] = ' ';
					spbuf[6] = ' ';
				}
				else
				{
					spbuf[0] = '-';
					spbuf[1] = 0x30;
					spbuf[2] = '.';
					spbuf[3] = (-disp_value%10) + 0x30;
					spbuf[4] = unit;
					spbuf[5] = ' ';
					spbuf[6] = ' ';
				}
				spbuf[7] = '\0';
			}
		}
	clear_line(2);
//	disp_str(FORM15X30, pos, MENU_VALUE_POS,  spbuf,  TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	disp_str(FORM15X30, 0, SYS_MODE_POS,  (char *)spbuf,  TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
}

