#include "unity.h"
#include <stdint.h>

// Extern functions from main/modbus.c
extern uint16_t crc16(uint8_t *p, uint8_t length);
extern void init_crc16(void);
extern void crc16_byte(uint8_t ch);



void test_crc16_standard_packet(void) {
    // Standard Modbus Request: Device 1, Read Holding Registers (03), Start 0x0000, Count 10 (0x000A)
    uint8_t packet[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    
    // Expected CRC: 0xC5CD (matching the production crc16 implementation)
    uint16_t expected_crc = 0xC5CD;
    uint16_t actual_crc = crc16(packet, sizeof(packet));
    
    TEST_ASSERT_EQUAL_UINT16(expected_crc, actual_crc);
}

void test_crc16_single_byte(void) {
    uint8_t packet[] = {0x05};
    uint16_t expected_crc = 0x7F43;
    uint16_t actual_crc = crc16(packet, sizeof(packet));
    
    TEST_ASSERT_EQUAL_UINT16(expected_crc, actual_crc);
}

void test_crc16_empty_packet(void) {
    uint8_t packet[] = {0};
    uint16_t actual_crc = crc16(packet, 0);
    // Uninitialized CRC value (starts at 0xFFFF in standard Modbus CRC)
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, actual_crc);
}
