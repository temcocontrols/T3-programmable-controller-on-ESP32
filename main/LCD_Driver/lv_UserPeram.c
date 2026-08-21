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

#include "sdkconfig.h"
#include "lv_UserPeram.h"
#include "esp_wifi.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "wifi.h"
#include "flash.h"
#include "esp_log.h"
#include "define.h"
#include "ud_str.h"
#include "controls.h"
#include "rtc.h"
#include "modbus.h"
#include "user_data.h"
#include "sntp_app.h"


#include "rtc.h"

#define TAG "lv_User"

#define WIFI_SCAN_LIST_MAX   768
#define PARAM_TABLE_BUILD_BATCH  10

/* wifi scan: background task fills results; UI thread applies on screen refresh */
typedef enum {
    WIFI_SCAN_UI_IDLE = 0,
    WIFI_SCAN_UI_PENDING,
    WIFI_SCAN_UI_READY,
    WIFI_SCAN_UI_FAILED
} wifi_scan_ui_state_t;

/* Global variables for point data and cached values to track changes and update UI accordingly */
Str_points_ptr Temperature_IndorrDataPt;
Str_points_ptr Temperature_AmbientDataPt;
Str_points_ptr Humidity_IutdorrDataPt;
Str_points_ptr Humidity_AmbientDataPt;
Str_points_ptr Temperature_SetpointDataPt;
Str_points_ptr FanModePt;
Str_points_ptr SysModePt;

/* Cache for last known time to avoid unnecessary UI updates */
static int8_t last_hour   = -1;
static int8_t last_minute = -1;

/* Forward declarations of static helper functions */
int32_t Temperature_InVal     = 0;
int32_t Humidity_InVal        = 0;
int32_t Temperature_SetpointVal= 0;
static bool s_show_outdoor_temperature = false;

int32_t FanModeVal = 0;
uint8_t FanMode_On_Val = 0; // 1 hour, 2 hours, 4 hours, 8 hours , untill i turn off fan
int32_t SysModeVal = 0;

/* Wifi screen related variables */
static wifi_scan_ui_state_t wifi_scan_ui_state = WIFI_SCAN_UI_IDLE;
static TaskHandle_t s_wifi_scan_task = NULL;
static char s_wifi_scan_list[WIFI_SCAN_LIST_MAX];
static bool isScreenChanged = false;

static bool isTimeUpdated = false;
static bool isDateUpdated = false;
lv_calendar_date_t selected_date;
static lv_obj_t *selected_schedule_time_cell = NULL;
static lv_obj_t * s_lv_table = NULL;

typedef enum
{
    PARAM_TABLE_INPUT = 0,
    PARAM_TABLE_OUTPUT,
    PARAM_TABLE_VARIABLE
} param_table_type_t;

static param_table_type_t s_param_table_type = PARAM_TABLE_INPUT;
static lv_coord_t s_param_table_widths[9] = { 40, 160, 70, 70, 60, 60, 45, 45, 90 };
static bool s_param_build_active = false;
static uint16_t s_param_build_row = 0;
static uint16_t s_param_build_total = 0;

static void param_table_cell_edit_cb(lv_event_t * e);
/* Forward declarations of static helper functions to refresh specific screen data */
static void lv_refresh_HomeScreen_Data(void);
static void lv_refresh_WifiConfig_Data(void);
static void lv_refresh_NetworkSetup_Data(void);
static void lv_refresh_Protocols_Data(void);
static void lv_refresh_ScheduleScreen_Data(void);
static void lv_refresh_ScheduleEditScreen_Data(void);
static void lv_refresh_Time_Data(void);
static void lv_refresh_Parameters_Data(void);
static void lv_refresh_calender_Data(void);

/* Forward declarations of static helper functions to update specific UI components */
static void ui_update_temperature(float temp);
static void ui_update_humidity(uint8_t humidity);
static void ui_update_time(const char *time_str);
static void ui_update_setpoint_arc(uint8_t setpoint);
static void ui_set_temperature_unit(bool is_fahrenheit);
static const lv_image_dsc_t * ui_wifi_symbol_for_status(void);
static void ui_update_wifi_symbol(void);

static void ui_update_textarea_from_int(lv_obj_t * obj, uint8_t value);
static uint8_t ui_get_int_from_textarea(lv_obj_t * obj);
static uint16_t param_table_get_row_count(void);
static uint16_t param_table_calc_total_width(bool is_output);
static void param_table_fill_row(uint16_t i);
static void param_table_build_begin(void);
static void param_table_build_step(void);
static void param_table_build(void);
static void param_Clear_table(void);
static void param_table_apply_updates(void);
static void param_table_copy_text(char *dest, uint16_t dest_size, const char *src);
static const char *param_table_range_text(param_table_type_t type, uint8_t digital_analog,
                                          uint8_t range);
static bool param_table_range_is_valid(param_table_type_t type, uint8_t digital_analog,
                                       uint8_t range);

extern void Sync_timestamp(S16_T newTZ,S16_T oldTZ,S8_T newDLS,S8_T oldDLS);

int16_t tz_offset_table[] =
{
    -1200, -1100, -1000, -900,
    -800, -700, -600, -500,
    -400, -300, -200, -100,
    0,
    100, 200, 300, 350,
    400, 450, 500, 550, 575,
    600, 650, 700, 800,
    900, 950, 1000, 1100, 1200
};
/**
 * @brief Initializes user parameters by linking them to the appropriate data points
 * @details This function retrieves pointers to the relevant data points for temperature, humidity, and setpoint values and assigns them to global variables for later use in UI updates.
 * @param[in] void No parameters
 * @return void
 * @note This function should be called during the initialization phase of the LCD task to ensure that the data points are properly linked before any UI updates occur.
 */
void lv_Init_UserParameters( void )
{
    Temperature_IndorrDataPt = put_io_buf(IN, 8); // VAR9 is for room temperature
    Temperature_AmbientDataPt = put_io_buf(IN, 9); // VAR10 is for outdoor temperature
    Humidity_IutdorrDataPt = put_io_buf(IN, 10); // VAR11 is for room humidity
    Humidity_AmbientDataPt = put_io_buf(IN, 11); // VAR12 is for outdoor humidity
    Temperature_SetpointDataPt = put_io_buf(VAR,0);
    FanModePt = put_io_buf(VAR, 2);
    SysModePt = put_io_buf(VAR, 1);
    FanMode_On_Val = 0; // Default to 1 hour // TODO: Need to read actual value from flash or data point if persisted
}

/**
 * @brief Updates the LCD screen time by checking the RTC for changes and refreshing the time display if needed
 * @details This function compares the current time from the RTC with cached values. If a change in hour or minute is detected, it formats the time string and updates the corresponding UI component to reflect the new time.
 * @param[in] void No parameters
 * @return void
 * @note This function should be called periodically (e.g., every second) to ensure that the time display on the LCD remains accurate and up-to-date with the RTC.
 */
static void ui_update_time_from_rtc_if_changed(const PCF_DateTime *rtc)
{
    if(rtc == NULL)
        return;

    /* Check change */
    if(rtc->hour == last_hour &&
       rtc->minute == last_minute)
    {
        return;
    }

    /* Update cache */
    last_hour   = rtc->hour;
    last_minute = rtc->minute;

    char time_buf[13];
    if(last_hour >= 12)
    {
        sprintf(time_buf, "%02d:%02d PM",
                 (last_hour == 12) ? 12 : (last_hour - 12),
                 last_minute);
    }
    else
    {
        sprintf(time_buf, "%02d:%02d AM",
                 last_hour,
                 last_minute);
    }

    /* Call existing UI API */
    ui_update_time(time_buf);
}

/**
 * @brief Updates all relevant LCD screen data based on current values from data points
 * @details This function checks the current values of temperature, humidity, and setpoint data points against cached values. If any changes are detected, it calls the appropriate UI update functions to refresh the display with the new data.
 * @param[in] void No parameters
 * @return void
 * @note This function should be called periodically (e.g., every second) to ensure that the LCD screen reflects the most current data from the underlying data points.
 */
void lv_Lcd_UpdateData(void)
{
    static uint32_t last_update_time = 0;

    if((xTaskGetTickCount() - last_update_time) >= UI_DATA_UPDATE_INTERVAL_MS) // Update Data at 100 mS
    {
        last_update_time = xTaskGetTickCount();

        lv_obj_t * current_Screen = lv_screen_active();
        static lv_obj_t * prv_Screen = NULL;

        if(current_Screen != prv_Screen)
        {
            isScreenChanged = true;
            ESP_LOGI(TAG, "Screen changed: %p -> %p", prv_Screen, current_Screen);
            prv_Screen = current_Screen;
        }
        else
        {
            isScreenChanged = false;
        }
        if(current_Screen == ui_HomeScreen)
        {
            lv_refresh_HomeScreen_Data();
        }
        else if(current_Screen == ui_WifiConfig)
        {
            lv_refresh_WifiConfig_Data();
        }
        else if(current_Screen == ui_NetworkConfig)
        {
            lv_refresh_NetworkSetup_Data();
        }
        else if(current_Screen == ui_Protocols)
        {
            lv_refresh_Protocols_Data();
        }
        else if(current_Screen == ui_ScheduleScreen)
        {
            lv_refresh_ScheduleScreen_Data();
        }
        else if(current_Screen == ui_ScheduleEditScreen)
        {
            lv_refresh_ScheduleEditScreen_Data();
        }
        else if(current_Screen == ui_Time)
        {
            lv_refresh_Time_Data();
        }
        else if(current_Screen == ui_Parameters)
        {
            lv_refresh_Parameters_Data();
        }
        else if(current_Screen == ui_HolidayCalenderScreen)
        {
            lv_refresh_calender_Data();
        }
    }
}

/**
 * @brief Refreshes the Home Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data for temperature, humidity, setpoint, and time, and calls the corresponding UI update functions to ensure that the Home Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the Home Screen data, such as after a significant change in underlying data points or when returning to the Home Screen from another screen.
 */

