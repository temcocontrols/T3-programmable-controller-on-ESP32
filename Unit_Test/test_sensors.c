#include "unity.h"
#include <stdint.h>

// Extern functions from main/sensirion_common.c
extern uint8_t sensirion_i2c_generate_crc(const uint8_t* data, uint16_t count);
extern int8_t sensirion_i2c_check_crc(const uint8_t* data, uint16_t count, uint8_t checksum);



void test_sensirion_crc8_generation(void) {
    // Standard test vector for Sensirion CRC-8:
    // For 2 bytes of data: 0xBE 0xEF, the expected CRC-8 checksum is 0x92.
    uint8_t data[] = {0xBE, 0xEF};
    uint8_t expected_checksum = 0x92;
    uint8_t actual_checksum = sensirion_i2c_generate_crc(data, sizeof(data));
    
    TEST_ASSERT_EQUAL_UINT8(expected_checksum, actual_checksum);
}

void test_sensirion_crc8_verification_success(void) {
    uint8_t data[] = {0xBE, 0xEF};
    uint8_t checksum = 0x92;
    int8_t status = sensirion_i2c_check_crc(data, sizeof(data), checksum);
    
    // Returns 0 on success in sensirion_i2c_check_crc
    TEST_ASSERT_EQUAL_INT8(0, status);
}

void test_sensirion_crc8_verification_failure(void) {
    uint8_t data[] = {0xBE, 0xEF};
    uint8_t checksum = 0x00; // Incorrect checksum
    int8_t status = sensirion_i2c_check_crc(data, sizeof(data), checksum);
    
    // Returns non-zero (often -1) on failure
    TEST_ASSERT_NOT_EQUAL_INT8(0, status);
}
