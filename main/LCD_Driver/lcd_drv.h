/**
 ****************************************************************************************************
 * @file        lcd.h
 * @author      EYA-DISPLAY
 * @version     V2.0
 * @date        2022-04-28
 * @brief       LCD
 * @license     Copyright (c) 2022-2032, �����տƼ�����(�㶫)
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:EYA-ETmcu������
 * ��˾��ַ:www.eya-display.com
 *
 ****************************************************************************************************
 **/
 #ifndef __LCD_DRV_H
#define __LCD_DRV_H

#include "stdlib.h"
#include "define.h"

#define LV_CONF_INCLUDE_SIMPLE 1

/*************************************** LCD WIDTH & HEIGHT *****************************/
#define LCD_WIDTH  320
#define LCD_HEIGHT 480

#define TFT_HOR_RES   LCD_HEIGHT // LCD_WIDTH
#define TFT_VER_RES   LCD_WIDTH  // LCD_HEIGHT

#define LCD_SPI_BUFFER_SIZE  TFT_HOR_RES * TFT_VER_RES * sizeof(uint16_t)

#define LCD_SPI_TX_SIZE 3072
/*************************************** LCD INTERFACE TYPE *****************************/

//#define Interface  I80_16BIT
//#define Interface  I80_H8BIT
//#define Interface  I80_L8BIT
#define Interface  D4WSPI

/***************************************** LCD ORIENTATION *****************************/
#define LCD_ORIENTATION  LCD_ORIENT_PORTRAIT

/***************************************** MADCTL BITS *****************************/
#define MADCTL_MY   (1 << 7)
#define MADCTL_MX   (1 << 6)
#define MADCTL_MV   (1 << 5)
#define MADCTL_RGB  (0 << 3)
#define MADCTL_BGR  (1 << 3)

/***************************************** LCD DISPLAY DIRECTION *****************************/
typedef enum
{
	LCD_ORIENT_PORTRAIT = 0,
	LCD_ORIENT_LANDSCAPE = 1,
} lcd_display_dir_t;

/***************************************** LCD SCANNING DIRECTION *****************************/
typedef enum
{
    LCD_DIR_0,      // 0°   Portrait
    LCD_DIR_90,     // 90°  Landscape
    LCD_DIR_180,    // 180° Portrait
    LCD_DIR_270     // 270° Landscape
} lcd_orientation_t;

/***************************************** LCD DEVICE STRUCT *****************************/
typedef struct
{
	u16 width;			// LCD width
	u16 height;			// LCD height
	u16 id;				// LCD ID
	u8  dir;			// LCD orientation
	u8	wramcmd;		// CMD to write to RAM
	u8  setxcmd;		// CMD to set x address
	u8  setycmd;		// CMD to set y address
}sLcd_Data_t;

// LcdStruct: LCD device information structure
extern sLcd_Data_t LcdStruct;

//////////////////////////////////////////////////////////////////////////////////
//-----------------LCD PIN definition----------------

#define LCD_PIN_MISO    26
#define LCD_PIN_MOSI    22
#define LCD_PIN_CLK     27

#define LCD_PIN_CS      0
#define LCD_PIN_DC      23
#define LCD_PIN_RST     25
#define LCD_PIN_BCKL    33

#define GPIO_LCD_RST_SEL   (1ULL<<LCD_PIN_RST)
#define GPIO_LCD_CS_SEL    (1ULL<<LCD_PIN_CS)
#define GPIO_LCD_RES_SEL   (1ULL<<LCD_PIN_DC)
#define GPIO_LCD_BACK_SEL  (1ULL<<LCD_PIN_BCKL)

// #define	LCD_LED PAout(LCD_PIN_BCKL)   //LCD    		 BL_EN
#define LCD_BCKL_EN  gpio_set_level(LCD_PIN_BCKL, 1)
#define LCD_BCKL_DIS gpio_set_level(LCD_PIN_BCKL, 0)

// #define	LCD_RST PCout(LCD_PIN_RST)   //LCD    		 RESET
#define LCD_RST_LO	gpio_set_level(LCD_PIN_RST, 0)
#define LCD_RST_HI	gpio_set_level(LCD_PIN_RST, 1)

// #define	LCD_CS  PBout(LCD_PIN_CS)
#define LCD_CS_LO	gpio_set_level(LCD_PIN_CS, 0)
#define LCD_CS_HI	gpio_set_level(LCD_PIN_CS, 1)

#define LCD_CLK_LO  gpio_set_level(LCD_PIN_CLK, 0)
#define LCD_CLK_HI  gpio_set_level(LCD_PIN_CLK, 1)

#define LCD_SDA_LO  gpio_set_level(LCD_PIN_MOSI, 0)
#define LCD_SDA_HI  gpio_set_level(LCD_PIN_MOSI, 1)

// #define	LCD_WR  PAout(12)   //LCD     WR
// #define	LCD_RD  PAout(15)   //LCD     RD

// LCD DC
#define L2R_U2D  0 // Display left to right, top to bottom
#define L2R_D2U  1 // Display left to right, bottom to top
#define R2L_U2D  2 // Display right to left, top to bottom
#define R2L_D2U  3 // Display right to left, bottom to top

#define U2D_L2R  4 // Display top to bottom, left to right
#define U2D_R2L  5 // Display top to bottom, right to left
#define D2U_L2R  6 // Display bottom to top, left to right
#define D2U_R2L  7 // Display bottom to top, right to left

extern u8 DFT_SCAN_DIR;

// Color definitions

#define WHITE      0xFFFF
#define BLACK      0x0000
#define BLUE       0x001F
#define BRED       0XF81F
#define GRED 	   0XFFE0
#define GBLUE	   0X07FF
#define RED        0xF800
#define MAGENTA    0xF81F
#define GREEN      0x07E0
#define CYAN       0x7FFF
#define YELLOW     0xFFE0
#define BROWN 	   0XBC40
#define BRRED 	   0XFC07
#define GRAY  	   0X8430

#define DARKBLUE   0X01CF
#define LIGHTBLUE  0X7D7C
#define GRAYBLUE   0X5458

// light colors

#define LIGHTGREEN     	 0X841F
#define LGRAY 			 0XC618

#define LGRAYBLUE        0XA651
#define LBBLUE           0X2B12

// Function prototypes
void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(u16 Color);
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_DrawPoint(u16 x,u16 y, uint32_t color);
void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color);
u16  LCD_ReadPoint(u16 x,u16 y);
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color);
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color);
void LCD_WriteBitmap(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t *color);
void LCD_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color);
void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode,u16 color);
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p,u16 color);
void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height);
void IO_init(void);


void LCD_WR_REG(u8);
void LCD_WR_DATA(u8);
void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteRAM(u16 RGB_Code);
void LCD_Scan_Dir(u8 dir);
void LCD_Display_Dir(u8 dir);
void TIM2_PWM_Init(u16 arr,u16 psc);
void SendPWMval(u16 value);

#endif





