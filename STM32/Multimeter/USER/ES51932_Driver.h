#ifndef ES51932_DRIVER_H
#define ES51932_DRIVER_H

#include "stm32f10x.h"
#include "stm32f10x_usart.h"

#define ES51932_USART USART3
#define ES51932_BAUDRATE 19230

#define SELECT_DC 0
#define SELECT_AC 1

// Function definitions
#define FUNCTION_VOLTAGE             59 // 0b0111011
#define FUNCTION_AUTO_UA_CURRENT     61 // 0b0111101
#define FUNCTION_AUTO_MA_CURRENT     63 // 0b0111111
#define FUNCTION_22A_CURRENT         48 // 0b0110000
#define FUNCTION_MANUAL_A_CURRENT    57 // 0b0111001
#define FUNCTION_RESISTANCE          51 // 0b0110011
#define FUNCTION_CONTINUITY          53 // 0b0110101
#define FUNCTION_DIOE                49 // 0b0110001
#define FUNCTION_FREQUENCY           50 // 0b0110010
#define FUNCTION_CAPACITANCE         54 // 0b0110110
#define FUNCTION_TEMPERATURE         52 // 0b0110100
#define FUNCTION_ADP                 62 // 0b0111110

// Range definitions
#define RANGE_2_2000V                48 // 0b0110000
#define RANGE_22_000V                49 // 0b0110001
#define RANGE_220_00V                50 // 0b0110010
#define RANGE_2200_0V                51 // 0b0110011
#define RANGE_220_00MV               52 // 0b0110100
#define RANGE_2_2000A                48 // 0b0110000
#define RANGE_22_000A                49 // 0b0110001
#define RANGE_220_00A                50 // 0b0110010
#define RANGE_2200_0A                51 // 0b0110011
#define RANGE_22000A                 52 // 0b0110100
#define RANGE_LOWER_AUTO_A           48 // 0b0110000
#define RANGE_HIGHER_AUTO_A          49 // 0b0110001
#define RANGE_220_00_OHM             48 // 0b0110000
#define RANGE_2_2000K_OHM            49 // 0b0110001
#define RANGE_22_000K_OHM            50 // 0b0110010
#define RANGE_220_00K_OHM            51 // 0b0110011
#define RANGE_2_2000M_OHM            52 // 0b0110100
#define RANGE_22_000M_OHM            53 // 0b0110101
#define RANGE_220_00M_OHM            54 // 0b0110110

// Digit definitions for LCD panel
#define DIGIT_0                      48 // 0b0110000
#define DIGIT_1                      49 // 0b0110001
#define DIGIT_2                      50 // 0b0110010
#define DIGIT_3                      51 // 0b0110011
#define DIGIT_4                      52 // 0b0110100
#define DIGIT_5                      53 // 0b0110101
#define DIGIT_6                      54 // 0b0110110
#define DIGIT_7                      55 // 0b0110111
#define DIGIT_8                      56 // 0b0111000
#define DIGIT_9                      57 // 0b0111001

// Mode definitions
#define MODE_DC_VOLTAGE_MEASUREMENT              0
#define MODE_AUTO_DC_CURRENT_MEASUREMENT_UA      1
#define MODE_AUTO_DC_CURRENT_MEASUREMENT_MA      2
#define MODE_322A_DC_CURRENT_MEASUREMENT_A       3
#define MODE_DC_220MV                            4
#define MODE_MANUAL_DC_22A                       5
#define MODE_MANUAL_DC_220A                      6
#define MODE_MANUAL_DC_2200A                     7
#define MODE_MANUAL_DC_22000A                    8
#define MODE_RESISTANCE_MEASUREMENT              9
#define MODE_CONTINUITY_CHECK                    10
#define MODE_DIODE_MEASUREMENT                   11
#define MODE_FREQUENCY_MEASUREMENT               12
#define MODE_CAPACITANCE_MEASUREMENT             13
#define MODE_TEMPERATURE_MEASUREMENT_C           14
#define MODE_RESISTANCE_MEASUREMENT_ALT          15
#define MODE_AC_VOLTAGE_MEASUREMENT              16
#define MODE_AUTO_AC_CURRENT_MEASUREMENT_UA      17
#define MODE_AUTO_AC_CURRENT_MEASUREMENT_MA      18
#define MODE_322A_AC_CURRENT_MEASUREMENT_A       19
#define MODE_AC_220MV                            20
#define MODE_MANUAL_AC_22A                       21
#define MODE_MANUAL_AC_220A                      22
#define MODE_MANUAL_AC_2200A                     23
#define MODE_MANUAL_AC_22000A                    24
#define MODE_ADP0                                25
#define MODE_ADP1                                26
#define MODE_ADP2                                27
#define MODE_ADP3                                28
#define MODE_ADP4                                29
#define MODE_TEMPERATURE_MEASUREMENT_F           30
#define MODE_CAPACITANCE_MEASUREMENT_CLAMP       31

// Status field bit definitions
#define STATUS_JUDGE_C                           0x01 // Bit 0
#define STATUS_SIGN                              0x02 // Bit 1
#define STATUS_BATT_LOW                          0x04 // Bit 2
#define STATUS_OVERLOAD                          0x08 // Bit 3

// Option1 field bit definitions
#define OPTION1_MAX                              0x01 // Bit 0
#define OPTION1_MIN                              0x02 // Bit 1
#define OPTION1_RMR                              0x04 // Bit 2
#define OPTION1_REL                              0x08 // Bit 3

// Option2 field bit definitions
#define OPTION2_UL                               0x01 // Bit 0
#define OPTION2_PMAX                             0x02 // Bit 1
#define OPTION2_PMIN                             0x04 // Bit 2

// Option3 field bit definitions
#define OPTION3_VAHZ                             0x01 // Bit 0
#define OPTION3_AUTO                             0x02 // Bit 1
#define OPTION3_AC                               0x04 // Bit 2
#define OPTION3_DC                               0x08 // Bit 3

// Option4 field bit definitions
#define OPTION4_VBAR                             0x01 // Bit 0
#define OPTION4_HOLD                             0x02 // Bit 1
#define OPTION4_LPF0                             0x04 // Bit 2
#define OPTION4_LPF1                             0x08 // Bit 3

// DataPacket structure
typedef struct {
    uint8_t range;
    uint8_t digit4;
    uint8_t digit3;
    uint8_t digit2;
    uint8_t digit1;
    uint8_t digit0;
    uint8_t function;
    uint8_t status;
    uint8_t option1;
    uint8_t option2;
    uint8_t option3;
    uint8_t option4;
    uint8_t CR;
    uint8_t LF;
} DataPacket;

void USART_Configuration(void);
void ES51932_Init(void);
void GPIO_Configuration(void);
void ControlChannels(uint16_t value);
void SetMode(uint8_t mode);
void SetSLACDC(uint8_t mode);

extern volatile DataPacket receivedData;
extern uint8_t receivedSerialData;

#endif // ES51932_DRIVER_H