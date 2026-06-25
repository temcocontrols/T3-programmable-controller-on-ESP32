#include "unity.h"

// Declare the test functions
extern void test_conver_by_unit_5v_new_tiny(void);
extern void test_conver_by_unit_5v_tiny_arm(void);
extern void test_conver_by_unit_10v_new_tiny(void);
extern void test_conver_by_unit_10v_small_arm_new_rev(void);
extern void test_conver_by_unit_10v_small_arm_old_rev(void);

extern void test_mqtt_send_cov_real_value(void);
extern void test_mqtt_send_cov_boolean_value(void);

extern void test_crc16_standard_packet(void);
extern void test_crc16_single_byte(void);
extern void test_crc16_empty_packet(void);

extern void test_sensirion_crc8_generation(void);
extern void test_sensirion_crc8_verification_success(void);
extern void test_sensirion_crc8_verification_failure(void);

extern void test_timegm_epoch_start(void);
extern void test_timegm_known_date(void);
extern void test_timegm_invalid_month(void);
extern void test_timegm_before_epoch(void);

// New Test Functions from test_bacnet_codecs.c
extern void test_bacnet_tag_encoding_decoding(void);
extern void test_bacnet_primitive_codecs(void);
extern void test_bacnet_string_codecs(void);
extern void test_bacnet_datetime_codecs(void);
extern void test_bacnet_low_level_packing(void);

// New Test Functions from test_math_drivers.c
extern void test_io_conversions_and_raw_access(void);
extern void test_mlx90632_calculation_formulas(void);
extern void test_ina228_register_scaling(void);

// New Test Functions from test_bacnet_objects.c
extern void test_analog_input_properties(void);
extern void test_analog_output_properties(void);
extern void test_analog_value_properties(void);
extern void test_binary_input_properties(void);
extern void test_binary_output_properties(void);
extern void test_binary_value_properties(void);
extern void test_device_properties(void);

// New Test Functions from test_system_data.c
extern void test_local_points_database(void);
extern void test_remote_and_network_points_database(void);
extern void test_extra_apis_suite(void);

#include "define.h"
#include "controls.h"
#include <string.h>

extern Str_in_point inputs[];
extern Str_out_point outputs[];
extern Str_variable_point vars[];
extern U8_T base_in;
extern U8_T base_out;
extern Str_in_point *new_inputs;
extern Str_out_point *new_outputs;
extern Str_variable_point *new_vars;
extern uint8_t max_inputs;
extern uint8_t max_outputs;
extern uint8_t max_vars;

void setUp(void) {
    base_in = 32;
    base_out = 32;
    max_inputs = 32;
    max_outputs = 32;
    max_vars = 128;
    new_inputs = inputs;
    new_outputs = outputs;
    new_vars = vars;
    for (int i = 0; i < 32; i++) {
        inputs[i].digital_analog = (i < 16) ? 1 : 0;
        strcpy((char*)inputs[i].description, "Mock Input");
        outputs[i].digital_analog = (i < 16) ? 1 : 0;
        strcpy((char*)outputs[i].description, "Mock Output");
    }
    for (int i = 0; i < 128; i++) {
        vars[i].digital_analog = (i < 64) ? 1 : 0;
        vars[i].range = 1;
        strcpy((char*)vars[i].description, "Mock Var");
    }

    extern void Count_IN_Object_Number(void);
    extern void Count_OUT_Object_Number(void);
    extern void Count_VAR_Object_Number(uint8_t base_var);
    Count_IN_Object_Number();
    Count_OUT_Object_Number();
    Count_VAR_Object_Number(max_vars);
}

void tearDown(void) {
    // Shared teardown code (if any)
}

int main(void)
{
    UNITY_BEGIN();

    // 1. IO Control Tests
    RUN_TEST(test_conver_by_unit_5v_new_tiny);
    RUN_TEST(test_conver_by_unit_5v_tiny_arm);
    RUN_TEST(test_conver_by_unit_10v_new_tiny);
    RUN_TEST(test_conver_by_unit_10v_small_arm_new_rev);
    RUN_TEST(test_conver_by_unit_10v_small_arm_old_rev);

    // 2. MQTT Tests
    RUN_TEST(test_mqtt_send_cov_real_value);
    RUN_TEST(test_mqtt_send_cov_boolean_value);

    // 3. Modbus Tests
    RUN_TEST(test_crc16_standard_packet);
    RUN_TEST(test_crc16_single_byte);
    RUN_TEST(test_crc16_empty_packet);

    // 4. Sensor CRC Tests
    RUN_TEST(test_sensirion_crc8_generation);
    RUN_TEST(test_sensirion_crc8_verification_success);
    RUN_TEST(test_sensirion_crc8_verification_failure);

    // 5. RTC/Time Tests
    RUN_TEST(test_timegm_epoch_start);
    RUN_TEST(test_timegm_known_date);
    RUN_TEST(test_timegm_invalid_month);
    RUN_TEST(test_timegm_before_epoch);

    // 6. BACnet Codec Tests
    RUN_TEST(test_bacnet_tag_encoding_decoding);
    RUN_TEST(test_bacnet_primitive_codecs);
    RUN_TEST(test_bacnet_string_codecs);
    RUN_TEST(test_bacnet_datetime_codecs);
    RUN_TEST(test_bacnet_low_level_packing);

    // 7. Math & Driver Tests
    RUN_TEST(test_io_conversions_and_raw_access);
    RUN_TEST(test_mlx90632_calculation_formulas);
    RUN_TEST(test_ina228_register_scaling);

    // 8. BACnet Object Property Tests
    RUN_TEST(test_analog_input_properties);
    RUN_TEST(test_analog_output_properties);
    RUN_TEST(test_analog_value_properties);
    RUN_TEST(test_binary_input_properties);
    RUN_TEST(test_binary_output_properties);
    RUN_TEST(test_binary_value_properties);
    RUN_TEST(test_device_properties);

    // 9. System Logic & Local DB Tests
    RUN_TEST(test_local_points_database);
    RUN_TEST(test_remote_and_network_points_database);

    // 10. Automatically Generated Extra APIs Verification
    RUN_TEST(test_extra_apis_suite);

    return UNITY_END();
}
