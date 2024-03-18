#include "define.h"
#include "LCD_TSTAT.h"
#include "menu.h"
#include "wifi.h"
#include "bacnet.h"
#include "user_data.h"
#include <string.h>

#define	NODES_POLL_PERIOD	30

char UI_DIS_LINE1[4]; //��Ӧ֮ǰ setpoint  fan �Լ� sys
char UI_DIS_LINE2[4];
char UI_DIS_LINE3[4];
char UI_DIS_TOP[9];

uint8 *scroll;
uint8 scroll_buf[11];
uint8 scroll_ram[5][MAX_SCOROLL];
uint8 scroll_fan = 0;

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
void MenuIdle_init(void)
{
	uint8 i,j;
	
	//LCDtest();
	ClearScreen(TSTAT8_BACK_COLOR);
	flag_digital_top_area = 0;
	digital_top_area_type = 0;
	digital_top_area_num = 0;
	digital_top_area_changed = 0;

	digital_top_area_type = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.point_type;
	digital_top_area_num = Setting_Info.reg.display_lcd.lcd_mod_reg.npoint.number - 1;	
						
	memset(UI_DIS_TOP,0,9);
	digital_top_area_changed = 0;
	
	disp_str(FORM15X30, SCH_XPOS,  0, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);					
	disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "            ",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
	disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);
	disp_str(FORM15X30, SCH_XPOS,  CH_HEIGHT * 2 - 7, "              ",SCH_COLOR,TSTAT8_BACK_COLOR);	
	
	if(digital_top_area_type == IN)
	{
		memcpy(UI_DIS_TOP, inputs[digital_top_area_num].label, 9);		
	}
	else if(digital_top_area_type == OUT)
		memcpy(UI_DIS_TOP, outputs[digital_top_area_num].label, 9);
	else if(digital_top_area_type == VAR)
		memcpy(UI_DIS_TOP, vars[digital_top_area_num].label, 9);		

	disp_null_icon(240, 36, 0, 0,TIME_POS,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR2);
	
 // scroll = &scroll_ram[0][0];
//	fanspeedbuf = fan_speed_user;
	
	
	draw_tangle(102,105);
	draw_tangle(102,148);
	draw_tangle(102,191);

	memcpy(UI_DIS_LINE1, vars[0].label, 3);UI_DIS_LINE1[3] = 0;
	memcpy(UI_DIS_LINE2, vars[1].label, 3);UI_DIS_LINE2[3] = 0;
	memcpy(UI_DIS_LINE3, vars[2].label, 3);UI_DIS_LINE3[3] = 0;
	
//	disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, str,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
	disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
	disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
	disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);

	//msv_data[MAX_MSV][STR_MSV_MULTIPLE_COUNT]
	for (i = 0;i < MAX_MSV;i++)
		for (j = 0; j < STR_MSV_MULTIPLE_COUNT;j++)
		{
			if(msv_data[i][j].status == 255)
			{
				msv_data[i][j].status = 0;
			}
		}
//#if ARM_UART_DEBUG
//	uart1_init(115200);
//	DEBUG_EN = 1;
//	printf("IDLE init \r\n");
//#endif	
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
 
