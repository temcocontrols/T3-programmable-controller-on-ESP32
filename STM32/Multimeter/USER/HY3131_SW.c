/*
 * HY3131 SPI driver - SPL based implementation
 * Replaces software bit-banged SPI with STM32F10x Standard Peripheral Library (SPI1)
 *
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "delay.h"   // existing project delay
#include <stdint.h>

/* Pin mapping (same as original)
 *  CS   -> PA4
 *  SCK  -> PA5 (SPI1_SCK)
 *  MISO -> PA6 (SPI1_MISO)
 *  MOSI -> PA7 (SPI1_MOSI)
 */

/* HY3131 physical pins (use GPIOA) */
#define HY3131_PORT GPIOA
#define SCS_PIN     GPIO_Pin_4
#define SCLK_PIN    GPIO_Pin_5
#define MISO_PIN    GPIO_Pin_6
#define MOSI_PIN    GPIO_Pin_7

/* Helper macros for CS */
static inline void HY3131_CS_LOW(void)  { GPIO_ResetBits(HY3131_PORT, SCS_PIN); }
static inline void HY3131_CS_HIGH(void) { GPIO_SetBits  (HY3131_PORT, SCS_PIN); }

/* Initialize SPI1 and GPIO for HY3131 (call early in system init) */
void HY3131_SPI_Init_Spl(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* Enable clocks: GPIO port used by HY3131 and SPI1 */
    /* Ensure the port macro is on APB2 (GPIOA..GPIOD). */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_SPI1, ENABLE);

    /* Configure CS (SCS_PIN) as push-pull output */
    GPIO_InitStructure.GPIO_Pin = SCS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(HY3131_PORT, &GPIO_InitStructure);

    /* Configure SCLK as AF push-pull (SPI1_SCK), MOSI as AF push-pull, MISO as input floating */
    GPIO_InitStructure.GPIO_Pin = SCLK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(HY3131_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(HY3131_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(HY3131_PORT, &GPIO_InitStructure);

    /* SPI1 configuration: Master, Mode 0, 8-bit, software NSS */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;    /* idle low (Mode0) */
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; /* adjust if needed */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);

    /* CS idle high */
    HY3131_CS_HIGH();
}

/* 8-bit full-duplex transfer using SPL */
static uint8_t HY3131_SPI_Transfer(uint8_t tx)
{
    /* wait until TXE set */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) { }
    SPI_I2S_SendData(SPI1, tx);

    /* wait until RXNE set */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) { }
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

/* Compatibility wrappers to keep existing interface names */

/* Write multiple registers to HY3131 (start address, byte count) */
void HY3131_WriteData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount)
{
    uint32_t i;
    HY3131_CS_LOW();

    /* Send Register Address + write bit (WriteComm expected 0) */
    HY3131_SPI_Transfer((uint8_t)((StartAddress << 1) | (WriteComm & 0x01)));

    for (i = 0; i < ByteCount; ++i) {
        HY3131_SPI_Transfer(DataBuffer[i]);
    }

    HY3131_CS_HIGH();
}

/* Read multiple registers from HY3131 (start address, byte count) */
void HY3131_ReadData (uint8_t *DataBuffer, uint32_t StartAddress, uint32_t ByteCount)
{
    uint32_t i;
    HY3131_CS_LOW();

    /* Send Address with read bit */
    HY3131_SPI_Transfer((uint8_t)((StartAddress << 1) | (ReadComm & 0x01)));

    /* per device protocol: send dummy clock (one dummy byte) then read bytes */
    HY3131_SPI_Transfer(0x00);

    for (i = 0; i < ByteCount; ++i) {
        DataBuffer[i] = HY3131_SPI_Transfer(0x00);
    }

    HY3131_CS_HIGH();
}

/* Single byte write (kept for compatibility) */
void SPI_Write(uint8_t SPIData)
{
    (void)HY3131_SPI_Transfer(SPIData);
}

/* Single byte read (kept for compatibility) */
uint8_t SPI_Read(void)
{
    return HY3131_SPI_Transfer(0x00);
}




