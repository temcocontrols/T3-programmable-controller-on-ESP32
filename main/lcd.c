/*
 * lcd.c
 *
 *  Created on: 2022��11��2��
 *      Author: Administrator
 */

#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "i2c_task.h"
#include "unistd.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "define.h"
#include "airlab.h" // only for airlab
#include "wifi.h"
#include "user_data.h"
#include "LcdTheme.h"
#include "LCD_TSTAT.h"

#ifdef COLOR_TEST
uint32 TSTAT8_BACK_COLOR = 0xd6dd;
uint32 TSTAT8_MENU_COLOR2 = 0xd6dd;
uint32 TANGLE_COLOR = 0xd6dd;
#endif

extern U16_T Test[50];

uint8_t display_config[5];
uint8 EEP_DEGCorF;
lcdconfig display_lcd;

uint8 const chlib16x24[] =
{
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0xFC,0x01,0x8E,0x03 ,
 0x06,0x03,0x00,0x03,0x80,0x03,0xC0,0x01,0xF0,0x00,0x38,0x00,0x1C,0x00,0x0C,0x00 ,
 0x06,0x00,0xFE,0x03,0xFE,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,/*"2",0*/

 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,
 0x1C,0x00,0x1C,0x00,0x1C,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,/*".",1*/

 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x01,0xFC,0x01,0x0C,0x00 ,
 0x0C,0x00,0x06,0x00,0x7E,0x00,0xFE,0x01,0x86,0x01,0x00,0x03,0x00,0x03,0x06,0x03 ,
 0x8E,0x01,0xFC,0x01,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 /*"5",2*/
};

// Airlab lcd io
#define GPIO_LCD_RS     23
#define GPIO_LCD_CS     26
#define GPIO_LCD_SDA    22
#define GPIO_LCD_SCL    27
#define GPIO_LCD_RES    25  //reset
#define GPIO_LCD_BACK	33



#define GPIO_LCD_RST_SEL  (1ULL<<GPIO_LCD_RS)
#define GPIO_LCD_CS_SEL  (1ULL<<GPIO_LCD_CS)
#define GPIO_LCD_SDA_SEL  (1ULL<<GPIO_LCD_SDA)
#define GPIO_LCD_SCL_SEL  (1ULL<<GPIO_LCD_SCL)
#define GPIO_LCD_RES_SEL  (1ULL<<GPIO_LCD_RES)
#define GPIO_LCD_BACK_SEL  (1ULL<<GPIO_LCD_BACK)


#define LCD_RS_LO	gpio_set_level(GPIO_LCD_RS, 0)
#define LCD_RS_HI	gpio_set_level(GPIO_LCD_RS, 1)
#define LCD_CS_LO	gpio_set_level(GPIO_LCD_CS, 0)
#define LCD_CS_HI	gpio_set_level(GPIO_LCD_CS, 1)
#define LCD_SDA_LO	gpio_set_level(GPIO_LCD_SDA, 0)
#define LCD_SDA_HI	gpio_set_level(GPIO_LCD_SDA, 1)
#define LCD_SCL_LO	gpio_set_level(GPIO_LCD_SCL, 0)
#define LCD_SCL_HI	gpio_set_level(GPIO_LCD_SCL, 1)
#define LCD_RES_LO	gpio_set_level(GPIO_LCD_RES, 0)
#define LCD_RES_HI	gpio_set_level(GPIO_LCD_RES, 1)
#define LCD_BACK_LO	gpio_set_level(GPIO_LCD_BACK, 0)
#define LCD_BACK_HI	gpio_set_level(GPIO_LCD_BACK, 1)



#define PM25_TX_DISABLE  gpio_set_level(12, 1)
#define PM25_RX_DISABLE  gpio_set_level(15, 1)

void ILI9341_Initial(void);

void lcd_back_set(uint8_t status)
{
	if(status == 0)
		LCD_BACK_LO;
	else
		LCD_BACK_HI;
}

void LCD_IO_Init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19

    io_conf.pin_bit_mask = GPIO_LCD_RST_SEL | GPIO_LCD_CS_SEL | GPIO_LCD_SDA_SEL | GPIO_LCD_SCL_SEL | GPIO_LCD_RES_SEL | GPIO_LCD_RES_SEL | GPIO_LCD_BACK_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings

    gpio_config(&io_conf);

    ILI9341_Initial();
    LCD_BACK_HI;
}

void delay_ms(unsigned int t);
//unsigned char const chlib[];
//uint8 const athome[];


void Write_Cmd_Data(unsigned char CMDP);
void Write_Cmd(unsigned char CMD);
void Write_Data(unsigned char DH,unsigned char DL);
//void delay_ms(unsigned int tt);
void  Write_Data_U16(unsigned int y);
static void LCD_SetPos(unsigned char x0,unsigned char x1,unsigned  int y0,unsigned int y1);
void ClearScreen(unsigned int bColor);
void disp_ch(uint8_t form, uint16_t x, uint16_t y,uint8_t value,uint16_t dcolor,uint16_t bgcolor);

void disp_icon(uint16_t cp, uint16_t pp, uint16_t const *icon_name, uint16_t x,uint16_t y,uint16_t dcolor, uint16_t bgcolor);
//===============================================================
//CLEAR SCREEN
void ClearScreen(unsigned int bColor)
{
 uint16_t i,j;
 LCD_SetPos(0,239,0,319);//320x240
 for (i=0;i<320;i++)
	{
	   for (j=0;j<240;j++)
	       Write_Data_U16(LcdThemeUpdateColors(bColor));
	}
}

//==============================================================
//WRITE DATA WORD
void  Write_Data_U16(unsigned int y)
{
	unsigned char m,n;
	m=y>>8;
	n=y;
	Write_Data(m,n);

}
//=============================================================
//WRITE COMMAND

void Write_Cmd(unsigned char CMD)
{ unsigned char i;

    LCD_CS_LO;
    LCD_RS_LO;
    for(i=0;i<8;i++)
     {
    	LCD_SCL_LO;
        if(CMD&0x80) LCD_SDA_HI;
       	else LCD_SDA_LO;
       	LCD_SCL_HI;
        CMD=CMD<<1;
     }
   	LCD_CS_HI;


}

