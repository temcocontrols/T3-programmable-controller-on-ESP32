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
#include "LcdTheme.h"
#include "lcd.h"
#include "LCD_TSTAT.h"
#include "driver/uart.h"
#include "Menu.h"
#include "controls.h"
#include "ud_str.h"

#define TEST_USE_SAME_VALUE_FOR_AMBIENT 0

#define START_XPOS          8
#define START_YPOS          18

#define BOX_HEIGHT          150

// #define WEATHER_STR_XPOS    70
// #define WEATHER_STR_YPOS    6

#define START_XPOS_2        START_XPOS
#define START_YPOS_2        START_YPOS + BOX_HEIGHT + 3

// Icon Menu position
#define MENU_DOT_XSIZE      6
#define MENU_DOT_YSIZE      6
#define MENU_DOT_XPOS       10
#define MENU_DOT_XPOS_1     MENU_DOT_XPOS
#define MENU_DOT_XPOS_2     MENU_DOT_XPOS + 10
#define MENU_DOT_XPOS_3     MENU_DOT_XPOS + 20
#define MENU_DOT_YPOS       8

#define HUMIDITY_STR        "HUM"
#define HUMIDITY_STR_LEN     4

#define OUTDOOR_BOX_YPOS  START_YPOS
#define INDOOR_BOX_YPOS   START_YPOS_2

// Outdoor temperature position
#define TEMP_OUTDOOR_XPOS   40
#define TEMP_OUTDOOR_YPOS   OUTDOOR_BOX_YPOS + 48

#define HUMIDITY_OUTDOOR_XPOS  30
#define HUMIDITY_OUTDOOR_YPOS  OUTDOOR_BOX_YPOS + BOX_HEIGHT - 40

#define OUTDOOR_STR_XPOS  120
#define OUTDOOR_STR_YPOS  OUTDOOR_BOX_YPOS + 5

#define OUT_ICON_XPOS     START_XPOS + 2
#define OUT_ICON_YPOS     OUTDOOR_BOX_YPOS + 2

// Indoor temperature position
#define TEMP_INDOOR_XPOS    TEMP_OUTDOOR_XPOS
#define TEMP_INDOOR_YPOS    INDOOR_BOX_YPOS + 45

#define HUMIDITY_INDOOR_XPOS  HUMIDITY_OUTDOOR_XPOS
#define HUMIDITY_INDOOR_YPOS  INDOOR_BOX_YPOS + BOX_HEIGHT - 40

#define INDOOR_STR_XPOS  OUTDOOR_STR_XPOS
#define INDOOR_STR_YPOS  INDOOR_BOX_YPOS + 5

#define INDOOR_ICON_XPOS  START_XPOS + 4
#define INDOOR_ICON_YPOS  INDOOR_BOX_YPOS + 2

// Humidity value X position
#define HUMIDITY_VALUE_XPOS   HUMIDITY_INDOOR_XPOS + (HUMIDITY_STR_LEN * 12) + 4

static void DisplayDrawFormat( void );
void DisplayTemperature( uint8_t index , S16_T value, uint8 unit);
void DisplayHumidity( uint8_t index , S16_T value );

Str_points_ptr Temperature_OutdorrDataPtr;
Str_points_ptr Temperature_AmbientDataPtr;
Str_points_ptr Humidity_OutdorrDataPtr;
Str_points_ptr Humidity_AmbientDataPtr;

int32_t Temperature_OutValue     = 0;
int32_t Temperature_AmbientValue = 0;
int32_t Humidity_OutValue        = 0;
int32_t Humidity_AmbientValue    = 0;

void DisplayHomeScreen( bool isHomeScreen )
{
    Temperature_OutdorrDataPtr = put_io_buf(IN, 8); // VAR9 is for room temperature
    Temperature_AmbientDataPtr = put_io_buf(IN, 9); // VAR10 is for outdoor temperature
    Humidity_OutdorrDataPtr    = put_io_buf(IN, 10); // VAR11 is for room humidity
    Humidity_AmbientDataPtr    = put_io_buf(IN, 11); // VAR12 is for outdoor humidity

    if(!isHomeScreen)
    {
        ClearScreen(TSTAT8_BACK_COLOR);
        DisplayDrawFormat();
        Temperature_OutValue     = 0;
        Temperature_AmbientValue = 0;
        Humidity_OutValue        = 0;
        Humidity_AmbientValue    = 0;
    }
    else
    {
        if(Temperature_OutValue != Temperature_OutdorrDataPtr.pin->value)
        {
            Temperature_OutValue = Temperature_OutdorrDataPtr.pin->value;
            if(Temperature_OutdorrDataPtr.pin->range == 4 ) /* R10K_40_250DegF*/
                DisplayTemperature(1, Temperature_OutdorrDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_F);
            else if(Temperature_OutdorrDataPtr.pin->range == 3 ) /*R10K_40_120DegC*/
                DisplayTemperature(1, Temperature_OutdorrDataPtr.pin->value / 100, TOP_AREA_DISP_UNIT_C);
            // disp_str_16_24(FORM15X30, HUMIDITY_OUTDOOR_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
        }
#if TEST_USE_SAME_VALUE_FOR_AMBIENT
        Temperature_AmbientDataPtr.pin->value = -Temperature_OutdorrDataPtr.pin->value; // For testing only, remove this line in actual code
        Temperature_AmbientDataPtr.pin->range = Temperature_OutdorrDataPtr.pin->range; // For testing only, remove this line in actual code
#endif
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
        Humidity_OutdorrDataPtr.pin->value = 55; // For testing only, remove this line in actual code
        Humidity_AmbientDataPtr.pin->value = 45; // For testing only, remove this line in actual code
#endif
        if(Humidity_OutValue != Humidity_OutdorrDataPtr.pin->value)
        {
            Humidity_OutValue = Humidity_OutdorrDataPtr.pin->value;
            DisplayHumidity(1, Humidity_OutdorrDataPtr.pin->value ); // Display indoor humidity at position 1
        }
        if(Humidity_AmbientValue != Humidity_AmbientDataPtr.pin->value)
        {
            Humidity_AmbientValue = Humidity_AmbientDataPtr.pin->value;
            DisplayHumidity(0, Humidity_AmbientDataPtr.pin->value ); // Display outdoor humidity at position 0
        }
    }
}