void MenuIdle_display(void)
{
   	static u8 count_tx = 0;
	static u8 count_rx = 0;

	if(memcmp(UI_DIS_LINE1,vars[0].label,3))
	{
		disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, "   ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE1,'\0',4);
		memcpy(UI_DIS_LINE1, vars[0].label, 3);
	}
	if(memcmp(UI_DIS_LINE2,vars[1].label,3))
	{
		disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, "   ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE2,'\0',4);
		memcpy(UI_DIS_LINE2, vars[1].label, 3);
	}
	if(memcmp(UI_DIS_LINE3,vars[2].label,3))
	{
		disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, "   ",SCH_COLOR,TSTAT8_BACK_COLOR);
		memset(UI_DIS_LINE3,'\0',4);
		memcpy(UI_DIS_LINE3, vars[2].label, 3);
	}
		
    //display_input_value(inputs[0].value);
	//display_value(inputs[0].value);
	display_screen_value( 1); // �ֱ���var��ֵ��ʾ��ԭ�ȵ� set fan �Լ�sys �ط�
	display_screen_value( 2);
	display_screen_value( 3);
	//display_SP(inputs[0].value / 1000);
	//display_fanspeed(outputs[0].value / 1000);
	//display_mode(vars[0].value / 1000);
	if(Modbus.disable_tstat10_display == 0)
	{
		display_scroll();
		display_icon();
		display_fan();
	}
	else if(Modbus.disable_tstat10_display == 1)
	{
		disp_str(FORM15X30, 0,TIME_POS,"            ",TSTAT8_CH_COLOR,TSTAT8_MENU_COLOR2);
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

		{
			uint8 type,num;
			//if(Setting_Info.reg.display_lcd.lcddisplay[0] == 1) // modbus
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
					if(inputs[num].digital_analog == 1)
					{
						flag_digital_top_area = 0;		
						if(inputs[num].range == 4/* R10K_40_250DegF*/)
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, inputs[num].value / 100, TOP_AREA_DISP_UNIT_F);
						else if(inputs[num].range == 3/*R10K_40_120DegC*/)
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, inputs[num].value / 100, TOP_AREA_DISP_UNIT_C);
						else if(inputs[num].range == 27)  // humidity
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, inputs[num].value / 100, TOP_AREA_DISP_UNIT_RH);
						else 
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, inputs[num].value / 1000, TOP_AREA_DISP_UNIT_NONE);
					}
					else
					{
						flag_digital_top_area = 1;						
						memcpy(UI_DIS_TOP, inputs[digital_top_area_num].label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						
						if(inputs[num].control)
						{
							if(inputs[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STRAT",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(inputs[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(inputs[num].range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE1_POS, "LOW",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
					
					}
				}
				else if(type == OUT)
				{
					if(outputs[num].digital_analog == 1)
					{
						flag_digital_top_area = 0;
					}
					else
					{
						flag_digital_top_area = 1;

						memcpy(UI_DIS_TOP, outputs[digital_top_area_num].label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						if(outputs[num].control)
						{
							if(outputs[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "START",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(outputs[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(outputs[num].range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE1_POS, "LOW",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
					
					}
				}
				else if(type == VAR)
				{
					if(vars[num].digital_analog == 1)
					{
						flag_digital_top_area = 0;	
						if(vars[num].range == degF) //���rangeѡ����10K type2 F ����ʾ F
						{	
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 100, TOP_AREA_DISP_UNIT_F);
						}
						else	if(vars[num].range == degC) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 100, TOP_AREA_DISP_UNIT_C);
						}
						else	if(vars[num].range == KPa) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 1000, TOP_AREA_DISP_UNIT_kPa);
						}
						else	if(vars[num].range == Pa) //���rangeѡ����10K type2 F ����ʾ F
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 1000, TOP_AREA_DISP_UNIT_Pa);
						}
						else	if(vars[num].range == RH)
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 1000, TOP_AREA_DISP_UNIT_RH);
						}
						else
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, vars[num].value / 1000, TOP_AREA_DISP_UNIT_NONE);
					}
					else
					{
						flag_digital_top_area = 1;						
						
						memcpy(UI_DIS_TOP, vars[digital_top_area_num].label, 9);
						disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
						
						if(vars[num].control)
						{
							if(vars[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OPEN",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "START",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ENABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ALARM",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == LOW_HIGH)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "HIGH",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "ON",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
						{
							if(vars[num].range == OFF_ON)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "OFF",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == CLOSED_OPEN)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "CLOSED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == STOP_START)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "STOP",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == DISABLED_ENABLED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "DISABLED",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == NORMAL_ALARM)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "NORMAL",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == UNOCCUPIED_OCCUPIED)
								disp_str(FORM15X30, SCH_XPOS,  IDLE_LINE2_POS, "UNOCC",SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
							else if(vars[num].range == LOW_HIGH)
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
			}
// 			if(Setting_Info.reg.display_lcd.lcddisplay[0] == 1) // bacnet
//			{
//			}
			
		}

		if(count_left_key > 5) 
			disp_index = 0;
		else
			count_left_key++;

		if(disp_index == 1)
		{
			if(flag_digital_top_area == 1)
				disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR1);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);
		}
		else if(disp_index == 2)
		{
			if(flag_digital_top_area == 1)
				disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR1);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);
		}
		else if(disp_index == 3)
		{
			if(flag_digital_top_area == 1)
				disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR1);
		}
		else if(disp_index == 4) // top area
		{
			if(flag_digital_top_area == 1)
				disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR1);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);
		}
		else
		{
			if(flag_digital_top_area == 1)
				disp_str_16_24(FORM15X30, SCH_XPOS + 20,  IDLE_LINE1_POS, (uint8 *)UI_DIS_TOP,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS, UI_DIS_LINE1,SCH_COLOR,TSTAT8_BACK_COLOR);//TSTAT8_BACK_COLOR
			disp_str(FORM15X30, SCH_XPOS,  FAN_MODE_POS, UI_DIS_LINE2,SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS, UI_DIS_LINE3,SCH_COLOR,TSTAT8_BACK_COLOR);
		}

        //sprintf(test_char, "%d", SSID_Info.IP_Wifi_Status); //�����ã�����Ļ���Ͻ� ��ʾ wifi״̬����ֵ;
        //disp_str(FORM15X30, 0, 0, test_char, SCH_COLOR, TSTAT8_BACK_COLOR);