//===============================================================
//WRITE COMMAND PARAMETER

void  Write_Cmd_Data (unsigned char CMDP)
{ unsigned char i;

    LCD_CS_LO;
   	LCD_RS_HI;
    for(i=0;i<8;i++)
     {
    	LCD_SCL_LO;
        if(CMDP&0x80) LCD_SDA_HI;
       	else LCD_SDA_LO;
       	LCD_SCL_HI;
        CMDP=CMDP<<1;
     }
   	LCD_CS_HI;

}


//===================================================================
//WRITE DATA CHAR

void Write_Data(unsigned char DH,unsigned char DL)
{
	unsigned char i;

    LCD_CS_LO;
   	LCD_RS_HI;
    for(i=0;i<8;i++)
     {
    	LCD_SCL_LO;
        if(DH&0x80) LCD_SDA_HI;
       	else LCD_SDA_LO;
       	LCD_SCL_HI;
        DH=DH<<1;
     }

    for(i=0;i<8;i++)
     {  LCD_SCL_LO;
        if(DL&0x80) LCD_SDA_HI;
       	else LCD_SDA_LO;
       	LCD_SCL_HI;
        DL=DL<<1;
     }

   	LCD_CS_HI;

}



//============================================================
//DELAY
//void delay_ms(unsigned int count)
//{
//    int i,j;
//    for(i=0;i<count;i++)
//       {
//	     for(j=0;j<125;j++);
//       }
//}


void LCD_Enter_Sleep(void)
{
	Write_Cmd(0x28);     // Display off
	Write_Cmd(0x10);     // Enter Sleep mode
}

void LCD_Exit_Sleep(void)
{
	Write_Cmd(0x11);     // Sleep out
	vTaskDelay(100);//delay_ms(120);
	Write_Cmd(0x29);     // Display on
}


//=============================================================
//LCD Initial
void ILI9341_Initial(void)
{
 	LCD_CS_HI;
 	delay_ms(5);
	LCD_RES_LO;
	delay_ms(10);
	LCD_RES_HI;
	delay_ms(120);

	Write_Cmd(0x11);//exsit sleep mode
	delay_ms(120);

 	Write_Cmd(0xCF);   //power control B
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0xc3);
	Write_Cmd_Data(0X30);

 	Write_Cmd(0xED); //power on sequence ontrol
	Write_Cmd_Data(0x64);
	Write_Cmd_Data(0x03);
	Write_Cmd_Data(0X12);
	Write_Cmd_Data(0X81);

 	Write_Cmd(0xE8); //driver timing control A
	Write_Cmd_Data(0x85);
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x79);

 	Write_Cmd(0xCB);//power control A
	Write_Cmd_Data(0x39);
	Write_Cmd_Data(0x2C);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x34);
	Write_Cmd_Data(0x02);

 	Write_Cmd(0xF7);//pump ratio control
	Write_Cmd_Data(0x20);

 	Write_Cmd(0xEA);//driver timing control B
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);

 	Write_Cmd(0xC0);    //Power control
	Write_Cmd_Data(0x22);   //VRH[5:0]

 	Write_Cmd(0xC1);    //Power control
	Write_Cmd_Data(0x11);   //SAP[2:0];BT[3:0]

 	Write_Cmd(0xC5);    //VCM control
	Write_Cmd_Data(0x3d);
    //LCD_DataWrite_ILI9341(0x30);
	Write_Cmd_Data(0x20);

 	Write_Cmd(0xC7);    //VCM control2
    //LCD_DataWrite_ILI9341(0xBD);
	Write_Cmd_Data(0xAA); //0xB0

 	Write_Cmd(0x36);    // Memory Access Control
	Write_Cmd_Data(0xc8); //Row address order//XY exchange and X mirror

 	Write_Cmd(0x3A);   //color mode
	Write_Cmd_Data(0x55);

 	Write_Cmd(0xB1); //frame rate control
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x13);

 	Write_Cmd(0xB6);    // Display Function Control
	Write_Cmd_Data(0x0A);
	Write_Cmd_Data(0xA2);

 	Write_Cmd(0xF6); //interface control
	Write_Cmd_Data(0x01);
//	Write_Cmd_Data(0x00);//data will be ignore if over the range
	Write_Cmd_Data(0x30);

 	Write_Cmd(0xF2);    // 3Gamma Function Disable
	Write_Cmd_Data(0x00);

 	Write_Cmd(0x26);    //Gamma curve selected
	Write_Cmd_Data(0x01);

 	Write_Cmd(0xE0);    //Set Gamma
	Write_Cmd_Data(0x0F);
	Write_Cmd_Data(0x3F);
	Write_Cmd_Data(0x2F);
	Write_Cmd_Data(0x0C);
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x0A);
	Write_Cmd_Data(0x53);
	Write_Cmd_Data(0XD5);
	Write_Cmd_Data(0x40);
	Write_Cmd_Data(0x0A);
	Write_Cmd_Data(0x13);
	Write_Cmd_Data(0x03);
	Write_Cmd_Data(0x08);
	Write_Cmd_Data(0x03);
	Write_Cmd_Data(0x00);

 	Write_Cmd(0XE1);    //Set Gamma
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x10);
	Write_Cmd_Data(0x03);
	Write_Cmd_Data(0x0F);
	Write_Cmd_Data(0x05);
	Write_Cmd_Data(0x2C);
	Write_Cmd_Data(0xA2);
	Write_Cmd_Data(0x3F);
	Write_Cmd_Data(0x05);
	Write_Cmd_Data(0x0E);
	Write_Cmd_Data(0x0C);
	Write_Cmd_Data(0x37);
	Write_Cmd_Data(0x3C);
	Write_Cmd_Data(0x0F);

//	Write_Cmd(0x33);   //color mode
//	Write_Cmd_Data(0x55);
//	Write_Cmd_Data(0x55);
//	Write_Cmd_Data(0x55);
//	Write_Cmd_Data(0x55);
//	Write_Cmd_Data(0x55);
//	Write_Cmd_Data(0x55);

 	Write_Cmd(0x11);    //Exit Sleep
	delay_ms(120);
 	Write_Cmd(0x29);    //Display on
	delay_ms(50);

	//if(isBlankScreen == false)
		ClearScreen(TSTAT8_BACK_COLOR);  //CLEAR DISPLAY
	//else
		//ClearScreen(TSTAT8_CH_COLOR);  //CLEAR DISPLAY
}



