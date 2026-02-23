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

#define UI_DATA_UPDATE_INTERVAL_MS              500

#define UI_OBJ_READY(obj)   ((obj) != NULL && lv_obj_is_valid(obj))

void lv_Init_UserParameters( void );
void lv_Lcd_UpdateData( void );

void Event_Cb_SetSetpointValue(lv_event_t * e);
void Event_Cb_WifiEn(lv_event_t * e);
void Event_Cb_WifiIpAutoSelect(lv_event_t * e);
void Event_Cb_KeyPress(lv_event_t * e);
void Event_Cb_FanSetAutoMode(lv_event_t * e);
void Event_Cb_CirculateBtnClicked(lv_event_t * e);
void Event_Cb_StartFanBtn(lv_event_t * e);
void Event_Cb_SysModeHeatFunc(lv_event_t * e);
void Event_Cb_AutoModeBtnFunc(lv_event_t * e);
void Event_Cb_ModeOffBtnEventFunc(lv_event_t * e);
void Event_Cb_SysModeCoolFunc(lv_event_t * e);
void Event_Cb_UpdateWifiConfig(lv_event_t * e);
void Event_Cb_WifiKeyboardPressFunc(lv_event_t * e);
void Event_Cb_NetworkConfigKeyPressFunc(lv_event_t * e);
void Event_Cb_ScheduleTimeSelectedFunc(lv_event_t * e);
void Event_Cb_ScheduleTimeUpdateCallback(lv_event_t * e);
void Event_Cb_ChangeTemperatureTypeCallBack(lv_event_t * e);
void Event_Cb_ParamInputShowCallBackFunc(lv_event_t * e);
void Event_Cb_ParamOutputShowCallBackFunc(lv_event_t * e);
void Event_Cb_ParamVariableShowCallBackFunc(lv_event_t * e);
void Event_Cb_SSIDShowEventFunc(lv_event_t * e);
void Event_Cb_IpAutoNext(lv_event_t * e);
void Event_Cb_UpdateParameterTableFunc(lv_event_t * e);
void Event_Cb_SysTimeUpdateCallback(lv_event_t * e);
void Event_Cb_CalenderValueChangeCallback(lv_event_t * e);
void Event_Cb_NetworkConfigUpdateFunc(lv_event_t * e);
void Event_Cb_UpdateProtocolFunc(lv_event_t * e);
void Event_Cb_RefreshTimeFunc(lv_event_t * e);
void Event_Cb_TimeSyncLocalPcFunc(lv_event_t * e);
void Event_Cb_TimeSyncUpdateFunc(lv_event_t * e);
void Event_Cb_TimeSyncServerUpdateFunc(lv_event_t * e);
void Event_Cb_ParameterUpdateFunc(lv_event_t * e);
void Event_Cb_ScheduleAutoManualValChangeFun(lv_event_t * e);
void Event_Cb_ScheduleSwithValueChangeFunc(lv_event_t * e);
void Event_Cb_ScheduleSetupUpdateBtnFunc(lv_event_t * e);
void Event_Cb_ScheduleKeyboardPressFunc(lv_event_t * e);
void Event_Cb_SchSaveBtnFunc(lv_event_t * e);
void Event_Cb_SchClearAllFunc(lv_event_t * e);
void Event_Cb_SchCopyAllFunc(lv_event_t * e);

#endif  // LV_USER_PERAM_H