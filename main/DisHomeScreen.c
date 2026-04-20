/**
 * @file  DisHomeScreen.c
 * @brief Home screen display management for the LCD subsystem.
 * Detailed description:
 * - This module manages the home screen display of the LCD.
 * - It handles the rendering of key information and status indicators.
 * Responsibilities:
 * - Render the home screen layout.
 * - Update dynamic elements on the home screen.
 *
 * @author  Bhavik Panchal
 * @date    1-12-2025
 * @version 1.0
 *
 */

#include <string.h>
#include <limits.h>
#include "LcdTheme.h"
#include "lcd.h"
#include "LCD_TSTAT.h"
#include "driver/uart.h"
#include "Menu.h"
#include "controls.h"
#include "ud_str.h"
#include "define.h"

static void DisplayDrawFormat( void );
void DisplayTemperature( uint8_t index , S16_T value, uint8 unit);
void DisplayHumidity( uint8_t index , S16_T value );

Str_points_ptr Temperature_OutdorrDataPtr;
Str_points_ptr Temperature_AmbientDataPtr;
Str_points_ptr Humidity_OutdorrDataPtr;
Str_points_ptr Humidity_AmbientDataPtr;

#define UNKNOWN_VALUE 0xFFFF

int32_t Temperature_OutValue     = UNKNOWN_VALUE;
int32_t Temperature_AmbientValue = UNKNOWN_VALUE;
int32_t Humidity_OutValue        = UNKNOWN_VALUE;
int32_t Humidity_AmbientValue    = UNKNOWN_VALUE;
int8_t Prev_TempSign[3] = {2, 2, 2}; // index 2 is reserved for setpoint
bool IsOutdoorTempValid = false;
bool HomeScreenSetpointMode = false; // toggled by keypad on home screen
static bool Prev_SetpointMode = false;

static void DisplaySetpointValue(void);

