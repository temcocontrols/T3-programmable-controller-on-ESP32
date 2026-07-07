#include "unity.h"
#include "bacnet.h"
#include "cov.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "device.h"
#include <string.h>
#include "define.h"
#include "controls.h"
#include "user_data.h"
#include <stdio.h>

extern bool Analog_Output_Object_Name(uint32_t object_instance, BACNET_CHARACTER_STRING * object_name);
extern bool Binary_Output_Object_Name(uint32_t object_instance, BACNET_CHARACTER_STRING * object_name);

void test_analog_input_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    // Test instance validation
    TEST_ASSERT_TRUE(Analog_Input_Valid_Instance(1));
    TEST_ASSERT_EQUAL_UINT32(1, Analog_Input_Index_To_Instance(0));

    // Test object name getter
    extern uint8_t base_in;
    extern char* get_label(uint8_t type, uint8_t num);
    extern Str_in_point inputs[];
    extern uint8_t Get_index_by_AIx(uint8_t ai_index, uint8_t *in_index);
    
    Str_points_ptr p_io = put_io_buf(IN, 0);
    
    uint8_t io_idx = 99;
    uint8_t res = Get_index_by_AIx(0, &io_idx);
    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Analog_Input_Object_Name(1, &name));
    TEST_ASSERT_TRUE(characterstring_length(&name) > 0);

    // Test encoding identifier property
    int len = Analog_Input_Encode_Property_APDU(apdu, 1, PROP_OBJECT_IDENTIFIER, 
                                                BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);

    // Test encoding name property
    memset(apdu, 0, sizeof(apdu));
    len = Analog_Input_Encode_Property_APDU(apdu, 1, PROP_OBJECT_NAME, 
                                            BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);

    // Test encoding type property
    memset(apdu, 0, sizeof(apdu));
    len = Analog_Input_Encode_Property_APDU(apdu, 1, PROP_OBJECT_TYPE, 
                                            BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_analog_output_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    TEST_ASSERT_TRUE(Analog_Output_Valid_Instance(1));
    TEST_ASSERT_EQUAL_UINT32(1, Analog_Output_Index_To_Instance(0));

    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Analog_Output_Object_Name(1, &name));

    int len = Analog_Output_Encode_Property_APDU(apdu, 1, PROP_OBJECT_IDENTIFIER, 
                                                 BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_analog_value_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    TEST_ASSERT_TRUE(Analog_Value_Valid_Instance(1));
    TEST_ASSERT_EQUAL_UINT32(1, Analog_Value_Index_To_Instance(0));

    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Analog_Value_Object_Name(1, &name));

    int len = Analog_Value_Encode_Property_APDU(apdu, 1, PROP_OBJECT_IDENTIFIER, 
                                                BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_binary_input_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    TEST_ASSERT_TRUE(Binary_Input_Valid_Instance(17));
    TEST_ASSERT_EQUAL_UINT32(17, Binary_Input_Index_To_Instance(0));

    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Binary_Input_Object_Name(17, &name));

    int len = Binary_Input_Read_Property(apdu, 17, PROP_OBJECT_IDENTIFIER, 
                                         BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_binary_output_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    TEST_ASSERT_TRUE(Binary_Output_Valid_Instance(17));
    TEST_ASSERT_EQUAL_UINT32(17, Binary_Output_Index_To_Instance(0));

    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Binary_Output_Object_Name(17, &name));

    int len = Binary_Output_Encode_Property_APDU(apdu, 17, PROP_OBJECT_IDENTIFIER, 
                                                 BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_binary_value_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    TEST_ASSERT_TRUE(Binary_Value_Valid_Instance(65));
    TEST_ASSERT_EQUAL_UINT32(65, Binary_Value_Index_To_Instance(0));

    BACNET_CHARACTER_STRING name;
    TEST_ASSERT_TRUE(Binary_Value_Object_Name(65, &name));

    int len = Binary_Value_Encode_Property_APDU(apdu, 65, PROP_OBJECT_IDENTIFIER, 
                                                BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}

void test_device_properties(void) {
    uint8_t apdu[128] = {0};
    BACNET_ERROR_CLASS error_class = 0;
    BACNET_ERROR_CODE error_code = 0;

    Device_Set_Object_Instance_Number(1028);
    TEST_ASSERT_TRUE(Device_Valid_Object_Instance_Number(1028));
    
    // Check device identifier encoding
    int len = Device_Encode_Property_APDU(apdu, 1028, PROP_OBJECT_IDENTIFIER, 
                                          BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);

    // Check device name encoding
    memset(apdu, 0, sizeof(apdu));
    len = Device_Encode_Property_APDU(apdu, 1028, PROP_OBJECT_NAME, 
                                      BACNET_ARRAY_ALL, &error_class, &error_code);
    TEST_ASSERT_TRUE(len > 0);
}
