#ifndef MMI2C_H
#define MMI2C_H

#include "stm32f10x.h"

// I2C Slave Address
#define I2CSLAVE_ADDR (0x74 << 1)  // I2C address is 0x74

// I2C Clock Frequency
#define I2C1_CLOCK_FRQ 100000  // I2C frequency in Hz (100 kHz)

// I2C Modes
#define I2C1_MODE_WAITING       0  // Waiting for commands
#define I2C1_MODE_SLAVE_ADR_WR  1  // Received slave address (writing)
#define I2C1_MODE_ADR_BYTE      2  // Received ADR byte
#define I2C1_MODE_DATA_BYTE_WR  3  // Data byte (writing)
#define I2C1_MODE_SLAVE_ADR_RD  4  // Received slave address (to read)
#define I2C1_MODE_DATA_BYTE_RD  5  // Data byte (to read)

// Function Prototypes
void I2C1_Slave_Init(void);
void Set_I2C1_Ram(uint8_t adr, uint8_t val);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);

#endif // MMI2C_H