//        if (SSID_Info.IP_Wifi_Status == WIFI_NORMAL) 
//        {
//            disp_icon(26, 26, wificonnect, 210, 0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
//        }
//         //  
//        else if (SSID_Info.IP_Wifi_Status == WIFI_NO_WIFI || SSID_Info.IP_Wifi_Status == WIFI_NONE)
//        {
//            disp_null_icon(26, 26, 0, 210, 0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
//        }
//        else
//            disp_icon(26, 26, wifinocnnct, 210, 0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
				
	if(SSID_Info.IP_Wifi_Status == WIFI_NORMAL)//����Ļ���Ͻ���ʾwifi��״̬
	{
		if(SSID_Info.rssi < 70)
			disp_icon(26, 26, wifi_4, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else if(SSID_Info.rssi < 80)
			disp_icon(26, 26, wifi_3, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else if(SSID_Info.rssi < 90)
			disp_icon(26, 26, wifi_2, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		else
			disp_icon(26, 26, wifi_1, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
	}
	else	if((SSID_Info.IP_Wifi_Status == WIFI_NO_CONNECT)
		|| (SSID_Info.IP_Wifi_Status == WIFI_SSID_FAIL))
			disp_icon(26, 26, wifi_0, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		// if WIFI_NONE, do not show wifi flag
	else //if((SSID_Info.IP_Wifi_Status == WIFI_NO_WIFI)
		disp_icon(26, 26, wifi_none, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

	// show TX,RX

	if(flagLED_sub_tx > 0)
	{
		if(count_tx++ % 2 == 0)
			disp_icon(13, 26, cmnct_send, 	0,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
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
			disp_icon(13, 26, cmnct_rcv, 	13,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
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


extern uint8_t item_to_adjust;
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
	switch(key_value /*& KEY_SPEED_MASK*/)
	{
		case 0:
			break;
		case KEY_UP_MASK:
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				if ((vars[disp_index - 1].range >= 101) && (vars[disp_index - 1].range <= 103))  // 101 102 103 	MSV range
				{
//					if (vars[disp_index - 1].range == 101)  //�ж�range �ǲ��Ƕ�̬���ǵĻ� ������̬��ֵ;
					{
						uint8 len;
						len = check_msv_data_len(disp_index - 1);
						for (i = 0; i < len; i++)
						{
							if (vars[disp_index - 1].value / 1000 == msv_data[disp_index - 1][i].msv_value)
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
								vars[disp_index - 1].value = msv_data[disp_index - 1][i + 1].msv_value * 1000;
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
					if(vars[disp_index - 1].digital_analog == 0)
					{
						if(vars[disp_index - 1].control == 0)
							vars[disp_index - 1].control = 1;
						else
							vars[disp_index - 1].control = 0;
					}
					else
					{
						if(vars[disp_index - 1].value < 999 * 1000)
								vars[disp_index - 1].value = vars[disp_index - 1].value + 1000;
							else
								vars[disp_index - 1].value = 0;
					}
				}
			}
			else // disp_index == 4
			{
				digital_top_area_changed = 1;
				if(digital_top_area_type == IN)
				{
					inputs[digital_top_area_num].control = ((inputs[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == VAR)
				{
					vars[digital_top_area_num].control = ((vars[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == OUT)
				{
					outputs[digital_top_area_num].control = ((outputs[digital_top_area_num].control) == 0) ? 1 : 0;
					if(outputs[digital_top_area_num].control) 					
						set_output_raw(digital_top_area_num,1000);
					else 
						set_output_raw(digital_top_area_num,0);	
				}
			}

			ChangeFlash = 1;
			break;
		case KEY_SPEED_10 | KEY_UP_MASK:	
			count_left_key = 0;
			if((disp_index >= 1) && (disp_index <= 3))
			{
				if ((vars[disp_index - 1].range >= 101) && (vars[disp_index - 1].range <= 103))  // 101 102 103 	MSV range
				{					
					char len;
					len = check_msv_data_len(disp_index - 1);
					for (i = 0; i < len; i++)
					{
						if (vars[disp_index - 1].value / 1000 == msv_data[disp_index - 1][i].msv_value)
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
							vars[disp_index - 1].value = msv_data[disp_index - 1][i + 1].msv_value * 1000;
							break;
						}
					}
				}
				else
				{
					if(vars[disp_index - 1].digital_analog == 0)
					{
						if(vars[disp_index - 1].control == 0)
							vars[disp_index - 1].control = 1;
						else
							vars[disp_index - 1].control = 0;
					}
					else
					{
					if(vars[disp_index - 1].value < 999 * 1000)
							vars[disp_index - 1].value = vars[disp_index - 1].value + 10000;
						else
							vars[disp_index - 1].value = 0;
					}
				}
			}
			else // disp_index == 4
			{
				digital_top_area_changed = 1;
				if(digital_top_area_type == IN)
				{
					inputs[digital_top_area_num].control = ((inputs[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == VAR)
				{
					vars[digital_top_area_num].control = ((vars[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == OUT)
				{
					outputs[digital_top_area_num].control = ((outputs[digital_top_area_num].control) == 0) ? 1 : 0;
					if(outputs[digital_top_area_num].control) 					
						set_output_raw(digital_top_area_num,1000);
					else 
						set_output_raw(digital_top_area_num,0);	
				}
			}

			ChangeFlash = 1;
			break;

		case KEY_DOWN_MASK:
			count_left_key = 0;			
			if((disp_index >= 1) && (disp_index <= 3))
			{
				if ((vars[disp_index - 1].range >= 101) && (vars[disp_index - 1].range <= 103))  // 101 102 103 	MSV range
				{
					//if(vars[disp_index - 1].range == 101)  //�ж�range �ǲ��Ƕ�̬���ǵĻ� ������̬��ֵ;
					{
						// check the lenght of msv_data
						char len;
						len = check_msv_data_len(disp_index - 1);
							for (i = 0; i < len; i++)
							{
									if (vars[disp_index - 1].value / 1000 == msv_data[disp_index - 1][i].msv_value)
									{
											temp_value = i;
											break;
									}
							}

							for (i = temp_value; i > 0; i--)
							{
									if (strlen(msv_data[disp_index - 1][i - 1].msv_name) != 0)
									{
											vars[disp_index - 1].value = msv_data[disp_index - 1][i - 1].msv_value * 1000;
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
					if(vars[disp_index - 1].digital_analog == 0)
					{
						if(vars[disp_index - 1].control == 0)
							vars[disp_index - 1].control = 1;
						else
							vars[disp_index - 1].control = 0;
					}
					else
					{
//					if(vars[disp_index - 1].value > 1000)
							vars[disp_index - 1].value = vars[disp_index - 1].value - 1000;
//						else
//							vars[disp_index - 1].value = 99 * 1000;
					}
				}
			}
			else // disp_index == 4
			{
				digital_top_area_changed = 1;
				if(digital_top_area_type == IN)
				{
					inputs[digital_top_area_num].control = ((inputs[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == VAR)
				{
					vars[digital_top_area_num].control = ((vars[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == OUT)
				{
					outputs[digital_top_area_num].control = ((outputs[digital_top_area_num].control) == 0) ? 1 : 0;
					if(outputs[digital_top_area_num].control) 					
						set_output_raw(digital_top_area_num,1000);
					else 
						set_output_raw(digital_top_area_num,0);	
				}
			}
		

			ChangeFlash = 1;
			break;		
		case KEY_SPEED_10 | KEY_DOWN_MASK: 
			count_left_key = 0;			
			if((disp_index >= 1) && (disp_index <= 3))
			{
				if ((vars[disp_index - 1].range >= 101) && (vars[disp_index - 1].range <= 103))  // 101 102 103 	MSV range
				{
					char len;
					len = check_msv_data_len(disp_index - 1);
					for (i = 0; i < len; i++)
					{
							if (vars[disp_index - 1].value / 1000 == msv_data[disp_index - 1][i].msv_value)
							{
									temp_value = i;
									break;
							}
					}

					for (i = temp_value; i > 0; i--)
					{
							if (strlen(msv_data[disp_index - 1][i - 1].msv_name) != 0)
							{
									vars[disp_index - 1].value = msv_data[disp_index - 1][i - 1].msv_value * 1000;
									break;
							}
					}
				}
				else
				{
					if(vars[disp_index - 1].digital_analog == 0)
					{
						if(vars[disp_index - 1].control == 0)
							vars[disp_index - 1].control = 1;
						else
							vars[disp_index - 1].control = 0;
					}
					else
					{
						vars[disp_index - 1].value = vars[disp_index - 1].value - 10000;
					}
				}
			}
			else // disp_index == 4
			{
				digital_top_area_changed = 1;
				if(digital_top_area_type == IN)
				{
					inputs[digital_top_area_num].control = ((inputs[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == VAR)
				{
					vars[digital_top_area_num].control = ((vars[digital_top_area_num].control) == 0) ? 1 : 0;
				}
				else if(digital_top_area_type == OUT)
				{
					outputs[digital_top_area_num].control = ((outputs[digital_top_area_num].control) == 0) ? 1 : 0;
					if(outputs[digital_top_area_num].control) 					
						set_output_raw(digital_top_area_num,1000);
					else 
						set_output_raw(digital_top_area_num,0);	
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
    int i = 0;
    uint8 spbuf[20];
    float show_value = 0;
    uint8 str_length = 0;
    memset(spbuf, 0x20, 5);
    spbuf[5] = 0; //³õÊ¼»¯5¸ö×Ö½ÚÎª¿Õ¸ñ ±ÜÃâ µÚÒ»´ÎÏÔÊ¾12345 Öµ±äÎªABCµÄÊ±ºò   »áÏÔÊ¾ABC45
    if (type >= 1 && type <= 3)  // ÏÔÊ¾ÔÚLCDµÄÊý¾Ý¹Ì¶¨ÎªVAR1-VAR3
    {// ONLY var1-3 support MSV
		if ((vars[type - 1].range >= 101) && (vars[type - 1].range <= 103))  // 101 102 103 	MSV range
		{
			//sprintf(spbuf, "%d", msv_data[0][0].msv_value);
			for ( i = 0; i < 8; i++)
			{
				if ((vars[type - 1].value/1000) == msv_data[vars[type - 1].range - 101][i].msv_value)
				{
						str_length = strlen(msv_data[vars[type - 1].range - 101][i].msv_name);
						if (str_length >= 5)
								str_length = 5;
						memcpy(spbuf, msv_data[vars[type - 1].range - 101][i].msv_name, str_length);
						break;
				}
			}
		}
		else
		{
			show_value = ((float)vars[type - 1].value) / 1000;
			if(vars[type - 1].value / 1000 >= 10000)
				show_value = 9999;

			if(vars[type - 1].digital_analog == 0)
			{
				show_value = vars[type - 1].control ? 1 : 0;

				switch(vars[type - 1].range)
				{
					case OFF_ON:
						if(show_value == 0)			memcpy(spbuf, "OFF  ", 5);
						else							memcpy(spbuf, "ON   ", 5);
						break;
					case CLOSED_OPEN:
							if(show_value == 0)			memcpy(spbuf, "CLOSE", 5);
						else							memcpy(spbuf, "OPEN ", 5);
						break;
					case STOP_START:
						if(show_value == 0)					memcpy(spbuf, "STOP ", 5);
						else						memcpy(spbuf, "START", 5);
					break;
					case DISABLED_ENABLED:
						if(show_value == 0)			memcpy(spbuf, "DISAB", 5);
						else							memcpy(spbuf, "ENABL", 5);
						break;
					case NORMAL_ALARM:
						if(show_value == 0)			memcpy(spbuf, "NAORM", 5);
						else							memcpy(spbuf, "ALARM", 5);
						break;
					case NORMAL_HIGH:
							if(show_value == 0)			memcpy(spbuf, "NAORM", 5);
						else							memcpy(spbuf, "HIGH ", 5);
						break;
					case NORMAL_LOW:
						if(show_value == 0)					memcpy(spbuf, "NAORM", 5);
						else						memcpy(spbuf, "LOW  ", 5);
					break;
					case NO_YES:
						if(show_value == 0)			memcpy(spbuf, "NO   ", 5);
						else							memcpy(spbuf, "YES  ", 5);
						break;
					case COOL_HEAT:
						if(show_value == 0)			memcpy(spbuf, "COOL ", 5);
						else							memcpy(spbuf, "HEAT ", 5);
						break;
					case UNOCCUPIED_OCCUPIED:
							if(show_value == 0)			memcpy(spbuf, "UNOCC", 5);
						else							memcpy(spbuf, "OCC  ", 5);
						break;
					case LOW_HIGH:
						if(show_value == 0)					memcpy(spbuf, "LOW  ", 5);
						else						memcpy(spbuf, "HIGH ", 5);
					break;
					case ON_OFF:
						if(show_value == 0)			memcpy(spbuf, "ON   ", 5);
						else							memcpy(spbuf, "OFF  ", 5);
						break;
					case OPEN_CLOSED:
						if(show_value == 0)			memcpy(spbuf, "OPEN ", 5);
						else							memcpy(spbuf, "CLOSE", 5);
						break;
					case START_STOP:
							if(show_value == 0)			memcpy(spbuf, "START", 5);
						else							memcpy(spbuf, "STOP ", 5);
						break;
					case ENABLED_DISABLED:
						if(show_value == 0)					memcpy(spbuf, "ENABL", 5);
						else						memcpy(spbuf, "DISAB", 5);
					break;
					case ALARM_NORMAL:
						if(show_value == 0)			memcpy(spbuf, "ALARM", 5);
						else							memcpy(spbuf, "NORMA", 5);
						break;
					case HIGH_NORMAL:
						if(show_value == 0)			memcpy(spbuf, "HIGH ", 5);
						else							memcpy(spbuf, "NORMA", 5);
						break;
					case LOW_NORMAL:
							if(show_value == 0)			memcpy(spbuf, "LOW  ", 5);
						else							memcpy(spbuf, "NORMA", 5);
						break;
					case YES_NO:
						if(show_value == 0)					memcpy(spbuf, "YES  ", 5);
						else						memcpy(spbuf, "NO   ", 5);
					break;
					case HEAT_COOL:
						if(show_value == 0)			memcpy(spbuf, "HEAT ", 5);
						else							memcpy(spbuf, "COOL ", 5);
						break;
					case OCCUPIED_UNOCCUPIED:
						if(show_value == 0)					memcpy(spbuf, "OCC  ", 5);
						else						memcpy(spbuf, "UNOCC", 5);
					break;
					case HIGH_LOW:
						if(show_value == 0)			memcpy(spbuf, "HIGH ", 5);
						else							memcpy(spbuf, "LOW  ", 5);
						break;
					default:
						break;
				}
			}
			else
			{
				if(show_value >= 1000)
				{
					sprintf((char *)spbuf, "%d",(int)show_value);
				}
				else if(vars[type - 1].value / 1000 >= 10)
					sprintf((char *)spbuf, "%.1f", show_value);
				else
					sprintf((char *)spbuf, "%.2f", show_value);
			}
		}
    }

	if(type == 1)
			disp_str(FORM15X30, SCH_XPOS + 96, SETPOINT_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR2);
    else if (type == 2)
			disp_str(FORM15X30, SCH_XPOS + 96, FAN_MODE_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR2);
    else if (type == 3)
      disp_str(FORM15X30, SCH_XPOS + 96, SYS_MODE_POS, (char *)spbuf, SCH_COLOR, TSTAT8_MENU_COLOR2);
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

	disp_str(FORM15X30, SCH_XPOS+96,SETPOINT_POS, (char *)spbuf,  SCH_COLOR,TSTAT8_MENU_COLOR2);

}

uint8 lcd_rotate_max;
uint8 lcd_rotate_index;
void Refresh_scroll(void)
{
	uint8 i,j;
	uint8 mode;
	memset(&scroll_ram[0][0],0,MAX_SCOROLL);
	//if(lcd_rotate_max > 9)
		lcd_rotate_max = 1;
	//lcd_rotate_index = 0;
	if(lcd_rotate_max > 0)
	{
		if(SSID_Info.IP_Wifi_Status <= 1)
		{
			j = 0;
			scroll_ram[0][j++] = '2';
			scroll_ram[0][j++] = '0';
			scroll_ram[0][j++] =  Rtc.Clk.year / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.year % 10 + '0';
			scroll_ram[0][j++] = '-';
			scroll_ram[0][j++] =  Rtc.Clk.mon / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.mon % 10 + '0';
			scroll_ram[0][j++] = '-';
			scroll_ram[0][j++] =  Rtc.Clk.day / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.day % 10 + '0';
			scroll_ram[0][j++] = ' ';
			scroll_ram[0][j++] =  Rtc.Clk.hour / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.hour % 10 + '0';
			scroll_ram[0][j++] = ':';
			scroll_ram[0][j++] =  Rtc.Clk.min / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.min % 10 + '0';
			scroll_ram[0][j++] = ':';
			scroll_ram[0][j++] =  Rtc.Clk.sec / 10 + '0';
			scroll_ram[0][j++] =  Rtc.Clk.sec % 10 + '0';
			scroll_ram[0][j++] = ' ';
		}
		else if(SSID_Info.IP_Wifi_Status == 2)
		{// tbd::::::::::::
			uint8 len;
			sprintf((char*)&scroll_ram[0][0],"%d.%d.%d.%d",(uint8)SSID_Info.ip_addr[0],(uint8)SSID_Info.ip_addr[1],(uint8)SSID_Info.ip_addr[2], (uint8)SSID_Info.ip_addr[3]);
			len = strlen(&scroll_ram[0]);
			scroll_ram[0][len] = ' ';
		}
		else if(SSID_Info.IP_Wifi_Status == 3)
		{
			memcpy(&scroll_ram[0][0],"wifi connected ",MAX_SCOROLL);
		}
		else if(SSID_Info.IP_Wifi_Status == 4)
		{
			memcpy(&scroll_ram[0][0],"wifi disconnct ",MAX_SCOROLL);
		}
		else if(SSID_Info.IP_Wifi_Status == 5)
		{
			memcpy(&scroll_ram[0][0],"wifi configure ",MAX_SCOROLL);
		}
	}

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
			if(i < 11)
				scroll_buf[i] = *(scroll+i);
			else
				;//Test[20]++;
		}
	}
	else
	{
		for(i=0;i<10;i++)
			scroll_buf[i] = *(scroll+i);
	}

	if(scroll == &scroll_ram[0][MAX_SCOROLL - 1])
		scroll = &scroll_ram[0][0];
	else
		scroll++;

	scroll_buf[10] = '\0';
	disp_str(FORM15X30, 30,TIME_POS,(char *)scroll_buf,TSTAT8_CH_COLOR,TSTAT8_MENU_COLOR2);


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

