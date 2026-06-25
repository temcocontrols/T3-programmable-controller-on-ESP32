#include "unity.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Include config and product headers
#include "sdkconfig.h"
#include "define.h"
#include "ProductModel.h"
#include "ud_str.h"
#include "user_data.h"
#include "bacdef.h"
#include "bacenum.h"
#include "bacapp.h"
#include "bacstr.h"
#include "datetime.h"
#include "address.h"
#include "cov.h"
#include "abort.h"
#include "bacerror.h"
#include "reject.h"
#include "fifo.h"
#include "ringbuf.h"
#include "bactext.h"
#include "indtext.h"
#include "bacprop.h"

extern void eth_start(void);
extern void save_LSW_ON_OFF_TIME(uint8_t index,uint16_t time);
extern void Flash_Inital(void);
extern void Set_Object_Name(char * name);
extern void Save_SNTP_sever(void);
extern void Save_Email_Setting(void);
extern void Save_MSV(void);
extern void save_TemcoAV_AIRALB(uint16_t index, uint16_t value);
extern uint16_t get_TemcoAVS_airlab(uint8_t index);
extern void Initial_points(uint8_t point_type);
extern void read_point_info(void);
extern uint16_t Filter(uint8_t channel,uint16_t input);
extern void adc_init(void);
extern float SHT10_CalcuDewPoint(float t, float h);
extern float SHT10_CalcuEnthalpy(float t, float h);
extern float SHT10_CalcuAbsHumi(float t, float h);
extern void transducer_switch_init(void);
extern void led_init(void);
extern void led_pwm_init(void);
extern U8_T change_value_by_range(U8_T channel);
extern void Set_Input_Type(U8_T point);
extern U16_T get_input_raw(U8_T point);
extern void set_output_raw(U8_T point,U16_T value);
extern U16_T get_output_raw(U8_T point);
extern U32_T conver_by_unit_5v(U32_T sample);
extern U32_T conver_by_unit_custable(U8_T point,U32_T sample);
extern U8_T get_max_input(void);
extern U8_T get_max_output(void);
extern U8_T get_max_internal_input(void);
extern U8_T get_max_internal_output(void);
extern uint32_t get_high_spd_counter(uint8_t point);
extern void Store_Pulse_Counter(uint8 flag);
extern void initial_HSP(void);
extern void clear_pulse_counter(uint8_t i);
extern void Check_Pulse_Counter(void);
extern void calculate_RPM(void);
extern uint32_t get_rpm(uint8_t point);
extern void map_extern_output(U8_T point);
extern S8_T check_external_in_on_line(U8_T index);
extern S8_T check_external_out_on_line(U8_T index);
extern int mlx90632_start_measurement(void);
extern double mlx90632_calc_temp_object_reflected(int32_t object, int32_t ambient, double reflected,
                                           int32_t Ea, int32_t Eb, int32_t Ga, int32_t Fa, int32_t Fb,
                                           int16_t Ha, int16_t Hb);

void test_extra_api_eth_start(void) {
    printf("[CRASH_DETECTOR] test_extra_api_eth_start\n");
    fflush(stdout);
    eth_start();
}

void test_extra_api_save_LSW_ON_OFF_TIME(void) {
    printf("[CRASH_DETECTOR] test_extra_api_save_LSW_ON_OFF_TIME\n");
    fflush(stdout);
    save_LSW_ON_OFF_TIME((uint8_t){0}, (uint16_t){0});
}

void test_extra_api_Flash_Inital(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Flash_Inital\n");
    fflush(stdout);
    Flash_Inital();
}

void test_extra_api_Set_Object_Name(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Set_Object_Name\n");
    fflush(stdout);
    Set_Object_Name(NULL);
}

void test_extra_api_Save_SNTP_sever(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Save_SNTP_sever\n");
    fflush(stdout);
    Save_SNTP_sever();
}

void test_extra_api_Save_Email_Setting(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Save_Email_Setting\n");
    fflush(stdout);
    Save_Email_Setting();
}

void test_extra_api_Save_MSV(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Save_MSV\n");
    fflush(stdout);
    Save_MSV();
}

void test_extra_api_save_TemcoAV_AIRALB(void) {
    printf("[CRASH_DETECTOR] test_extra_api_save_TemcoAV_AIRALB\n");
    fflush(stdout);
    save_TemcoAV_AIRALB((uint16_t){0}, (uint16_t){0});
}

