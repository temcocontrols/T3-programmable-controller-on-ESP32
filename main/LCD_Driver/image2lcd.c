/**
 ****************************************************************************************************
 * @file        image2lcd.c
 * @author      EYA-DISPLAY
 * @version     V2.0
 * @date        2022-04-28
 * @brief       Һ��������Demo
 * @license     Copyright (c) 2022-2032, �����տƼ�����(�㶫)
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:EYA-ETmcu������
 * ��˾��ַ:www.eya-display.com
 *
 ****************************************************************************************************
 **/
#include "lcd_drv.h"
#include "image2lcd.h"

/*
 * @brief Get color value from image data
 * mode: Color format mode (0: LSB first, 1: MSB first)
 * str: Pointer to image data
 * return: 16-bit color value
 */
u16 image_getcolor(u8 mode,u8 *str)
{
	u16 color;
	if(mode)
	{
		color=((u16)*str++)<<8;
		color|=*str;
	}
	else
	{
		color=*str++;
		color|=((u16)*str)<<8;
	}
	return color;
}

/* @brief Display image on LCD at specified position
 * xsta: Starting X coordinate
 * ysta: Starting Y coordinate
 * width: Image width
 * height: Image height
 * scan: Scan mode (bit 4 indicates color format)
 * p: Pointer to image data
 */
void image_show(u16 xsta,u16 ysta,u16 width,u16 height,u8 scan,u8 *p)
{
	u32 i;
	u32 len=0;

	LCD_Set_Window(xsta,ysta,width,height);

	LCD_WriteRAM_Prepare();
	len=width*height;
	for(i=0;i<len;i++)
	{
		LCD_WriteRAM(image_getcolor(scan&(1<<4),p));
		p+=2;
	}
	LCD_Set_Window(0,0,LcdStruct.width,LcdStruct.height);
}

/* @brief Display image on LCD using image header
 * x: X coordinate
 * y: Y coordinate
 * imgx: Pointer to image data with header
 */
void image_display(u16 x,u16 y,u8 * imgx)
{
	HEADCOLOR *imginfo;
 	u8 ifosize=sizeof(HEADCOLOR);//�õ�HEADCOLOR�ṹ��Ĵ�С
	imginfo=(HEADCOLOR*)imgx;
 	image_show(x,y,imginfo->w,imginfo->h,imginfo->scan,imgx+ifosize);
}

/**************************** END OF FILE *********************************/