static void lv_refresh_HomeScreen_Data(void)
{
    Str_points_ptr active_temp_pt = s_show_outdoor_temperature ? Temperature_AmbientDataPt : Temperature_IndorrDataPt;
    Str_points_ptr active_humidity_pt = s_show_outdoor_temperature ? Humidity_AmbientDataPt : Humidity_IutdorrDataPt;
    static bool s_last_show_outdoor_temperature = false;

    if((active_temp_pt.pin != NULL) && (Temperature_InVal != active_temp_pt.pin->value))
    {
        Temperature_InVal = active_temp_pt.pin->value;
        ui_update_temperature(active_temp_pt.pin->value / 1000);
        if(active_temp_pt.pin->range == 4 ) /* R10K_40_250DegF*/
            ui_set_temperature_unit(1);
        else
            ui_set_temperature_unit(0);
    }
    if((active_humidity_pt.pin != NULL) && (Humidity_InVal != active_humidity_pt.pin->value))
    {
        Humidity_InVal = active_humidity_pt.pin->value;
        ui_update_humidity(active_humidity_pt.pin->value);
    }

    if(s_last_show_outdoor_temperature != s_show_outdoor_temperature)
    {
        s_last_show_outdoor_temperature = s_show_outdoor_temperature;

        if(UI_OBJ_READY(ui_SPLable))
        {
            lv_label_set_text(ui_SPLable, s_show_outdoor_temperature ? "Outdoor" : "Target : ");
        }

        if(UI_OBJ_READY(ui_TemperatureSetPoint))
        {
            if(s_show_outdoor_temperature)
            {
                lv_obj_add_flag(ui_TemperatureSetPoint, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_clear_flag(ui_TemperatureSetPoint, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    if((!s_show_outdoor_temperature) && (Temperature_SetpointVal != Temperature_SetpointDataPt.pvar->value))
    {
        Temperature_SetpointVal = Temperature_SetpointDataPt.pvar->value;
        ui_update_setpoint_arc(Temperature_SetpointDataPt.pvar->value / 1000);
    }

    /* WiFi symbol: strength + connection state (same rules as DisplayHeaderSymbol). */
    static uint8_t last_wifi_status = 0xFFU;
    static int8_t last_wifi_rssi = 0;

    bool wifi_connected = (SSID_Info.IP_Wifi_Status == WIFI_NORMAL) ||
                          (SSID_Info.IP_Wifi_Status == WIFI_CONNECTED);
    bool wifi_changed = (last_wifi_status != SSID_Info.IP_Wifi_Status) ||
                        (wifi_connected && (last_wifi_rssi != SSID_Info.rssi));

    if(wifi_changed)
    {
        last_wifi_status = SSID_Info.IP_Wifi_Status;
        last_wifi_rssi = SSID_Info.rssi;
        ui_update_wifi_symbol();
    }

    if (UI_OBJ_READY(ui_RunningModeLabel) && SysModePt.pvar != NULL)
    {
        static const char * const running_modes[] = {
            "Off", "Auto", "Heat", "Cool"
        };

        static uint32_t last_mode = UINT32_MAX;

        uint32_t mode = (uint32_t)(SysModePt.pvar->value / 1000);

        if (mode != last_mode)
        {
            char running_text[24];

            lv_snprintf(running_text,
                        sizeof(running_text),
                        "Mode: %s",
                        mode < 4U ? running_modes[mode] : "Unknown");

            lv_label_set_text(ui_RunningModeLabel, running_text);

            last_mode = mode;
        }
    }
    /* Consume one-shot activity counters so a stale startup value cannot leave
       an arrow permanently visible. */

    if(UI_OBJ_READY(ui_RS485ArrowUpImg) && UI_OBJ_READY(ui_RS485ArrowDnImg))
    {
        static bool rx_arrow_visible = false;
        static bool tx_arrow_visible = false;
        if(flagLED_sub_rx)
        {
            lv_obj_clear_flag(ui_RS485ArrowDnImg, LV_OBJ_FLAG_HIDDEN);
            rx_arrow_visible = true;
            flagLED_sub_rx--;
        }
        else if(rx_arrow_visible)
        {
            rx_arrow_visible = false;
            lv_obj_add_flag(ui_RS485ArrowDnImg, LV_OBJ_FLAG_HIDDEN);
        }
        if(flagLED_sub_tx)
        {
            lv_obj_clear_flag(ui_RS485ArrowUpImg, LV_OBJ_FLAG_HIDDEN);
            tx_arrow_visible = true;
            flagLED_sub_tx--;
        }
        else if(tx_arrow_visible)
        {
            tx_arrow_visible = false;
            lv_obj_add_flag(ui_RS485ArrowUpImg, LV_OBJ_FLAG_HIDDEN);
        }
    }

    ui_update_time_from_rtc_if_changed(&rtc_date);
    if(SysModeVal != SysModePt.pvar->value)
    {
        SysModeVal = SysModePt.pvar->value;
        lv_obj_clear_state(ui_OffModeBtn, LV_STATE_FOCUSED);  // Mode 0
        lv_obj_clear_state(ui_AutoModeBtn, LV_STATE_FOCUSED); // Mode 1
        lv_obj_clear_state(ui_HeatModeBtn, LV_STATE_FOCUSED); // Mode 2
        lv_obj_clear_state(ui_CoolModeBtn, LV_STATE_FOCUSED); // Mode 3

        if(SysModePt.pvar->value == 0)
        {
            lv_obj_add_state(ui_OffModeBtn, LV_STATE_FOCUSED);
        }
        else
        {
            switch(SysModePt.pvar->value/1000)
            {
                case 1:
                    lv_obj_add_state(ui_AutoModeBtn, LV_STATE_FOCUSED);
                    break;
                case 2:
                    lv_obj_add_state(ui_HeatModeBtn, LV_STATE_FOCUSED);
                    break;
                case 3:
                    lv_obj_add_state(ui_CoolModeBtn, LV_STATE_FOCUSED);
                    break;
                default:
                    break;
            }
        }
    }

    if(FanModeVal != FanModePt.pvar->value)
    {
        FanModeVal = FanModePt.pvar->value;
        lv_obj_clear_state(ui_AutoBtn, LV_STATE_FOCUSED);      // Mode 0
        lv_obj_clear_state(ui_FanONBtn, LV_STATE_FOCUSED);     // Mode 1
        lv_obj_clear_state(ui_CurculateBtn, LV_STATE_FOCUSED); // Mode 2

        if(FanModePt.pvar->value == 0)
        {
            lv_obj_add_state(ui_AutoBtn, LV_STATE_FOCUSED);
        }
        else
        {
            switch(FanModePt.pvar->value/1000)
            {
                case 1:
                    lv_obj_add_state(ui_FanONBtn, LV_STATE_FOCUSED);
                    lv_roller_set_selected(ui_Roller1, FanMode_On_Val, LV_ANIM_ON);
                    break;
                case 2:
                    lv_obj_add_state(ui_CurculateBtn, LV_STATE_FOCUSED);
                    break;
                default:
                    break;
            }
        }
    }
    ui_update_time_from_rtc_if_changed(&rtc_date);
}

/**
 * @brief Build dropdown SSID list from scan results (dedupe by SSID, keep strongest RSSI)
 */
static void wifi_build_ssid_list(const wifi_ap_record_t *ap_info,
                                 uint16_t ap_count,
                                 char *list,
                                 size_t list_size)
{
    if (list == NULL || list_size == 0U) {
        return;
    }

    list[0] = '\0';

    if (ap_info == NULL || ap_count == 0U) {
        return;
    }

    size_t used = 0U;

    for (uint16_t i = 0U; i < ap_count; i++) {

        char ssid[sizeof(ap_info[i].ssid) + 1U];

        memcpy(ssid, ap_info[i].ssid, sizeof(ap_info[i].ssid));
        ssid[sizeof(ap_info[i].ssid)] = '\0';

        size_t ssid_len = strnlen(ssid, sizeof(ap_info[i].ssid));

        if (ssid_len == 0U) {
            continue;
        }

        ESP_LOGI(TAG,
                 "AP[%u]: SSID='%s', RSSI=%d dBm",
                 i,
                 ssid,
                 ap_info[i].rssi);

        if ((used + ssid_len + 2U) > list_size) {
            ESP_LOGW(TAG, "SSID list buffer full");
            break;
        }

        int written = snprintf(&list[used],
                               list_size - used,
                               "%s\n",
                               ssid);

        if (written < 0) {
            break;
        }

        used += (size_t)written;
    }

    if (used > 0U && list[used - 1U] == '\n') {
        list[used - 1U] = '\0';
    }
}

static void wifi_dropdown_select_saved_ssid(void)
{
    if (!UI_OBJ_READY(ui_Dropdown2) || SSID_Info.name[0] == '\0') {
        return;
    }

    const char *options = lv_dropdown_get_options(ui_Dropdown2);

    if (options == NULL) {
        return;
    }

    uint16_t idx = 0;
    const char *start = options;

    while (*start != '\0') {
        const char *end = strchr(start, '\n');

        size_t len = end ? (size_t)(end - start) : strlen(start);

        if (strlen(SSID_Info.name) == len &&
            strncmp(start, SSID_Info.name, len) == 0) {

            lv_dropdown_set_selected(ui_Dropdown2, idx);
            return;
        }

        idx++;

        if (end == NULL) {
            break;
        }

        start = end + 1;
    }
}
static wifi_ap_record_t wifi_ap_info[WIFI_SCAN_MAX_AP];

extern TaskHandle_t main_task_handle[];

static void wifi_scan_worker(void *arg)
{
    (void)arg;
    uint16_t ap_count = 0;

    ReconnectWithWifi = false;

    if(SSID_Info.IP_Wifi_Status != WIFI_NORMAL) {
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    esp_err_t err = wifi_scan_networks(wifi_ap_info, &ap_count, WIFI_SCAN_MAX_AP);
    if(err != ESP_OK) {
        wifi_scan_ui_state = WIFI_SCAN_UI_FAILED;
    }
    else if(ap_count == 0U) {
        snprintf(s_wifi_scan_list, sizeof(s_wifi_scan_list), "No networks found");
        wifi_scan_ui_state = WIFI_SCAN_UI_READY;
    }
    else {
        wifi_build_ssid_list(wifi_ap_info, ap_count, s_wifi_scan_list, sizeof(s_wifi_scan_list));
        if(s_wifi_scan_list[0] == '\0') {
            snprintf(s_wifi_scan_list, sizeof(s_wifi_scan_list), "No networks found");
        }
        wifi_scan_ui_state = WIFI_SCAN_UI_READY;
    }

    ReconnectWithWifi = true;
    s_wifi_scan_task = NULL;
    vTaskDelete(NULL);
}

static void wifi_scan_request_on_screen_enter(void)
{
    wifi_mode_t mode;

    if(s_wifi_scan_task != NULL) {
        return;
    }

    if(esp_wifi_get_mode(&mode) != ESP_OK || mode == WIFI_MODE_NULL) {
        lv_dropdown_set_options(ui_Dropdown2, "WiFi Disabled");
        wifi_scan_ui_state = WIFI_SCAN_UI_IDLE;
        return;
    }

    s_wifi_scan_list[0] = '\0';
    wifi_scan_ui_state = WIFI_SCAN_UI_PENDING;
    lv_dropdown_set_options(ui_Dropdown2, "Scanning...");

    if(xTaskCreate(wifi_scan_worker, "wifi_scan", 4096, NULL, 3, &s_wifi_scan_task) != pdPASS) {
        s_wifi_scan_task = NULL;
        lv_dropdown_set_options(ui_Dropdown2, "Scan failed");
        wifi_scan_ui_state = WIFI_SCAN_UI_FAILED;
        ReconnectWithWifi = true;
    }
}

static void wifi_scan_apply_results(void)
{
    if(!UI_OBJ_READY(ui_Dropdown2)) {
        return;
    }

    if(wifi_scan_ui_state == WIFI_SCAN_UI_READY) {
        lv_dropdown_set_options(ui_Dropdown2, s_wifi_scan_list);
        wifi_dropdown_select_saved_ssid();
        wifi_scan_ui_state = WIFI_SCAN_UI_IDLE;
    }
    else if(wifi_scan_ui_state == WIFI_SCAN_UI_FAILED) {
        lv_dropdown_set_options(ui_Dropdown2, "Scan failed");
        wifi_scan_ui_state = WIFI_SCAN_UI_IDLE;
    }
}

/**
 * @brief Refreshes the WiFi Configuration Screen data
 */
static void lv_refresh_WifiConfig_Data(void)
{
    if(isScreenChanged == true)
    {
        if(SSID_Info.MANUEL_EN == 1)
        {
            lv_obj_add_state(ui_WifiEnSw, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(ui_WifiEnSw, LV_STATE_CHECKED);
        }

        if(UI_OBJ_READY(ui_PasswordText))
        {
            lv_textarea_set_text(ui_PasswordText, SSID_Info.password);
        }

        wifi_scan_request_on_screen_enter();
    }
    else if(wifi_scan_ui_state == WIFI_SCAN_UI_READY || wifi_scan_ui_state == WIFI_SCAN_UI_FAILED)
    {
        wifi_scan_apply_results();
    }
}

/**
 * @brief Converts uint8_t integer value to string and updates UI textarea
 * @details Formats an unsigned 8-bit integer (range 0-255) as a decimal string
 *          and sets it as the text content of an LVGL textarea object
 * @param[in] obj Pointer to the LVGL textarea object to update
 * @param[in] value Unsigned 8-bit integer value to convert and display (0-255)
 * @return void
 * @note Used for displaying numeric values in IP address octets, configuration parameters, etc.
 * @see ui_get_int_from_textarea()
 */
static void ui_update_textarea_from_int(lv_obj_t * obj, uint8_t value)
{
    char buf[4]; // Max value is 255, so 3 digits + null terminator
    sprintf(buf, "%d", value);
    lv_textarea_set_text(obj, buf);
}

/**
 * @brief Reads integer value from UI textarea text and validates range
 * @details Retrieves text content from an LVGL textarea object, converts it to
 *          an integer using atoi(), and enforces range bounds (0-255). Out-of-range
 *          values are clamped to valid limits.
 * @param[in] obj Pointer to the LVGL textarea object containing the text
 * @return uint8_t Converted and validated integer value (0-255)
 * @note Safer alternative to raw atoi() with automatic bounds checking
 * @warning Returns 0 if text cannot be converted to integer (e.g., empty or non-numeric)
 * @see ui_update_textarea_from_int()
 */
static uint8_t ui_get_int_from_textarea(lv_obj_t * obj)
{
    const char * txt = lv_textarea_get_text(obj);
    int val = atoi(txt);

    // Safety check: ensure the value stays within 0-255
    if(val > 255) val = 255;
    if(val < 0)   val = 0;

    return (uint8_t)val;
}

/**
 * @brief Copy a NUL-terminated string into a bounded buffer safely
 * @details Copies up to `dest_size - 1` characters from `src` into `dest`
 *          and NUL-terminates `dest`. If `src` is NULL, `dest` will be
 *          set to an empty string. If `dest` is NULL or `dest_size` is
 *          zero, the function returns without action.
 * @param[out] dest Destination buffer
 * @param[in] dest_size Size of destination buffer in bytes
 * @param[in] src Source string to copy (may be NULL)
 */
static void param_table_copy_text(char *dest, uint16_t dest_size, const char *src)
{
    if((dest == NULL) || (dest_size == 0U))
    {
        return;
    }

    if(src == NULL)
    {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, (size_t)(dest_size - 1U));
    dest[dest_size - 1U] = '\0';
}

/**
 * @brief Get integer row count to use for parameter table
 * @details Returns the appropriate number of rows for the currently
 *          selected parameter table type (input/output/variable), clamped
 *          to the configured maximums to prevent out-of-range indexing.
 * @return uint16_t Row count for the table
 */
static uint16_t param_table_get_row_count(void)
{
    uint16_t rows = 0;

    if(s_param_table_type == PARAM_TABLE_INPUT)
    {
        if(max_inputs > MAX_INS)
        {
            rows = MAX_INS;
        }
        else
        {
            rows = max_inputs;
        }
    }
    else if(s_param_table_type == PARAM_TABLE_OUTPUT)
    {
        if(max_outputs > MAX_OUTS)
        {
            rows = MAX_OUTS;
        }
        else
        {
            rows = max_outputs;
        }
    }
    else
    {
        if(max_vars > MAX_VARS)
        {
            rows = MAX_VARS;
        }
        else
        {
            rows = max_vars;
        }
    }

    return rows;
}

static uint16_t param_table_calc_total_width(bool is_output)
{
    uint16_t total_w = 0;

    if(is_output) {
        for(uint16_t i = 0; i < 9U; i++) {
            total_w += s_param_table_widths[i];
        }
    }
    else {
        for(uint16_t i = 0; i < 7U; i++) {
            total_w += s_param_table_widths[i];
        }
        total_w += s_param_table_widths[8];
    }

    return total_w;
}

static void param_table_fill_row(uint16_t i)
{
    if(!UI_OBJ_READY(s_lv_table) || i >= s_param_build_total) {
        return;
    }

    char buf[32];
    char desc_buf[22];
    char label_buf[10];
    uint16_t r = i + 1U;

    lv_snprintf(buf, sizeof(buf), "%u", (unsigned)(i + 1U));
    lv_table_set_cell_value(s_lv_table, r, 0, buf);

    if(s_param_table_type == PARAM_TABLE_INPUT)
    {
        memset(desc_buf, 0, sizeof(desc_buf));
        memcpy(desc_buf, inputs[i].description, 21);
        memset(label_buf, 0, sizeof(label_buf));
        memcpy(label_buf, inputs[i].label, 9);

        lv_table_set_cell_value(s_lv_table, r, 1, desc_buf);
        lv_table_set_cell_value(s_lv_table, r, 2, label_buf);
        lv_snprintf(buf, sizeof(buf), "%ld", (long)inputs[i].value);
        lv_table_set_cell_value(s_lv_table, r, 3, buf);
        lv_table_set_cell_value(s_lv_table, r, 4,
                    inputs[i].auto_manual == 0 ? "Auto" : "Manual");
        lv_table_set_cell_value(s_lv_table, r, 5,
                    inputs[i].digital_analog == 1 ? "Analog" : "Digital");
        lv_snprintf(buf, sizeof(buf), "%d", inputs[i].control);
        lv_table_set_cell_value(s_lv_table, r, 6, buf);
        lv_snprintf(buf, sizeof(buf), "%u: %s", (unsigned)inputs[i].range,
                    param_table_range_text(PARAM_TABLE_INPUT, inputs[i].digital_analog,
                                           inputs[i].range));
        lv_table_set_cell_value(s_lv_table, r, 7, buf);
    }
    else if(s_param_table_type == PARAM_TABLE_OUTPUT)
    {
        memset(desc_buf, 0, sizeof(desc_buf));
        memcpy(desc_buf, outputs[i].description, 21);
        memset(label_buf, 0, sizeof(label_buf));
        memcpy(label_buf, outputs[i].label, 9);

        lv_table_set_cell_value(s_lv_table, r, 1, desc_buf);
        lv_table_set_cell_value(s_lv_table, r, 2, label_buf);
        lv_snprintf(buf, sizeof(buf), "%ld", (long)outputs[i].value);
        lv_table_set_cell_value(s_lv_table, r, 3, buf);
        lv_table_set_cell_value(s_lv_table, r, 4,
                    outputs[i].auto_manual == 0 ? "Auto" : "Manual");
        lv_table_set_cell_value(s_lv_table, r, 5,
                    outputs[i].digital_analog == 1 ? "Analog" : "Digital");
        lv_snprintf(buf, sizeof(buf), "%d", outputs[i].control);
        lv_table_set_cell_value(s_lv_table, r, 6, buf);
        lv_snprintf(buf, sizeof(buf), "%d", outputs[i].switch_status);
        lv_table_set_cell_value(s_lv_table, r, 7, buf);
        lv_snprintf(buf, sizeof(buf), "%u: %s", (unsigned)(uint8_t)outputs[i].range,
                    param_table_range_text(PARAM_TABLE_OUTPUT, outputs[i].digital_analog,
                                           (uint8_t)outputs[i].range));
        lv_table_set_cell_value(s_lv_table, r, 8, buf);
    }
    else
    {
        memset(desc_buf, 0, sizeof(desc_buf));
        memcpy(desc_buf, vars[i].description, 21);
        memset(label_buf, 0, sizeof(label_buf));
        memcpy(label_buf, vars[i].label, 9);

        lv_table_set_cell_value(s_lv_table, r, 1, desc_buf);
        lv_table_set_cell_value(s_lv_table, r, 2, label_buf);
        lv_snprintf(buf, sizeof(buf), "%ld", (long)vars[i].value);
        lv_table_set_cell_value(s_lv_table, r, 3, buf);
        lv_table_set_cell_value(s_lv_table, r, 4,
                    vars[i].auto_manual == 0 ? "Auto" : "Manual");
        lv_table_set_cell_value(s_lv_table, r, 5,
                    vars[i].digital_analog == 1 ? "Analog" : "Digital");
        lv_snprintf(buf, sizeof(buf), "%d", vars[i].control);
        lv_table_set_cell_value(s_lv_table, r, 6, buf);
        lv_snprintf(buf, sizeof(buf), "%u: %s", (unsigned)vars[i].range,
                    param_table_range_text(PARAM_TABLE_VARIABLE, vars[i].digital_analog,
                                           vars[i].range));
        lv_table_set_cell_value(s_lv_table, r, 7, buf);
    }
}

static void param_table_build_begin(void)
{
    if(!UI_OBJ_READY(ui_Panel4)) {
        return;
    }

    param_Clear_table();

    bool is_output = (s_param_table_type == PARAM_TABLE_OUTPUT);
    uint16_t col_count = is_output ? 9U : 8U;
    uint16_t total_w = param_table_calc_total_width(is_output);

    s_param_build_total = param_table_get_row_count();
    s_param_build_row = 0;
    s_param_build_active = (s_param_build_total > 0U);

    s_lv_table = lv_table_create(ui_Panel4);
    lv_obj_set_width(s_lv_table, total_w);
    lv_obj_set_height(s_lv_table, LV_SIZE_CONTENT);
    lv_obj_align(s_lv_table, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(s_lv_table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_column_count(s_lv_table, col_count);
    lv_table_set_row_count(s_lv_table, s_param_build_total + 1U);
    lv_obj_set_style_pad_left(s_lv_table, 2, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(s_lv_table, 2, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(s_lv_table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(s_lv_table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_row(s_lv_table, 0, LV_PART_MAIN);

    lv_table_set_column_width(s_lv_table, 0, s_param_table_widths[0]);
    lv_table_set_column_width(s_lv_table, 1, s_param_table_widths[1]);
    lv_table_set_column_width(s_lv_table, 2, s_param_table_widths[2]);
    lv_table_set_column_width(s_lv_table, 3, s_param_table_widths[3]);
    lv_table_set_column_width(s_lv_table, 4, s_param_table_widths[4]);
    lv_table_set_column_width(s_lv_table, 5, s_param_table_widths[5]);
    lv_table_set_column_width(s_lv_table, 6, s_param_table_widths[6]);
    if(is_output)
    {
        lv_table_set_column_width(s_lv_table, 7, s_param_table_widths[7]);
        lv_table_set_column_width(s_lv_table, 8, s_param_table_widths[8]);
    }
    else
    {
        lv_table_set_column_width(s_lv_table, 7, s_param_table_widths[8]);
    }

    lv_table_set_cell_value(s_lv_table, 0, 0, "No");
    lv_table_set_cell_value(s_lv_table, 0, 1, "Description");
    lv_table_set_cell_value(s_lv_table, 0, 2, "Label");
    lv_table_set_cell_value(s_lv_table, 0, 3, "Value");
    lv_table_set_cell_value(s_lv_table, 0, 4, "A/M");
    lv_table_set_cell_value(s_lv_table, 0, 5, "D/A");
    lv_table_set_cell_value(s_lv_table, 0, 6, "Ctrl");
    if(is_output)
    {
        lv_table_set_cell_value(s_lv_table, 0, 7, "Sw");
        lv_table_set_cell_value(s_lv_table, 0, 8, "Range");
    }
    else
    {
        lv_table_set_cell_value(s_lv_table, 0, 7, "Range");
    }

    lv_obj_set_style_text_font(s_lv_table, &lv_font_montserrat_12,
                               LV_PART_ITEMS | LV_STATE_DEFAULT);

    if(s_param_build_total == 0U) {
        s_param_build_active = false;
        lv_obj_add_event_cb(s_lv_table, param_table_cell_edit_cb,
                            LV_EVENT_VALUE_CHANGED, NULL);
    }
}

static void param_table_build_step(void)
{
    if(!s_param_build_active || !UI_OBJ_READY(s_lv_table)) {
        return;
    }

    for(uint16_t batch = 0;
        batch < PARAM_TABLE_BUILD_BATCH && s_param_build_row < s_param_build_total;
        batch++, s_param_build_row++)
    {
        param_table_fill_row(s_param_build_row);
        if((s_param_build_row % PARAM_TABLE_BUILD_BATCH) == 0U) {
            vTaskDelay(1);
        }
    }

    if(s_param_build_row >= s_param_build_total) {
        lv_obj_add_event_cb(s_lv_table, param_table_cell_edit_cb,
                            LV_EVENT_VALUE_CHANGED, NULL);
        s_param_build_active = false;
    }
}

static lv_obj_t * s_edit_popup = NULL;
static lv_obj_t * s_edit_ta    = NULL;
static lv_obj_t * s_range_dropdown = NULL;
static uint32_t   s_edit_row   = 0;
static uint32_t   s_edit_col   = 0;

typedef struct
{
    uint8_t value;
    const char *name;
} param_range_option_t;

/* Range values are protocol values.  Keep these tables aligned with ud_str.h. */
static const param_range_option_t s_analog_unit_ranges[] = {
    { unused, "Unused" }, { degC, "deg C" }, { degF, "deg F" },
    { FPM, "FPM" }, { Pa, "Pa" }, { KPa, "kPa" }, { psi, "psi" },
    { in_w, "in. W.C." }, { Watts, "Watts" }, { KW, "kW" },
    { KWH, "kWh" }, { Volts, "Volts" }, { KV, "kV" }, { Amps, "Amps" },
    { ma, "mA" }, { CFM, "CFM" }, { Sec, "Seconds" }, { Min, "Minutes" },
    { Hours, "Hours" }, { Days, "Days" }, { time_unit, "Time" },
    { ohms, "Ohms" }, { procent, "Percent" }, { RH, "RH" }, { ppm, "ppm" },
    { counts, "Counts" }, { Open, "Open" }, { CFH, "CFH" }, { GPM, "GPM" },
    { GPH, "GPH" }, { GAL, "Gallons" }, { CF, "CF" }, { BTU, "BTU" },
    { CMH, "CMH" }, { custom1, "Custom 1" }, { custom2, "Custom 2" },
    { custom3, "Custom 3" }, { custom4, "Custom 4" }, { custom5, "Custom 5" },
    { custom6, "Custom 6" }, { custom7, "Custom 7" }, { custom8, "Custom 8" }
};

static const param_range_option_t s_digital_ranges[] = {
    { UNUSED, "Unused" }, { OFF_ON, "Off/On" }, { CLOSED_OPEN, "Closed/Open" },
    { STOP_START, "Stop/Start" }, { DISABLED_ENABLED, "Disabled/Enabled" },
    { NORMAL_ALARM, "Normal/Alarm" }, { NORMAL_HIGH, "Normal/High" },
    { NORMAL_LOW, "Normal/Low" }, { NO_YES, "No/Yes" }, { COOL_HEAT, "Cool/Heat" },
    { UNOCCUPIED_OCCUPIED, "Unoccupied/Occupied" }, { LOW_HIGH, "Low/High" },
    { ON_OFF, "On/Off" }, { OPEN_CLOSED, "Open/Closed" }, { START_STOP, "Start/Stop" },
    { ENABLED_DISABLED, "Enabled/Disabled" }, { ALARM_NORMAL, "Alarm/Normal" },
    { HIGH_NORMAL, "High/Normal" }, { LOW_NORMAL, "Low/Normal" }, { YES_NO, "Yes/No" },
    { HEAT_COOL, "Heat/Cool" }, { OCCUPIED_UNOCCUPIED, "Occupied/Unoccupied" },
    { HIGH_LOW, "High/Low" }, { custom_digital1, "Custom digital 1" },
    { custom_digital2, "Custom digital 2" }, { custom_digital3, "Custom digital 3" },
    { custom_digital4, "Custom digital 4" }, { custom_digital5, "Custom digital 5" },
    { custom_digital6, "Custom digital 6" }, { custom_digital7, "Custom digital 7" },
    { custom_digital8, "Custom digital 8" }
};

static const param_range_option_t s_input_analog_ranges[] = {
    { not_used_input, "Not used" }, { Y3K_40_150DegC, "3K: -40..150 C" },
    { Y3K_40_300DegF, "3K: -40..300 F" }, { R10K_40_120DegC, "10K: -40..120 C" },
    { R10K_40_250DegF, "10K: -40..250 F" }, { R3K_40_150DegC, "3K: -40..150 C" },
    { R3K_40_300DegF, "3K: -40..300 F" }, { KM10K_40_120DegC, "K10K: -40..120 C" },
    { KM10K_40_250DegF, "K10K: -40..250 F" }, { PT1000_200_300DegC, "PT1000: -200..300 C" },
    { PT1000_200_570DegF, "PT1000: -200..570 F" }, { V0_5, "0..5 V" },
    { I0_100Amps, "0..100 A" }, { I0_20ma, "0..20 mA" }, { I0_20psi, "0..20 psi" },
    { N0_2_32counts, "0..2^32 counts" }, { P0_100_0_10V, "0..100%, 0..10 V" },
    { P0_100_0_5V, "0..100%, 0..5 V" }, { P0_100_4_20ma, "0..100%, 4..20 mA" },
    { V0_10_IN, "0..10 V" }, { table1, "Table 1" }, { table2, "Table 2" },
    { table3, "Table 3" }, { table4, "Table 4" }, { table5, "Table 5" },
    { HI_spd_count, "High-speed count" }, { Frequence, "Frequency" },
    { Humidty, "Humidity" }, { CO2_PPM, "CO2 ppm" }, { RPM, "RPM" },
    { TVOC_PPB, "TVOC ppb" }, { UG_M3, "ug/m3" }, { NUM_CM3, "#/cm3" },
    { DB, "dB" }, { LUX, "Lux" }, { AC_PWM, "AC PWM" }
};

static const param_range_option_t s_output_analog_ranges[] = {
    { not_used_output, "Not used" }, { V0_10, "0..10 V" },
    { P0_100_Open, "0..100% Open" }, { P0_20psi, "0..20 psi" },
    { P0_100, "0..100%" }, { P0_100_Close, "0..100% Close" },
    { I_0_20ma, "0..20 mA" }, { P0_100_PWM, "0..100% PWM" },
    { P0_100_2_10V, "0..100%, 2..10 V" }
};

static const param_range_option_t *param_table_get_range_options(param_table_type_t type,
    uint8_t digital_analog, uint16_t *count)
{
    if(digital_analog == 0U)
    {
        *count = sizeof(s_digital_ranges) / sizeof(s_digital_ranges[0]);
        return s_digital_ranges;
    }

    if(type == PARAM_TABLE_INPUT)
    {
        *count = sizeof(s_input_analog_ranges) / sizeof(s_input_analog_ranges[0]);
        return s_input_analog_ranges;
    }
    if(type == PARAM_TABLE_OUTPUT)
    {
        *count = sizeof(s_output_analog_ranges) / sizeof(s_output_analog_ranges[0]);
        return s_output_analog_ranges;
    }

    *count = sizeof(s_analog_unit_ranges) / sizeof(s_analog_unit_ranges[0]);
    return s_analog_unit_ranges;
}

static const char *param_table_range_text(param_table_type_t type, uint8_t digital_analog,
                                          uint8_t range)
{
    uint16_t count;
    const param_range_option_t *options = param_table_get_range_options(type, digital_analog, &count);
    for(uint16_t i = 0; i < count; i++)
    {
        if(options[i].value == range) return options[i].name;
    }
    return "Invalid";
}

static bool param_table_range_is_valid(param_table_type_t type, uint8_t digital_analog,
                                       uint8_t range)
{
    return strcmp(param_table_range_text(type, digital_analog, range), "Invalid") != 0;
}

static uint8_t param_table_mode_from_cell(uint32_t row)
{
    const char *mode = lv_table_get_cell_value(s_lv_table, row, 5);
    return (mode != NULL && strcmp(mode, "Analog") == 0) ? 1U : 0U;
}

static void param_table_range_popup_done_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED && UI_OBJ_READY(s_lv_table) &&
       UI_OBJ_READY(s_range_dropdown))
    {
        uint16_t count;
        uint16_t selected = lv_dropdown_get_selected(s_range_dropdown);
        uint8_t mode = param_table_mode_from_cell(s_edit_row);
        const param_range_option_t *options = param_table_get_range_options(s_param_table_type,
                                                                              mode, &count);
        if(selected < count)
        {
            char text[32];
            lv_snprintf(text, sizeof(text), "%u: %s", options[selected].value, options[selected].name);
            lv_table_set_cell_value(s_lv_table, s_edit_row, s_edit_col, text);
        }
    }

    if(UI_OBJ_READY(s_edit_popup)) lv_obj_del(s_edit_popup);
    s_edit_popup = NULL;
    s_range_dropdown = NULL;
}

static void param_table_range_popup_cancel_cb(lv_event_t * e)
{
    (void)e;
    if(UI_OBJ_READY(s_edit_popup)) lv_obj_del(s_edit_popup);
    s_edit_popup = NULL;
    s_range_dropdown = NULL;
}

static void param_table_show_range_popup(uint8_t current_range)
{
    uint16_t count;
    uint16_t selected = 0;
    uint8_t mode = param_table_mode_from_cell(s_edit_row);
    const param_range_option_t *options = param_table_get_range_options(s_param_table_type, mode, &count);
    char options_text[1024] = {0};

    for(uint16_t i = 0; i < count; i++)
    {
        char item[48];
        lv_snprintf(item, sizeof(item), "%u: %s%s", options[i].value, options[i].name,
                    (i + 1U < count) ? "\n" : "");
        strncat(options_text, item, sizeof(options_text) - strlen(options_text) - 1U);
        if(options[i].value == current_range) selected = i;
    }

    s_edit_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_edit_popup, 300, 210);
    lv_obj_center(s_edit_popup);
    lv_obj_clear_flag(s_edit_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_edit_popup, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(s_edit_popup);
    lv_label_set_text(title, mode ? "Select analog range" : "Select digital range");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    s_range_dropdown = lv_dropdown_create(s_edit_popup);
    lv_obj_set_width(s_range_dropdown, 280);
    lv_obj_align(s_range_dropdown, LV_ALIGN_TOP_MID, 0, 28);
    lv_dropdown_set_options(s_range_dropdown, options_text);
    lv_dropdown_set_selected(s_range_dropdown, selected);

    lv_obj_t *apply = lv_btn_create(s_edit_popup);
    lv_obj_set_size(apply, 125, 42);
    lv_obj_align(apply, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t *apply_label = lv_label_create(apply);
    lv_label_set_text(apply_label, "Apply");
    lv_obj_center(apply_label);
    lv_obj_add_event_cb(apply, param_table_range_popup_done_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_btn_create(s_edit_popup);
    lv_obj_set_size(cancel, 125, 42);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t *cancel_label = lv_label_create(cancel);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel, param_table_range_popup_cancel_cb, LV_EVENT_CLICKED, NULL);
}

static void param_table_edit_done_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_READY && s_edit_ta && UI_OBJ_READY(s_lv_table))
    {
        lv_table_set_cell_value(s_lv_table, s_edit_row, s_edit_col,
                                lv_textarea_get_text(s_edit_ta));
    }
    if(s_edit_popup)
    {
        lv_obj_del(s_edit_popup);
        s_edit_popup = NULL;
        s_edit_ta    = NULL;
        s_range_dropdown = NULL;
    }
}

static void param_table_cell_edit_cb(lv_event_t * e)
{
    lv_obj_t * table = lv_event_get_target(e);
    lv_table_get_selected_cell(table, &s_edit_row, &s_edit_col);

    if(s_edit_row == 0 || s_edit_col == 0) return; // header or No column

    const char * cur = lv_table_get_cell_value(table, s_edit_row, s_edit_col);

    // A/M column
    if (s_edit_col == 4)
    {
        if (cur && strcmp(cur, "Manual") == 0)
        {
            lv_table_set_cell_value(table, s_edit_row, s_edit_col, "Auto");
        }
        else
        {
            lv_table_set_cell_value(table, s_edit_row, s_edit_col, "Manual");
        }
        return;
    }

    // D/A column.  A range picker always uses the mode currently displayed here.
    if(s_edit_col == 5)
    {
        uint8_t new_mode = (cur && strcmp(cur, "Analog") == 0) ? 0U : 1U;
        uint32_t range_col = (s_param_table_type == PARAM_TABLE_OUTPUT) ? 8U : 7U;
        lv_table_set_cell_value(table, s_edit_row, s_edit_col,
                                new_mode ? "Analog" : "Digital");
        /* The previous code may be valid in both enums but mean something different. */
        lv_table_set_cell_value(table, s_edit_row, range_col,
                                new_mode ? "0: Not used" : "0: Unused");
        return;
    }

    if(UI_OBJ_READY(s_edit_popup))
    {
        lv_obj_del(s_edit_popup);
        s_edit_popup = NULL;
        s_range_dropdown = NULL;
    }
    /* Range is selected from the valid enum values, never entered as a raw number. */
    if(s_edit_col == (s_param_table_type == PARAM_TABLE_OUTPUT ? 8U : 7U))
    {
        param_table_show_range_popup((uint8_t)atoi(cur ? cur : "0"));
        return;
    }

    s_edit_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_edit_popup, 300, 170);
    lv_obj_center(s_edit_popup);
    lv_obj_clear_flag(s_edit_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_edit_popup, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_edit_ta = lv_textarea_create(s_edit_popup);
    lv_obj_set_size(s_edit_ta, 280, 45);
    lv_obj_align(s_edit_ta, LV_ALIGN_TOP_MID, 0, 0);
    lv_textarea_set_one_line(s_edit_ta, true);
    lv_textarea_set_max_length(s_edit_ta, 24);
    lv_textarea_set_text(s_edit_ta, cur ? cur : "");

    lv_obj_t * kb = lv_keyboard_create(s_edit_popup);
    lv_obj_set_size(kb, 284, 110);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, s_edit_ta);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_SCROLLABLE);
    // Numeric columns: Value(3), Ctrl(6), Sw(7 output).
    lv_keyboard_set_mode(kb, (s_edit_col >= 3) ? LV_KEYBOARD_MODE_NUMBER
                                               : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(kb, param_table_edit_done_cb, LV_EVENT_READY,  NULL);
    lv_obj_add_event_cb(kb, param_table_edit_done_cb, LV_EVENT_CANCEL, NULL);
}

/**
 * @brief Build wrapper retained for existing call sites
 */
static void param_table_build(void)
{
    param_table_build_begin();
    while(s_param_build_active) {
        param_table_build_step();
    }
}

/**
 * @brief Clears the parameter table and any associated edit popups
 * @details Deletes the LVGL table object and any active edit popup, resetting pointers to NULL
 * to ensure a clean state when switching between different parameter table types or refreshing the UI.
 */
static void param_Clear_table(void)
{
    if(UI_OBJ_READY(s_edit_popup))
    {
        lv_obj_del(s_edit_popup);
        s_edit_popup = NULL;
        s_edit_ta    = NULL;
        s_range_dropdown = NULL;
    }
    if(UI_OBJ_READY(s_lv_table))
    {
        lv_obj_del(s_lv_table);
        s_lv_table = NULL;
    }
}

/**
 * @brief Apply pending edits from the parameter table UI back to model
 * @details Iterates all registered edit bindings and, depending on the
 *          active table type, writes the updated values from the bound
 *          LVGL textarea objects into the corresponding inputs/outputs/vars
 *          structures. Performs basic bounds checking where applicable.
 */
static void param_table_apply_updates(void)
{
    if(!UI_OBJ_READY(s_lv_table)) return;

    uint16_t row_count = param_table_get_row_count();

    #define CELL(r,c)  lv_table_get_cell_value(s_lv_table, (r), (c))
    #define GETI(r,c)  ((int32_t)atoi(CELL(r,c)))
    #define U8(v)      ((uint8_t)(((v)<0)?0:(((v)>255)?255:(v))))
    #define S8(v)      ((int8_t)(((v)<-128)?-128:(((v)>127)?127:(v))))

    for(uint16_t i = 0; i < row_count; i++)
    {
        uint16_t r = i + 1U;

        if(s_param_table_type == PARAM_TABLE_INPUT)
        {
            param_table_copy_text((char *)inputs[i].description,
                sizeof(inputs[i].description), CELL(r, 1));
            param_table_copy_text((char *)inputs[i].label,
                sizeof(inputs[i].label), CELL(r, 2));
            inputs[i].value          = GETI(r, 3);
            inputs[i].auto_manual    = (strcmp(CELL(r, 4), "Manual") == 0) ? 1 : 0;
            inputs[i].digital_analog = (strcmp(CELL(r, 5), "Analog") == 0) ? 1 : 0;
            inputs[i].control        = S8(GETI(r, 6));
            inputs[i].range          = U8(GETI(r, 7));
            if(!param_table_range_is_valid(PARAM_TABLE_INPUT, inputs[i].digital_analog,
                                           inputs[i].range))
            {
                inputs[i].range = 0;
            }
        }
        else if(s_param_table_type == PARAM_TABLE_OUTPUT)
        {
            param_table_copy_text((char *)outputs[i].description,
                sizeof(outputs[i].description), CELL(r, 1));
            param_table_copy_text((char *)outputs[i].label,
                sizeof(outputs[i].label), CELL(r, 2));
            outputs[i].value          = GETI(r, 3);
            outputs[i].auto_manual    = (strcmp(CELL(r, 4), "Manual") == 0) ? 1 : 0;
            outputs[i].digital_analog = (strcmp(CELL(r, 5), "Analog") == 0) ? 1 : 0;
            outputs[i].control        = S8(GETI(r, 6));
            outputs[i].switch_status  = U8(GETI(r, 7));
            outputs[i].range          = S8(GETI(r, 8));
            if(!param_table_range_is_valid(PARAM_TABLE_OUTPUT, outputs[i].digital_analog,
                                           (uint8_t)outputs[i].range))
            {
                outputs[i].range = 0;
            }
        }
        else // PARAM_TABLE_VARIABLE
        {
            param_table_copy_text((char *)vars[i].description,
                sizeof(vars[i].description), CELL(r, 1));
            param_table_copy_text((char *)vars[i].label,
                sizeof(vars[i].label), CELL(r, 2));
            vars[i].value          = GETI(r, 3);
            vars[i].auto_manual    = (strcmp(CELL(r, 4), "Manual") == 0) ? 1 : 0;
            vars[i].digital_analog = (strcmp(CELL(r, 5), "Analog") == 0) ? 1 : 0;
            vars[i].control        = U8(GETI(r, 6));
            vars[i].range          = U8(GETI(r, 7));
            if(!param_table_range_is_valid(PARAM_TABLE_VARIABLE, vars[i].digital_analog,
                                           vars[i].range))
            {
                vars[i].range = 0;
            }
        }
    }

    #undef CELL
    #undef GETI
    #undef U8
    #undef S8
}

/**
 * @brief Refreshes the Network Setup Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data related to network setup and calls the corresponding UI update functions to ensure that the Network Setup Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the Network Setup Screen data, such as after a change in network settings or when navigating to the Network Setup Screen.
 */
static void lv_refresh_NetworkSetup_Data(void)
{
    if(isScreenChanged)
    {
            // ui_Checkbox2 is for Static IP
            // ui_Checkbox3 is for Auto DHCP
        lv_obj_clear_state(ui_Checkbox2, LV_STATE_CHECKED);
        lv_obj_clear_state(ui_Checkbox3, LV_STATE_CHECKED);
        if(SSID_Info.IP_Auto_Manual == 0) // Auto DHCP
        {
            lv_obj_add_state(ui_Checkbox3, LV_STATE_CHECKED);
            lv_obj_clear_flag(ui_GatewayPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(ui_SubnetPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(ui_IpPanel1, LV_OBJ_FLAG_CLICKABLE);
            set_ip_fields_editable(false);
        }
        else
        {
            lv_obj_add_state(ui_Checkbox2, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_GatewayPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(ui_SubnetPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(ui_IpPanel1, LV_OBJ_FLAG_CLICKABLE);
            set_ip_fields_editable(true);
        }
        // --- Update IP Address UI ---
        ui_update_textarea_from_int(ui_IPoct1, SSID_Info.ip_addr[0]);
        ui_update_textarea_from_int(ui_IPoct2, SSID_Info.ip_addr[1]);
        ui_update_textarea_from_int(ui_IPoct3, SSID_Info.ip_addr[2]);
        ui_update_textarea_from_int(ui_IPoct4, SSID_Info.ip_addr[3]);

        ui_update_textarea_from_int(ui_IPoct6, SSID_Info.net_mask[0]);
        ui_update_textarea_from_int(ui_IPoct7, SSID_Info.net_mask[1]);
        ui_update_textarea_from_int(ui_IPoct8, SSID_Info.net_mask[2]);
        ui_update_textarea_from_int(ui_IPoct9, SSID_Info.net_mask[3]);

        ui_update_textarea_from_int(ui_IPoct10, SSID_Info.getway[0]);
        ui_update_textarea_from_int(ui_IPoct11, SSID_Info.getway[1]);
        ui_update_textarea_from_int(ui_IPoct12, SSID_Info.getway[2]);
        ui_update_textarea_from_int(ui_IPoct13, SSID_Info.getway[3]);

        Modbus.ip_addr[0] = SSID_Info.ip_addr[0];
        Modbus.ip_addr[1] = SSID_Info.ip_addr[1];
        Modbus.ip_addr[2] = SSID_Info.ip_addr[2];
        Modbus.ip_addr[3] = SSID_Info.ip_addr[3];
        Modbus.subnet[0] = SSID_Info.net_mask[0];
        Modbus.subnet[1] = SSID_Info.net_mask[1];
        Modbus.subnet[2] = SSID_Info.net_mask[2];
        Modbus.subnet[3] = SSID_Info.net_mask[3];
        Modbus.getway[0] = SSID_Info.getway[0];
        Modbus.getway[1] = SSID_Info.getway[1];
        Modbus.getway[2] = SSID_Info.getway[2];
        Modbus.getway[3] = SSID_Info.getway[3];
    }
}

/**
 * @brief Convert a baud rate string to the corresponding enum value
 * @details Parses a numeric baud rate string (e.g., "9600") and returns
 *          the matching `E_BAUD` enum. If the string cannot be parsed or
 *          does not match a known baud rate, a fallback value from
 *          `Modbus.baudrate[0]` is returned.
 * @param[in] str NUL-terminated string containing numeric baud rate
 * @return E_BAUD Corresponding enum value for the parsed baud rate
 */

E_BAUD get_baud_enum_from_str(const char * str)
{
    int val = atoi(str); // Convert "9600" to 9600

    switch(val) {
        case 1200:   return UART_1200;
        case 2400:   return UART_2400;
        case 3600:   return UART_3600;
        case 4800:   return UART_4800;
        case 7200:   return UART_7200;
        case 9600:   return UART_9600;
        case 19200:  return UART_19200;
        case 38400:  return UART_38400;
        case 57600:  return UART_57600;
        case 76800:  return UART_76800;
        case 115200: return UART_115200;
        case 921600: return UART_921600;
        default:     return Modbus.baudrate[0]; // Default fallback
    }
}

/**
 * @brief Convert a `E_BAUD` enum value to its numeric baud rate
 * @details Maps the `E_BAUD` enumeration to the corresponding integer
 *          baud rate value (e.g., `UART_9600` -> 9600). Returns 9600 as a
 *          safe default for unknown enum values.
 * @param[in] baud_enum Baud rate enum to convert
 * @return uint32_t Numeric baud rate corresponding to the enum
 */
uint32_t get_baud_val_from_enum(E_BAUD baud_enum)
{
    switch(baud_enum) {
        case UART_1200:   return 1200;
        case UART_2400:   return 2400;
        case UART_3600:   return 3600;
        case UART_4800:   return 4800;
        case UART_7200:   return 7200;
        case UART_9600:   return 9600;
        case UART_19200:  return 19200;
        case UART_38400:  return 38400;
        case UART_57600:  return 57600;
        case UART_76800:  return 76800;
        case UART_115200: return 115200;
        case UART_921600: return 921600;
        default:          return 9600; // Safe default
    }
}

/**
 * @brief Refreshes the Protocols Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data related to protocols and calls the corresponding UI update functions to ensure that the Protocols Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the Protocols Screen data, such as after a change in protocol settings or when navigating to the Protocols Screen.
 */
static void lv_refresh_Protocols_Data(void)
{
    if(isScreenChanged)
    {
        char id_buf[8];
        sprintf(id_buf, "%d", Modbus.address);
        lv_textarea_set_text(ui_ModbusIdText, id_buf);

        char baud_buf[8];
        sprintf(baud_buf, "%ld", get_baud_val_from_enum(Modbus.baudrate[0]));
        lv_textarea_set_text(ui_BaudRateText, baud_buf);

        lv_textarea_set_text(ui_PanelNameText, panelname);

        if(Modbus.com_config[0] == BACNET_SLAVE)
        {
            lv_dropdown_set_selected(ui_DropdownNetdata2, 0);
        }
        else
        {
            lv_dropdown_set_selected(ui_DropdownNetdata2, 1); // 	MODBUS_SLAVE
        }

    }
}

/**
 * @brief Refreshes the Schedule Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data related to scheduling and calls the corresponding UI update functions to ensure that the Schedule Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the Schedule Screen data, such as after a change in schedule settings or when navigating to the Schedule Screen.
 */
static void lv_refresh_ScheduleScreen_Data(void)
{
    lv_obj_t *name_labels[MAX_WR] = {
        ui_SchText1, ui_SchText2, ui_SchText3, ui_SchText4,
        ui_SchText5, ui_SchText6, ui_SchText7, ui_SchText8
    };
    lv_obj_t *auto_manual_btns[MAX_WR] = {
        ui_SchAutoMan1, ui_SchAutoMan2, ui_SchAutoMan3, ui_SchAutoMan4,
        ui_SchAutoMan5, ui_SchAutoMan6, ui_SchAutoMan7, ui_SchAutoMan8
    };
    lv_obj_t *mode_labels[MAX_WR] = {
        ui_SchAutoManLabel1, ui_SchAutoManLabel2, ui_SchAutoManLabel3, ui_SchAutoManLabel4,
        ui_SchAutoManLabel5, ui_SchAutoManLabel6, ui_SchAutoManLabel7, ui_SchAutoManLabel8
    };
    lv_obj_t *value_switches[MAX_WR] = {
        ui_SchSwitch1, ui_SchSwitch2, ui_SchSwitch3, ui_SchSwitch4,
        ui_SchSwitch5, ui_SchSwitch6, ui_SchSwitch7, ui_SchSwitch8
    };

    if(!isScreenChanged)
    {
        return;
    }

    for(uint8_t i = 0; i < MAX_WR; i++)
    {
        char name_buf[21];
        memcpy(name_buf, weekly_routines[i].description, sizeof(weekly_routines[i].description));
        name_buf[sizeof(name_buf) - 1] = '\0';

        /* Guard against non-text garbage from stored buffers. */
        for(uint8_t j = 0; j < (uint8_t)(sizeof(name_buf) - 1U); j++)
        {
            unsigned char c = (unsigned char)name_buf[j];
            if(c == '\0')
            {
                break;
            }
            if((c < 32U) || (c > 126U))
            {
                name_buf[j] = '\0';
                break;
            }
        }

        for(int8_t j = (int8_t)sizeof(name_buf) - 2; j >= 0; j--)
        {
            if(name_buf[j] == '\0' || name_buf[j] == ' ')
            {
                name_buf[j] = '\0';
            }
            else
            {
                break;
            }
        }

        if(name_buf[0] == '\0')
        {
            memcpy(name_buf, weekly_routines[i].label, 9);
            memset(&name_buf[9], 0, sizeof(name_buf) - 9);

            for(uint8_t j = 0; j < (uint8_t)(sizeof(name_buf) - 1U); j++)
            {
                unsigned char c = (unsigned char)name_buf[j];
                if(c == '\0')
                {
                    break;
                }
                if((c < 32U) || (c > 126U))
                {
                    name_buf[j] = '\0';
                    break;
                }
            }

            for(int8_t j = (int8_t)sizeof(name_buf) - 2; j >= 0; j--)
            {
                if(name_buf[j] == '\0' || name_buf[j] == ' ')
                {
                    name_buf[j] = '\0';
                }
                else
                {
                    break;
                }
            }
        }
        if((name_buf[0] == '\0') ||
           (strcmp(name_buf, "Placeholder...") == 0) ||
           (strcmp(name_buf, "Placeholder") == 0))
        {
            snprintf(name_buf, sizeof(name_buf), "Schedule %u", (unsigned)(i + 1));
        }

        if(UI_OBJ_READY(name_labels[i]))
        {
            lv_textarea_set_text(name_labels[i], name_buf);
        }

        if(UI_OBJ_READY(mode_labels[i]))
        {
            lv_label_set_text(mode_labels[i], weekly_routines[i].auto_manual ? "MANUAL" : "AUTO");
        }
        if(UI_OBJ_READY(auto_manual_btns[i]))
        {
            lv_obj_add_flag(auto_manual_btns[i], LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_bg_color(auto_manual_btns[i], lv_color_hex(0x414041), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(auto_manual_btns[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(auto_manual_btns[i], lv_color_hex(0x414041), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(auto_manual_btns[i], 255, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_color(auto_manual_btns[i], lv_color_hex(0x414041), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_opa(auto_manual_btns[i], 255, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_shadow_width(auto_manual_btns[i], 0, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_outline_width(auto_manual_btns[i], 0, LV_PART_MAIN | LV_STATE_CHECKED);
            if(weekly_routines[i].auto_manual != 0U)
            {
                lv_obj_add_state(auto_manual_btns[i], LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_clear_state(auto_manual_btns[i], LV_STATE_CHECKED);
            }
        }

        uint8_t value_state = (weekly_routines[i].value != 0U) ? 1U : 0U;
        if(UI_OBJ_READY(value_switches[i]))
        {
            if(value_state)
            {
                lv_obj_add_state(value_switches[i], LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_clear_state(value_switches[i], LV_STATE_CHECKED);
            }
        }
    }
}

/**
 * @brief Refreshes the Schedule Edit Screen UI cells from internal schedule data
 * @details Copies schedule times from the `wr_times` internal structure into
 *          the visible UI textarea cells for the selected schedule. Uses a
 *          small cache to avoid rewriting unchanged cells and reduces UI churn.
 */
static void lv_refresh_ScheduleEditScreen_Data(void)
{
    lv_obj_t *time_cells[2 * MAX_INTERVALS_PER_DAY][7] = {
        { ui_ScheduleText1,  ui_ScheduleText2,  ui_ScheduleText3,  ui_ScheduleText4,  ui_ScheduleText5,  ui_ScheduleText6,  ui_ScheduleText7  },
        { ui_ScheduleText8,  ui_ScheduleText9,  ui_ScheduleText10, ui_ScheduleText11, ui_ScheduleText12, ui_ScheduleText13, ui_ScheduleText14 },
        { ui_ScheduleText15, ui_ScheduleText16, ui_ScheduleText17, ui_ScheduleText18, ui_ScheduleText19, ui_ScheduleText20, ui_ScheduleText21 },
        { ui_ScheduleText22, ui_ScheduleText23, ui_ScheduleText24, ui_ScheduleText25, ui_ScheduleText26, ui_ScheduleText27, ui_ScheduleText28 },
        { ui_ScheduleText29, ui_ScheduleText30, ui_ScheduleText31, ui_ScheduleText32, ui_ScheduleText33, ui_ScheduleText34, ui_ScheduleText35 },
        { ui_ScheduleText36, ui_ScheduleText37, ui_ScheduleText38, ui_ScheduleText39, ui_ScheduleText40, ui_ScheduleText41, ui_ScheduleText42 },
        { ui_ScheduleText43, ui_ScheduleText44, ui_ScheduleText45, ui_ScheduleText46, ui_ScheduleText47, ui_ScheduleText48, ui_ScheduleText49 },
        { ui_ScheduleText50, ui_ScheduleText51, ui_ScheduleText52, ui_ScheduleText53, ui_ScheduleText54, ui_ScheduleText55, ui_ScheduleText56 }
    };
    static uint8_t last_schedule = 0xFF;
    static uint16_t last_time_cache[2 * MAX_INTERVALS_PER_DAY][7];
    static bool cache_valid = false;
    uint8_t schedule_index = 0;

    if(UI_OBJ_READY(ui_Dropdown9))
    {
        schedule_index = (uint8_t)lv_dropdown_get_selected(ui_Dropdown9);
    }
    if(schedule_index >= MAX_WR)
    {
        schedule_index = 0;
    }

    if(isScreenChanged)
    {
        cache_valid = false;
    }

    for(uint8_t row = 0; row < (2 * MAX_INTERVALS_PER_DAY); row++)
    {
        for(uint8_t day = 0; day < 7; day++)
        {
            uint8_t hour = wr_times[schedule_index][day].time[row].hours;
            uint8_t min = wr_times[schedule_index][day].time[row].minutes;
            uint16_t packed_time = (uint16_t)(((uint16_t)hour << 8) | (uint16_t)min);

            if(cache_valid && (last_schedule == schedule_index) && (last_time_cache[row][day] == packed_time))
            {
                continue;
            }

            last_time_cache[row][day] = packed_time;

            if(UI_OBJ_READY(time_cells[row][day]))
            {
                char time_buf[12];
                snprintf(time_buf, sizeof(time_buf), "%02u : %02u", (unsigned)hour, (unsigned)min);
                lv_textarea_set_text(time_cells[row][day], time_buf);
            }
        }
    }

    last_schedule = schedule_index;
    cache_valid = true;
}
void Get_RTC_by_timestamp(U32_T timestamp,UN_Time* rtc,U8_T source);

/**
 * @brief Refresh time/date-related UI controls when the Time screen is shown
 * @details Initializes Time screen widgets from current RTC/system state:
 *          sync mode checkboxes, date/time labels, SNTP mode, last sync date,
 *          calendar month/day, and timezone dropdown selection.
 * @param[in] void No parameters
 * @return void
 */
static void lv_refresh_Time_Data(void)
{
    if(isScreenChanged)
    {
        isTimeUpdated = false;
        isDateUpdated = false;
        lv_obj_clear_state(ui_SyncLocalPcCheckbox, LV_STATE_CHECKED);
        lv_obj_clear_state(ui_SyncLocalPcCheckbox2, LV_STATE_CHECKED);

        if (Setting_Info.reg.en_time_sync_with_pc == 0) // Time server
        {
            lv_obj_add_state(ui_SyncLocalPcCheckbox2, LV_STATE_CHECKED);
            set_sync_pc_fields_editable(false);
        }
        else                                            // Sync with pc
        {
            lv_obj_add_state(ui_SyncLocalPcCheckbox, LV_STATE_CHECKED);
            set_sync_pc_fields_editable(true);
        }

        char buf[32];

        // Update Date
        sprintf(buf, "%02d-%02d-%04d",
                rtc_date.day,
                rtc_date.month,
                (rtc_date.year));

        lv_label_set_text(ui_Label70, buf);

        // Update Time
        memset(buf,0x00,sizeof(buf));
        sprintf(buf, "%02d : %02d" ,rtc_date.hour , rtc_date.minute);
        lv_label_set_text(ui_Label68, buf);

        if(Modbus.en_sntp >= 2 && Modbus.en_sntp < 5)
        {
            lv_dropdown_set_selected(ui_Dropdown5, Modbus.en_sntp - 2);
        }
        memcpy(&selected_date, &rtc_date, sizeof(rtc_date));
        if(rtc_date.year < 2025) // Basic sanity check to avoid setting calendar to garbage date if RTC isn't set
        {
            rtc_date.year = 2026;
            selected_date.year = 2026;
        }
        lv_calendar_set_today_date(ui_Calendar3, rtc_date.year, rtc_date.month,  rtc_date.day);
        lv_calendar_set_showed_date(ui_Calendar3, rtc_date.year, rtc_date.month);

        memset(buf,0x00,sizeof(buf));

        UN_Time RtcData;
        Get_RTC_by_timestamp(update_sntp_last_time,&RtcData,1);
        sprintf(buf, "%02d-%02d-%04d",
                RtcData.Clk.day,
                RtcData.Clk.mon,
                (RtcData.Clk.year));
        lv_label_set_text(ui_Label19, buf);

        for(int i = 0; i < sizeof(tz_offset_table)/sizeof(int16_t); i++)
        {
            if(tz_offset_table[i] == timezone)
            {
                lv_dropdown_set_selected(ui_Dropdown4, i);
                break;
            }
        }
    }
}

/**
 * @brief Refreshes the Parameters Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data related to user parameters and calls the corresponding UI update functions to ensure that the Parameters Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the Parameters Screen data, such as after a change in user parameters or when navigating to the Parameters Screen.
 */
static void lv_refresh_Parameters_Data(void)
{
    if(s_param_build_active) {
        param_table_build_step();
    }
}

static bool calendar_is_leap_year(uint16_t year)
{
    return ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

static int calendar_day_of_year(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int doy = day;

    if(month == 0U || month > 12U || day == 0U) {
        return -1;
    }

    for(uint8_t m = 1; m < month; m++) {
        doy += mdays[m - 1U];
        if(m == 2U && calendar_is_leap_year(year)) {
            doy++;
        }
    }

    return doy;
}

static void calendar_day_of_year_to_date(uint16_t year, int doy, lv_calendar_date_t *out)
{
    static const uint8_t mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t month_len[12];
    int remaining = doy;

    memcpy(month_len, mdays, sizeof(mdays));
    if(calendar_is_leap_year(year)) {
        month_len[1] = 29;
    }

    out->year = year;
    for(uint8_t m = 0; m < 12U; m++) {
        if(remaining <= month_len[m]) {
            out->month = m + 1U;
            out->day = (uint8_t)remaining;
            return;
        }
        remaining -= month_len[m];
    }
}

static bool calendar_is_holiday_day(int doy)
{
    if(doy < 1 || doy > 366) {
        return false;
    }

    int octet = (doy - 1) / 8;
    uint8_t mask = (uint8_t)(1U << ((doy - 1) % 8));

    for(int i = 0; i < MAX_AR; i++) {
        if(ar_dates[i][octet] & mask) {
            return true;
        }
    }

    return false;
}

static void calendar_toggle_holiday_day(int doy)
{
    if(doy < 1 || doy > 366) {
        return;
    }

    int octet = (doy - 1) / 8;
    uint8_t mask = (uint8_t)(1U << ((doy - 1) % 8));

    for(int i = 0; i < MAX_AR; i++) {
        ar_dates[i][octet] ^= mask;
    }

    save_point_info(0);
}

static void calendar_refresh_highlights(uint16_t year)
{
    static lv_calendar_date_t highlighted[366];
    uint16_t count = 0;

    if(!UI_OBJ_READY(ui_Calendar1)) {
        return;
    }

    int days_in_year = calendar_is_leap_year(year) ? 366 : 365;
    for(int doy = 1; doy <= days_in_year && count < 366U; doy++) {
        if(calendar_is_holiday_day(doy)) {
            calendar_day_of_year_to_date(year, doy, &highlighted[count]);
            count++;
        }
    }

    lv_calendar_set_highlighted_dates(ui_Calendar1, highlighted, count);
}

/**
 * @brief Refreshes the Holiday Calendar screen from RTC and ar_dates
 */
static void lv_refresh_calender_Data(void)
{
    if(!UI_OBJ_READY(ui_Calendar1)) {
        return;
    }

    if(isScreenChanged)
    {
        uint16_t year = rtc_date.year;
        uint8_t month = rtc_date.month;
        uint8_t day = rtc_date.day;

        if(year < 2025U) {
            year = 2026U;
            month = 1U;
            day = 1U;
        }

        lv_calendar_set_today_date(ui_Calendar1, year, month, day);
        lv_calendar_set_showed_date(ui_Calendar1, year, month);
        calendar_refresh_highlights(year);

        if(UI_OBJ_READY(ui_HolidayDateLabel)) {
            char buf[40];
            snprintf(buf, sizeof(buf), "Tap a date to toggle holiday");
            lv_label_set_text(ui_HolidayDateLabel, buf);
        }
    }
}

/**
 * @brief Updates the temperature value displayed on the LCD screen
 * @details Converts a floating-point temperature value to a formatted string
 *          (one decimal place) and updates the temperature text area component
 * @param[in] temp Temperature value to display (in degrees Celsius)
 * @return void
 * @note Checks if the UI object uic_TemperatureVal is ready before updating
 */
static void ui_update_temperature(float temp)
{
    if(!UI_OBJ_READY(uic_TemperatureVal))
        return;

    char buf[8];
    snprintf(buf, sizeof(buf), " %04.1f", temp);
    lv_textarea_set_text(uic_TemperatureVal, buf);
}

/**
 * @brief Updates the humidity percentage value displayed on the LCD screen
 * @details Displays humidity level as a percentage with '%' symbol.
 *          Automatically clamps value to maximum of 100%
 * @param[in] humidity Humidity percentage value (0-100)
 * @return void
 * @note Checks if the UI object ui_TextArea4 is ready before updating
 */
static void ui_update_humidity(uint8_t humidity)
{
    if(!UI_OBJ_READY(ui_TextArea4))
        return;

    if(humidity > 100)
        humidity = 100;

    char buf[6];
    snprintf(buf, sizeof(buf), "%d%%", humidity);

    lv_textarea_set_text(ui_TextArea4, buf);
}

/**
 * @brief Updates the time/runtime display on the LCD screen
 * @details Sets the runtime text area with a provided time string.
 *          Validates the input pointer before updating the UI
 * @param[in] time_str Pointer to a time string (e.g., "14:30:45")
 * @return void
 * @note Checks if the UI object uic_RunTime is ready before updating
 * @warning Returns early if time_str is NULL
 */
static void ui_update_time(const char *time_str)
{
    if(!UI_OBJ_READY(uic_RunTime))
        return;

    if(time_str == NULL)
        return;

    lv_textarea_set_text(uic_RunTime, time_str);
}

/**
 * @brief Updates the temperature setpoint arc/slider and displays the setpoint value
 * @details Updates two arc UI components and a text display to show the current
 *          temperature setpoint. Synchronizes visual arc with numeric display.
 *          Values are clamped to a maximum of 50°C
 * @param[in] setpoint Temperature setpoint value in degrees Celsius (0-50)
 * @return void
 * @note Checks if all three UI objects are ready before updating
 * @note Clamps setpoint to maximum 50°C
 */
static void ui_update_setpoint_arc(uint8_t setpoint)
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

/**
 * @brief Sets the temperature unit symbol (°C or °F) displayed on the screen
 * @details Updates the temperature unit indicator to show either Celsius or
 *          Fahrenheit based on the boolean parameter
 * @param[in] is_fahrenheit If true, sets unit to "°F"; if false, sets unit to "°C"
 * @return void
 * @note Checks if the UI object uic_TempratureSymbol is ready before updating
 * @note Does not convert numeric values, only updates the unit symbol
 */
static void ui_set_temperature_unit(bool is_fahrenheit)
{
    if(!UI_OBJ_READY(uic_TempratureSymbol))
        return;

    lv_textarea_set_text(uic_TempratureSymbol,
                         is_fahrenheit ? "°F" : "°C");
}

/**
 * @brief Select WiFi icon matching menuidle.c DisplayHeaderSymbol()
 */
static const lv_image_dsc_t * ui_wifi_symbol_for_status(void)
{
    if((SSID_Info.IP_Wifi_Status == WIFI_NORMAL) ||
       (SSID_Info.IP_Wifi_Status == WIFI_CONNECTED))
    {
        if(SSID_Info.rssi < -80) {
            return &ui_img_wifisym_1_png;
        }
        if(SSID_Info.rssi < -70) {
            return &ui_img_wifisym_2_png;
        }
        if(SSID_Info.rssi < -60) {
            return &ui_img_wifisym_3_png;
        }
        return &ui_img_wifisym_4_png;
    }

    if((SSID_Info.IP_Wifi_Status == WIFI_NO_CONNECT) ||
       (SSID_Info.IP_Wifi_Status == WIFI_SSID_FAIL) ||
       (SSID_Info.IP_Wifi_Status == WIFI_DISCONNECTED))
    {
        return &ui_img_wifisym_0_png;
    }

    return &ui_img_wifisym_Disable_png;
}

/**
 * @brief Update home-screen WiFi symbol image from connection state and RSSI
 */
static void ui_update_wifi_symbol(void)
{
    if(!UI_OBJ_READY(uic_WifiSymb)) {
        return;
    }

    const lv_image_dsc_t *symbol = ui_wifi_symbol_for_status();
    if(symbol == NULL) {
        lv_obj_set_style_opa(uic_WifiSymb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }

    lv_image_set_src(uic_WifiSymb, symbol);
    lv_obj_set_style_opa(uic_WifiSymb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}


/* ============================================================================
 * EVENT CALLBACK IMPLEMENTATIONS
 * ============================================================================ */

/**
 * @brief Event callback for setpoint value input/change
 * @details Handles user interaction when setting the temperature setpoint value.
 *          Reads the arc slider value and updates the setpoint data point value,
 *          scaling by 1000 to match internal fixed-point precision
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SetSetpointValue(lv_event_t * e)
{
    Temperature_SetpointDataPt.pvar->value = 1000 * lv_arc_get_value(ui_TempSetPoint1);
}

/**
 * @brief Event callback for WiFi enable/disable action
 * @details Handles user action to toggle WiFi manual configuration mode. Synchronizes
 *          the UI switch state with the SSID_Info.MANUEL_EN flag to enable or disable
 *          manual WiFi network configuration
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_WifiEn(lv_event_t * e)
{
    ESP_LOGI(TAG, "WiFi Enable Switch toggled");
    if(SSID_Info.MANUEL_EN == 1)
    {
        // Disable manual WiFi config
        ESP_LOGI(TAG, "WiFi manual configuration disabled");
        SSID_Info.MANUEL_EN = 0;
        lv_obj_clear_state(ui_WifiEnSw, LV_STATE_CHECKED);
    }
    else
    {
        // Enable manual WiFi config
        ESP_LOGI(TAG, "WiFi manual configuration enabled");
        SSID_Info.MANUEL_EN = 1;
        lv_obj_add_state(ui_WifiEnSw, LV_STATE_CHECKED);
    }

    save_block(FLASH_BLOCK1_SSID);
    connect_wifi_non_blocking();
}

/**
 * @brief Event callback for WiFi IP auto-selection
 * @details Reserved for handling WiFi auto-IP selection transitions.
 *          No behavior is currently implemented in this callback.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_WifiIpAutoSelect(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for general keypad press events
 * @details Reserved for generic keypad handling. No behavior is currently
 *          implemented in this callback.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_KeyPress(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for fan auto mode toggle
 * @details Handles user action to switch the fan to automatic operation mode.
 *          Sets the fan mode data point to 0 (auto mode)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_FanSetAutoMode(lv_event_t * e)
{
    FanModePt.pvar->value = 1000 * 0; // Set to auto mode
}

/**
 * @brief Event callback for air circulation button press
 * @details Handles user action to enable air circulation mode. Sets the fan mode
 *          data point to 2 (circulate/blower mode)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_CirculateBtnClicked(lv_event_t * e)
{
    FanModePt.pvar->value = 1000 * 2; // Set to circulate mode
}

/**
 * @brief Event callback for fan start button press
 * @details Handles user action to start the fan in on mode. Sets the fan mode
 *          data point to 1 (fan on) and captures the selected duration from the
 *          fan timer roller widget (1h, 2h, 4h, 8h, or continuous)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_StartFanBtn(lv_event_t * e)
{
    FanModePt.pvar->value = 1000 * 1; // Set to fan on mode
    FanMode_On_Val = lv_roller_get_selected(ui_Roller1);
}

/**
 * @brief Event callback for system heat mode activation
 * @details Handles user action to switch the HVAC system to heating mode.
 *          Sets the system mode data point to 2 (heat mode)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SysModeHeatFunc(lv_event_t * e)
{
    SysModePt.pvar->value = 1000 * 2; // Set to heat mode
}

/**
 * @brief Event callback for auto mode button press
 * @details Handles user action to activate automatic system mode where the HVAC
 *          system automatically switches between heating and cooling based on
 *          temperature setpoint. Sets the system mode data point to 1 (auto mode)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_AutoModeBtnFunc(lv_event_t * e)
{
    SysModePt.pvar->value = 1000 * 1; // Set to auto mode
}

/**
 * @brief Event callback for mode off button press
 * @details Handles user action to turn off the HVAC system completely.
 *          Sets the system mode data point to 0 (off mode), disabling all
 *          heating and cooling operations
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ModeOffBtnEventFunc(lv_event_t * e)
{
    SysModePt.pvar->value = 1000 * 0; // Set to off mode
}

/**
 * @brief Event callback for system cool mode activation
 * @details Handles user action to switch the HVAC system to cooling mode.
 *          Sets the system mode data point to 3 (cool mode)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SysModeCoolFunc(lv_event_t * e)
{
    SysModePt.pvar->value = 1000 * 3; // Set to cool mode
}

/**
 * @brief Event callback for WiFi configuration update
 * @details Handles user action to save and apply WiFi network configuration settings.
 *          Reads the selected SSID from dropdown and password from text input,
 *          then stores them in SSID_Info structure for connection attempt.
 *          Connection to the selected network occurs asynchronously after this callback.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 * @note TODO: Implement network connection after configuration is stored
 */
void Event_Cb_UpdateWifiConfig(lv_event_t * e)
{
    const char * password = lv_textarea_get_text(ui_PasswordText);
    ReconnectWithWifi = true;
    memset(SSID_Info.name, 0x00, sizeof(SSID_Info.name));
    lv_dropdown_get_selected_str(ui_Dropdown2, SSID_Info.name, sizeof(SSID_Info.name));
    memset(SSID_Info.password, 0x00, sizeof(SSID_Info.password));
    if(password != NULL)
    {
        strncpy(SSID_Info.password, password, sizeof(SSID_Info.password) - 1U);
        SSID_Info.password[sizeof(SSID_Info.password) - 1U] = '\0';
    }
    save_wifi_info();
    connect_wifi_non_blocking();
}

/**
 * @brief Event callback for WiFi password/SSID keyboard input
 * @details Reserved for custom keyboard integration while entering WiFi
 *          credentials. No behavior is currently implemented.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_WifiKeyboardPressFunc(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for network configuration keyboard input
 * @details Reserved for keyboard routing on network configuration fields.
 *          No behavior is currently implemented.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_NetworkConfigKeyPressFunc(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for schedule time selection
 * @details Captures the selected schedule time cell, parses the current
 *          `HH : MM` value, clamps invalid fields, and updates hour/minute
 *          dropdowns used by the schedule time popup editor.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleTimeSelectedFunc(lv_event_t * e)
{
    if((e == NULL) || !UI_OBJ_READY(ui_Dropdown6) || !UI_OBJ_READY(ui_Dropdown7))
    {
        return;
    }

    lv_obj_t *target = lv_event_get_target(e);
    if(target == NULL)
    {
        return;
    }
    selected_schedule_time_cell = target;

    const char *txt = lv_textarea_get_text(target);
    uint16_t hour = 0U;
    uint16_t min = 0U;

    if((txt != NULL) && (txt[0] != '\0'))
    {
        const char *p = txt;
        uint16_t h = 0U;
        uint16_t m = 0U;

        while(*p == ' ')
        {
            p++;
        }

        while((*p >= '0') && (*p <= '9'))
        {
            h = (uint16_t)((h * 10U) + (uint16_t)(*p - '0'));
            p++;
        }

        while(*p == ' ')
        {
            p++;
        }

        if(*p == ':')
        {
            p++;
            while(*p == ' ')
            {
                p++;
            }

            while((*p >= '0') && (*p <= '9'))
            {
                m = (uint16_t)((m * 10U) + (uint16_t)(*p - '0'));
                p++;
            }

            if((h <= 24U) && (m <= 59U))
            {
                hour = h;
                min  = m;
            }
        }
    }

    if(hour > 24U)
    {
        hour = 0U;   // "255"
    }
    if(min > 59U)
    {
        min = 0U;    // "255"
    }

    lv_dropdown_set_selected(ui_Dropdown6, hour);
    lv_dropdown_set_selected(ui_Dropdown7, min);
}

/**
 * @brief Event callback for schedule time update
 * @details Applies selected hour/minute dropdown values to the previously
 *          selected schedule time cell in `HH : MM` format.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleTimeUpdateCallback(lv_event_t * e)
{
    (void)e;

    if(!UI_OBJ_READY(selected_schedule_time_cell) ||
       !UI_OBJ_READY(ui_Dropdown6) ||
       !UI_OBJ_READY(ui_Dropdown7))
    {
        return;
    }

    uint16_t hour = lv_dropdown_get_selected(ui_Dropdown6);
    uint16_t min  = lv_dropdown_get_selected(ui_Dropdown7);
    char time_buf[32];

    snprintf(time_buf, sizeof(time_buf), "%02u : %02u", (unsigned)hour, (unsigned)min);
    lv_textarea_set_text(selected_schedule_time_cell, time_buf);
}

/**
 * @brief Event callback for outdoor/current temperature display mode toggle
 * @details Toggles whether the home screen shows outdoor temperature or the
 *          regular current/room temperature source.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ChangeTemperatureTypeCallBack(lv_event_t * e)
{
    (void)e;
    s_show_outdoor_temperature = !s_show_outdoor_temperature;
}

/**
 * @brief Event callback for parameter input display
 * @details Selects the INPUT parameter table mode used by table build/apply
 *          routines.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ParamInputShowCallBackFunc(lv_event_t * e)
{
    (void)e;
    s_param_table_type = PARAM_TABLE_INPUT;
}

/**
 * @brief Event callback for parameter output display
 * @details Selects the OUTPUT parameter table mode used by table build/apply
 *          routines.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ParamOutputShowCallBackFunc(lv_event_t * e)
{
    (void)e;
    s_param_table_type = PARAM_TABLE_OUTPUT;
}

/**
 * @brief Event callback for parameter variable display
 * @details Selects the VARIABLE parameter table mode used by table build/apply
 *          routines.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ParamVariableShowCallBackFunc(lv_event_t * e)
{
    (void)e;
    s_param_table_type = PARAM_TABLE_VARIABLE;
}

/**
 * @brief Event callback for WiFi SSID display event
 * @details Reserved for explicit SSID list refresh/display behavior.
 *          No behavior is currently implemented in this callback.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SSIDShowEventFunc(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for IP auto-configuration next action
 * @details Reserved for IP auto-configuration flow control. No behavior is
 *          currently implemented in this callback.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_IpAutoNext(lv_event_t * e)
{
    (void)e;

}

/**
 * @brief Event callback for parameter table updates
 * @details Rebuilds the on-screen parameter table for the currently selected
 *          parameter category (input/output/variable).
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_UpdateParameterTableFunc(lv_event_t * e)
{
    (void)e;
    param_table_build_begin();
}

/**
 * @brief Event callback for system time updates
 * @details Reads selected hour/minute from rollers, updates the visible time
 *          label, and marks time as modified so it can be applied later.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SysTimeUpdateCallback(lv_event_t * e)
{
    uint16_t hour_index = lv_dropdown_get_selected(ui_Dropdown11);
    uint16_t min_index  = lv_dropdown_get_selected(ui_Dropdown8);

    char buf[32] = {0};

    sprintf(buf, "%02d : %02d" ,hour_index , min_index);
    rtc_date.hour = hour_index;
    rtc_date.minute = min_index;
    lv_label_set_text(ui_Label68, buf);

    isTimeUpdated = true;

}

/**
 * @brief Event callback for calendar/date value changes
 * @details Captures the pressed date from calendar widget, updates date label,
 *          and marks date as modified so it can be applied later.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_CalenderValueChangeCallback(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_current_target(e);

    if(obj == NULL) return;

    if(lv_calendar_get_pressed_date(obj, &selected_date) != LV_RES_OK) {
        return;
    }

    if(obj == ui_Calendar1)
    {
        int doy = calendar_day_of_year(selected_date.year,
                                       (uint8_t)selected_date.month,
                                       (uint8_t)selected_date.day);
        if(doy > 0) {
            calendar_toggle_holiday_day(doy);
            calendar_refresh_highlights(selected_date.year);
        }

        if(UI_OBJ_READY(ui_HolidayDateLabel)) {
            char buf[40];
            bool is_holiday = (doy > 0) ? calendar_is_holiday_day(doy) : false;
            snprintf(buf, sizeof(buf), "%02u-%02u-%04u: %s",
                     (unsigned)selected_date.day,
                     (unsigned)selected_date.month,
                     (unsigned)selected_date.year,
                     is_holiday ? "Holiday" : "Normal");
            lv_label_set_text(ui_HolidayDateLabel, buf);
        }
        return;
    }

    if(obj == ui_Calendar3)
    {
        char buf[32];

        sprintf(buf, "%02d-%02d-%04d",
                selected_date.day,
                selected_date.month,
                selected_date.year);

        lv_label_set_text(ui_Label70, buf);
        isDateUpdated = true;
    }
}

/**
 * @brief Event callback for network configuration updates
 * @details Handles user action to save and apply network configuration changes.
 *          Processes updates to IP address, subnet mask, and gateway settings
 * @param[in] e LVGL event pointer containing event data
 * @return void
 * @note Full validation/persistence is handled elsewhere.
 */
void Event_Cb_NetworkConfigUpdateFunc(lv_event_t * e)
{
    (void)e;

    if(lv_obj_has_state(ui_Checkbox2, LV_STATE_CHECKED)) {
        SSID_Info.IP_Auto_Manual = 1;
    }
    else if(lv_obj_has_state(ui_Checkbox3, LV_STATE_CHECKED)) {
        SSID_Info.IP_Auto_Manual = 0;
    }

    if(SSID_Info.IP_Auto_Manual)
    {
        SSID_Info.ip_addr[0] = Modbus.ip_addr[0] = ui_get_int_from_textarea(ui_IPoct1);
        SSID_Info.ip_addr[1] = Modbus.ip_addr[1] = ui_get_int_from_textarea(ui_IPoct2);
        SSID_Info.ip_addr[2] = Modbus.ip_addr[2] = ui_get_int_from_textarea(ui_IPoct3);
        SSID_Info.ip_addr[3] = Modbus.ip_addr[3] = ui_get_int_from_textarea(ui_IPoct4);

        SSID_Info.net_mask[0] = Modbus.subnet[0] = ui_get_int_from_textarea(ui_IPoct6);
        SSID_Info.net_mask[1] = Modbus.subnet[1] = ui_get_int_from_textarea(ui_IPoct7);
        SSID_Info.net_mask[2] = Modbus.subnet[2] = ui_get_int_from_textarea(ui_IPoct8);
        SSID_Info.net_mask[3] = Modbus.subnet[3] = ui_get_int_from_textarea(ui_IPoct9);

        SSID_Info.getway[0] = Modbus.getway[0] = ui_get_int_from_textarea(ui_IPoct10);
        SSID_Info.getway[1] = Modbus.getway[1] = ui_get_int_from_textarea(ui_IPoct11);
        SSID_Info.getway[2] = Modbus.getway[2] = ui_get_int_from_textarea(ui_IPoct12);
        SSID_Info.getway[3] = Modbus.getway[3] = ui_get_int_from_textarea(ui_IPoct13);
    }

    save_wifi_info();
}

/**
 * @brief Event callback for protocol configuration updates
 * @details Handles user action to save and apply changes to communication protocol settings
 *          (e.g., BACnet settings, Modbus configuration, etc.)
 * @param[in] e LVGL event pointer containing event data
 * @return void
 * @note Performs direct structure updates from current UI values.
 */
void Event_Cb_UpdateProtocolFunc(lv_event_t * e)
{
    const char * id_textStr = lv_textarea_get_text(ui_ModbusIdText);
    const char * baud_textStr = lv_textarea_get_text(ui_BaudRateText);
    const char * panel_name = lv_textarea_get_text(ui_PanelNameText);

    // dropdown index 0 = BACnet Slave
    // dropdown index 1 = Modbus Slave
    uint16_t comm_type_index = lv_dropdown_get_selected(ui_DropdownNetdata2);
    // char buf[32];
    // lv_dropdown_get_selected_str(ui_Comm_Dropdown, buf, sizeof(buf));

    if(comm_type_index == 0)
    {
        Modbus.com_config[0] = 	BACNET_SLAVE;
    }
    else if(comm_type_index == 1)
    {
        Modbus.com_config[0] = 	MODBUS_SLAVE;
    }

    Modbus.address = (uint8_t)(atoi(id_textStr));
    Modbus.baudrate[0] = get_baud_enum_from_str(baud_textStr);
    if(panel_name != NULL)
    {
        strncpy(panelname, panel_name, 19);
        panelname[19] = '\0';
    }

}

/**
 * @brief Event callback for system time refresh
 * @details Handles request to refresh and update the system time display from RTC.
 *          Forces synchronization between RTC value and UI time display
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_RefreshTimeFunc(lv_event_t * e)
{
    (void)e;
    char buf[32] = {0};
    // Update Date
    sprintf(buf, "%02d-%02d-%04d",
            rtc_date.day,
            rtc_date.month,
            (rtc_date.year));

    lv_label_set_text(ui_Label70, buf); // update time

            // Update Time
    memset(buf,0x00,sizeof(buf));
    sprintf(buf, "%02d : %02d" ,rtc_date.hour , rtc_date.minute);
    lv_label_set_text(ui_Label68, buf);
}

/**
 * @brief Event callback for update time on Update time pop up
 * @details Handles time update on pop up window for change time
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ChangeTimePopUp(lv_event_t * e)
{
        // Set dropdowns to current RTC time
    lv_dropdown_set_selected(ui_Dropdown11, rtc_date.hour);    // hours
    lv_dropdown_set_selected(ui_Dropdown8,  rtc_date.minute);  // minutes
}

/**
 * @brief Event callback for local PC time synchronization
 * @details Handles time sync request to align system time with connected PC/host time.
 *          Typically used when device is connected to development/configuration PC
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_TimeSyncLocalPcFunc(lv_event_t * e)
{
    if(Setting_Info.reg.en_time_sync_with_pc)
    {
        flag_Update_Sntp = 0; // start sync
		Update_Sntp_Retry = 0;

        Event_Cb_RefreshTimeFunc(e);
    }
}

/**
 * @brief Event callback for applying manual time/date/timezone updates
 * @details Applies pending date/time edits to RTC, clears pending flags, and
 *          updates timezone plus timestamp synchronization when TZ changes.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_TimeSyncUpdateFunc(lv_event_t * e)
{
    (void)e;

    if(isDateUpdated || isTimeUpdated)
    {
        if(isDateUpdated)
        {
            rtc_date.year = selected_date.year;
            rtc_date.month = (uint8_t)selected_date.month;
            rtc_date.day = (uint8_t)selected_date.day;
        }

        if(isTimeUpdated)
        {
            rtc_date.hour = (uint8_t)lv_dropdown_get_selected(ui_Dropdown11);
            rtc_date.minute = (uint8_t)lv_dropdown_get_selected(ui_Dropdown8);
            rtc_date.second = 0U;
        }
        ESP_LOGI(TAG, "Applying manual time update: %04u-%02u-%02u %02u:%02u:%02u",
                 rtc_date.year, rtc_date.month, rtc_date.day,
                 rtc_date.hour, rtc_date.minute, rtc_date.second);
        PCF_SetDateTime(&rtc_date);
		update_timers();
        isDateUpdated = false;
        isTimeUpdated = false;
    }

    uint16_t index = lv_dropdown_get_selected(ui_Dropdown4);
    if(index < (sizeof(tz_offset_table) / sizeof(S16_T)))
    {
        S16_T newTZ = tz_offset_table[index];
        if(newTZ != timezone)
        {
            S16_T oldTZ = timezone;
            timezone = newTZ;   // update stored timezone
            Sync_timestamp(newTZ, oldTZ, 0, 0);
        }
    }
}

/**
 * @brief Event callback for time server configuration update
 * @details Handles user configuration changes for the time server address and settings.
 *          Updates NTP/SNTP server parameters that will be used for automatic
 *          periodic time synchronization
 * @param[in] e LVGL event pointer containing event data
 * @return void
 * @note Current implementation refreshes/displays last SNTP sync date.
 */
void Event_Cb_TimeSyncServerUpdateFunc(lv_event_t * e)
{
    char buf[32] = {0};
    UN_Time RtcData;
    Get_RTC_by_timestamp(update_sntp_last_time,&RtcData,1);
    sprintf(buf, "%02d-%02d-%04d",
            RtcData.Clk.day,
            RtcData.Clk.mon,
            (RtcData.Clk.year));
    lv_label_set_text(ui_Label19, buf);
}

/**
 * @brief Event callback for clearing parameter table edits
 * @details Discards any pending edits in the parameter table and resets the
 *          display to match the current underlying data model values.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ParameterClearTableFunc(lv_event_t * e)
{
    (void)e;
    param_Clear_table();
}

/**
 * @brief Event callback for committing parameter table edits
 * @details Applies all buffered parameter table field edits back into the
 *          underlying input/output/variable data model.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ParameterUpdateFunc(lv_event_t * e)
{
    (void)e;
    param_table_apply_updates();
    /* The table edits change point configuration, so retain them across reboot. */
    if(save_point_info(0) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save parameter point settings");
    }
    param_Clear_table();
}

/**
 * @brief Event callback for schedule auto/manual mode toggles
 * @details Identifies which schedule row button was toggled, flips its
 *          `auto_manual` state, and updates corresponding mode label text.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleAutoManualValChangeFun(lv_event_t * e)
{
    lv_obj_t *target = lv_event_get_current_target(e);
    if(target == NULL)
    {
        target = lv_event_get_target(e);
    }
    if(target == NULL)
    {
        return;
    }

    lv_obj_t *auto_manual_btns[MAX_WR] = {
        ui_SchAutoMan1, ui_SchAutoMan2, ui_SchAutoMan3, ui_SchAutoMan4,
        ui_SchAutoMan5, ui_SchAutoMan6, ui_SchAutoMan7, ui_SchAutoMan8
    };
    lv_obj_t *mode_labels[MAX_WR] = {
        ui_SchAutoManLabel1, ui_SchAutoManLabel2, ui_SchAutoManLabel3, ui_SchAutoManLabel4,
        ui_SchAutoManLabel5, ui_SchAutoManLabel6, ui_SchAutoManLabel7, ui_SchAutoManLabel8
    };

    for(uint8_t i = 0; i < MAX_WR; i++)
    {
        if(target != auto_manual_btns[i])
        {
            continue;
        }

        weekly_routines[i].auto_manual = lv_obj_has_state(auto_manual_btns[i], LV_STATE_CHECKED) ? 1U : 0U;
        if(UI_OBJ_READY(mode_labels[i]))
        {
            lv_label_set_text(mode_labels[i], weekly_routines[i].auto_manual ? "MANUAL" : "AUTO");
        }
        break;
    }
}

/**
 * @brief Event callback for schedule on/off switch changes
 * @details Maps the changed switch widget to its routine index and stores
 *          checked state into `weekly_routines[i].value`.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleSwithValueChangeFunc(lv_event_t * e)
{
    lv_obj_t *target = lv_event_get_current_target(e);
    if(target == NULL)
    {
        target = lv_event_get_target(e);
    }
    if(target == NULL)
    {
        return;
    }

    lv_obj_t *value_switches[MAX_WR] = {
        ui_SchSwitch1, ui_SchSwitch2, ui_SchSwitch3, ui_SchSwitch4,
        ui_SchSwitch5, ui_SchSwitch6, ui_SchSwitch7, ui_SchSwitch8
    };

    for(uint8_t i = 0; i < MAX_WR; i++)
    {
        if(target != value_switches[i])
        {
            continue;
        }

        weekly_routines[i].value = lv_obj_has_state(target, LV_STATE_CHECKED) ? 1U : 0U;
        break;
    }
}

/**
 * @brief Event callback for schedule setup update button
 * @details Copies routine names, auto/manual states, and switch values from
 *          schedule setup UI widgets into `weekly_routines`.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleSetupUpdateBtnFunc(lv_event_t * e)
{
    (void)e;

    lv_obj_t *name_fields[MAX_WR] = {
        ui_SchText1, ui_SchText2, ui_SchText3, ui_SchText4,
        ui_SchText5, ui_SchText6, ui_SchText7, ui_SchText8
    };
    lv_obj_t *auto_manual_btns[MAX_WR] = {
        ui_SchAutoMan1, ui_SchAutoMan2, ui_SchAutoMan3, ui_SchAutoMan4,
        ui_SchAutoMan5, ui_SchAutoMan6, ui_SchAutoMan7, ui_SchAutoMan8
    };
    lv_obj_t *value_switches[MAX_WR] = {
        ui_SchSwitch1, ui_SchSwitch2, ui_SchSwitch3, ui_SchSwitch4,
        ui_SchSwitch5, ui_SchSwitch6, ui_SchSwitch7, ui_SchSwitch8
    };

    for(uint8_t i = 0; i < MAX_WR; i++)
    {
        if(UI_OBJ_READY(name_fields[i]))
        {
            const char *name = lv_textarea_get_text(name_fields[i]);
            if(name == NULL)
            {
                weekly_routines[i].description[0] = '\0';
            }
            else
            {
                strncpy((char *)weekly_routines[i].description, name, sizeof(weekly_routines[i].description) - 1U);
                weekly_routines[i].description[sizeof(weekly_routines[i].description) - 1U] = '\0';
            }
        }

        if(UI_OBJ_READY(auto_manual_btns[i]))
        {
            weekly_routines[i].auto_manual = lv_obj_has_state(auto_manual_btns[i], LV_STATE_CHECKED) ? 1U : 0U;
        }

        if(UI_OBJ_READY(value_switches[i]))
        {
            weekly_routines[i].value = lv_obj_has_state(value_switches[i], LV_STATE_CHECKED) ? 1U : 0U;
        }
    }
    save_point_info(0);
}

/**
 * @brief Event callback for schedule keyboard input events
 * @details Intentionally unused. Schedule values are committed through
 *          explicit update/save button callbacks.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_ScheduleKeyboardPressFunc(lv_event_t * e)
{
    (void)e;
    // Not required if all changed updated with Update Button
}

/**
 * @brief Event callback for saving schedule time-grid edits
 * @details Parses all visible `HH : MM` cells and writes values into
 *          `wr_times[schedule][day].time[row]` for the selected routine.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SchSaveBtnFunc(lv_event_t * e)
{
    (void)e;

    lv_obj_t *time_cells[2 * MAX_INTERVALS_PER_DAY][7] = {
        { ui_ScheduleText1,  ui_ScheduleText2,  ui_ScheduleText3,  ui_ScheduleText4,  ui_ScheduleText5,  ui_ScheduleText6,  ui_ScheduleText7  },
        { ui_ScheduleText8,  ui_ScheduleText9,  ui_ScheduleText10, ui_ScheduleText11, ui_ScheduleText12, ui_ScheduleText13, ui_ScheduleText14 },
        { ui_ScheduleText15, ui_ScheduleText16, ui_ScheduleText17, ui_ScheduleText18, ui_ScheduleText19, ui_ScheduleText20, ui_ScheduleText21 },
        { ui_ScheduleText22, ui_ScheduleText23, ui_ScheduleText24, ui_ScheduleText25, ui_ScheduleText26, ui_ScheduleText27, ui_ScheduleText28 },
        { ui_ScheduleText29, ui_ScheduleText30, ui_ScheduleText31, ui_ScheduleText32, ui_ScheduleText33, ui_ScheduleText34, ui_ScheduleText35 },
        { ui_ScheduleText36, ui_ScheduleText37, ui_ScheduleText38, ui_ScheduleText39, ui_ScheduleText40, ui_ScheduleText41, ui_ScheduleText42 },
        { ui_ScheduleText43, ui_ScheduleText44, ui_ScheduleText45, ui_ScheduleText46, ui_ScheduleText47, ui_ScheduleText48, ui_ScheduleText49 },
        { ui_ScheduleText50, ui_ScheduleText51, ui_ScheduleText52, ui_ScheduleText53, ui_ScheduleText54, ui_ScheduleText55, ui_ScheduleText56 }
    };
    uint8_t schedule_index = 0;

    if(UI_OBJ_READY(ui_Dropdown9))
    {
        schedule_index = (uint8_t)lv_dropdown_get_selected(ui_Dropdown9);
    }
    if(schedule_index >= MAX_WR)
    {
        schedule_index = 0;
    }

    for(uint8_t row = 0; row < (2 * MAX_INTERVALS_PER_DAY); row++)
    {
        for(uint8_t day = 0; day < 7; day++)
        {
            uint8_t hour = 255U;
            uint8_t min = 255U;

            if(UI_OBJ_READY(time_cells[row][day]))
            {
                const char *txt = lv_textarea_get_text(time_cells[row][day]);
                if((txt != NULL) && (txt[0] != '\0'))
                {
                    const char *p = txt;
                    uint16_t h = 0U;
                    uint16_t m = 0U;

                    while(*p == ' ')
                    {
                        p++;
                    }
                    while((*p >= '0') && (*p <= '9'))
                    {
                        h = (uint16_t)((h * 10U) + (uint16_t)(*p - '0'));
                        p++;
                    }
                    while(*p == ' ')
                    {
                        p++;
                    }
                    if(*p == ':')
                    {
                        p++;
                    }
                    while(*p == ' ')
                    {
                        p++;
                    }
                    while((*p >= '0') && (*p <= '9'))
                    {
                        m = (uint16_t)((m * 10U) + (uint16_t)(*p - '0'));
                        p++;
                    }

                    if((h <= 255U) && (m <= 255U))
                    {
                        hour = (uint8_t)h;
                        min = (uint8_t)m;
                    }
                }
            }

            wr_times[schedule_index][day].time[row].hours = hour;
            wr_times[schedule_index][day].time[row].minutes = min;
        }
    }
}

/**
 * @brief Event callback for clearing all schedule grid cells
 * @details Sets every schedule time cell to `255 : 255` sentinel text, which
 *          represents an unused interval slot.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SchClearAllFunc(lv_event_t * e)
{
    (void)e;

    lv_obj_t *time_cells[2 * MAX_INTERVALS_PER_DAY][7] = {
        { ui_ScheduleText1,  ui_ScheduleText2,  ui_ScheduleText3,  ui_ScheduleText4,  ui_ScheduleText5,  ui_ScheduleText6,  ui_ScheduleText7  },
        { ui_ScheduleText8,  ui_ScheduleText9,  ui_ScheduleText10, ui_ScheduleText11, ui_ScheduleText12, ui_ScheduleText13, ui_ScheduleText14 },
        { ui_ScheduleText15, ui_ScheduleText16, ui_ScheduleText17, ui_ScheduleText18, ui_ScheduleText19, ui_ScheduleText20, ui_ScheduleText21 },
        { ui_ScheduleText22, ui_ScheduleText23, ui_ScheduleText24, ui_ScheduleText25, ui_ScheduleText26, ui_ScheduleText27, ui_ScheduleText28 },
        { ui_ScheduleText29, ui_ScheduleText30, ui_ScheduleText31, ui_ScheduleText32, ui_ScheduleText33, ui_ScheduleText34, ui_ScheduleText35 },
        { ui_ScheduleText36, ui_ScheduleText37, ui_ScheduleText38, ui_ScheduleText39, ui_ScheduleText40, ui_ScheduleText41, ui_ScheduleText42 },
        { ui_ScheduleText43, ui_ScheduleText44, ui_ScheduleText45, ui_ScheduleText46, ui_ScheduleText47, ui_ScheduleText48, ui_ScheduleText49 },
        { ui_ScheduleText50, ui_ScheduleText51, ui_ScheduleText52, ui_ScheduleText53, ui_ScheduleText54, ui_ScheduleText55, ui_ScheduleText56 }
    };

    for(uint8_t row = 0; row < (2 * MAX_INTERVALS_PER_DAY); row++)
    {
        for(uint8_t day = 0; day < 7; day++)
        {
            if(UI_OBJ_READY(time_cells[row][day]))
            {
                lv_textarea_set_text(time_cells[row][day], "255 : 255");
            }
        }
    }
}

/**
 * @brief Event callback for copying first-day schedule to all days
 * @details For each interval row, copies the first column time value to the
 *          remaining day columns.
 * @param[in] e LVGL event pointer containing event data
 * @return void
 */
void Event_Cb_SchCopyAllFunc(lv_event_t * e)
{
    (void)e;

    lv_obj_t *time_cells[2 * MAX_INTERVALS_PER_DAY][7] = {
        { ui_ScheduleText1,  ui_ScheduleText2,  ui_ScheduleText3,  ui_ScheduleText4,  ui_ScheduleText5,  ui_ScheduleText6,  ui_ScheduleText7  },
        { ui_ScheduleText8,  ui_ScheduleText9,  ui_ScheduleText10, ui_ScheduleText11, ui_ScheduleText12, ui_ScheduleText13, ui_ScheduleText14 },
        { ui_ScheduleText15, ui_ScheduleText16, ui_ScheduleText17, ui_ScheduleText18, ui_ScheduleText19, ui_ScheduleText20, ui_ScheduleText21 },
        { ui_ScheduleText22, ui_ScheduleText23, ui_ScheduleText24, ui_ScheduleText25, ui_ScheduleText26, ui_ScheduleText27, ui_ScheduleText28 },
        { ui_ScheduleText29, ui_ScheduleText30, ui_ScheduleText31, ui_ScheduleText32, ui_ScheduleText33, ui_ScheduleText34, ui_ScheduleText35 },
        { ui_ScheduleText36, ui_ScheduleText37, ui_ScheduleText38, ui_ScheduleText39, ui_ScheduleText40, ui_ScheduleText41, ui_ScheduleText42 },
        { ui_ScheduleText43, ui_ScheduleText44, ui_ScheduleText45, ui_ScheduleText46, ui_ScheduleText47, ui_ScheduleText48, ui_ScheduleText49 },
        { ui_ScheduleText50, ui_ScheduleText51, ui_ScheduleText52, ui_ScheduleText53, ui_ScheduleText54, ui_ScheduleText55, ui_ScheduleText56 }
    };

    for(uint8_t row = 0; row < (2 * MAX_INTERVALS_PER_DAY); row++)
    {
        if(!UI_OBJ_READY(time_cells[row][0]))
        {
            continue;
        }

        const char *first_col_text = lv_textarea_get_text(time_cells[row][0]);
        if(first_col_text == NULL)
        {
            continue;
        }

        for(uint8_t day = 1; day < 7; day++)
        {
            if(UI_OBJ_READY(time_cells[row][day]))
            {
                lv_textarea_set_text(time_cells[row][day], first_col_text);
            }
        }
    }
}


// End of file..










