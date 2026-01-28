#ifndef __IMAGE2LCD_H
#define __IMAGE2LCD_H

#include "define.h"

//////////////////////////////////////////////////////////////////////////////////
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//�����տƼ�ETmcu������
//�ڲ�FLASHͼƬ��ʾ
//����:www.eya-display.com
//�޸�����:2016/3/26
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) �㶫ʡ�����տƼ����޹�˾ 2016-2026
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////

__packed typedef struct _HEADCOLOR
{
   unsigned char scan;
   unsigned char gray;
   unsigned short w;
   unsigned short h;
   unsigned char is565;
   unsigned char rgb;
}HEADCOLOR;
//Scan:bit4:0=LSB first,1=MSB first
//     bit3~0:reserved
// gray:0:not gray image; 1:gray image
// w:image width
// h:image height
// is565:0:not 16bit color; 1:16bit color
// rgb:0:RGB; 1:BGR

/* Function prototypes */
void image_display(u16 x,u16 y,u8 * imgx);
void image_show(u16 xsta,u16 ysta,u16 xend,u16 yend,u8 scan,u8 *p);
u16 image_getcolor(u8 mode,u8 *str);

#endif