//===============================================================
//Define the coordinate
static void LCD_SetPos(unsigned char x0,unsigned char x1,unsigned int y0,unsigned int y1)
{
	unsigned char YSH,YSL,YEH,YEL;
	YSH=y0>>8;
	YSL=y0;

	YEH=y1>>8;
	YEL=y1;

 	Write_Cmd(0x2A);
	Write_Cmd_Data (0x00);
	Write_Cmd_Data (x0);
	Write_Cmd_Data (0x00);
	Write_Cmd_Data (x1);
	Write_Cmd(0x2B);
	Write_Cmd_Data (YSH);
	Write_Cmd_Data (YSL);
	Write_Cmd_Data (YEH);
	Write_Cmd_Data (YEL);
	Write_Cmd(0x2C);//LCD_WriteCMD(GRAMWR);
}

void disp_special_ch(uint8 form, uint16 x, uint16 y, uint8 value, uint16 dcolor, uint16 bgcolor)
{
    uint16 i, j;
    uint8 const *temp;//= chlibsmall;
    uint16 cp = 0;
    uint16 pp = 0;
    uint16 cnum;
	uint16 themeColor_text = LcdThemeUpdateColors(dcolor);
	uint16 themeColor_bg = LcdThemeUpdateColors(bgcolor);

    if (form == FORM32X64)
    {
        temp = chlib;
        cp = 48; pp = 96;

        if (value == '-')
            temp += 10 * 576;
        else if (value == ' ')
            temp += 11 * 576;
        else
            temp += (value - 48) * 576;
        //	LCD_SetPos(x,x+47,y,y+119);
    }
    else
    {
        temp = chlib16x24;
        cp = 16; pp = 24;
        if (value == 0x32)
            temp = chlib16x24;
        else if(value == 0x2e)
            temp += 1 * 48;
        else if (value == 0x35)
            temp += 2 * 48;


        //temp += (value - 32) * 108;
    }
    //	if(form == FORM32X64)
    LCD_SetPos(x, x + cp - 1, y, y + pp - 1);
    //	else
    //		LCD_SetPos(x,x+14,y,y+pp-1);
    cnum = pp*cp / 8;


    for (j = 0;j<cnum;j++)
    {
        for (i = 0;i<8;i++)
        {
            if (((*temp >> i) & 0x01) != 0)
            {
                Write_Data(themeColor_text >> 8, themeColor_text);
            }
            else
            {
                Write_Data(themeColor_bg >> 8, themeColor_bg);
            }
        }
        temp++;
    }
}

void disp_ch(uint8_t form, uint16_t x, uint16_t y,uint8_t value,uint16_t dcolor,uint16_t bgcolor)
{
	uint16_t i,j;
	uint8_t const *temp ;//= chlibsmall;
	uint16_t cp = 0;
	uint16_t pp = 0;
	uint16_t cnum;
	uint16 themeColor_text = LcdThemeUpdateColors(dcolor);
	uint16 themeColor_bg = LcdThemeUpdateColors(bgcolor);

	if(form == FORM32X64)
	{
		temp = chlib;
		cp = 48; pp = 96;

		if(value == '-')
			temp += 10 * 576;
		else if(value == ' ')
			temp += 11 * 576;
		else
			temp += (value - 48) * 576;
		//	LCD_SetPos(x,x+47,y,y+119);
	}
	else if(form == FORM48X64)
	{
		temp = chlib_48x64;
		cp = 48; pp = 64;

		if(value == '-')
			temp += 10 * 384;
		else if(value == ' ')
			temp += 11 * 384;
		else
			temp += (value - 48) * 384;
	}
	else
	{
		temp = chlibsmall;
		cp = 24; pp = 36;
		temp += (value - 32) * 108;
	}
//	if(form == FORM32X64)
		LCD_SetPos(x,x+cp-1,y,y+pp-1);
//	else
//		LCD_SetPos(x,x+14,y,y+pp-1);
	cnum = pp*cp/8 ;

	for(j=0;j<cnum;j++)
	{
		for(i=0;i<8;i++)
		{
			if(((*temp >> i) & 0x01)!=0)
			{
				Write_Data(themeColor_text >> 8,themeColor_text);
			}
			else
			{
				Write_Data(themeColor_bg >> 8,themeColor_bg);
			}
		}
		temp++;
	}
}

//only use for show " 2.5"
void disp_special_str(uint8_t form, uint16_t x, uint16_t y, char *str, uint16_t dcolor, uint16_t bgcolor)
{
    unsigned int x1, y1;
    x1 = x;
    y1 = y;
    while (*str != '\0')
    {
        disp_special_ch(form, x1, y1, *str, dcolor, bgcolor);
        if (form == FORM32X64) //big form
            x1 += 31;
        else
            x1 += 15;
        str++;
    }
}


void disp_str(uint8_t form, uint16_t x,uint16_t y,char *str,uint16_t dcolor,uint16_t bgcolor)
{
	unsigned int x1,y1;
	x1=x;
	y1=y;
	while(*str!='\0')
	{
		disp_ch(form,x1,y1,*str,dcolor,bgcolor);
//		if(form == FORM48X120) //big form
//			x1+=47;
//		else
//			x1+=15;

		if(form == FORM32X64) //big form
			x1+=31;
		else
			x1+=23;
		str++;
	}
}

