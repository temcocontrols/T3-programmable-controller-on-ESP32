/**
 * @file lcdTheme.c
 * @brief Display theme management for the LCD subsystem.
 *
 * Detailed description:
 * - This module centralizes creation, storage, retrieval and application of
 *   display themes (color palette, brightness, and simple effects).
 * - It provides a small API to initialize theme management and apply
 *   themes to the LCD driver.
 * - Persistence and LCD-driver integration points are intentionally abstracted:
 *   backend-specific read/write and apply hooks should be implemented by the
 *   platform layer and wired into this module.
 *
 * Responsibilities:
 * - Maintain an in-memory "current theme".
 * - Validate and normalize theme inputs.
 * - Mark theme as dirty when it differs from persisted state.
 * - Provide synchronous apply() that updates the LCD through a platform hook.
 *
 * Thread-safety:
 * - The module is not internally synchronized. Caller must serialize access
 *   (e.g., call from a single UI thread or protect with a mutex).
 * - Functions that may be called concurrently: none guaranteed safe.
 *
 * Memory ownership:
 * - All API calls that accept pointers either copy the contents (no ownership
 *   transfer) or require valid pointers for the duration of the call.
 * - The module holds its own internal copy of the current theme.
 *
 *
 * @author  Bhavik Panchal
 * @date    13-11-2025
 * @version 1.0
 *
 * @see lcd.h, flash.h
 */

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "LcdTheme.h"
#include "LCD_TSTAT.h"
#include "driver/uart.h"
#include "Menu.h"
#include "wifi.h"

#define TOTAL_THEMES   10