void test_extra_api_get_TemcoAVS_airlab(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_TemcoAVS_airlab\n");
    fflush(stdout);
    get_TemcoAVS_airlab((uint8_t){0});
}

void test_extra_api_Initial_points(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Initial_points\n");
    fflush(stdout);
    Initial_points((uint8_t){0});
}

void test_extra_api_read_point_info(void) {
    printf("[CRASH_DETECTOR] test_extra_api_read_point_info\n");
    fflush(stdout);
    read_point_info();
}

void test_extra_api_Filter(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Filter\n");
    fflush(stdout);
    Filter((uint8_t){0}, (uint16_t){0});
}

void test_extra_api_adc_init(void) {
    printf("[CRASH_DETECTOR] test_extra_api_adc_init\n");
    fflush(stdout);
    adc_init();
}

void test_extra_api_SHT10_CalcuDewPoint(void) {
    printf("[CRASH_DETECTOR] test_extra_api_SHT10_CalcuDewPoint\n");
    fflush(stdout);
    SHT10_CalcuDewPoint((float){0}, (float){0});
}

void test_extra_api_SHT10_CalcuEnthalpy(void) {
    printf("[CRASH_DETECTOR] test_extra_api_SHT10_CalcuEnthalpy\n");
    fflush(stdout);
    SHT10_CalcuEnthalpy((float){0}, (float){0});
}

void test_extra_api_SHT10_CalcuAbsHumi(void) {
    printf("[CRASH_DETECTOR] test_extra_api_SHT10_CalcuAbsHumi\n");
    fflush(stdout);
    SHT10_CalcuAbsHumi((float){0}, (float){0});
}

void test_extra_api_transducer_switch_init(void) {
    printf("[CRASH_DETECTOR] test_extra_api_transducer_switch_init\n");
    fflush(stdout);
    transducer_switch_init();
}

void test_extra_api_led_init(void) {
    printf("[CRASH_DETECTOR] test_extra_api_led_init\n");
    fflush(stdout);
    led_init();
}

void test_extra_api_led_pwm_init(void) {
    printf("[CRASH_DETECTOR] test_extra_api_led_pwm_init\n");
    fflush(stdout);
    led_pwm_init();
}

void test_extra_api_change_value_by_range(void) {
    printf("[CRASH_DETECTOR] test_extra_api_change_value_by_range\n");
    fflush(stdout);
    change_value_by_range((U8_T){0});
}

void test_extra_api_Set_Input_Type(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Set_Input_Type\n");
    fflush(stdout);
    Set_Input_Type((U8_T){0});
}

void test_extra_api_get_input_raw(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_input_raw\n");
    fflush(stdout);
    get_input_raw((U8_T){0});
}

void test_extra_api_set_output_raw(void) {
    printf("[CRASH_DETECTOR] test_extra_api_set_output_raw\n");
    fflush(stdout);
    set_output_raw((U8_T){0}, (U16_T){0});
}

void test_extra_api_get_output_raw(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_output_raw\n");
    fflush(stdout);
    get_output_raw((U8_T){0});
}

void test_extra_api_conver_by_unit_5v(void) {
    printf("[CRASH_DETECTOR] test_extra_api_conver_by_unit_5v\n");
    fflush(stdout);
    conver_by_unit_5v((U32_T){0});
}

void test_extra_api_conver_by_unit_custable(void) {
    printf("[CRASH_DETECTOR] test_extra_api_conver_by_unit_custable\n");
    fflush(stdout);
    conver_by_unit_custable((U8_T){0}, (U32_T){0});
}

void test_extra_api_get_max_input(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_max_input\n");
    fflush(stdout);
    get_max_input();
}

void test_extra_api_get_max_output(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_max_output\n");
    fflush(stdout);
    get_max_output();
}

void test_extra_api_get_max_internal_input(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_max_internal_input\n");
    fflush(stdout);
    get_max_internal_input();
}

void test_extra_api_get_max_internal_output(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_max_internal_output\n");
    fflush(stdout);
    get_max_internal_output();
}

void test_extra_api_get_high_spd_counter(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_high_spd_counter\n");
    fflush(stdout);
    get_high_spd_counter((uint8_t){0});
}

void test_extra_api_Store_Pulse_Counter(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Store_Pulse_Counter\n");
    fflush(stdout);
    Store_Pulse_Counter((uint8){0});
}

