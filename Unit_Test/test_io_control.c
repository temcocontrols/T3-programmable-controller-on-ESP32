#include "unity.h"
#include "define.h"
#include "controls.h"

extern STR_MODBUS Modbus;

// Extern functions from main/io.c
extern U32_T conver_by_unit_5v(U32_T sample);
extern U32_T conver_by_unit_10v(U32_T sample);



void test_conver_by_unit_5v_new_tiny(void) {
    Modbus.mini_type = MINI_NEW_TINY; // 4
    uint32_t result = conver_by_unit_5v(512); // Half range of 1024
    // (5000 * 512) >> 10 = 2500
    TEST_ASSERT_EQUAL_UINT32(2500, result);
}

void test_conver_by_unit_5v_tiny_arm(void) {
    Modbus.mini_type = MINI_TINY_ARM; // 7
    uint32_t result = conver_by_unit_5v(1024); // Full range of 1024
    // (5000 * 1024) >> 10 = 5000
    TEST_ASSERT_EQUAL_UINT32(5000, result);
}

void test_conver_by_unit_10v_new_tiny(void) {
    Modbus.mini_type = MINI_NEW_TINY; // 4
    uint32_t result = conver_by_unit_10v(512);
    // (10000 * 512) >> 10 = 5000
    TEST_ASSERT_EQUAL_UINT32(5000, result);
}

void test_conver_by_unit_10v_small_arm_new_rev(void) {
    Modbus.mini_type = MINI_SMALL_ARM; // 6
    Modbus.hardRev = 6;
    uint32_t result = conver_by_unit_10v(1024);
    // (10000 * 1024) >> 10 = 10000
    TEST_ASSERT_EQUAL_UINT32(10000, result);
}

void test_conver_by_unit_10v_small_arm_old_rev(void) {
    Modbus.mini_type = MINI_SMALL_ARM; // 6
    Modbus.hardRev = 2; // Old rev (< 6)
    uint32_t result = conver_by_unit_10v(1024);
    // conver_by_unit_10v always returns (10000 * sample) >> 10 in this codebase
    TEST_ASSERT_EQUAL_UINT32(10000, result);
}
