
/**
 * @file lcdTheme.h
 * @brief Display theme management for the LCD subsystem.
 *
 *  *
 * @author  Bhavik Panchal - Electrobit Technologies
 * @date    13-11-2025
 * @version 1.0
 *
 */

 #ifndef _LCD_THEME_H_
 #define _LCD_THEME_H_

#include "define.h"
#include "types.h"
//==================================================
//  RGB565 Color Macros
//==================================================
#define COLOR_BLACK           0x0000  //   0,   0,   0
#define COLOR_WHITE           0xFFFF  // 255, 255, 255
#define COLOR_RED             0xF800  // 255,   0,   0
#define COLOR_GREEN           0x07E0  //   0, 255,   0
#define COLOR_BLUE            0x001F  //   0,   0, 255
#define COLOR_SKY_BLUE        0x867D  // 135, 206, 235
#define COLOR_YELLOW          0xFFE0  // 255, 255,   0
#define COLOR_CYAN            0x07FF  //   0, 255, 255
#define COLOR_MAGENTA         0xF81F  // 255,   0, 255
#define COLOR_ORANGE          0xFD20  // 255, 165,   0
#define COLOR_PURPLE          0x8010  // 128,   0, 128
#define COLOR_LIGHT_PURPLE    0xF81F  // 215,   0, 215
#define COLOR_PINK            0xF81F  // 255, 192, 203
#define COLOR_GRAY            0x8410  // 128, 128, 128
#define COLOR_LIGHTGRAY       0xC618  // 192, 192, 192
#define COLOR_DARKGRAY        0x4208  //  64,  64,  64
#define COLOR_BROWN           0xFB4D  // 165,  42,  42
#define COLOR_GOLD            0xFEA0  // 255, 215,   0
#define COLOR_SILVER          0xC618  // 192, 192, 192
#define COLOR_MAROON          0x8000  // 128,   0,   0
#define COLOR_NAVY            0x000F  //   0,   0, 128
#define COLOR_TEAL            0x0410  //   0, 128, 128
#define COLOR_OLIVE           0x8400  // 128, 128,   0
#define COLOR_SKYBLUE         0x867D  // 135, 206, 235
#define COLOR_LIME            0x9FF3  //  19,  63,  19
#define COLOR_CORAL           0xFBEA  // 255, 127,  80
#define COLOR_VIOLET          0xEC1D  // 238, 130, 238
#define COLOR_BEIGE           0xF7BB  // 245, 245, 220
#define COLOR_IVORY           0xFFFE  // 255, 255, 240
#define COLOR_TURQUOISE       0x471A  //  64, 224, 208
#define COLOR_AQUA            0xA7FF  // 165, 255, 255
#define COLOR_INDIGO          0x4810  //  75,   0, 130
#define COLOR_CHOCOLATE       0xD343  // 210, 105,  30

// ==============================
// Brightness Adjust Macro Helpers
// ==============================

// Extract 5-6-5 components
#define RGB565_R(color)       (((color) >> 11) & 0x1F)
#define RGB565_G(color)       (((color) >> 5) & 0x3F)
#define RGB565_B(color)       ((color) & 0x1F)

// Combine back to RGB565
#define RGB565(r, g, b)       (((r) << 11) | ((g) << 5) | (b))

// Clamp value between 0–31 or 0–63
#define CLAMP_5(v)            ((v) > 31 ? 31 : ((v) < 0 ? 0 : (v)))
#define CLAMP_6(v)            ((v) > 63 ? 63 : ((v) < 0 ? 0 : (v)))

// Darken or lighten by percentage
#define COLOR_DARKEN(color, percent)  \
    RGB565( CLAMP_5(RGB565_R(color) * (100 - percent) / 100), \
            CLAMP_6(RGB565_G(color) * (100 - percent) / 100), \
            CLAMP_5(RGB565_B(color) * (100 - percent) / 100) )

#define COLOR_LIGHTEN(color, percent) \
    RGB565( CLAMP_5(RGB565_R(color) + (31 - RGB565_R(color)) * percent / 100), \
            CLAMP_6(RGB565_G(color) + (63 - RGB565_G(color)) * percent / 100), \
            CLAMP_5(RGB565_B(color) + (31 - RGB565_B(color)) * percent / 100) )

#define COLOR_BLEND(color1, color2, percent) \
    RGB565( CLAMP_5( (RGB565_R(color1) * (100 - (percent)) + RGB565_R(color2) * (percent)) / 100 ), \
            CLAMP_6( (RGB565_G(color1) * (100 - (percent)) + RGB565_G(color2) * (percent)) / 100 ), \
            CLAMP_5( (RGB565_B(color1) * (100 - (percent)) + RGB565_B(color2) * (percent)) / 100 )  \
    )

//==================================================
//  Application Theme Colors
//==================================================

#define TSTAT8_CH_COLOR      COLOR_WHITE
#define TSTAT8_BACK_COLOR    0x7E19      // Main Background Color
#define TSTAT8_BACK_COLOR1   0x3cef      // Menu Highlight Background
#define TSTAT8_MENU_COLOR    0x7e17
#define TSTAT8_MENU_COLOR2   0x7e17
#define TANGLE_COLOR         0xbe9c
#define DIS_SYMBOL_COLOR_HL  0x4576
#define DIS_COLOR_ERROR      COLOR_RED

//==================================================
//  Auto Text Color Selection Based on Background
//==================================================

#define SCH_COLOR            TSTAT8_CH_COLOR
#define DIS_SYMBOL_BG        TSTAT8_BACK_COLOR
#define DIS_SYMBOL_COLOR     TSTAT8_CH_COLOR

//==================================================
//  Theme Structure Definition
//==================================================
typedef struct
{
    U16_T textColor;        // Default text color
    U16_T backgroundColor;  // Screen background color
    U16_T backgroundColor1; // Secondary background color
    U16_T menuColor;        // Menu color
    U16_T menuColor2;       // Secondary menu color
    U16_T borderColor;      // Border color for boxes
    U16_T highlightColor;   // Highlight color for selected items
    U16_T errorColor;       // Color for error messages
} sLcdTheme_t;

typedef enum
{
	DEVICE_STAGE_POWERON = 0,
	DEVICE_STAGE_INIT,
	DEVICE_STAGE_RUNNING,
	DEVICE_STAGE_ERROR,
	DEVICE_STAGE_MAX
} eDeviceStage_t;

void LcdThemeInit(void);
void LcdThemeLoop(void);
eDeviceStage_t Get_Device_Stage(void);
void LcdThemeMarkForUpdate( void );
void Set_Device_Stage(eDeviceStage_t stage);

U16_T LcdThemeUpdateColors( U16_T inColour );
U16_T LcdThemeUpdateSymbol( U16_T inColour );

#endif /* _LCD_THEME_H_ */
