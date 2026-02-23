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
#include "define.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "wifi.h"
#include "ud_str.h"
#include "controls.h"
#include "rtc.h"
#include "modbus.h"
#include "user_data.h"
#include "sntp_app.h"

/* wifi scan state machine states */
typedef enum {
    WIFI_SCAN_IDLE = 0,
    WIFI_SCAN_CHECK_MODE,
    WIFI_SCAN_START,
    WIFI_SCAN_WAIT,
    WIFI_SCAN_READ
} wifi_scan_state_t;

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
static wifi_scan_state_t wifi_scan_state = WIFI_SCAN_IDLE;
static bool isScreenChanged = false;

static bool isTimeUpdated = false;
static bool isDateUpdated = false;
lv_calendar_date_t selected_date;
static lv_obj_t *selected_schedule_time_cell = NULL;

typedef enum
{
    PARAM_TABLE_INPUT = 0,
    PARAM_TABLE_OUTPUT,
    PARAM_TABLE_VARIABLE
} param_table_type_t;

typedef enum
{
    PARAM_BIND_DESCRIPTION = 0,
    PARAM_BIND_LABEL,
    PARAM_BIND_VALUE,
    PARAM_BIND_AUTO_MANUAL,
    PARAM_BIND_DIGITAL_ANALOG,
    PARAM_BIND_CONTROL,
    PARAM_BIND_SWITCH_STATUS,
    PARAM_BIND_RANGE
} param_bind_field_t;

typedef struct
{
    lv_obj_t *obj;
    uint16_t row;
    param_bind_field_t field;
} param_edit_bind_t;

#define PARAM_MAX_EDIT_BINDINGS (MAX_VARS * 8U)
#define PARAM_TABLE_BUILD_ROWS_PER_STEP 16U

typedef enum
{
    PARAM_TABLE_BUILD_IDLE = 0,
    PARAM_TABLE_BUILD_INIT,
    PARAM_TABLE_BUILD_HEADER,
    PARAM_TABLE_BUILD_ROWS
} param_table_build_state_t;

static param_table_type_t s_param_table_type = PARAM_TABLE_INPUT;
static lv_obj_t *s_param_table_root = NULL;
static param_edit_bind_t s_param_edit_binds[PARAM_MAX_EDIT_BINDINGS];
static uint16_t s_param_edit_bind_count = 0;
static param_table_build_state_t s_param_table_build_state = PARAM_TABLE_BUILD_IDLE;
static uint16_t s_param_table_build_next_row = 0;
static uint16_t s_param_table_build_total_rows = 0;
static uint32_t s_param_table_build_token = 0;
static lv_coord_t s_param_table_widths[9] = { 34, 160, 90, 80, 45, 45, 45, 45, 70 };
static lv_coord_t s_param_table_row_width = 0;

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
static void ui_set_wifi_visible(bool visible);

