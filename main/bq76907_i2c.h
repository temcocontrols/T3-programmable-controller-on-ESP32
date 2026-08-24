#ifndef __BQ76907_I2C_H
#define __BQ76907_I2C_H

#include <stdint.h>

void BQ76907_I2C_Init(void);
void BQ76907_I2C_BusRecovery(void);
uint8_t BQ76907_I2C_Scan(uint8_t *addr_7bit);
uint8_t BQ76907_I2C_ReadReg(uint16_t reg, uint16_t *value);
uint8_t BQ76907_I2C_ReadBlock(uint8_t reg_addr, uint8_t *buf, uint8_t len);
uint8_t BQ76907_I2C_WriteReg(uint16_t reg, uint16_t value);
uint8_t BQ76907_I2C_WriteByte(uint8_t reg_addr, uint8_t value);
uint8_t BQ76907_I2C_WriteBlock(uint8_t reg_addr, const uint8_t *buf, uint8_t len);
uint8_t BQ76907_I2C_GetLastError(void);

#endif
