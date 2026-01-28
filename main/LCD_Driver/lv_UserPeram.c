/**
 * @file  lv_UserPeram.c
 * @brief User Parameters Implementation for LCD Screens
 * Detailed description:
 * - This module implements user-defined parameters and configurations for various LCD screens.
 *
 * @author  Bhavik Panchal
 * @date    07-01-2026
 * @version 1.0
 *
 */



 #include "lv_UserPeram.h"
 #include "stdio.h"
 #include "string.h"

 void ui_update_temperature(float temp)
{
    if(!UI_OBJ_READY(uic_TemperatureVal))
        return;

    char buf[8];
    snprintf(buf, sizeof(buf), "%.1f", temp);

    lv_textarea_set_text(uic_TemperatureVal, buf);
}

void ui_update_humidity(uint8_t humidity)
{
    if(!UI_OBJ_READY(ui_TextArea4))
        return;

    if(humidity > 100)
        humidity = 100;

    char buf[6];
    snprintf(buf, sizeof(buf), "%d%%", humidity);

    lv_textarea_set_text(ui_TextArea4, buf);
}

void ui_update_time(const char *time_str)
{
    if(!UI_OBJ_READY(uic_RunTime))
        return;

    if(time_str == NULL)
        return;

    lv_textarea_set_text(uic_RunTime, time_str);
}

void ui_update_setpoint_arc(uint8_t setpoint)
{
    if(!UI_OBJ_READY(ui_TempSetPoint1) ||
       !UI_OBJ_READY(ui_TempSetPoint2) ||
       !UI_OBJ_READY(uic_TemperatureSetPoint))
    {
        return;
    }

    /* Clamp to valid range */
    if(setpoint > 50)
        setpoint = 50;

    /* Update arcs */
    lv_arc_set_value(ui_TempSetPoint1, setpoint);
    lv_arc_set_value(ui_TempSetPoint2, setpoint);

    /* Update text */
    char buf[10];
    snprintf(buf, sizeof(buf), "%d °C", setpoint);
    lv_textarea_set_text(uic_TemperatureSetPoint, buf);
}

void ui_set_temperature_unit(bool is_fahrenheit)
{
    if(!UI_OBJ_READY(uic_TempratureSymbol))
        return;

    lv_textarea_set_text(uic_TempratureSymbol,
                         is_fahrenheit ? "°F" : "°C");
}

void ui_set_wifi_visible(bool visible)
{
    if(!UI_OBJ_READY(uic_WifiSymb))
        return;

    lv_obj_set_style_opa(uic_WifiSymb,
                         visible ? 255 : 0,
                         LV_PART_MAIN | LV_STATE_DEFAULT);
}