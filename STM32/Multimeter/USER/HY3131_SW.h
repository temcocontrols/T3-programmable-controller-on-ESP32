#include "HY3131_Reg.h"

void HY3131_ADC_Initial (const unsigned char Fun[][20], int8_t Range, int8_t ADCIRQ);
void HY3131_Count_Initial (uint8_t Status);
void HY3131_WriteData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount);
void HY3131_ReadData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount);
void HY3131_count_PreFilter_100K();
void HY3131_count_PreFilter_0K();
void HY3131_Count_Reset();
void SPI_Write(uint8_t SPI_Data);
uint8_t SPI_Read(void);