void disp_ch_16_24(uint8 form, uint16 x, uint16 y, uint8 value, uint16 dcolor, uint16 bgcolor)
{
	uint16 i, j;
	uint8 const *temp;//= chlibsmall;
	uint16 cp = 0;
	uint16 pp = 0;
	uint16 cnum;
	uint16 themeColor_text = LcdThemeUpdateColors(dcolor);
	uint16 themeColor_bg = LcdThemeUpdateColors(bgcolor);

	if (form == FORM32X64)
	{
		temp = chlib;
		cp = 48; pp = 96;

		if (value == '-')
			temp += 10 * 576;
		else if (value == ' ')
			temp += 11 * 576;
		else
			temp += (value - 48) * 576;
	}
	else if (form == FORM16X24)
	{
		temp = char_16_24;
		cp = 16; pp = 24;
		temp += (value - 32) * 48;
	}
	else
	{
		temp = ArialBlack_18x26;
		cp = 24; pp = 26;
		temp += (value - 32) * 78;
	}
	LCD_SetPos(x, x + cp - 1, y, y + pp - 1);
	cnum = pp*cp / 8;


	for (j = 0; j<cnum; j++)
	{
		for (i = 0; i<8; i++)
		{
			if (((*temp >> i) & 0x01) != 0)
			{
				Write_Data(themeColor_text >> 8, themeColor_text);
			}
			else
			{
				Write_Data(themeColor_bg >> 8, themeColor_bg);
			}
		}
		temp++;
	}
}

void disp_str_16_24(uint8 form, uint16 x, uint16 y, uint8 *str, uint16 dcolor, uint16 bgcolor)
{
	unsigned int x1, y1;
	x1 = x;
	y1 = y;
	while (*str != '\0')
	{
		disp_ch_16_24(form, x1, y1, *str, dcolor, bgcolor);
		if (form == FORM32X64) //big form
			x1 += 24;
		else if(form == FORM16X24)
		{
			x1 += 16;
		}
		else
		{
			if(*str == 'i')
				x1 += 9;
			else if(*str == 't' || *str == 'l' || *str == 'I' || *str == 'j')
				x1 += 12;
			else if(*str == 'W' || *str == 'M')
				x1 += 16;
			else if( *str == 'U' || *str == 'O')
				x1 += 15;
			else
				x1 += 14;
		}
		str++;
	}
}

//uint16 Get_color(uint8 const *icon_name, uint16 pos,uint16 dot)
//{
//uint16 color = 0;
//uint8 const *temp = icon_name;
//color =
//}

void disp_icon(uint16 cp, uint16 pp, uint16 const *icon_name, uint16 x,uint16 y,uint16 dcolor, uint16 bgcolor)
{
	uint16 j;
	uint16 cnum;

	cnum = pp*cp;

  	LCD_SetPos(x,x+cp-1,y,y+pp-1);
	for(j=0;j<cnum;j++)
	{
		uint16 themeColor = LcdThemeUpdateSymbol(icon_name[j]);
		Write_Data( themeColor >> 8, themeColor);
	}
}

void disp_edge(uint16 cp, uint16 pp, uint16 const *icon_name, uint16 x,uint16 y,uint16 dcolor, uint16 bgcolor)
{
	uint16 j;
	uint16 cnum;

	cnum = pp*cp;

  	LCD_SetPos(x,x+cp-1,y,y+pp-1);
	for(j=0;j<cnum;j++)
	{
		uint16 themeColor = LcdThemeUpdateColors(icon_name[j]);
		Write_Data( themeColor >> 8, themeColor);
	}
}

void disp_null_icon(uint16 cp, uint16 pp, uint16 const *icon_name, uint16 x,uint16 y,uint16 dcolor, uint16 bgcolor)
{
	uint16 j;
	uint16 cnum;
	uint16 themeColor_bg = LcdThemeUpdateColors(bgcolor);

	cnum = pp*cp;

  	LCD_SetPos(x,x+cp-1,y,y+pp-1);
	for(j=0;j<cnum;j++)
	{
		Write_Data(themeColor_bg>>8,themeColor_bg);
	}
}