void test_extra_api_initial_HSP(void) {
    printf("[CRASH_DETECTOR] test_extra_api_initial_HSP\n");
    fflush(stdout);
    initial_HSP();
}

void test_extra_api_clear_pulse_counter(void) {
    printf("[CRASH_DETECTOR] test_extra_api_clear_pulse_counter\n");
    fflush(stdout);
    clear_pulse_counter((uint8_t){0});
}

void test_extra_api_Check_Pulse_Counter(void) {
    printf("[CRASH_DETECTOR] test_extra_api_Check_Pulse_Counter\n");
    fflush(stdout);
    Check_Pulse_Counter();
}

void test_extra_api_calculate_RPM(void) {
    printf("[CRASH_DETECTOR] test_extra_api_calculate_RPM\n");
    fflush(stdout);
    calculate_RPM();
}

void test_extra_api_get_rpm(void) {
    printf("[CRASH_DETECTOR] test_extra_api_get_rpm\n");
    fflush(stdout);
    get_rpm((uint8_t){0});
}

void test_extra_api_map_extern_output(void) {
    printf("[CRASH_DETECTOR] test_extra_api_map_extern_output\n");
    fflush(stdout);
    map_extern_output((U8_T){0});
}

void test_extra_api_check_external_in_on_line(void) {
    printf("[CRASH_DETECTOR] test_extra_api_check_external_in_on_line\n");
    fflush(stdout);
    check_external_in_on_line((U8_T){0});
}

void test_extra_api_check_external_out_on_line(void) {
    printf("[CRASH_DETECTOR] test_extra_api_check_external_out_on_line\n");
    fflush(stdout);
    check_external_out_on_line((U8_T){0});
}

void test_extra_api_mlx90632_start_measurement(void) {
    printf("[CRASH_DETECTOR] test_extra_api_mlx90632_start_measurement\n");
    fflush(stdout);
    mlx90632_start_measurement();
}

void test_extra_api_mlx90632_calc_temp_object_reflected(void) {
    printf("[CRASH_DETECTOR] test_extra_api_mlx90632_calc_temp_object_reflected\n");
    fflush(stdout);
    mlx90632_calc_temp_object_reflected((int32_t){0}, (int32_t){0}, (double){0}, (int32_t){0}, (int32_t){0}, (int32_t){0}, (int32_t){0}, (int32_t){0}, (int16_t){0}, (int16_t){0});
}

void test_extra_apis_suite(void) {
    test_extra_api_eth_start();
    test_extra_api_save_LSW_ON_OFF_TIME();
    test_extra_api_Flash_Inital();
    test_extra_api_Set_Object_Name();
    test_extra_api_Save_SNTP_sever();
    test_extra_api_Save_Email_Setting();
    test_extra_api_Save_MSV();
    test_extra_api_save_TemcoAV_AIRALB();
    test_extra_api_get_TemcoAVS_airlab();
    test_extra_api_Initial_points();
    test_extra_api_read_point_info();
    test_extra_api_Filter();
    test_extra_api_adc_init();
    test_extra_api_SHT10_CalcuDewPoint();
    test_extra_api_SHT10_CalcuEnthalpy();
    test_extra_api_SHT10_CalcuAbsHumi();
    test_extra_api_transducer_switch_init();
    test_extra_api_led_init();
    test_extra_api_led_pwm_init();
    test_extra_api_change_value_by_range();
    test_extra_api_Set_Input_Type();
    test_extra_api_get_input_raw();
    test_extra_api_set_output_raw();
    test_extra_api_get_output_raw();
    test_extra_api_conver_by_unit_5v();
    test_extra_api_conver_by_unit_custable();
    test_extra_api_get_max_input();
    test_extra_api_get_max_output();
    test_extra_api_get_max_internal_input();
    test_extra_api_get_max_internal_output();
    test_extra_api_get_high_spd_counter();
    test_extra_api_Store_Pulse_Counter();
    test_extra_api_initial_HSP();
    test_extra_api_clear_pulse_counter();
    test_extra_api_Check_Pulse_Counter();
    test_extra_api_calculate_RPM();
    test_extra_api_get_rpm();
    test_extra_api_map_extern_output();
    test_extra_api_check_external_in_on_line();
    test_extra_api_check_external_out_on_line();
    test_extra_api_mlx90632_start_measurement();
    test_extra_api_mlx90632_calc_temp_object_reflected();
}