void DisplayHomeScreen( bool isHomeScreen )
{
    Temperature_AmbientDataPtr = put_io_buf(IN, 8); // VAR9 is for room temperature
    Temperature_OutdorrDataPtr = put_io_buf(IN, 6); // VAR7 is for outdoor temperature
    Humidity_AmbientDataPtr    = put_io_buf(IN, 10);// VAR11 is for room humidity
    Humidity_OutdorrDataPtr    = put_io_buf(IN, 7); // VAR8 is for outdoor humidity

    if(Prev_SetpointMode != HomeScreenSetpointMode)
    {
        isHomeScreen = false; // force redraw when mode changes
        Prev_SetpointMode = HomeScreenSetpointMode;
    }

    if(Temperature_OutdorrDataPtr.pin->range == R10K_40_120DegC ||
            Temperature_OutdorrDataPtr.pin->range == R10K_40_250DegF)
    {
        if(!IsOutdoorTempValid)
        {
            IsOutdoorTempValid = true;
            isHomeScreen = false; // Force refresh the screen to show outdoor temperature now that it's valid
        }
    }
    else
    {
        if(IsOutdoorTempValid)
        {
            IsOutdoorTempValid = false;
            isHomeScreen = false; // Force refresh the screen to hide outdoor temperature now that it's not valid
        }
    }

    if(!isHomeScreen)
    {
        ClearScreen(TSTAT8_BACK_COLOR);
        DisplayDrawFormat();
        Temperature_OutValue     = UNKNOWN_VALUE;
        Temperature_AmbientValue = UNKNOWN_VALUE;
        Humidity_OutValue        = UNKNOWN_VALUE;
        Humidity_AmbientValue    = UNKNOWN_VALUE;
        Prev_TempSign[0]   = 2;
        Prev_TempSign[1]   = 2;
        Prev_TempSign[2]   = 2;
    }
    else
    {
        if(Temperature_AmbientValue != Temperature_AmbientDataPtr.pin->value)
        {
            Temperature_AmbientValue = Temperature_AmbientDataPtr.pin->value;
            if(Temperature_AmbientDataPtr.pin->range == 4 ) /* R10K_40_250DegF*/
                DisplayTemperature(0, Temperature_AmbientDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_F);
            else if(Temperature_AmbientDataPtr.pin->range == 3 ) /*R10K_40_120DegC*/
                DisplayTemperature(0, Temperature_AmbientDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_C);
            // disp_str_16_24(FORM15X30, HUMIDITY_INDOOR_XPOS, HUMIDITY_INDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        }
#if TEST_USE_SAME_VALUE_FOR_AMBIENT
        Temperature_OutdorrDataPtr.pin->value = -Temperature_AmbientDataPtr.pin->value; // For testing only, remove this line in actual code
        Temperature_OutdorrDataPtr.pin->range = Temperature_AmbientDataPtr.pin->range; // For testing only, remove this line in actual code
#endif
        if(!HomeScreenSetpointMode && Temperature_OutValue != Temperature_OutdorrDataPtr.pin->value)
        {
            Temperature_OutValue = Temperature_OutdorrDataPtr.pin->value;
            if(Temperature_OutdorrDataPtr.pin->range == 4 ) /* R10K_40_250DegF*/
                DisplayTemperature(1, Temperature_OutdorrDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_F);
            else if(Temperature_OutdorrDataPtr.pin->range == 3 ) /*R10K_40_120DegC*/
                DisplayTemperature(1, Temperature_OutdorrDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_C);
            // disp_str_16_24(FORM15X30, HUMIDITY_OUTDOOR_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        }

#if TEST_USE_SAME_VALUE_FOR_AMBIENT
        Humidity_OutdorrDataPtr.pin->value = 55; // For testing only, remove this line in actual code
        Humidity_AmbientDataPtr.pin->value = 45; // For testing only, remove this line in actual code
#endif

        if(Humidity_AmbientValue != Humidity_AmbientDataPtr.pin->value)
        {
            Humidity_AmbientValue = Humidity_AmbientDataPtr.pin->value;
            DisplayHumidity(0, Humidity_AmbientDataPtr.pin->value ); // Display indoor humidity at position 0
        }
        if(!HomeScreenSetpointMode && Humidity_OutValue != Humidity_OutdorrDataPtr.pin->value)
        {
            Humidity_OutValue = Humidity_OutdorrDataPtr.pin->value;
            DisplayHumidity(1, Humidity_OutdorrDataPtr.pin->value ); // Display outdoor humidity at position 1
        }
        if(HomeScreenSetpointMode)
        {
            DisplaySetpointValue();
        }
        display_icon();
        display_fan();
    }
}

void DisDraw_Square(uint16 xPos , uint16 yPos )
{
    uint8_t boxHeight = 0;
    if(IsOutdoorTempValid)
        boxHeight = BOX_HEIGHT;
    else
        boxHeight = BOX_HEIGHT + 50; // If outdoor temperature is not valid, make the box taller to accommodate "N/A" text
    // LEFT-UP CORNER  (top-left)
    disp_null_icon(1,1,0, xPos + 3, yPos + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 2, yPos + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 1, yPos + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 0, yPos + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    // RIGHT-UP CORNER  (top-right)
    disp_null_icon(1,1,0, xPos + 222 + 0, yPos + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 222 + 1, yPos + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 222 + 2, yPos + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 222 + 3, yPos + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    // LEFT-DOWN CORNER  (bottom-left)
    int yLD = yPos + boxHeight - 6;

    disp_null_icon(1,1,0, xPos + 3, yLD + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 2, yLD + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 1, yLD + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 0, yLD + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    // RIGHT-DOWN CORNER  (bottom-right)
    int xRD = xPos + 222;
    int yRD = yPos + boxHeight - 6;

    disp_null_icon(1,1,0, xRD + 0, yRD + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 1, yRD + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 2, yRD + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 3, yRD + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    disp_null_icon(219, 1, 0, xPos + 3, yPos + boxHeight - 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(219, 1, 0, xPos + 3, yPos, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1, boxHeight - 10, 0, xPos, yPos + 4, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1, boxHeight - 10, 0, xPos + 226, yPos + 4, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
}

static void DisplayDrawFormat( void )
{

    if(HomeScreenSetpointMode)
    {
        // Keep indoor data in the lower box and use the upper area for setpoint
        DisDraw_Square( START_XPOS_2 , START_YPOS_2 );
        disp_str_16_24(FORM15X30, INDOOR_STR_XPOS,  INDOOR_STR_YPOS,  (uint8 *)"Inside",SCH_COLOR,TSTAT8_BACK_COLOR);

        disp_edge(ICON_XDOTS, ICON_YDOTS, indoorTemp, INDOOR_ICON_XPOS,	INDOOR_ICON_YPOS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

        disp_str_16_24(FORM15X30, HUMIDITY_INDOOR_XPOS, HUMIDITY_INDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_INDOOR_YPOS, (uint8_t*)" 0 ", SCH_COLOR, TSTAT8_BACK_COLOR);

        // Setpoint label replaces the outdoor title
        disp_str(FORM15X30, HOME_SETPOINT_STR_XPOS, HOME_SETPOINT_STR_YPOS, (char *)"Setpoint", SCH_COLOR, TSTAT8_BACK_COLOR);
        draw_tangle( 57 , 112 - 30 );
    }
    else if(IsOutdoorTempValid)
    {
        // disp_str_16_24(FORM15X30, WEATHER_STR_XPOS,  WEATHER_STR_YPOS,  (uint8 *)"Weather",SCH_COLOR,TSTAT8_BACK_COLOR);
        DisDraw_Square( START_XPOS , START_YPOS );
        disp_str_16_24(FORM15X30, INDOOR_STR_XPOS,  INDOOR_STR_YPOS,  (uint8 *)"Inside",SCH_COLOR,TSTAT8_BACK_COLOR);

        disp_edge(ICON_XDOTS, ICON_YDOTS, indoorTemp, INDOOR_ICON_XPOS,	INDOOR_ICON_YPOS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

        disp_str_16_24(FORM15X30, HUMIDITY_INDOOR_XPOS, HUMIDITY_INDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_INDOOR_YPOS, (uint8_t*)" 0 ", SCH_COLOR, TSTAT8_BACK_COLOR);

        DisDraw_Square( START_XPOS_2 , START_YPOS_2 );
        disp_str_16_24(FORM15X30, OUTDOOR_STR_XPOS,  OUTDOOR_STR_YPOS, (uint8 *)"Outside",SCH_COLOR,TSTAT8_BACK_COLOR);

        disp_edge(ICON_XDOTS, ICON_YDOTS, outsideSymbol, OUT_ICON_XPOS,	OUT_ICON_YPOS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

        disp_str_16_24(FORM15X30, HUMIDITY_OUTDOOR_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8_t*)" 0 ", SCH_COLOR, TSTAT8_BACK_COLOR);
    }
    else
    {
        // disp_str_16_24(FORM15X30, WEATHER_STR_XPOS,  WEATHER_STR_YPOS,  (uint8 *)"Weather",SCH_COLOR,TSTAT8_BACK_COLOR);
        DisDraw_Square( START_XPOS , START_YPOS + 50 ); // Move the box down to make large square
        disp_str_16_24(FORM15X30, INDOOR_STR_XPOS,  INDOOR_STR_YPOS - 100,  (uint8 *)"Inside",SCH_COLOR,TSTAT8_BACK_COLOR);

        disp_edge(ICON_XDOTS, ICON_YDOTS, indoorTemp, INDOOR_ICON_XPOS,	INDOOR_ICON_YPOS - 100,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

        disp_str_16_24(FORM15X30, HUMIDITY_INDOOR_XPOS, HUMIDITY_INDOOR_YPOS - 60, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_INDOOR_YPOS - 60, (uint8_t*)" 0 ", SCH_COLOR, TSTAT8_BACK_COLOR);
    }
}

void DisplayTemperature( uint8_t index , S16_T value, uint8 unit)
{
    int16 value_buf;
    int8_t curr_sign;
    uint16_t xPos, yPos;
    uint16_t xSymPos, ySymPos;
    uint16_t yUnitPos, xUnitPos;
    uint16_t xDecPos, yDecPos;

    if(index > 1)
        return; // Invalid index, only 0 (indoor), 1 (outdoor)

    if(index == 1) // outdoor temperature
    {
        if(HomeScreenSetpointMode)
            return; // Outdoor box is hidden in setpoint mode
        if(!IsOutdoorTempValid)
            return; // Don't display outdoor temperature if it's not valid
        xPos = TEMP_OUTDOOR_XPOS;
        yPos = TEMP_OUTDOOR_YPOS;
    }
    else
    {
        xPos = TEMP_INDOOR_XPOS;
        if(!IsOutdoorTempValid && !HomeScreenSetpointMode)
            yPos = TEMP_INDOOR_YPOS - 80;
        else
            yPos = TEMP_INDOOR_YPOS;
    }

    xSymPos = xPos - 27;
    ySymPos = yPos ;
    curr_sign = (value < 0) ? 1 : 0;

    if(unit == TOP_AREA_DISP_UNIT_C || unit == TOP_AREA_DISP_UNIT_F)
    {
        value_buf = value;
        if(Prev_TempSign[index] != curr_sign)
        {
            disp_ch(FORM48X64, xSymPos,  ySymPos, (curr_sign == 0) ? ' ' : '-', SCH_COLOR, TSTAT8_BACK_COLOR);
            Prev_TempSign[index] = curr_sign;
        }
        if(value < 0)
            value_buf = -value;
        if(value >= 1000)
        {
            value_buf /= 10;
        }
        if((value_buf >= 100))
        {
            disp_ch(FORM48X64,xPos,yPos ,0x30+value_buf/100,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
        }
        else
        {
            disp_ch(FORM48X64,xPos, yPos,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
        }
        xPos = xPos + 46;  // Second position
        disp_ch(FORM48X64,xPos,  yPos,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
        xDecPos = xPos + 42 + 6; // Decimal position
        yDecPos = yPos + 75 - 22;
        xPos = xPos + 46 + 10; // Third position
        disp_ch(FORM48X64,xPos , yPos,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);

        if(value > 1000)
            disp_null_icon(6, 6,0, xDecPos, yDecPos,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
        else//hide dec
            disp_null_icon(6, 6,0, xDecPos, yDecPos,TSTAT8_CH_COLOR, TSTAT8_CH_COLOR);

        xUnitPos = xPos + 46 + 15;
        yUnitPos = yPos;

        disp_edge(14, 14, degree_o, xUnitPos - 14,yUnitPos ,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

        if(unit == TOP_AREA_DISP_UNIT_C)
        {
            disp_str(FORM15X30, xUnitPos, yUnitPos, "C",TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
        }
        else if(unit == TOP_AREA_DISP_UNIT_F)
        {
            disp_str(FORM15X30, xUnitPos, yUnitPos, "F", TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);
        }
    }
}

void DisplayHumidity( uint8_t index , S16_T value )
{
    int16 value_buf;
    uint16_t xPos, yPos;

    xPos = HUMIDITY_VALUE_XPOS;

    if(index == 1)
    {
        if(HomeScreenSetpointMode || !IsOutdoorTempValid)
            return; // Outdoor box is hidden or invalid
        yPos = HUMIDITY_OUTDOOR_YPOS;
    }
    else            // indoor Humidity
    {
        if(!IsOutdoorTempValid && !HomeScreenSetpointMode) // If outdoor temp is not valid and we're not in setpoint mode, move indoor humidity up to fill the space
            yPos = HUMIDITY_INDOOR_YPOS - 60;
        else
            yPos = HUMIDITY_INDOOR_YPOS;
    }

    value_buf = value;
    if(value < 0)
    {
        value_buf = -value; // Humidity can't be negative, but just in case
    }
    if(value >= 1000) // Max 100%
    {
        value_buf /= 100;
    }
    if((value > 100))
    {
        value_buf /= 10;
    }

    disp_str_16_24(FORM16X24, xPos, yPos, (uint8_t*)"    ", SCH_COLOR, TSTAT8_BACK_COLOR);

    // First position
    disp_ch_16_24(FORM15X30,xPos,  yPos,0x30+(value_buf%100)/10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);

    xPos = xPos + 14; // Second position
    disp_ch_16_24(FORM15X30,xPos , yPos,0x30+value_buf%10,TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);

    xPos = xPos + 16; // Symbol position
    disp_str_16_24(FORM15X30, xPos, yPos, (uint8_t*)"%", SCH_COLOR, TSTAT8_BACK_COLOR);
}

static void DisplaySetpointValue(void)
{
    Str_points_ptr setpointPtr = put_io_buf(VAR, 0);
    // Label at the same spot as idle screen
    // disp_str(FORM15X30, SCH_XPOS, SETPOINT_POS - 20, (char*)setpointPtr.pvar->label, SCH_COLOR, TSTAT8_BACK_COLOR);
    // Value with the same formatter used by idle screen
    display_screen_value(1);
    // draw_tangle( 52 , 112 - 30 );
}

/* End of File */
