#include "mm_spi.h"
#include "hy3131_sw.h"
#include "hy3131_func.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>


extern spi_device_handle_t hy3131_handle;

static const char *TAG = "HY3131_SW";

void HY3131_Count_Reset(void)
{
    uint8_t SPIDataBuffer[1] = {0x60};
    HY3131_WriteData(SPIDataBuffer, HY3131_R37, 1);
    ets_delay_us(1); // 替代 NOP
}
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
void HY3131_ADC_Initial (const unsigned char Fun[][20], int8_t Range, int8_t ADCIRQ)
{
  uint8_t SPIDataBuffer[20];
  memcpy(SPIDataBuffer, Fun[Range], 20);
  HY3131_WriteData(SPIDataBuffer, HY3131_R20, 20);
  //HY3131_ReadData (SPIDataBuffer,HY3131_R20,20); //For Debug

  //IRQ Enable
  SPIDataBuffer[0]=0x02;
  SPIDataBuffer[1]=ADCIRQ;
  HY3131_WriteData (SPIDataBuffer,HY3131_INTF,2);
  //HY3131_ReadData (SPIDataBuffer,HY3131_INTF,2); //For Debug
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
void HY3131_Count_Initial (uint8_t Status)
{
  uint8_t SPIDataBuffer[3];
  if(flag.b_Freqlarge1M == 1) {
        SPIDataBuffer[0] = 0x00;
        SPIDataBuffer[1] = CTA_Initial & 0xFF;
        SPIDataBuffer[2] = CTA_Initial >> 8;
    } else {
        SPIDataBuffer[0] = 0x00;
        SPIDataBuffer[1] = CTA_Initial_Low & 0xFF;
        SPIDataBuffer[2] = CTA_Initial_Low >> 8;
    }
    HY3131_WriteData(SPIDataBuffer, HY3131_CTA0, 3);
    SPIDataBuffer[0] = Status | ENCTR;
    HY3131_WriteData(SPIDataBuffer, HY3131_R20, 1);
}

void HY3131_count_PreFilter_100K (void)
{
  uint8_t SPIDataBuffer[1] = {0xA0};
  HY3131_WriteData(SPIDataBuffer, HY3131_R33, 1);
}

void HY3131_count_PreFilter_0K (void)
{
  uint8_t SPIDataBuffer[1] = {0xA8};
  HY3131_WriteData(SPIDataBuffer, HY3131_R33, 1);
}

// ESP32 SPI写寄存器
void HY3131_WriteData(uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount)
{
    uint8_t tx[1 + ByteCount];
    tx[0] = ((StartAddress << 1) | 0x00); // WriteComm = 0
    memcpy(&tx[1], DataBuffer, ByteCount);
    spi_transaction_t t = {
        .length = (1 + ByteCount) * 8,
        .tx_buffer = tx,
        .rx_buffer = NULL
    };
    gpio_set_level(HY3131_CS_PIN, 0);
    ESP_ERROR_CHECK(spi_device_transmit(hy3131_handle, &t));
    gpio_set_level(HY3131_CS_PIN, 1);
}

// ESP32 SPI读寄存器
void HY3131_ReadData(uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount)
{
    uint8_t tx[1 + ByteCount];
    uint8_t rx[1 + ByteCount];
    memset(tx, 0x00, sizeof(tx));
    tx[0] = ((StartAddress << 1) | 0x01); // ReadComm = 1
    spi_transaction_t t = {
        .length = (1 + ByteCount) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx
    };
    gpio_set_level(HY3131_CS_PIN, 0);
    ESP_ERROR_CHECK(spi_device_transmit(hy3131_handle, &t));
    gpio_set_level(HY3131_CS_PIN, 1);
    memcpy(DataBuffer, &rx[1], ByteCount);
}
