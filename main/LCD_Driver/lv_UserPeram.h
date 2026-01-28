/**
 * @file  lv_UserPeram.h
 * @brief User Parameters Header for LCD Screens.
 * Detailed description:
 * - This header file contains user-defined parameters and configurations for various LCD screens.
 *
 * @author  Bhavik Panchal
 * @date    07-01-2026
 * @version 1.0
 *
 */

#ifndef LV_USER_PERAM_H
#define LV_USER_PERAM_H

#include "lvgl.h"
#include "TemcoScreen/ui.h"
#include "TemcoScreen/ui_helpers.h"
#include "TemcoScreen/ui_StartUpScreen.h"
#include "TemcoScreen/ui_HomeScreen.h"
#include "TemcoScreen/ui_MainMenu.h"

#define UI_OBJ_READY(obj)   ((obj) != NULL && lv_obj_is_valid(obj))

void ui_update_temperature(float temp);
void ui_update_humidity(uint8_t humidity);
void ui_update_time(const char *time_str);
void ui_update_setpoint_arc(uint8_t setpoint);
void ui_set_temperature_unit(bool is_fahrenheit);
void ui_set_wifi_visible(bool visible);

#endif  // LV_USER_PERAM_H