#if AL_OLD_DISPLAY
void display_config_line123(uint8 line,uint16 disp_setpoint)
{
	char spbuf[7];
	uint8 unit = 0;
	uint8 pos = 0;

	switch(line)
	{
		case 1: pos = SETPOINT_POS;break;
		case 2: pos = FAN_MODE_POS;break;
		case 3: pos = SYS_MODE_POS;break;
		default:break;
	}
	if(display_config[line] == ITEM_NONE)  // empty
	{
		disp_str(FORM15X30, SCH_XPOS,  pos,  "          ",SCH_COLOR,TSTAT8_BACK_COLOR);

		return;
	}
	if(display_config[line] == ITEM_CO2 || display_config[line] == ITEM_VOC || display_config[line] == ITEM_LIGHT)
	{ // CO2 OR VOC OR LIGHT
		if(disp_setpoint >= 50000) disp_setpoint = 50000;
		if(disp_setpoint >= 10000)
		{
			spbuf[0] = disp_setpoint/10000 + 0x30;
			spbuf[1] = (disp_setpoint%10000)/1000 + 0x30;
			spbuf[2] = (disp_setpoint%1000)/100 + 0x30;
			spbuf[3] = (disp_setpoint%100)/10 + 0x30;
			spbuf[4] = (disp_setpoint%10) + 0x30;
			spbuf[5] =  '\0';
		}
		else if(disp_setpoint >= 1000)
		{
			spbuf[0] = disp_setpoint/1000 + 0x30;
			spbuf[1] = (disp_setpoint%1000)/100 + 0x30;
			spbuf[2] = (disp_setpoint%100)/10 + 0x30;
			spbuf[3] = (disp_setpoint%10) + 0x30;
			spbuf[4] =  ' ';
			spbuf[5] =  '\0';
		}
		else if(disp_setpoint >= 100)
		{
			spbuf[0] = disp_setpoint/100 + 0x30;
			spbuf[1] = (disp_setpoint%100)/10 + 0x30;
			spbuf[2] = (disp_setpoint%10) + 0x30;
			spbuf[3] = ' ';
			spbuf[4] = ' ';
			spbuf[5] = '\0';
		}
		else if(disp_setpoint >= 10)
		{
			spbuf[0] = (disp_setpoint%100)/10 + 0x30;
			spbuf[1] = (disp_setpoint%10) + 0x30;
			spbuf[2] =  ' ';
			spbuf[3] =  ' ';
			spbuf[4] =  ' ';
			spbuf[5] =  '\0';
		}
		else if(disp_setpoint < 10)
		{
			spbuf[0] = (disp_setpoint%10) + 0x30;
			spbuf[1] =  ' ';
			spbuf[2] =  ' ';
			spbuf[3] =  ' ';
			spbuf[4] =  ' ';
			spbuf[5] =  '\0';
		}

		disp_str(FORM15X30, SCH_XPOS+96,pos,  spbuf,  SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	else if(display_config[line] == ITEM_TEMP || display_config[line] == ITEM_HUM)
	{ // HUM OR TEMPERATURE
		if(display_config[line] == 1)
		{
			unit = '%';
		}
		else if(display_config[line] == 0)
		{
			if(EEP_DEGCorF == 0)
				unit = 'C';
			else
				unit = 'F';
		}

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

		disp_str(FORM15X30, SCH_XPOS+96,pos,  spbuf,  SCH_COLOR,TSTAT8_BACK_COLOR);



	}

	if(display_config[line] == ITEM_TEMP) // temp
	{
		disp_str(FORM15X30, SCH_XPOS,  pos,"Tem:",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	else if(display_config[line] == ITEM_HUM) // hum
	{
		disp_str(FORM15X30, SCH_XPOS,  pos, "Hum:",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	else if(display_config[line] == ITEM_CO2) // co2
	{
		disp_str(FORM15X30, SCH_XPOS,  pos, "CO2:",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	else if(display_config[line] == ITEM_VOC) // voc
	{
		disp_str(FORM15X30, SCH_XPOS,  pos, "VOC:",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
	else if(display_config[line] == ITEM_LIGHT)  // light
	{
		disp_str(FORM15X30, SCH_XPOS, pos,  "LUX:",SCH_COLOR,TSTAT8_BACK_COLOR);
	}
}

#endif

void display_pm25w(uint16 value)
{
	char spbuf[7];
    if (value > 9999) //ÆÁÄ»Ö»ÄÜÏÔÊ¾ 9999
    {
        value = 9999;
    }

	if(value >= 1000)
	{
		spbuf[0] = value/1000 + 0x30;
		spbuf[1] = (value%1000)/100 + 0x30;
		spbuf[2] = (value%100)/10 + 0x30;
		spbuf[3] = (value%10) + 0x30;
		spbuf[4] =  '\0';
	}
	else if(value >= 100)
	{
		spbuf[0] = value/100 + 0x30;
		spbuf[1] = (value%100)/10 + 0x30;
		spbuf[2] = (value%10) + 0x30;
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
	else if(value >= 10)
	{
		spbuf[0] = value/10 + 0x30;
		spbuf[1] = (value%10) + 0x30;
		spbuf[2] = ' ';
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
	else if(value >= 0)
	{

		spbuf[0] = (value%10) + 0x30;
		spbuf[1] = ' ';
		spbuf[2] = ' ';
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
	//disp_str(FORM15X30, SCH_XPOS+128,PM25_W_POS,  spbuf,  SCH_COLOR,TSTAT8_BACK_COLOR);
	if(AQI_level == GOOD) disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == MODERATE) disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == POOL_FOR_SOME) disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == UNHEALTHY)  disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == MORE_UNHEALTHY) disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else	disp_str(FORM15X30, SCH_XPOS+116,PM25_W_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
}

void display_pm25n(uint16 value)
{
	char spbuf[7];
    if (value > 9999)
    {
        value = 9999; //ÆÁÄ»Ö»ÄÜÏÔÊ¾ 9999
    }
	if(value >= 1000)
	{
		spbuf[0] = value/1000 + 0x30;
		spbuf[1] = (value%1000)/100 + 0x30;
		spbuf[2] = (value%100)/10 + 0x30;
		spbuf[3] = (value%10) + 0x30;
		spbuf[4] =  '\0';
	}
	else if(value >= 100)
	{
		spbuf[0] = value/100 + 0x30;
		spbuf[1] = (value%100)/10 + 0x30;
		spbuf[2] = (value%10) + 0x30;
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
	else if(value >= 10)
	{
		spbuf[0] = value/10 + 0x30;
		spbuf[1] = (value%10) + 0x30;
		spbuf[2] = ' ';
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
	else if(value >= 0)
	{

		spbuf[0] = (value%10) + 0x30;
		spbuf[1] = ' ';
		spbuf[2] = ' ';
		spbuf[3] = ' ';
		spbuf[4] = '\0';
	}
//	disp_str(FORM15X30, SCH_XPOS+128,PM25_N_POS,  spbuf,  SCH_COLOR,TSTAT8_BACK_COLOR);
	if(AQI_level == GOOD) disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == MODERATE) disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == POOL_FOR_SOME) disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == UNHEALTHY)  disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else if(AQI_level == MORE_UNHEALTHY) disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
	else	disp_str(FORM15X30, SCH_XPOS+116,PM25_N_POS,  spbuf, 0xffff - aqi_background_color, aqi_background_color);
}


void display_dec(uint8 blink)
{
	if(blink)//show dec
		disp_null_icon(8, 8,0, SECOND_CH_POS + 48 + 4,85,TSTAT8_CH_COLOR, TSTAT8_CH_COLOR);
	else//hide dec
		disp_null_icon(8, 8,0, SECOND_CH_POS + 48 + 4,85,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
}

void Display_DeviceName(void)
{
    char *device_name;
    uint16_t x_center;

    // ==========================
    // Select Device Name
    // ==========================
    if (Modbus.mini_type == MINI_BIG_ARM)
	{
        device_name = "Big-ARM";
		x_center = SCH_XPOS + 40;
	}
    else if (Modbus.mini_type == MINI_SMALL_ARM)
	{
        device_name = "Small-ARM";
		x_center = SCH_XPOS + 20;
	}
    else if (Modbus.mini_type == MINI_TINY_ARM)
	{
        device_name = "Tiny-ARM";
		x_center = SCH_XPOS + 30;
	}
    else if (Modbus.mini_type == MINI_NANO)
	{
        device_name = "Nano";
		x_center = SCH_XPOS + 50;
	}
    else if (Modbus.mini_type == MINI_TSTAT10)
	{
        device_name = "TSTAT-10";
		x_center = SCH_XPOS + 25;
	}
    else if (Modbus.mini_type == MINI_T10P)
	{
        device_name = "T10-P";
		x_center = SCH_XPOS + 45;
	}
    else
	{
        device_name = "T-Stat8";
		x_center = SCH_XPOS + 40;
	}

    // ==========================
    // Center Device Name
    // ==========================

	ClearScreen(TSTAT8_BACK_COLOR);
    disp_str(FORM15X30, x_center, DEVICE_NAME_POS, device_name, SCH_COLOR, TSTAT8_BACK_COLOR);

    // ==========================
    // Display “Initialising…” Below
    // ==========================
    char *init_text = "Initialising..";

    disp_str_16_24(FORM16X24, SCH_XPOS, DEVICE_INIT_POS, (unsigned char*)init_text, SCH_COLOR, TSTAT8_BACK_COLOR);
	vTaskDelay(100);
}

void Top_area_display(uint8 item, S16_T value, uint8 unit)
{
	int16 value_buf;
	switch(item)
	{
		case TOP_AREA_DISP_ITEM_TEMPERATURE:
			if(unit == TOP_AREA_DISP_UNIT_C || unit == TOP_AREA_DISP_UNIT_F || unit == TOP_AREA_DISP_UNIT_RH)
			{
				if(value >=0)
				{
					value_buf = value;
					disp_str(FORM15X30, 6,  32,  " ",SCH_COLOR,TSTAT8_BACK_COLOR);
					if(value >= 1000)
					{
						value_buf /= 10;
						if((value_buf >= 100))
							disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value_buf/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						else
							disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
					if(value<1000 )
					{
		//				disp_null_icon(30, 96, 0, THERM_METER_XPOS,THERM_METER_POS,TSTAT8_MENU_COLOR, TSTAT8_MENU_COLOR);
						//disp_ch(0,0,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						if((value >= 100))
							disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value_buf/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						else
							disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
					else if(value <100)
					{
						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value_buf/1000,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%1000)/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
				}
				else//nagtive value
				{
					value_buf = -value;
					disp_str(FORM15X30, 6,  32,  "-",SCH_COLOR,TSTAT8_BACK_COLOR);
					//disp_null_icon(0, 8, 0, THERM_METER_XPOS+2,53,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
					if(value_buf >= 100)
					{
						//disp_ch(0,0,THERM_METER_POS,'-',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);

						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value_buf/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
					else if(value_buf < 100)
					{
						//disp_ch(0,0,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						//disp_str(FORM15X30, FIRST_CH_POS,  25,  ' ',SCH_COLOR,TSTAT8_BACK_COLOR);
						//disp_str(FORM15X30,FIRST_CH_POS,THERM_METER_POS,'-',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
				}
			}
			else
			{
				if(value > 999) value = 999;
				disp_str(FORM15X30, 6,  32," ",SCH_COLOR,TSTAT8_BACK_COLOR);
				if(value >= 100)
				{
					disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30+value%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				}
				else if(value >= 10)
				{
					disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30+value%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				}
				else if(value >= 0)
				{
					disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,SECOND_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30+value%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				}
				else
				{
					value_buf = -value;
					disp_str(FORM15X30, 6,  32,  "-",SCH_COLOR,TSTAT8_BACK_COLOR);
					if(value_buf >= 100)
					{
						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,0x30+value_buf/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
					else if(value_buf >= 10)
					{
						//disp_ch(0,0,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						//disp_str(FORM15X30, FIRST_CH_POS,  25,  ' ',SCH_COLOR,TSTAT8_BACK_COLOR);
						//disp_str(FORM15X30,FIRST_CH_POS,THERM_METER_POS,'-',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
					else
					{
						disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,SECOND_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						disp_ch(0,THIRD_CH_POS-16,THERM_METER_POS,0x30 + value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
					}
				}
			}
			break;

		default:
			break;

	}

	    if(unit == TOP_AREA_DISP_UNIT_C)
		{
			//disp_null_icon(240, 36, 0, 0,TIME_POS,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR2);
			disp_edge(14, 14, degree_o, UNIT_POS - 14,56 ,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

			if(value > 1000)
				display_dec(0);
			else
				display_dec(1);

			disp_str(FORM15X30, UNIT_POS,56,"C",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
		}
	    else if(unit == TOP_AREA_DISP_UNIT_F)
		{
			//disp_null_icon(240, 36, 0, 0,TIME_POS,TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR2);
			disp_edge(14, 14, degree_o, UNIT_POS - 14,56 ,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			if(value>1000)
				display_dec(0);
			else
				display_dec(1);
			disp_str(FORM15X30, UNIT_POS, 56, "F", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else if(unit == TOP_AREA_DISP_UNIT_RH)
		{
			display_dec(1);
	      	disp_str(FORM15X30, UNIT_POS - 23, 56, "%R", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else if(unit == TOP_AREA_DISP_UNIT_PPM)
		{
			display_dec(0);
	        disp_str(FORM15X30, UNIT_POS - 23, 56, "pM", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else if(unit == TOP_AREA_DISP_UNIT_PERCENT)
		{
			display_dec(0);
	        disp_str(FORM15X30, UNIT_POS, 56, "%", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else if(unit == TOP_AREA_DISP_UNIT_kPa)
		{
			display_dec(0);
	        disp_str(FORM15X30, UNIT_POS - 23, 56, "kP", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else if(unit == TOP_AREA_DISP_UNIT_Pa)
		{
			display_dec(0);
	        disp_str(FORM15X30, UNIT_POS - 23, 56, "Pa", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
		else //if(unit == TOP_AREA_DISP_UNIT_NONE)
		{
			display_dec(0);
	        disp_str(FORM15X30, UNIT_POS - 16, 56, "  ", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
		}
	//		else
	//			disp_str(FORM15X30, UNIT_POS,56,"F",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
	//		icon.unit = 0;
	//	}

}

#if AL_OLD_DISPLAY   // 整合T10和AL之前 AL的display task
extern g_sensor_t g_sensors;

void Display_Configure(void)
{
	uint8_t line;
	for (line = 1; line <= 3; line++)
	{
		switch(display_config[line])
		{
			case ITEM_TEMP:
				display_config_line123(line,g_sensors.temperature);
				break;
			case ITEM_HUM:
				display_config_line123(line,g_sensors.humidity);
				break;
			case ITEM_CO2:
				display_config_line123(line,g_sensors.co2);
				break;
			case ITEM_VOC:
				display_config_line123(line,g_sensors.voc_value);
				break;
			case ITEM_LIGHT:
				display_config_line123(line,g_sensors.light_value);
				break;
			default:
				display_config_line123(line,0);
				break;
		}
	}

}



void LCDtest(void)
{
	Test[6]= 1;
	ClearScreen(0x0000);  //CLEAR DISPLAY
	delay_ms(1000);Test[6]= 2;
	ClearScreen(0xf800);  //RED
	delay_ms(1000);Test[6]= 3;
	ClearScreen(0x07e0);  //GREEN
	delay_ms(1000);Test[6]= 4;
	ClearScreen(0x001f);  //BLUE
	delay_ms(1000);Test[6]= 5;
	ClearScreen(0xffff);  //WHITE
	delay_ms(1000);Test[6]= 6;

	ClearScreen(0x0eff);  //CYAN
	delay_ms(1000);Test[6]= 7;
	ClearScreen(0xffe0);  //YELLOW
	delay_ms(1000);Test[6]= 8;
	ClearScreen(0xf81f);  //PINK
	delay_ms(1000);Test[6]= 9;
	ClearScreen(TSTAT8_BACK_COLOR);  //CLEAR DISPLAY

}
//8*12??


uint16 aqi_background_color = 0;
uint8 rx_icon = 0;
uint8 tx_icon = 0;

void Lcd_task(void *arg)
{
	Str_points_ptr ptr;
	uint16 old_aqi_background_color = 0;
	uint8 top_refresh = 0;
	static uint8 last_comm_display; // 0 - tx 1 - rx
	LCD_IO_Init();
	if(Modbus.mini_type == PROJECT_TSTAT9 || Modbus.mini_type == PROJECT_AIRLAB)
	{
		SCD40_ENABLE;  // ENABLE SCD40
		PM25_ENABLE; // ENALBLE PM25

		display_config[0] = ITEM_TEMP;
		display_config[1] = ITEM_HUM;
		display_config[2] = ITEM_CO2;

		Display_Configure();
		disp_str(FORM15X30, SCH_XPOS, PM25_W_POS, "PM  :", 0xffff - aqi_background_color, aqi_background_color);//Õâ¾ä»°Ö»ÊÇµ¥´¿µÄÎªÁËË¢ÐÂ±³¾°É«
		disp_str(FORM15X30, SCH_XPOS,  PM25_W_POS,  "PM", 0xffff - aqi_background_color, aqi_background_color);
		disp_special_str(FORM15X30, SCH_XPOS + 24 * 2 - 1, PM25_W_POS + 12, "2.5", 0xffff - aqi_background_color, aqi_background_color);
		disp_str(FORM15X30, SCH_XPOS + 24 * 4 - 3, PM25_W_POS , ":", 0xffff - aqi_background_color, aqi_background_color);
		disp_str(FORM15X30, SCH_XPOS,  PM25_N_POS, "PM10:", 0xffff - aqi_background_color, aqi_background_color);


	}
	//ILI9341_Initial();
	//LCD_BACK_HI;
	Test[4]++;
	//LCDtest();


	task_test.enable[6] = 1;

	old_aqi_background_color = aqi_background_color;
	top_refresh = 4;
	for (;;)
	{
		task_test.count[6]++;
		top_refresh++;
		disp_str(FORM15X30, SCH_XPOS,  PM25_N_POS, "PM10:", 0xffff - aqi_background_color, aqi_background_color);
		if(top_refresh > 5)
			top_refresh = 0;

		Display_Configure();

		if(AQI_level == GOOD) aqi_background_color = 0x9689;
		else if(AQI_level == MODERATE) aqi_background_color = 0xffe0;
		else if(AQI_level == POOL_FOR_SOME) aqi_background_color = 0xfde0;
		else if(AQI_level == UNHEALTHY)  aqi_background_color = 0xf800;
		else if(AQI_level == MORE_UNHEALTHY) aqi_background_color = 0x7194;
		else	aqi_background_color = 0xb800;

		if ((old_aqi_background_color != aqi_background_color) || (flag_refresh_PM25 == 1))//ÅÐ¶Ï±³¾°É«±ä»¯ÁËÃ»ÓÐ£¬Ã»ÓÐ±ä»¯¾Í²»ÒªË¢ÐÂ;
		{
			flag_refresh_PM25 = 0;
			disp_str(FORM15X30, SCH_XPOS, PM25_W_POS, "PM  :", 0xffff - aqi_background_color, aqi_background_color);//Õâ¾ä»°Ö»ÊÇµ¥´¿µÄÎªÁËË¢ÐÂ±³¾°É«
			disp_str(FORM15X30, SCH_XPOS, PM25_W_POS, "PM", 0xffff - aqi_background_color, aqi_background_color);
			disp_special_str(FORM15X30, SCH_XPOS + 24 * 2 - 1, PM25_W_POS + 12, "2.5", 0xffff - aqi_background_color, aqi_background_color);//Õâ¾ä²ÅÊµ¼ÊÏÔÊ¾ËõÐ¡°æµÄ2.5
			disp_str(FORM15X30, SCH_XPOS + 24 * 4 - 3, PM25_W_POS, ":", 0xffff - aqi_background_color, aqi_background_color);
			disp_str(FORM15X30, SCH_XPOS, PM25_N_POS, "PM10:", 0xffff - aqi_background_color, aqi_background_color);
			old_aqi_background_color = aqi_background_color;
		}

		// tbd:
		if(update_flag == 1)
		{
			ClearScreen(TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, 30,  30,  "Updating",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			//serial_restart();
			//SoftReset();
		}
		else if(update_flag == 2)
		{
			update_flag = 0;
			ClearScreen(TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, 30,  30,  "Restart",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			//initialize_eeprom( ) ; // set default parameters
			//DisRestart( );
		}
		/*else if(update_flag == 4)//switch baudrate
		{
			update_flag = 0;
			uart_init(0);
		}
		else if((update_flag == 5)||(update_flag == 7))
		{
			Set_night_setpoint(((setpoint_buf >> 8)&0xff), (setpoint_buf & 0xff));
			refresh_setpoint(NIGHT_MODE);
		  update_flag = 0;
		}
		else if((update_flag == 6)||(update_flag == 8))
		{
			Set_day_setpoint(((setpoint_buf >> 8)&0xff), (setpoint_buf & 0xff));
			refresh_setpoint(DAY_MODE);
		  update_flag = 0;
		}

		else if(update_flag == 11)
		{
			mass_flash_init();
			update_flag = 0;
		}*/

		else if(update_flag == 15)
		{
			disp_str(FORM15X30, SCH_XPOS,  SETPOINT_POS,  "SET",SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,	 FAN_MODE_POS,  "FAN",SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_str(FORM15X30, SCH_XPOS,  SYS_MODE_POS,  "SYS",SCH_COLOR,TSTAT8_BACK_COLOR);
			update_flag = 0;
		}
		else
		{
				//blink_parameter = !blink_parameter;
				{
					//blink_flag = !blink_flag;
					//display_flag = 1;
					if(top_refresh == 5)
					{
						if(display_config[0] == ITEM_TEMP) // temperature
						{
							Top_area_display(TOP_AREA_DISP_ITEM_TEMPERATURE, (int)g_sensors.temperature, TOP_AREA_DISP_UNIT_C);
						}
						else if(display_config[0] == ITEM_HUM) // humidity
						{
							Top_area_display(TOP_AREA_DISP_ITEM_HUM, g_sensors.humidity, TOP_AREA_DISP_UNIT_PERCENT);
						}
						else if(display_config[0] == ITEM_NONE)
						{ // emtpy
							Top_area_display(TOP_AREA_DISP_ITEM_NONE, 0, 0);
						}
					}
#if 1

					/*if(voltage_overshoot == 1) // rs485 lines have high voltage
					{
						clear_lines();//disp_str(FORM15X30, 0,  0,  "testing",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
						//103,TSTAT8_CH_COLOR
						disp_str(FORM15X30, 0,  103,  "High",0xf800,TSTAT8_BACK_COLOR);
						disp_str(FORM15X30, 0,  103+BTN_OFFSET,  "Voltage",0xf800,TSTAT8_BACK_COLOR);
						disp_str(FORM15X30, 0,  103+BTN_OFFSET+BTN_OFFSET,  "on RS485",0xf800,TSTAT8_BACK_COLOR);

						rs485_warning = 1;
					}
					else*/
					{
						ptr = put_io_buf(IN,1);
						if(!ptr.pin->calibration_sign)
							g_sensors.humidity += (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
						else
							g_sensors.humidity += -(ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
#if 0
						uint16 co2_tmp;
						co2_tmp = Filter(2, co2_data_org);
						ptr = put_io_buf(IN,2);
						if(!ptr.pin->calibration_sign)
							co2_data = co2_tmp + (inputs[2].calibration_hi * 256 + ptr.pin->calibration_lo) / 10;
						else
							co2_data = co2_tmp -(ptr.pin->calibration_hi * 256 + inputs[2].calibration_lo) / 10;
#endif
						/*if(!inputs[3].calibration_sign)
							g_sensors.voc_value = voc_value_raw + (inputs[3].calibration_hi * 256 + inputs[3].calibration_lo) / 10;
						else
							g_sensors.voc_value = voc_value_raw -(inputs[3].calibration_hi * 256 + inputs[3].calibration_lo) / 10;
*/
						//bac_input[11] = voc_value;

						// first line
						Display_Configure();

						ptr = put_io_buf(IN,5);

						if(!ptr.pin->calibration_sign)
							disp_pm25_weight_25 += (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
						else
							disp_pm25_weight_25 += -(ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
						ptr = put_io_buf(IN,7);
						if(!ptr.pin->calibration_sign)
							disp_pm25_number_25 += (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
						else
							disp_pm25_number_25 += -(ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
//
						if(display_config[4] == ITEM_NONE)
						{
							disp_str(FORM15X30, SCH_XPOS,  PM25_W_POS,  "          ",SCH_COLOR,TSTAT8_BACK_COLOR);
							disp_str(FORM15X30, SCH_XPOS,  PM25_N_POS,  "          ",SCH_COLOR,TSTAT8_BACK_COLOR);
						}
						else
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


#endif
					if((flagLED_sub_rx > 0) || (flagLED_sub_tx > 0))
					{
						if(flagLED_sub_tx == 0) // only rx
						{
							flagLED_sub_rx--;
							disp_icon(13, 26,cmnct_rcv,0,0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
							last_comm_display = 1;
						}
						else if(flagLED_sub_rx == 0) // only tx
						{
							flagLED_sub_tx--;
							disp_icon(13, 26,cmnct_send,0,0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
							last_comm_display = 0;
						}
						else // tx and rx
						{
							if(last_comm_display == 1)
							{
								flagLED_sub_tx--;
								disp_icon(13, 26,cmnct_send,0,0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
								last_comm_display = 0;
							}
							else
							{
								flagLED_sub_rx--;
								disp_icon(13, 26,cmnct_rcv,0,0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
								last_comm_display = 1;
							}
						}
					}
					else
					{
						disp_null_icon(13, 26, 0, 0,0,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
					}
				}
//					DealWithKey(menu_id);

				// wifi icon


				if(SSID_Info.IP_Wifi_Status == WIFI_NORMAL)//����Ļ���Ͻ���ʾwifi��״̬
				{
					if(SSID_Info.rssi < -90)
						disp_icon(26, 26, wifi_1, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
					else if(SSID_Info.rssi < -70)
						disp_icon(26, 26, wifi_2, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
					else if(SSID_Info.rssi < -67)
						disp_icon(26, 26, wifi_3, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
					else
						disp_icon(26, 26, wifi_4, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
				}
				else
						disp_icon(26, 26, wifi_0, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
#if 0
			if(SSID_Info.IP_Wifi_Status == WIFI_NORMAL)
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
			else if((SSID_Info.IP_Wifi_Status == WIFI_NO_CONNECT)
				 || (SSID_Info.IP_Wifi_Status == WIFI_SSID_FAIL)
			     || (SSID_Info.IP_Wifi_Status == WIFI_NO_WIFI))
					disp_icon(26, 26, wifi_0, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
			else // if WIFI_NONE, do not show wifi flag
				disp_icon(26, 26, wifi_none, 210,	0, TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
#endif

		}

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

#endif