const sLcdTheme_t ThemeList[TOTAL_THEMES] =
{
    // ---------------------------------------------------------------
    // 0 – Classic Light
    // ---------------------------------------------------------------
    {
        TSTAT8_CH_COLOR,      // textColor
        TSTAT8_BACK_COLOR,    // backgroundColor
        TSTAT8_BACK_COLOR1,   // backgroundColor1
        TSTAT8_MENU_COLOR,    // menuColor
        TSTAT8_MENU_COLOR2,   // menuColor2
        TANGLE_COLOR,         // borderColor
        DIS_SYMBOL_COLOR_HL,  // highlightColor
        DIS_COLOR_ERROR       // errorColor
    },

    // ---------------------------------------------------------------
    // 1 – Classic Dark
    // ---------------------------------------------------------------
    {
        COLOR_WHITE,        // textColor
        COLOR_BLACK,        // backgroundColor
        COLOR_GRAY,         // backgroundColor1
        COLOR_GRAY,         // menuColor
        COLOR_GRAY,         // menuColor2
        COLOR_CYAN,         // borderColor
        COLOR_CYAN,         // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 2 – Sky Theme
    // ---------------------------------------------------------------
    {
        COLOR_BLUE,         // textColor
        COLOR_SKYBLUE,      // backgroundColor
        COLOR_SILVER,       // backgroundColor1
        COLOR_WHITE,        // menuColor
        COLOR_SILVER,       // menuColor2
        COLOR_NAVY,         // borderColor
        COLOR_BLUE,         // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 3 – Green Energy
    // ---------------------------------------------------------------
    {
        COLOR_WHITE,        // textColor
        COLOR_GREEN,        // backgroundColor
        TSTAT8_BACK_COLOR1, // backgroundColor1
        TSTAT8_MENU_COLOR,  // menuColor
        COLOR_DARKGRAY,     // menuColor2
        COLOR_BLACK,        // borderColor
        COLOR_SKYBLUE,      // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 4 – Warning Style
    // ---------------------------------------------------------------
    {
        COLOR_BLACK,        // textColor
        COLOR_YELLOW,       // backgroundColor
        COLOR_RED,          // backgroundColor1
        COLOR_ORANGE,       // menuColor
        COLOR_BROWN,        // menuColor2
        COLOR_RED,          // borderColor
        COLOR_RED,          // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 5 – Fire Alert
    // ---------------------------------------------------------------
    {
        COLOR_WHITE,        // textColor
        COLOR_RED,          // backgroundColor
        COLOR_ORANGE,       // backgroundColor1
        COLOR_ORANGE,       // menuColor
        COLOR_DARKGRAY,     // menuColor2
        COLOR_YELLOW,       // borderColor
        COLOR_YELLOW,       // highlightColor
        COLOR_BLACK         // errorColor
    },

    // ---------------------------------------------------------------
    // 6 – Aqua Mode
    // ---------------------------------------------------------------
    {
        COLOR_BLACK,        // textColor
        COLOR_CYAN,         // backgroundColor
        TSTAT8_MENU_COLOR,  // backgroundColor1
        TSTAT8_MENU_COLOR,  // menuColor
        COLOR_TEAL,         // menuColor2
        COLOR_NAVY,         // borderColor
        COLOR_NAVY,         // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 7 – Royal Theme
    // ---------------------------------------------------------------
    {
        COLOR_WHITE,        // textColor
        COLOR_MAROON,       // backgroundColor
        COLOR_BROWN,        // backgroundColor1
        COLOR_BROWN,        // menuColor
        COLOR_GOLD,         // menuColor2
        COLOR_YELLOW,       // borderColor
        COLOR_GOLD,         // highlightColor
        COLOR_BLACK         // errorColor
    },

    // ---------------------------------------------------------------
    // 8 – Metallic
    // ---------------------------------------------------------------
    {
        COLOR_BLACK,        // textColor
        COLOR_SILVER,       // backgroundColor
        COLOR_GRAY,         // backgroundColor1
        COLOR_GRAY,         // menuColor
        COLOR_DARKGRAY,     // menuColor2
        TANGLE_COLOR,       // borderColor
        COLOR_BLUE,         // highlightColor
        COLOR_RED           // errorColor
    },

    // ---------------------------------------------------------------
    // 9 – Violet Accent
    // ---------------------------------------------------------------
    {
        COLOR_WHITE,       // textColor
        COLOR_PURPLE,      // backgroundColor
        TANGLE_COLOR,      // backgroundColor1
        COLOR_LIGHT_PURPLE,// menuColor
        COLOR_DARKGRAY,    // menuColor2
        TANGLE_COLOR,      // borderColor
        COLOR_PINK,        // highlightColor
        COLOR_RED          // errorColor
    }
};

/* Internal current theme */
static sLcdTheme_t CurrentTheme = ThemeList[0];
static eDeviceStage_t DeviceStage = DEVICE_STAGE_POWERON;
static bool LcdThemeUpdateRequired = false;

/* Static function prototypes */
static void LcdThemeSetCurrent(U8_T themeIndex);

/**************************************************/

/**
 * @brief Initialize the theme system.
 *
 * Loads persisted theme if available via platform_theme_load(), otherwise
 * initializes a sensible default. Must be called before other API calls.
 *
 * @return 0 on success, negative errno on failure.
 */
void LcdThemeInit(void)
{
    if(Modbus.LcdTheme >= 0 && Modbus.LcdTheme < TOTAL_THEMES)
    {
        LcdThemeSetCurrent(Modbus.LcdTheme);
    }
}

/**
 * @brief Mark the theme as needing update.
 *
 * Sets a flag indicating that the current theme should be reapplied to
 * the LCD on the next LcdThemeLoop() call.
 */
void LcdThemeMarkForUpdate( void )
{
    if(!LcdThemeUpdateRequired)
    {
        LcdThemeUpdateRequired = true;
    }
}

/**
 * @brief Theme loop processing.
 * Called periodically to apply pending theme updates.
 */
void LcdThemeLoop( void )
{
    if(LcdThemeUpdateRequired)
    {
        LcdThemeSetCurrent(Modbus.LcdTheme);
        LcdThemeUpdateRequired = false;
    }
}

/**
 * @brief Update input color to current theme color.
 *
 * Maps a given color identifier to the corresponding color in the
 * current theme. If the input color does not match any theme identifiers,
 * it is returned unchanged.
 *
 * @param[in] inColour The input color identifier.
 * @return U16_T The updated color from the current theme or the original color.
 */
U16_T LcdThemeUpdateColors( U16_T inColour )
{
    U16_T outColour = inColour;
    switch(inColour)
    {
        case TSTAT8_CH_COLOR:
            outColour = CurrentTheme.textColor;
            break;
        case TSTAT8_BACK_COLOR:
            outColour = CurrentTheme.backgroundColor;
            break;
        case TSTAT8_BACK_COLOR1:
            outColour = CurrentTheme.backgroundColor1;
            break;
        case TSTAT8_MENU_COLOR:
            outColour = CurrentTheme.menuColor;
            break;
#if TSTAT8_MENU_COLOR != TSTAT8_MENU_COLOR2
        case TSTAT8_MENU_COLOR2:
            outColour = CurrentTheme.menuColor2;
            break;
#endif
        case TANGLE_COLOR:
            outColour = CurrentTheme.borderColor;
            break;
        case DIS_SYMBOL_COLOR_HL:
            outColour = CurrentTheme.highlightColor;
            break;
        case DIS_COLOR_ERROR:
            outColour = CurrentTheme.errorColor;
            break;
        default:
            // No change
            break;
    }
    return outColour;
}

/**
 * @brief Update input symbol color to current theme highlight color.
 *
 * Maps a given symbol color identifier to the corresponding highlight color in the
 * current theme. If the input color does not match the highlight identifier,
 * it is returned unchanged.
 *
 * @param[in] inColour The input symbol color identifier.
 * @return U16_T The updated highlight color from the current theme or the original color.
 */
U16_T LcdThemeUpdateSymbol( U16_T inColour )
{
    U16_T outColour = inColour;

    if( inColour == TSTAT8_BACK_COLOR )
    {
        outColour = CurrentTheme.backgroundColor;
    }
    else
    {
        switch(Modbus.LcdTheme)
        {
            case 0:
                // No special symbol colors
                break;
            case 1:
                outColour = LcdThemeUpdateColors( inColour );
                break;
            case 2:
                // No special symbol colors
                break;
            case 3:
                //outColour = LcdThemeUpdateColors( inColour );
                break;
            case 4:

                // if( inColour == 0xb67b || inColour == 0x9659 ||
                //     inColour == 0x8e39 || inColour == 0x965a ||
                //     inColour == 0xae7b || inColour == 0x9e59 ||
                //     inColour == 0x8617 || inColour == 0x8e38 ||
                //     inColour == 0x9e5a || inColour == 0xa65a ||
                //     inColour == 0xae7a || inColour == 0xb69b ||
                //     inColour ==  )
                // {
                //     outColour = CurrentTheme.borderColor;
                // }
                // else
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            case 5:
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            case 6:
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            case 7:
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            case 8:
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            case 9:
                if( inColour == TSTAT8_MENU_COLOR)
                {
                    outColour = CurrentTheme.menuColor;
                }
                // else if( inColour != TSTAT8_CH_COLOR )
                // {
                //     outColour = LcdThemeUpdateColors( inColour );
                // }
                break;
            default:
                break;
        }
    }
    return outColour;
}

/**
 * @brief Get the current Device Stage
 *
 * Retrieves the current device stage.
 *
 * @param  None
 * @return eDeviceStage_t Current device stage.
 */
eDeviceStage_t Get_Device_Stage(void)
{
    return DeviceStage;
}

/**
 * @brief Set the current Device Stage
 *
 * Sets the current device stage.
 *
 * @param  stage The new device stage to set.
 * @return None
 */
void Set_Device_Stage(eDeviceStage_t stage)
{
    DeviceStage = stage;
}

/**
 * @brief Set the current theme.
 *
 * Sets the current theme to the specified index if valid.
 *
 * @param[in] themeIndex Index of the theme to set (0 to TOTAL_THEMES-1).
 */
static void LcdThemeSetCurrent(U8_T themeIndex)
{
    if(themeIndex < TOTAL_THEMES)
    {
        memcpy(&CurrentTheme, &ThemeList[themeIndex], sizeof(sLcdTheme_t));
        menu_init();
    }
}
