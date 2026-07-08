#include <stdint.h>
#include "user_data.h"
#include "../../main/define.h"
#include "wifi.h"
#include "rtc.h"
#include "scan.h"

// Define array dimensions
#ifndef MAX_INS
#define MAX_INS 32
#endif
#ifndef MAX_OUTS
#define MAX_OUTS 32
#endif
#ifndef MAX_VARS
#define MAX_VARS 128
#endif

// Global tables inputs, outputs, vars removed to avoid duplicate symbol collision with production definitions.

// Define BACnet Instance ID
uint32_t Instance = 1028;

// Mock variables to resolve linking errors
uint8_t Update_Sntp_Retry = 0;
uint8_t flag_Update_Sntp = 0;
Byte Station_NUM = 0;
lcdconfig display_lcd = {0};
U8_T prg_code[MAX_PRGS][MAX_CODE * CODE_ELEMENT] = {{0}};
U8_T wr_time_on_off[8][9][8] = {{{0}}};
Str_controller_point controllers[MAX_CONS] = {{0}};
U8_T ar_dates[MAX_AR][AR_DATES_SIZE] = {{0}};
Wr_one_day wr_times[MAX_WR][MAX_SCHEDULES_PER_WEEK] = {{{0}}};
Str_annual_routine_point annual_routines[MAX_AR] = {{0}};
Str_weekly_routine_point weekly_routines[MAX_WR] = {{0}};
Str_program_point programs[MAX_PRGS] = {{0}};
Str_Setting_Info Setting_Info = {0};
// max_outputs, max_inputs, max_vars removed to avoid duplicate symbol collision.
PCF_DateTime rtc_date = {0};
uint16 READ_POINT_TIMER = 0;
U8_T Daylight_Saving_Time = 0;
uint16_t input_cal[16] = {0};
U16_T SW_REV = 0;
uint8 reg_num = 0;
U8_T bacnet_to_modbus[300] = {0};
uint8 flagLED_main_rx = 0;
uint8 led_main_rx = 0;
uint32_t system_timer = 0;
U16_T modbus_send_len = 0;
u8 modbus_send_buf[500] = {0};
uint8 flagLED_sub_rx = 0;
U32_T com_rx[3] = {0};
uint8 led_sub_rx = 0;
STR_Task_Test task_test = {0};
STR_SSID SSID_Info = {0};
U8_T sub_no = 0;
uint32_t run_time = 0;
SCAN_DB scan_db[MAX_ID] = {{0}};
U8_T current_online[32] = {0};
U8_T lcddisplay[7] = {0};
uint8_t webview_json_flash = 0;
U8_T Send_I_Am_Flag = 0;
U8_T Send_Whois_Flag = 0;
char panelname[20] = {0};
uint8_t ChangeFlash = 0;
U16_T output_raw[MAX_OUTS] = {0};
// base_out removed to avoid duplicate symbol collision.
float output_priority[MAX_OUTS][16] = {{0}};
Units_element digi_units[MAX_DIG_UNIT] = {{0}};
uint8_t flag_updating = 0;
uint8_t panel_number = 0;
uint8_t count_gIdentify = 0;
uint8_t gIdentify = 0;
uint16_t count_lcd_time_off_delay = 0;
uint16 READ_POINT_TIMER_FROM_EEP = 0;
char sntp_server[30] = {0};
uint32_t count_sntp = 0;
STR_MAP_table sub_map[SUB_NO] = {{0}};
// base_in removed to avoid duplicate symbol collision.
uint16_t input_raw[MAX_INS] = {0};
uint8_t input_type[32] = {0};
uint8_t InputLed[32] = {0};
uint8_t flag_internal_temperature = 0;

Str_in_point *new_inputs = NULL;
Str_out_point *new_outputs = NULL;
Str_variable_point *new_vars = NULL;

Str_points_ptr put_io_buf(Point_type_equate type, uint8 point) {
    Str_points_ptr ptr;
    memset(&ptr, 0, sizeof(Str_points_ptr));
    if (type == IN) {
        ptr.pin = &inputs[point];
    } else if (type == OUT) {
        ptr.pout = &outputs[point];
    } else if (type == VAR) {
        ptr.pvar = &vars[point];
    }
    return ptr;
}

int Get_Number_by_Bacnet_Index(uint8_t type, uint8_t index) {
    (void)type;
    return index;
}
