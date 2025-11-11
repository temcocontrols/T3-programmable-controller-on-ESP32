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
#define COLOR_PINK            0xF81F  // 255, 192, 203
#define COLOR_GRAY            0x8410  // 128, 128, 128
#define COLOR_LIGHTGRAY       0xC618  // 192, 192, 192
#define COLOR_DARKGRAY        0x4208  //  64,  64,  64
#define COLOR_BROWN           0xA145  // 165,  42,  42
#define COLOR_GOLD            0xFEA0  // 255, 215,   0
#define COLOR_SILVER          0xC618  // 192, 192, 192
#define COLOR_MAROON          0x8000  // 128,   0,   0
#define COLOR_NAVY            0x000F  //   0,   0, 128
#define COLOR_TEAL            0x0410  //   0, 128, 128
#define COLOR_OLIVE           0x8400  // 128, 128,   0
#define COLOR_SKYBLUE         0x867D  // 135, 206, 235
#define COLOR_LIME            0x07E0  //   0, 255,   0
#define COLOR_CORAL           0xFBEA  // 255, 127,  80
#define COLOR_VIOLET          0xEC1D  // 238, 130, 238
#define COLOR_BEIGE           0xF7BB  // 245, 245, 220
#define COLOR_IVORY           0xFFFE  // 255, 255, 240
#define COLOR_TURQUOISE       0x471A  //  64, 224, 208
#define COLOR_AQUA            0x07FF  //   0, 255, 255
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

//==================================================
//  Application Theme Colors
//==================================================

#define TSTAT8_BACK_COLOR    COLOR_BLACK         // Main Background Color

#define TSTAT8_BACK_COLOR1   TSTAT8_BACK_COLOR

#define TSTAT8_MENU_COLOR    COLOR_LIGHTEN(TSTAT8_BACK_COLOR, 70)
#define TSTAT8_MENU_COLOR2   COLOR_LIGHTEN(TSTAT8_BACK_COLOR, 70)

//==================================================
//  Auto Text Color Selection Based on Background
//==================================================

// Default (fallback)
#define TSTAT8_CH_COLOR      COLOR_WHITE

#if   (TSTAT8_BACK_COLOR == COLOR_BLACK)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_WHITE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_RED)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_GREEN)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_BLUE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_YELLOW
#elif (TSTAT8_BACK_COLOR == COLOR_YELLOW)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_CYAN)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_MAGENTA)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_GRAY)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_DARKGRAY)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_LIGHTGRAY)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_NAVY)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_YELLOW
#elif (TSTAT8_BACK_COLOR == COLOR_BROWN)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_ORANGE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_PURPLE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_YELLOW
#elif (TSTAT8_BACK_COLOR == COLOR_SKY_BLUE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_MAROON)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_TEAL)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_WHITE
#elif (TSTAT8_BACK_COLOR == COLOR_GOLD)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#elif (TSTAT8_BACK_COLOR == COLOR_TURQUOISE)
    #undef  TSTAT8_CH_COLOR
    #define TSTAT8_CH_COLOR  COLOR_BLACK
#endif

//==================================================
//  Secondary UI Elements
//==================================================
#define SCH_COLOR          TSTAT8_CH_COLOR
#define TANGLE_COLOR       COLOR_DARKEN(TSTAT8_BACK_COLOR, 20)
#define SCH_BACK_COLOR     COLOR_LIGHTEN(SCH_COLOR, 20)