static void ui_update_textarea_from_int(lv_obj_t * obj, uint8_t value);
static uint8_t ui_get_int_from_textarea(lv_obj_t * obj);
static uint16_t param_table_get_row_count(void);
static void param_table_build_start(void);
static void param_table_build_async_step(void *user_data);
static void param_table_build_add_row(uint16_t i);
static lv_coord_t param_table_calc_row_width(void);
static void param_table_build(void);
static void param_table_apply_updates(void);
static void param_table_copy_text(char *dest, uint16_t dest_size, const char *src);
static int32_t param_table_get_int_from_textarea(lv_obj_t *obj);

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

    if((xTaskGetTickCount() - last_update_time) >= UI_DATA_UPDATE_INTERVAL_MS) // Update Data at 500 mS
    {
        last_update_time = xTaskGetTickCount();

        lv_obj_t * current_Screen = lv_screen_active();
        static lv_obj_t * prv_Screen = NULL;

        if(current_Screen != prv_Screen)
        {
            isScreenChanged = true;
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

    if(SSID_Info.IP_Wifi_Status == 0) // TODO: set wifi stage for all status.
    {
        ui_set_wifi_visible(false);
    }
    else
    {
        ui_set_wifi_visible(true);
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
 * @brief WiFi network scan state machine implementation
 * @details Manages the non-blocking WiFi AP scanning process through multiple states:
 *          - WIFI_SCAN_IDLE: Waiting state
 *          - WIFI_SCAN_CHECK_MODE: Verify WiFi is enabled
 *          - WIFI_SCAN_START: Initiate non-blocking scan
 *          - WIFI_SCAN_WAIT: Poll for scan completion
 *          - WIFI_SCAN_READ: Read and parse scan results into dropdown list
 * @return void
 * @note Non-blocking scan allows UI to remain responsive during WiFi scanning
 */
static void WifiScanStateMachine_Run(void)
{
    wifi_mode_t mode;
    uint16_t number;

    switch(wifi_scan_state)
    {
        case WIFI_SCAN_IDLE:
            break;

        // Check WiFi mode
        case WIFI_SCAN_CHECK_MODE:

            if(esp_wifi_get_mode(&mode) != ESP_OK || mode == WIFI_MODE_NULL)
            {
                lv_dropdown_set_options(ui_Dropdown2, "WiFi Disabled");
                wifi_scan_state = WIFI_SCAN_IDLE;
                break;
            }

            wifi_scan_state = WIFI_SCAN_START;
            break;

        // Start non-blocking scan
        case WIFI_SCAN_START:

            lv_dropdown_set_options(ui_Dropdown2, "Scanning...");
            esp_wifi_scan_start(NULL, false);  // non-blocking
            wifi_scan_state = WIFI_SCAN_WAIT;
            break;

        // Wait until scan completes (polling)
        case WIFI_SCAN_WAIT:

            if(esp_wifi_scan_get_ap_num(&number) == ESP_OK)
            {
                // When scan is done, AP count becomes available
                wifi_scan_state = WIFI_SCAN_READ;
            }

            break;

        // Read results
        case WIFI_SCAN_READ:
        {
            number = 0;
            esp_wifi_scan_get_ap_num(&number);

            if(number == 0)
            {
                lv_dropdown_set_options(ui_Dropdown2, "WiFi Not Available");
                wifi_scan_state = WIFI_SCAN_IDLE;
                break;
            }

            if(number > 20) number = 20;

            wifi_ap_record_t ap_info[20];
            esp_wifi_scan_get_ap_records(&number, ap_info);

            char list[512];
            list[0] = '\0';

            for(int i = 0; i < number; i++)
            {
                size_t used;
                int written;

                if(strlen((char*)ap_info[i].ssid) == 0)
                {
                    continue;
                }

                used = strlen(list);
                if(used >= (sizeof(list) - 1U))
                {
                    break;
                }

                written = snprintf(&list[used], sizeof(list) - used, "%s\n", (char*)ap_info[i].ssid);
                if((written < 0) || ((size_t)written >= (sizeof(list) - used)))
                {
                    break;
                }
            }

            lv_dropdown_set_options(ui_Dropdown2, list);
            lv_dropdown_set_selected(ui_Dropdown2, 0);

            wifi_scan_state = WIFI_SCAN_IDLE;
            break;
        }
    }
}
/**
 * @brief Refreshes the WiFi Configuration Screen data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details This function retrieves the most recent data related to WiFi configuration and calls the corresponding UI update functions to ensure that the WiFi Configuration Screen displays the current information.
 * @param[in] void No parameters
 * @return void
 * @note This function can be called whenever there is a need to refresh the WiFi Configuration Screen data, such as after a change in WiFi settings or when navigating to the WiFi Configuration Screen.
 */
static void lv_refresh_WifiConfig_Data(void)
{
    if (isScreenChanged == true)
    {
        wifi_scan_state = WIFI_SCAN_CHECK_MODE; // Start the WiFi scan state machine
        if(SSID_Info.MANUEL_EN == 1) //   Manual WiFi Config enabled
        {
            lv_obj_add_state(ui_WifiEnSw, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(ui_WifiEnSw, LV_STATE_CHECKED);
        }
    }
    else
    {
        WifiScanStateMachine_Run(); // Continue running the state machine to handle ongoing scan process
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
 * @brief Parse signed integer text from a parameter table textarea
 * @details Reads the text content from an LVGL textarea and converts it to
 *          a signed 32-bit integer using `atoi`. Returns `0` when the object
 *          is invalid or text is unavailable.
 * @param[in] obj Textarea object containing a numeric string
 * @return int32_t Parsed integer value, or `0` on invalid input
 */
static int32_t param_table_get_int_from_textarea(lv_obj_t *obj)
{
    if(!UI_OBJ_READY(obj))
    {
        return 0;
    }

    const char *txt = lv_textarea_get_text(obj);
    if(txt == NULL)
    {
        return 0;
    }

    return (int32_t)atoi(txt);
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

/**
 * @brief Register a UI object binding for deferred parameter-table apply
 * @details Stores the target object, row index, and field type in the edit
 *          binding array used later by `param_table_apply_updates()`. If the
 *          binding table is full or object is NULL, the request is ignored.
 * @param[in] obj UI object bound to a parameter field
 * @param[in] row Parameter table row index
 * @param[in] field Field selector (label/value/range/default)
 * @return void
 */
static void param_table_add_bind(lv_obj_t *obj, uint16_t row, param_bind_field_t field)
{
    if((obj == NULL) || (s_param_edit_bind_count >= PARAM_MAX_EDIT_BINDINGS))
    {
        return;
    }

    s_param_edit_binds[s_param_edit_bind_count].obj = obj;
    s_param_edit_binds[s_param_edit_bind_count].row = row;
    s_param_edit_binds[s_param_edit_bind_count].field = field;
    s_param_edit_bind_count++;
}

/**
 * @brief Create and configure a table row container
 * @details Returns a newly created LVGL object configured as a non-scrollable
 *          horizontal row with padding and styling matching the parameter table.
 * @param[in] parent Parent LVGL object
 * @param[in] height Row height in pixels
 * @return lv_obj_t* Pointer to the created row object
 */
static lv_obj_t *param_table_create_row(lv_obj_t *parent, lv_coord_t height)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, s_param_table_row_width);
    lv_obj_set_height(row, height);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_left(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(row, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return row;
}

/**
 * @brief Add a header label cell to a parameter table row
 * @param[in] row Parent row object
 * @param[in] txt Header text to display
 * @param[in] width Cell width in pixels
 * @return lv_obj_t* Created label object
 */
static lv_obj_t *param_table_add_header_cell(lv_obj_t *row, const char *txt, lv_coord_t width)
{
    lv_obj_t *cell = lv_label_create(row);
    lv_obj_set_width(cell, width);
    lv_label_set_text(cell, txt);
    lv_obj_set_style_text_font(cell, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    return cell;
}

/**
 * @brief Add an index label cell to a table row
 * @param[in] row Parent row
 * @param[in] index Zero-based index to display as 1-based
 * @param[in] width Cell width in pixels
 * @return lv_obj_t* Label object created for the index
 */
static lv_obj_t *param_table_add_index_cell(lv_obj_t *row, uint16_t index, lv_coord_t width)
{
    char buf[8];
    lv_obj_t *cell = lv_label_create(row);
    lv_obj_set_width(cell, width);
    snprintf(buf, sizeof(buf), "%u", (unsigned)(index + 1U));
    lv_label_set_text(cell, buf);
    return cell;
}

/**
 * @brief Add a one-line editable textarea cell to a parameter table row
 * @details Creates a textarea configured for either numeric-only input or
 *          general text input, and initializes it with the provided value.
 * @param[in] row Parent row object
 * @param[in] txt Initial text value (NULL allowed)
 * @param[in] width Cell width in pixels
 * @param[in] numeric True for numeric-only input, false for free text
 * @return lv_obj_t* Created textarea object
 */
static lv_obj_t *param_table_add_textarea_cell(lv_obj_t *row, const char *txt, lv_coord_t width, bool numeric)
{
    lv_obj_t *ta = lv_textarea_create(row);
    lv_obj_set_width(ta, width);
    lv_obj_set_height(ta, 22);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, numeric ? 12 : 24);
    if(numeric)
    {
        lv_textarea_set_accepted_chars(ta, "-0123456789");
    }
    lv_textarea_set_text(ta, (txt != NULL) ? txt : "");
    return ta;
}

/**
 * @brief Calculate total row width from configured column widths
 */
static lv_coord_t param_table_calc_row_width(void)
{
    lv_coord_t total = 0;
    for(uint8_t i = 0; i < 9U; i++)
    {
        total += s_param_table_widths[i];
    }

    /* Extra spacing for internal gaps/margins. */
    total += 20;
    return total;
}

/**
 * @brief Build and bind one parameter table row
 * @param[in] i Row index
 */
static void param_table_build_add_row(uint16_t i)
{
    char desc_buf[24] = {0};
    char label_buf[12] = {0};
    char num_buf[16];
    lv_obj_t *row = param_table_create_row(s_param_table_root, 26);

    param_table_add_index_cell(row, i, s_param_table_widths[0]);

    if(s_param_table_type == PARAM_TABLE_INPUT)
    {
        param_table_copy_text(desc_buf, sizeof(desc_buf), (const char *)inputs[i].description);
        param_table_copy_text(label_buf, sizeof(label_buf), (const char *)inputs[i].label);

        lv_obj_t *desc = param_table_add_textarea_cell(row, desc_buf, s_param_table_widths[1], false);
        lv_obj_t *label = param_table_add_textarea_cell(row, label_buf, s_param_table_widths[2], false);

        snprintf(num_buf, sizeof(num_buf), "%ld", (long)inputs[i].value);
        lv_obj_t *value = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[3], true);
        snprintf(num_buf, sizeof(num_buf), "%d", inputs[i].auto_manual);
        lv_obj_t *am = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[4], true);
        snprintf(num_buf, sizeof(num_buf), "%d", inputs[i].digital_analog);
        lv_obj_t *da = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[5], true);
        snprintf(num_buf, sizeof(num_buf), "%d", inputs[i].control);
        lv_obj_t *ctrl = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[6], true);
        snprintf(num_buf, sizeof(num_buf), "%d", inputs[i].range);
        lv_obj_t *range = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[8], true);

        param_table_add_bind(desc, i, PARAM_BIND_DESCRIPTION);
        param_table_add_bind(label, i, PARAM_BIND_LABEL);
        param_table_add_bind(value, i, PARAM_BIND_VALUE);
        param_table_add_bind(am, i, PARAM_BIND_AUTO_MANUAL);
        param_table_add_bind(da, i, PARAM_BIND_DIGITAL_ANALOG);
        param_table_add_bind(ctrl, i, PARAM_BIND_CONTROL);
        param_table_add_bind(range, i, PARAM_BIND_RANGE);
    }
    else if(s_param_table_type == PARAM_TABLE_OUTPUT)
    {
        param_table_copy_text(desc_buf, sizeof(desc_buf), (const char *)outputs[i].description);
        param_table_copy_text(label_buf, sizeof(label_buf), (const char *)outputs[i].label);

        lv_obj_t *desc = param_table_add_textarea_cell(row, desc_buf, s_param_table_widths[1], false);
        lv_obj_t *label = param_table_add_textarea_cell(row, label_buf, s_param_table_widths[2], false);

        snprintf(num_buf, sizeof(num_buf), "%ld", (long)outputs[i].value);
        lv_obj_t *value = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[3], true);
        snprintf(num_buf, sizeof(num_buf), "%d", outputs[i].auto_manual);
        lv_obj_t *am = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[4], true);
        snprintf(num_buf, sizeof(num_buf), "%d", outputs[i].digital_analog);
        lv_obj_t *da = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[5], true);
        snprintf(num_buf, sizeof(num_buf), "%d", outputs[i].control);
        lv_obj_t *ctrl = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[6], true);
        snprintf(num_buf, sizeof(num_buf), "%d", outputs[i].switch_status);
        lv_obj_t *sw = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[7], true);
        snprintf(num_buf, sizeof(num_buf), "%d", outputs[i].range);
        lv_obj_t *range = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[8], true);

        param_table_add_bind(desc, i, PARAM_BIND_DESCRIPTION);
        param_table_add_bind(label, i, PARAM_BIND_LABEL);
        param_table_add_bind(value, i, PARAM_BIND_VALUE);
        param_table_add_bind(am, i, PARAM_BIND_AUTO_MANUAL);
        param_table_add_bind(da, i, PARAM_BIND_DIGITAL_ANALOG);
        param_table_add_bind(ctrl, i, PARAM_BIND_CONTROL);
        param_table_add_bind(sw, i, PARAM_BIND_SWITCH_STATUS);
        param_table_add_bind(range, i, PARAM_BIND_RANGE);
    }
    else
    {
        param_table_copy_text(desc_buf, sizeof(desc_buf), (const char *)vars[i].description);
        param_table_copy_text(label_buf, sizeof(label_buf), (const char *)vars[i].label);

        lv_obj_t *desc = param_table_add_textarea_cell(row, desc_buf, s_param_table_widths[1], false);
        lv_obj_t *label = param_table_add_textarea_cell(row, label_buf, s_param_table_widths[2], false);

        snprintf(num_buf, sizeof(num_buf), "%ld", (long)vars[i].value);
        lv_obj_t *value = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[3], true);
        snprintf(num_buf, sizeof(num_buf), "%d", vars[i].auto_manual);
        lv_obj_t *am = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[4], true);
        snprintf(num_buf, sizeof(num_buf), "%d", vars[i].digital_analog);
        lv_obj_t *da = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[5], true);
        snprintf(num_buf, sizeof(num_buf), "%d", vars[i].control);
        lv_obj_t *ctrl = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[6], true);
        snprintf(num_buf, sizeof(num_buf), "%d", vars[i].range);
        lv_obj_t *range = param_table_add_textarea_cell(row, num_buf, s_param_table_widths[8], true);

        param_table_add_bind(desc, i, PARAM_BIND_DESCRIPTION);
        param_table_add_bind(label, i, PARAM_BIND_LABEL);
        param_table_add_bind(value, i, PARAM_BIND_VALUE);
        param_table_add_bind(am, i, PARAM_BIND_AUTO_MANUAL);
        param_table_add_bind(da, i, PARAM_BIND_DIGITAL_ANALOG);
        param_table_add_bind(ctrl, i, PARAM_BIND_CONTROL);
        param_table_add_bind(range, i, PARAM_BIND_RANGE);
    }
}