void DisDraw_Square(uint16 xPos , uint16 yPos )
{

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
    int yLD = yPos + BOX_HEIGHT - 6;

    disp_null_icon(1,1,0, xPos + 3, yLD + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 2, yLD + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 1, yLD + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xPos + 0, yLD + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    // RIGHT-DOWN CORNER  (bottom-right)
    int xRD = xPos + 222;
    int yRD = yPos + BOX_HEIGHT - 6;

    disp_null_icon(1,1,0, xRD + 0, yRD + 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 1, yRD + 2, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 2, yRD + 1, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1,1,0, xRD + 3, yRD + 0, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);

    disp_null_icon(219, 1, 0, xPos + 3, yPos + BOX_HEIGHT - 3, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(219, 1, 0, xPos + 3, yPos, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1, BOX_HEIGHT - 10, 0, xPos, yPos + 4, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
    disp_null_icon(1, BOX_HEIGHT - 10, 0, xPos + 226, yPos + 4, TSTAT8_CH_COLOR, TSTAT8_MENU_COLOR);
}

static void DisplayDrawFormat( void )
{

    // disp_str_16_24(FORM15X30, WEATHER_STR_XPOS,  WEATHER_STR_YPOS,  (uint8 *)"Weather",SCH_COLOR,TSTAT8_BACK_COLOR);
    DisDraw_Square( START_XPOS , START_YPOS );
    disp_str_16_24(FORM15X30, INDOOR_STR_XPOS,  INDOOR_STR_YPOS,  (uint8 *)"Inside",SCH_COLOR,TSTAT8_BACK_COLOR);

    disp_edge(ICON_XDOTS, ICON_YDOTS, outsideSymbol, OUT_ICON_XPOS,	OUT_ICON_YPOS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

    disp_str_16_24(FORM15X30, HUMIDITY_INDOOR_XPOS, HUMIDITY_INDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
    disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_INDOOR_YPOS, (uint8_t*)"-NA", SCH_COLOR, TSTAT8_BACK_COLOR);

    DisDraw_Square( START_XPOS_2 , START_YPOS_2 );
    disp_str_16_24(FORM15X30, OUTDOOR_STR_XPOS,  OUTDOOR_STR_YPOS, (uint8 *)"Outside",SCH_COLOR,TSTAT8_BACK_COLOR);

    disp_edge(ICON_XDOTS, ICON_YDOTS, indoorTemp, INDOOR_ICON_XPOS,	INDOOR_ICON_YPOS,TSTAT8_CH_COLOR, TSTAT8_BACK_COLOR);

    disp_str_16_24(FORM15X30, HUMIDITY_OUTDOOR_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8 *)HUMIDITY_STR, SCH_COLOR, TSTAT8_BACK_COLOR);
    disp_str_16_24(FORM15X30, HUMIDITY_VALUE_XPOS, HUMIDITY_OUTDOOR_YPOS, (uint8_t*)"-NA", SCH_COLOR, TSTAT8_BACK_COLOR);

    // Menu Dots
    disp_null_icon(MENU_DOT_XSIZE, MENU_DOT_YSIZE, 0, MENU_DOT_XPOS_1, MENU_DOT_YPOS, SCH_COLOR, SCH_COLOR);
    disp_null_icon(MENU_DOT_XSIZE, MENU_DOT_YSIZE, 0, MENU_DOT_XPOS_2, MENU_DOT_YPOS, SCH_COLOR, SCH_COLOR);
    disp_null_icon(MENU_DOT_XSIZE, MENU_DOT_YSIZE, 0, MENU_DOT_XPOS_3, MENU_DOT_YPOS, SCH_COLOR, SCH_COLOR);
}

void DisplayTemperature( uint8_t index , S16_T value, uint8 unit)
{
    int16 value_buf;
    uint16_t xPos, yPos;
    uint16_t xSymPos, ySymPos;
    uint16_t yUnitPos, xUnitPos;
    uint16_t xDecPos, yDecPos;

    if(index == 1) // outdoor temperature
    {
        xPos = TEMP_OUTDOOR_XPOS;
        yPos = TEMP_OUTDOOR_YPOS;
    }
    else // indoor temperature
    {
        xPos = TEMP_INDOOR_XPOS;
        yPos = TEMP_INDOOR_YPOS;
    }

    xSymPos = xPos - 27;
    ySymPos = yPos ;

    if(unit == TOP_AREA_DISP_UNIT_C || unit == TOP_AREA_DISP_UNIT_F)
    {
        value_buf = value;
        if(value >=0)
        {
            disp_ch(FORM48X64, xSymPos,  ySymPos, ' ',SCH_COLOR,TSTAT8_BACK_COLOR);
        }
        else
        {
            value_buf = -value;
            disp_ch(FORM48X64, xSymPos,  ySymPos, '-',SCH_COLOR,TSTAT8_BACK_COLOR);
        }
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

    if(index == 1) // outdoor temperature
    {
        xPos = HUMIDITY_VALUE_XPOS;
        yPos = HUMIDITY_OUTDOOR_YPOS;
    }
    else // indoor temperature
    {
        xPos = HUMIDITY_VALUE_XPOS;
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
