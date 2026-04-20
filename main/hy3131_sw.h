#ifndef _HY3131_SW_H_
#define _HY3131_SW_H_
#include "hy3131_reg.h"

#define HY3131_CS_PIN   PIN_NUM_CS
#define HY3131_MOSI_PIN PIN_NUM_MOSI
#define HY3131_MISO_PIN PIN_NUM_MISO
#define HY3131_CLK_PIN  PIN_NUM_CLK

void HY3131_ADC_Initial (const unsigned char Fun[][20], int8_t Range, int8_t ADCIRQ);
void HY3131_Count_Initial (uint8_t Status);
void HY3131_WriteData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount);
void HY3131_ReadData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount);
void HY3131_count_PreFilter_100K();
void HY3131_count_PreFilter_0K();
void HY3131_Count_Reset();
//void SPI_Write(uint8_t SPI_Data);
//uint8_t SPI_Read(void);

#endif // _HY3131_SW_H_