/**
 * @brief Non-blocking parameter table build step
 * @details Uses async callbacks to build the table in small chunks to keep UI responsive.
 * @param[in] user_data Build token
 */
static void param_table_build_async_step(void *user_data)
{
    uint32_t token = (uint32_t)(uintptr_t)user_data;
    if(token != s_param_table_build_token)
    {
        return;
    }

    if(!UI_OBJ_READY(ui_Panel4))
    {
        s_param_table_build_state = PARAM_TABLE_BUILD_IDLE;
        return;
    }

    if(s_param_table_build_state == PARAM_TABLE_BUILD_INIT)
    {
        if(UI_OBJ_READY(s_param_table_root))
        {
            lv_obj_del(s_param_table_root);
            s_param_table_root = NULL;
        }

        s_param_edit_bind_count = 0;
        s_param_table_build_total_rows = param_table_get_row_count();
        s_param_table_build_next_row = 0;
        s_param_table_row_width = param_table_calc_row_width();

        lv_obj_add_flag(ui_Panel4, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(ui_Panel4, (lv_dir_t)(LV_DIR_HOR | LV_DIR_VER));
        lv_obj_set_style_pad_all(ui_Panel4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_param_table_root = lv_obj_create(ui_Panel4);
        lv_obj_set_width(s_param_table_root, s_param_table_row_width);
        lv_obj_set_height(s_param_table_root, LV_SIZE_CONTENT);
        lv_obj_set_align(s_param_table_root, LV_ALIGN_TOP_LEFT);
        lv_obj_set_flex_flow(s_param_table_root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(s_param_table_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_remove_flag(s_param_table_root, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(s_param_table_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(s_param_table_root, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(s_param_table_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(s_param_table_root, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

        s_param_table_build_state = PARAM_TABLE_BUILD_HEADER;
        return;
    }

    if(s_param_table_build_state == PARAM_TABLE_BUILD_HEADER)
    {
        lv_obj_t *header = param_table_create_row(s_param_table_root, 22);

        if(s_param_table_type == PARAM_TABLE_INPUT)
        {
            param_table_add_header_cell(header, "No", s_param_table_widths[0]);
            param_table_add_header_cell(header, "Description", s_param_table_widths[1]);
            param_table_add_header_cell(header, "Label", s_param_table_widths[2]);
            param_table_add_header_cell(header, "Value", s_param_table_widths[3]);
            param_table_add_header_cell(header, "A/M", s_param_table_widths[4]);
            param_table_add_header_cell(header, "D/A", s_param_table_widths[5]);
            param_table_add_header_cell(header, "Ctrl", s_param_table_widths[6]);
            param_table_add_header_cell(header, "Range", s_param_table_widths[8]);
        }
        else if(s_param_table_type == PARAM_TABLE_OUTPUT)
        {
            param_table_add_header_cell(header, "No", s_param_table_widths[0]);
            param_table_add_header_cell(header, "Description", s_param_table_widths[1]);
            param_table_add_header_cell(header, "Label", s_param_table_widths[2]);
            param_table_add_header_cell(header, "Value", s_param_table_widths[3]);
            param_table_add_header_cell(header, "A/M", s_param_table_widths[4]);
            param_table_add_header_cell(header, "D/A", s_param_table_widths[5]);
            param_table_add_header_cell(header, "Ctrl", s_param_table_widths[6]);
            param_table_add_header_cell(header, "Sw", s_param_table_widths[7]);
            param_table_add_header_cell(header, "Range", s_param_table_widths[8]);
        }
        else
        {
            param_table_add_header_cell(header, "No", s_param_table_widths[0]);
            param_table_add_header_cell(header, "Description", s_param_table_widths[1]);
            param_table_add_header_cell(header, "Label", s_param_table_widths[2]);
            param_table_add_header_cell(header, "Value", s_param_table_widths[3]);
            param_table_add_header_cell(header, "A/M", s_param_table_widths[4]);
            param_table_add_header_cell(header, "D/A", s_param_table_widths[5]);
            param_table_add_header_cell(header, "Ctrl", s_param_table_widths[6]);
            param_table_add_header_cell(header, "Range", s_param_table_widths[8]);
        }

        s_param_table_build_state = PARAM_TABLE_BUILD_ROWS;
        return;
    }

    if(s_param_table_build_state == PARAM_TABLE_BUILD_ROWS)
    {
        uint8_t n = 0;
        while((s_param_table_build_next_row < s_param_table_build_total_rows) && (n < PARAM_TABLE_BUILD_ROWS_PER_STEP))
        {
            param_table_build_add_row(s_param_table_build_next_row);
            s_param_table_build_next_row++;
            n++;
        }

        if(s_param_table_build_next_row < s_param_table_build_total_rows)
        {
            return;
        }

        s_param_table_build_state = PARAM_TABLE_BUILD_IDLE;
        return;
    }
}

/**
 * @brief Start non-blocking parameter table build state machine
 */
static void param_table_build_start(void)
{
    s_param_table_build_token++;
    if(s_param_table_build_token == 0U)
    {
        s_param_table_build_token = 1U;
    }
    s_param_table_build_state = PARAM_TABLE_BUILD_INIT;
}

/**
 * @brief Build wrapper retained for existing call sites
 * @details Starts non-blocking state-machine build.
 */
static void param_table_build(void)
{
    param_table_build_start();
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
    uint16_t row_count = param_table_get_row_count();

    for(uint16_t i = 0; i < s_param_edit_bind_count; i++)
    {
        lv_obj_t *obj = s_param_edit_binds[i].obj;
        uint16_t row = s_param_edit_binds[i].row;
        param_bind_field_t field = s_param_edit_binds[i].field;
        int32_t v32;

        if((row >= row_count) || !UI_OBJ_READY(obj))
        {
            continue;
        }

        if(s_param_table_type == PARAM_TABLE_INPUT)
        {
            switch(field)
            {
                case PARAM_BIND_DESCRIPTION:
                    param_table_copy_text((char *)inputs[row].description, sizeof(inputs[row].description), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_LABEL:
                    param_table_copy_text((char *)inputs[row].label, sizeof(inputs[row].label), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_VALUE:
                    inputs[row].value = param_table_get_int_from_textarea(obj);
                    break;
                case PARAM_BIND_AUTO_MANUAL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    inputs[row].auto_manual = (int8_t)v32;
                    break;
                case PARAM_BIND_DIGITAL_ANALOG:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    inputs[row].digital_analog = (int8_t)v32;
                    break;
                case PARAM_BIND_CONTROL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    inputs[row].control = (int8_t)v32;
                    break;
                case PARAM_BIND_RANGE:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    inputs[row].range = (uint8_t)v32;
                    break;
                default:
                    break;
            }
        }
        else if(s_param_table_type == PARAM_TABLE_OUTPUT)
        {
            switch(field)
            {
                case PARAM_BIND_DESCRIPTION:
                    param_table_copy_text((char *)outputs[row].description, sizeof(outputs[row].description), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_LABEL:
                    param_table_copy_text((char *)outputs[row].label, sizeof(outputs[row].label), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_VALUE:
                    outputs[row].value = param_table_get_int_from_textarea(obj);
                    break;
                case PARAM_BIND_AUTO_MANUAL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    outputs[row].auto_manual = (int8_t)v32;
                    break;
                case PARAM_BIND_DIGITAL_ANALOG:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    outputs[row].digital_analog = (int8_t)v32;
                    break;
                case PARAM_BIND_CONTROL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    outputs[row].control = (int8_t)v32;
                    break;
                case PARAM_BIND_SWITCH_STATUS:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    outputs[row].switch_status = (uint8_t)v32;
                    break;
                case PARAM_BIND_RANGE:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    outputs[row].range = (int8_t)v32;
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(field)
            {
                case PARAM_BIND_DESCRIPTION:
                    param_table_copy_text((char *)vars[row].description, sizeof(vars[row].description), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_LABEL:
                    param_table_copy_text((char *)vars[row].label, sizeof(vars[row].label), lv_textarea_get_text(obj));
                    break;
                case PARAM_BIND_VALUE:
                    vars[row].value = param_table_get_int_from_textarea(obj);
                    break;
                case PARAM_BIND_AUTO_MANUAL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    vars[row].auto_manual = (uint8_t)v32;
                    break;
                case PARAM_BIND_DIGITAL_ANALOG:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    vars[row].digital_analog = (uint8_t)v32;
                    break;
                case PARAM_BIND_CONTROL:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    vars[row].control = (uint8_t)v32;
                    break;
                case PARAM_BIND_RANGE:
                    v32 = param_table_get_int_from_textarea(obj);
                    if(v32 < 0) v32 = 0;
                    if(v32 > 255) v32 = 255;
                    vars[row].range = (uint8_t)v32;
                    break;
                default:
                    break;
            }
        }
    }
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
        }
        else
        {
            lv_obj_add_state(ui_Checkbox2, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_GatewayPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(ui_SubnetPanel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(ui_IpPanel1, LV_OBJ_FLAG_CLICKABLE);
        }
        // --- Update IP Address UI ---
        ui_update_textarea_from_int(ui_IPoct1, Modbus.ip_addr[0]);
        ui_update_textarea_from_int(ui_IPoct2, Modbus.ip_addr[1]);
        ui_update_textarea_from_int(ui_IPoct3, Modbus.ip_addr[2]);
        ui_update_textarea_from_int(ui_IPoct4, Modbus.ip_addr[3]);

        // --- Update Subnet UI ---
        ui_update_textarea_from_int(ui_IPoct6, Modbus.subnet[0]);
        ui_update_textarea_from_int(ui_IPoct7, Modbus.subnet[1]);
        ui_update_textarea_from_int(ui_IPoct8, Modbus.subnet[2]);
        ui_update_textarea_from_int(ui_IPoct9, Modbus.subnet[3]);

        // --- Update Gateway UI ---
        ui_update_textarea_from_int(ui_IPoct10, Modbus.getway[0]);
        ui_update_textarea_from_int(ui_IPoct11, Modbus.getway[1]);
        ui_update_textarea_from_int(ui_IPoct12, Modbus.getway[2]);
        ui_update_textarea_from_int(ui_IPoct13, Modbus.getway[3]);
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
        sprintf(baud_buf, "%d", get_baud_val_from_enum(Modbus.baudrate[0]));
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
        }
        else                                            // Sync with pc
        {
            lv_obj_add_state(ui_SyncLocalPcCheckbox, LV_STATE_CHECKED);
        }

        char buf[32];

        // Update Date
        sprintf(buf, "%02d-%02d-%04d",
                rtc_date.day,
                rtc_date.month,
                (rtc_date.year-2000));

        lv_label_set_text(ui_Label70, buf);

        // Update Time
        memset(buf,0x00,sizeof(buf));
        sprintf(buf, "%02d : %02d" ,rtc_date.hour , rtc_date.minute);
        lv_label_set_text(ui_Label68, buf);

        if(Modbus.en_sntp >= 2 && Modbus.en_sntp < 5)
        {
            lv_dropdown_set_selected(ui_Dropdown5, Modbus.en_sntp - 2);
        }
        memset(buf,0x00,sizeof(buf));

        UN_Time RtcData;
        Get_RTC_by_timestamp(update_sntp_last_time,&RtcData,1);
        sprintf(buf, "%02d-%02d-%04d",
                RtcData.Clk.day,
                RtcData.Clk.mon,
                (RtcData.Clk.year-2000));
        lv_label_set_text(ui_Label19, buf);

        lv_calendar_set_today_date(ui_Calendar3, RtcData.Clk.year-2000, RtcData.Clk.mon,  RtcData.Clk.day);
        lv_calendar_set_showed_date(ui_Calendar3, RtcData.Clk.year-2000, RtcData.Clk.mon);

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
    if(s_param_table_build_state != PARAM_TABLE_BUILD_IDLE)
    {
        /* Drive non-blocking table creation from periodic refresh. */
        param_table_build_async_step((void *)(uintptr_t)s_param_table_build_token);
    }
}

/**
 * @brief Refreshes the Calendar data by fetching the latest values from the relevant data points and updating the UI components accordingly
 * @details Placeholder for Holiday Calendar screen refresh logic. This is the
 *          location to load calendar highlights/holiday dates and sync widget
 *          state when entering the calendar screen.
 * @param[in] void No parameters
 * @return void
 * @note Function currently has no active refresh behavior.
 */
static void lv_refresh_calender_Data(void)
{
    // TODO: Implement Holiday Calendar screen refresh logic.
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
    snprintf(buf, sizeof(buf), "%.1f", temp);

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
 * @brief Controls the visibility of the WiFi symbol on the LCD screen
 * @details Toggles the opacity of the WiFi indicator symbol between fully
 *          visible (opacity 255) and invisible (opacity 0)
 * @param[in] visible If true, makes WiFi symbol visible; if false, hides it
 * @return void
 * @note Checks if the UI object uic_WifiSymb is ready before updating
 * @note Uses LVGL opacity control (0 = invisible, 255 = fully opaque)
 */
static void ui_set_wifi_visible(bool visible)
{
    if(!UI_OBJ_READY(uic_WifiSymb))
        return;

    lv_obj_set_style_opa(uic_WifiSymb,
                         visible ? 255 : 0,
                         LV_PART_MAIN | LV_STATE_DEFAULT);
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
    if(SSID_Info.MANUEL_EN == 1 && !lv_obj_has_state(ui_WifiEnSw, LV_STATE_CHECKED))
    {
        // Disable manual WiFi config
        SSID_Info.MANUEL_EN = 0;
    }
    else if(SSID_Info.MANUEL_EN == 0 && lv_obj_has_state(ui_WifiEnSw, LV_STATE_CHECKED))
    {
        // Enable manual WiFi config
        SSID_Info.MANUEL_EN = 1;
    }
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

    memset(&SSID_Info.name,0x00,sizeof(SSID_Info.name));
    lv_dropdown_get_selected_str(ui_Dropdown2, SSID_Info.name, sizeof(SSID_Info.name));
    memset(&SSID_Info.password,0x00,sizeof(SSID_Info.password));
    if(password != NULL)
    {
        strncpy(SSID_Info.password, password, sizeof(SSID_Info.password) - 1U);
        SSID_Info.password[sizeof(SSID_Info.password) - 1U] = '\0';
    }
    // TODO: update wifi configuration in the system and attempt to connect to the selected network using the provided credentials
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
    uint16_t hour = 255U;
    uint16_t min = 255U;

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

            if((h <= 255U) && (m <= 255U))
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
    param_table_build();
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
    uint16_t hour_index = lv_roller_get_selected(ui_Dropdown11);
    uint16_t min_index  = lv_roller_get_selected(ui_Dropdown8);

    char buf[32] = {0};

    sprintf(buf, "%02d : %02d" ,hour_index , min_index);
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
    lv_obj_t * obj = lv_event_get_target(e);

    if(lv_calendar_get_pressed_date(obj, &selected_date) == LV_RES_OK)
    {
        char buf[32];

        sprintf(buf, "%02d-%02d-%04d",
                selected_date.day,
                selected_date.month,
                selected_date.year);

        // Update button label
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
            // ui_Checkbox2 is for Static IP = 1
            // ui_Checkbox3 is for Auto DHCP = 0
    if(lv_obj_has_state(ui_Checkbox2, LV_STATE_CHECKED))
    {
        // Select Static IP
        SSID_Info.IP_Auto_Manual = 1;
    }
    else if(lv_obj_has_state(ui_Checkbox3, LV_STATE_CHECKED))
    {
        // Select Auto DHCP
        SSID_Info.IP_Auto_Manual = 0;
    }

    if(SSID_Info.IP_Auto_Manual)
    {
            // --- Update IP Address ---
        Modbus.ip_addr[0] = ui_get_int_from_textarea( ui_IPoct1 );
        Modbus.ip_addr[1] = ui_get_int_from_textarea( ui_IPoct2 );
        Modbus.ip_addr[2] = ui_get_int_from_textarea( ui_IPoct3 );
        Modbus.ip_addr[3] = ui_get_int_from_textarea( ui_IPoct4 );

        // --- Update Subnet ---
        Modbus.subnet[0] = ui_get_int_from_textarea( ui_IPoct6 );
        Modbus.subnet[1] = ui_get_int_from_textarea( ui_IPoct7 );
        Modbus.subnet[2] = ui_get_int_from_textarea( ui_IPoct8 );
        Modbus.subnet[3] = ui_get_int_from_textarea( ui_IPoct9 );

        // --- Update Gateway ---
        Modbus.getway[0] = ui_get_int_from_textarea( ui_IPoct10 );
        Modbus.getway[1] = ui_get_int_from_textarea( ui_IPoct11 );
        Modbus.getway[2] = ui_get_int_from_textarea( ui_IPoct12 );
        Modbus.getway[3] = ui_get_int_from_textarea( ui_IPoct13 );
    }
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
            (rtc_date.year-2000));

    lv_label_set_text(ui_Label70, buf); // update time

            // Update Time
    memset(buf,0x00,sizeof(buf));
    sprintf(buf, "%02d : %02d" ,rtc_date.hour , rtc_date.minute);
    lv_label_set_text(ui_Label68, buf);
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
        uint16_t year = rtc_date.year;
        uint8_t month = rtc_date.month;
        uint8_t day = rtc_date.day;
        uint8_t hour = rtc_date.hour;
        uint8_t minute = rtc_date.minute;
        uint8_t second = rtc_date.second;

        if(isDateUpdated)
        {
            year = selected_date.year;
            month = (uint8_t)selected_date.month;
            day = (uint8_t)selected_date.day;
        }

        if(isTimeUpdated)
        {
            hour = (uint8_t)lv_roller_get_selected(ui_Dropdown11);
            minute = (uint8_t)lv_roller_get_selected(ui_Dropdown8);
            second = 0U;
        }

        Rtc_Set(year, month, day, hour, minute, second, 0);
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
            (RtcData.Clk.year-2000));
    lv_label_set_text(ui_Label19, buf);
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










