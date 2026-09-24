
 #if 0
 /***************************************************************
    *  HY3131 Driver
    *  Author: Lijun (Temco Controls)
    *
    *  This file implements:
    *  - SPI polling mode (STM32F103, register-level)
    *  - HY3131 register addresses (0x00..0x1F, 0x20..0x37)
    *  - Basic read/write interfaces
    *  - GPIO control for range relays: S1..S5 = PB11..PB15
    ***************************************************************/

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <string.h>
#include "delay.h"

/***************************************************************
 *  GPIO: Range Switch Relays (external)
 ***************************************************************/
#define S1_GPIO    GPIOB
#define S2_GPIO    GPIOB
#define S3_GPIO    GPIOB
#define S4_GPIO    GPIOB
#define S5_GPIO    GPIOB

#define S1_PIN     11
#define S2_PIN     12
#define S3_PIN     13
#define S4_PIN     14
#define S5_PIN     15

#define SET_PIN(gpio,pin)    ((gpio)->BSRR = (1 << (pin)))
#define CLR_PIN(gpio,pin)    ((gpio)->BRR  = (1 << (pin)))

/***************************************************************
 *  HY3131 SPI Pins (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI)
 ***************************************************************/
#define HY_CS_LOW()    (GPIOA->BRR  = GPIO_BRR_BR4)
#define HY_CS_HIGH()   (GPIOA->BSRR = GPIO_BSRR_BS4)

/***************************************************************
 *  Small delay
 ***************************************************************/
static inline void hy_delay_us(uint32_t us)
{
    uint32_t t = (SystemCoreClock / 8000000) * us;
    while (t--) __NOP();
}

/***************************************************************
 *  SPI Init (Polling, Mode0, Fpclk/16)
 ***************************************************************/
void HY_SPI_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_SPI1EN;

    /* PA4 CS */
    GPIOA->CRL &= ~(0xF << 16);
    GPIOA->CRL |=  (0x3 << 16);  // 50 MHz push-pull

    /* PA5 SCK */
    GPIOA->CRL &= ~(0xF << 20);
    GPIOA->CRL |=  (0xB << 20);  // AF push-pull

    /* PA6 MISO */
    GPIOA->CRL &= ~(0xF << 24);
    GPIOA->CRL |=  (0x4 << 24);  // input floating

    /* PA7 MOSI */
    GPIOA->CRL &= ~(0xF << 28);
    GPIOA->CRL |=  (0xB << 28);  // AF push-pull

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1;
    SPI1->CR1 |= 0;              // CPOL=0, CPHA=0
    SPI1->CR1 |= SPI_CR1_SPE;

    HY_CS_HIGH();

    /* Configure PB11~PB15 as outputs (range switches) */
    GPIOB->CRH &= ~(0xFFFFF << 12);
    GPIOB->CRH |=  (0x33333 << 12);  // PB11~PB15 = 50MHz push-pull
}

/***************************************************************
 *  SPI Send/Recv (8-bit)
 ***************************************************************/
static uint8_t HY_SPI_RW(uint8_t d)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = d;

    while (!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

/***************************************************************
 *  HY3131 Register Map (Official)
 ***************************************************************/

/* AD1 */
#define REG_AD1_H   0x00
#define REG_AD1_M   0x01
#define REG_AD1_L   0x02

/* AD2 (fast) */
#define REG_AD2_H   0x03
#define REG_AD2_M   0x04
#define REG_AD2_L   0x05

/* LPF */
#define REG_LPF_H   0x06
#define REG_LPF_M   0x07
#define REG_LPF_L   0x08

/* RMS */
#define REG_RMS_H   0x09
#define REG_RMS_M   0x0A
#define REG_RMS_L   0x0B

/* Peak Hold Max */
#define REG_PKMAX_H 0x0F
#define REG_PKMAX_M 0x10
#define REG_PKMAX_L 0x11

/* CTA counter (frequency) */
#define REG_CTA0    0x1B
#define REG_CTA1    0x1C
#define REG_CTA2    0x1D

/* Function Code Block R20~R2F (20 bytes) */
#define REG_FC_START 0x20

/***************************************************************
 *  Basic Register Write/Read
 ***************************************************************/
void HY_WriteReg(uint8_t reg, uint8_t val)
{
    HY_CS_LOW();
    HY_SPI_RW((reg << 1) | 0x00);
    HY_SPI_RW(val);
    HY_CS_HIGH();
    hy_delay_us(2);
}

uint8_t HY_ReadReg(uint8_t reg)
{
    uint8_t v;
    HY_CS_LOW();
    HY_SPI_RW((reg << 1) | 0x01);
    HY_SPI_RW(0x00);  // dummy
    v = HY_SPI_RW(0x00);
    HY_CS_HIGH();
    return v;
}

void HY_WriteBlock(uint8_t startReg, const uint8_t *buf, uint8_t len)
{
    HY_CS_LOW();
    HY_SPI_RW((startReg << 1) | 0x00);
    for (uint8_t i = 0; i < len; i++)
        HY_SPI_RW(buf[i]);
    HY_CS_HIGH();
    hy_delay_us(5);
}

void HY_ReadBlock(uint8_t startReg, uint8_t *buf, uint8_t len)
{
    HY_CS_LOW();
    HY_SPI_RW((startReg << 1) | 0x01);
    HY_SPI_RW(0x00);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = HY_SPI_RW(0x00);
    HY_CS_HIGH();
}

 /***************************************************************
    *  HY3131 Function Control Blocks (20 bytes each)
    *
    *  All register definitions are taken from the official datasheet DS-HY3131_EN.
    *  Mode definitions are based on the schematic (M3..M0 + S1..S5 switch matrix).
    *
    *  Notes:
    *  - R20..R2F is a 20-byte function block
    *  - Contains AC/DC path, buffer, gain and input selection
    *  - Contains internal mode bits M3..M0
    *  - External relays S1..S5 are controlled by MCU GPIO (not in R20..R2F)
    ***************************************************************/

/* ============================================================
 * Mode 1: DCV
 * ------------------------------------------------------------
 *  M3 M2 M1 M0 = 1 1 0 0
 *  S1 S2 S3 S4 S5 = 0 1 1 0 0
 *  Front-end path: through 30K/10M/10M divider -> PA6..PA9
 *  AD1 is used for DC sampling
 *  AD1RG = mid gain
 *  AC path disabled
 *  RMS/LPF disabled
 * ============================================================*/
static const uint8_t FC_DCV[20] = {
    /* R20 */ 0b11000000,   // M3..M0=1100
    /* R21 */ 0b00000010,   // AD1RG gain=mid
    /* R22 */ 0b00000001,   // AD1BUF enabled
    /* R23 */ 0b00000000,   // AC path disable
    /* R24 */ 0b01000000,   // SAD1I = DC input (PAx)
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b01010000,   // DS/FS select = DCV path
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};

/* ============================================================
 * Mode 2: ACV (RMS)
 * ------------------------------------------------------------
 * M3 M2 M1 M0 = 1 1 1 1
 * S1 S2 S3 S4 S5 = 0 1 0 0 0
 * AC Path enable
 * RMS (AD2) enable
 * LPF disable
 *  M3 M2 M1 M0 = 1 1 1 1
 *  S1 S2 S3 S4 S5 = 0 1 0 0 0
 *  AC path enabled
 *  RMS (AD2) enabled
 *  LPF disabled
 * ============================================================*/
static const uint8_t FC_ACV[20] = {
    /* R20 */ 0b11110000,   // AC mode
    /* R21 */ 0b00001000,   // AC buffer on
    /* R22 */ 0b00000000,
    /* R23 */ 0b10000000,   // enable RMS path
    /* R24 */ 0b01100000,   // SAD1I = AC input
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b01000000,
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};

/* ============================================================
 * Mode 3: OHM (电阻)
 * ------------------------------------------------------------
 *  M3 M2 M1 M0 = 1 0 1 1
 *  S1 S2 S3 S4 S5 = 0 1 0 1 0
 *  Use resistance excitation path: LM385 & resistor network
 *  AD1 is used to measure voltage drop
 * ============================================================*/
static const uint8_t FC_OHM[20] = {
    /* R20 */ 0b10110000,
    /* R21 */ 0b00000100,   // enable ohm excitation path
    /* R22 */ 0b00000001,   // AD1BUF enable
    /* R23 */ 0b00000000,
    /* R24 */ 0b01010000,   // SAD1I = ohm input
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b01110000,   // select ohm path
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};

/* ============================================================
 * Mode 4: DIODE
 * ------------------------------------------------------------
 *  M3 M2 M1 M0 = 1 1 1 1
 *  S1 S2 S3 S4 S5 = 1 0 1 1 0
 *  Use PB1 (OPIN) comparator path
 * ============================================================*/
static const uint8_t FC_DIODE[20] = {
    /* R20 */ 0b11110000,
    /* R21 */ 0b00000010,   // diode excitation
    /* R22 */ 0b00000001,
    /* R23 */ 0x00,
    /* R24 */ 0b01000000,   // SAD1I = diode
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b01010000,
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};

/* ============================================================
 * Mode 5: CONTINUITY
 * ------------------------------------------------------------
 *  M3 M2 M1 M0 = 1 1 1 1
 *  S1 S2 S3 S4 S5 = 1 0 1 1 0
 *  Same as diode, but comparator output is used for buzzer detection
 * ============================================================*/
static const uint8_t FC_CONT[20] = {
    /* R20 */ 0b11110000,
    /* R21 */ 0b00000010,
    /* R22 */ 0b00000001,
    /* R23 */ 0x00,
    /* R24 */ 0b01000000,
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b01010000,
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};

/* ============================================================
 * Mode 6: Frequency (Hz)
 * ------------------------------------------------------------
 *  M3 M2 M1 M0 = 0 1 0 1
 *  S1 S2 S3 S4 S5 = 1 0 0 1 1
 *  CTA counter enabled, CNT pin used
 * ============================================================*/
static const uint8_t FC_FREQ[20] = {
    /* R20 */ 0b01010000,
    /* R21 */ 0b00000000,
    /* R22 */ 0b10000000,  // enable CTA
    /* R23 */ 0b00000000,
    /* R24 */ 0b00010000,  // CNT input
    /* R25 */ 0x00,
    /* R26 */ 0x00,
    /* R27 */ 0x00,
    /* R28 */ 0x00,
    /* R29 */ 0x00,
    /* R2A */ 0b00010000,  // CNT path
    /* R2B */ 0x00,
    /* R2C */ 0x00,
    /* R2D */ 0x00,
    /* R2E */ 0x00,
    /* R2F */ 0x00
};


/***************************************************************
 *  HY3131 Measurement Functions
 *  - AD1 / AD2 / RMS / Peak / CTA
 *  - Mode switching (R20~R2F + S1~S5)
 *  - DCV, ACV, OHM, Diode, Continuity, Frequency
 ***************************************************************/

/***************************************************************
 *  Read 24-bit AD1
 ***************************************************************/
int32_t HY_Read_AD1(void)
{
    uint8_t b[3];
    HY_ReadBlock(REG_AD1_H, b, 3);

    int32_t raw = (b[0] << 16) | (b[1] << 8) | b[2];
    if (raw & 0x800000) raw |= 0xFF000000;  // sign extend

    return raw;
}

/***************************************************************
 *  Read 24-bit AD2 (RMS front-end)
 ***************************************************************/
uint32_t HY_Read_AD2(void)
{
    uint8_t b[3];
    HY_ReadBlock(REG_AD2_H, b, 3);

    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/***************************************************************
 *  Read 24-bit RMS (09h~0Bh)
 ***************************************************************/
uint32_t HY_Read_RMS(void)
{
    uint8_t b[3];
    HY_ReadBlock(REG_RMS_H, b, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/***************************************************************
 *  Peak Hold Max
 ***************************************************************/
uint32_t HY_Read_Peak(void)
{
    uint8_t b[3];
    HY_ReadBlock(REG_PKMAX_H, b, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/***************************************************************
 *  CTA Frequency Counter
 ***************************************************************/
uint32_t HY_Read_Frequency_Count(void)
{
    uint8_t b[3];
    HY_ReadBlock(REG_CTA0, b, 3);
    return (b[2] << 16) | (b[1] << 8) | b[0];
}

/***************************************************************
 *  Apply 20-byte Function Code
 ***************************************************************/
static void HY_Apply_Function(const uint8_t *fc)
{
    HY_WriteBlock(REG_FC_START, fc, 20);
    hy_delay_us(200);  // allow analog path to settle
}

/***************************************************************
 *  Relay Control: S1~S5 (PB11~PB15)
 ***************************************************************/
static void HY_Set_Relay(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5)
{
    (s1 ? SET_PIN : CLR_PIN)(S1_GPIO, S1_PIN);
    (s2 ? SET_PIN : CLR_PIN)(S2_GPIO, S2_PIN);
    (s3 ? SET_PIN : CLR_PIN)(S3_GPIO, S3_PIN);
    (s4 ? SET_PIN : CLR_PIN)(S4_GPIO, S4_PIN);
    (s5 ? SET_PIN : CLR_PIN)(S5_GPIO, S5_PIN);
}

/***************************************************************
 *  Mode Switching (DCV / ACV / OHM / DIODE / CONT / FREQ)
 ***************************************************************/
typedef enum {
    MODE_DCV,
    MODE_ACV,
    MODE_OHM,
    MODE_DIODE,
    MODE_CONT,
    MODE_FREQ
} HY_MODE;

/***************************************************************
 *  Mode → Function Code + Relay Combination
 ***************************************************************/
static void HY_Set_Mode(HY_MODE mode)
{
    switch (mode)
    {
        case MODE_DCV:
            HY_Set_Relay(0,1,1,0,0);
            HY_Apply_Function(FC_DCV);
            break;

        case MODE_ACV:
            HY_Set_Relay(0,1,0,0,0);
            HY_Apply_Function(FC_ACV);
            break;

        case MODE_OHM:
            HY_Set_Relay(0,1,0,1,0);
            HY_Apply_Function(FC_OHM);
            break;

        case MODE_DIODE:
            HY_Set_Relay(1,0,1,1,0);
            HY_Apply_Function(FC_DIODE);
            break;

        case MODE_CONT:
            HY_Set_Relay(1,0,1,1,0);
            HY_Apply_Function(FC_CONT);
            break;

        case MODE_FREQ:
            HY_Set_Relay(1,0,0,1,1);
            HY_Apply_Function(FC_FREQ);
            break;
    }
}

/***************************************************************
 *  DC Voltage Conversion
 *  (Your front-end: 30k / 10M / 10M 分压链)
 ***************************************************************/
float HY_Conv_DCV(int32_t raw)
{
    // raw → voltage before divider
    float v_adc = (float)raw / 50000.0f;

    // divider ratio:
    // Rtotal = 10M + 10M + 30k ≈ 20.03 M
    float ratio = (20000000.0f + 30000.0f) / 30000.0f; // ≈ 668.333

    return v_adc * ratio;
}

/***************************************************************
 *  AC Voltage (RMS)
 ***************************************************************/
float HY_Conv_ACV(uint32_t raw_rms)
{
    float v_adc = (float)raw_rms / 50000.0f;

    /* AC path uses same divider as DCV */
    float ratio = (20000000.0f + 30000.0f) / 30000.0f;

    return v_adc * ratio;
}

/***************************************************************
 *  OHM (Resistance) Calculation
 *  激励电流由前端电阻链决定
 ***************************************************************/
float HY_Conv_OHM(int32_t raw)
{
    float v_adc = (float)raw / 50000.0f;

    /* excitation ≈ 1mA（基于你的激励路径 R25/R26/R28 等） */
    const float I = 0.001f;

    return v_adc / I;
}

/***************************************************************
 *  Diode Voltage Drop
 ***************************************************************/
float HY_Conv_Diode(int32_t raw)
{
    return (float)raw / 50000.0f;
}

/***************************************************************
 *  Continuity
 ***************************************************************/
uint8_t HY_Conv_Continuity(int32_t raw)
{
    float v = (float)raw / 50000.0f;

    /* threshold ~ 50 ohm equivalent */
    return (v < 0.05f) ? 1 : 0;
}

/***************************************************************
 *  Frequency (Hz)
 ***************************************************************/
float HY_Conv_Freq(uint32_t cnt)
{
    /* CTA count per second = freq */
    return (float)cnt;
}

/***************************************************************
 *  High-level unified measurement API
 ***************************************************************/
typedef struct {
    HY_MODE mode;
    float value;      // main value
    float extra;      // secondary (continuity / peak / etc)
} HY_Result;

/***************************************************************
 *  HY_Measure(): perform one measurement in selected mode
 ***************************************************************/
HY_Result HY_Measure(HY_MODE mode)
{
    HY_Result r;
    r.mode = mode;
    r.extra = 0;

    /* Ensure mode correctly applied */
    HY_Set_Mode(mode);
    hy_delay_us(500);  // settle

    switch (mode)
    {
        case MODE_DCV:
        {
            int32_t raw = HY_Read_AD1();
            r.value = HY_Conv_DCV(raw);
            break;
        }

        case MODE_ACV:
        {
            uint32_t rms = HY_Read_RMS();
            r.value = HY_Conv_ACV(rms);
            break;
        }

        case MODE_OHM:
        {
            int32_t raw = HY_Read_AD1();
            r.value = HY_Conv_OHM(raw);
            break;
        }

        case MODE_DIODE:
        {
            int32_t raw = HY_Read_AD1();
            r.value = HY_Conv_Diode(raw);
            break;
        }

        case MODE_CONT:
        {
            int32_t raw = HY_Read_AD1();
            r.value = HY_Conv_Diode(raw);
            r.extra = HY_Conv_Continuity(raw);
            break;
        }

        case MODE_FREQ:
        {
            uint32_t cnt = HY_Read_Frequency_Count();
            r.value = HY_Conv_Freq(cnt);
            break;
        }
    }

    return r;
}

/***************************************************************
 *  FreeRTOS Continuous Measurement Task
 ***************************************************************/
void HY3131_Task(void *argument)
{
    HY_SPI_Init();

    HY_MODE mode = MODE_DCV; // default or user selection

    for (;;)
    {
        HY_Result r = HY_Measure(mode);

        /* Debug output - replace with your logging */
        switch (mode)
        {
            case MODE_DCV:
                test[200] = (uint16_t)(r.value * 100.0f);  // DCV in V*100
                break;

            case MODE_ACV:
                test[201] = (uint16_t)(r.value * 100.0f);  // ACV in V*100
                break;

            case MODE_OHM:
                test[202] = (uint16_t)(r.value * 100.0f);  // OHM in ohm*100
                break;

            case MODE_DIODE:
                test[203] = (uint16_t)(r.value * 1000.0f);  // DIODE in V*1000
                break;

            case MODE_CONT:
                test[204] = (uint16_t)(r.value * 1000.0f);  // CONT in V*1000
                test[205] = (uint16_t)(r.extra);             // CONT status (0=open, 1=closed)
                break;

            case MODE_FREQ:
                test[206] = (uint16_t)(r.value * 100.0f);  // FREQ in Hz*100
                break;
        }

        /* Update mode from external UI if needed */
        //vTaskDelay(pdMS_TO_TICKS(200));
        delay_ms(200);
    }
}
#endif

#if 1
/***************************************************************
 *  HY3131 Driver for STM32F103 (Polling SPI + LL/Register-Level)
  *  Includes:
 *    - SPI1 initialization (PA4/PA5/PA6/PA7)
 *    - GPIO setup
 *    - Polling SPI R/W
 *    - HY3131 low-level register read/write
 *    - AD1, AD2 RMS, AD3 Peak
 *    - Temperature
 *    - Frequency CTA
 *  Author: Lijun (Temco Controls)
 ***************************************************************/

/***************************************************************
 *  HY3131 Driver for STM32F103 (SPL-based)
 *  SPI1 (Polling) + SPL (GPIO/SPI/RCC)
 *  Rewritten from register-level to Standard Peripheral Library
 *  Author: converted by GitHub Copilot (GPT-5 mini)
 ***************************************************************/

#include "hy3131_spi.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdint.h>
#include "delay.h"

/* Pin mapping (same as original)
 *  CS   -> PA4
 *  SCK  -> PA5 (SPI1_SCK)
 *  MISO -> PA6 (SPI1_MISO)
 *  MOSI -> PA7 (SPI1_MOSI)
 */

#define HY3131_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define HY3131_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

extern uint16_t test[];

/* short busy-wait microsecond-ish delay (approx) */
static inline void hy_delay_us(uint32_t us)
{
    /* approximate loop, tuned for typical SYSCLK 72MHz */
    uint32_t ticks = (SystemCoreClock / 1000000UL) * us / 8U;
    while (ticks--) __NOP();
}

/* Initialize SPI1 and GPIO using SPL */
void HY3131_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    /* Enable clocks: GPIOA + AFIO not needed + SPI1 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    /* PA4 = CS : Output push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA5 = SCK : Alternate Function Push Pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA6 = MISO : Input Floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA7 = MOSI : Alternate Function Push Pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* SPI1 configuration (Mode0, Master, Baud /16 -> safe ~4.5MHz) */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; /* software NSS */
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    /* Enable SPI1 */
    SPI_Cmd(SPI1, ENABLE);

    /* CS idle HIGH */
    HY3131_CS_HIGH();
}

/* SPI polling send/receive 8-bit using SPL wrappers */
static uint8_t HY_SPI_SendRecv(uint8_t data)
{
    /* Wait until TXE set */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) { }
    SPI_I2S_SendData(SPI1, data);

    /* Wait until RXNE set */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) { }
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

/* command helpers */
#define HY_WRITE_CMD(reg)   (uint8_t)((((reg) & 0x7F) << 1) | 0x00)
#define HY_READ_CMD(reg)    (uint8_t)((((reg) & 0x7F) << 1) | 0x01)

/* Low-level register write */
void HY3131_WriteReg(uint8_t reg, uint8_t value)
{
    HY3131_CS_LOW();
    HY_SPI_SendRecv(HY_WRITE_CMD(reg));
    HY_SPI_SendRecv(value);
    HY3131_CS_HIGH();
    hy_delay_us(1);
}

/* Multi-byte write */
void HY3131_WriteData(uint8_t *buf, uint8_t reg, uint8_t len)
{
    HY3131_CS_LOW();
    HY_SPI_SendRecv(HY_WRITE_CMD(reg));
    for (uint8_t i = 0; i < len; i++)
        HY_SPI_SendRecv(buf[i]);
    HY3131_CS_HIGH();
    hy_delay_us(1);
}

/* Multi-byte read (dummy first byte) */
void HY3131_ReadData(uint8_t *buf, uint8_t reg, uint8_t len)
{
    HY3131_CS_LOW();
    HY_SPI_SendRecv(HY_READ_CMD(reg)); /* send command */
    HY_SPI_SendRecv(0x00);             /* dummy byte (ignored) */
    for (uint8_t i = 0; i < len; i++)
        buf[i] = HY_SPI_SendRecv(0x00);
    HY3131_CS_HIGH();
}

/* Register map constants (same as original) */
#define  HY3131_AD1_DATA0  0x00 //AD1<7:0>

#define HY_REG_AD1_H       0x40
#define HY_REG_AD1_M       0x41
#define HY_REG_AD1_L       0x42
#define HY_REG_AD2_H       0x43
#define HY_REG_AD2_M       0x44
#define HY_REG_AD2_L       0x45
#define HY_REG_AD3_H       0x46
#define HY_REG_AD3_M       0x47
#define HY_REG_AD3_L       0x48
#define HY_REG_TEMP_H      0x50
#define HY_REG_TEMP_L      0x51
#define HY_REG_CTA0        0x30
#define HY_REG_CTA1        0x31
#define HY_REG_CTA2        0x32

/* AD1 reading (24-bit signed) */
int32_t HY3131_Read_AD1(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD1_H, 3);
    int32_t raw = (b[0] << 16) | (b[1] << 8) | b[2];
    if (raw & 0x800000) raw |= 0xFF000000; /* sign extend */
    return raw;
}

/* AD2 RMS reading (24-bit unsigned) */
uint32_t HY3131_Read_AD2_RMS(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD2_H, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/* AD3 Peak reading (24-bit unsigned) */
uint32_t HY3131_Read_AD3_Peak(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD3_H, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/* Temperature reading (16-bit) -> Celsius conversion from datasheet */
float HY3131_Read_Temperature(void)
{
    uint8_t b[2];
    HY3131_ReadData(b, HY_REG_TEMP_H, 2);
    uint16_t raw = (b[0] << 8) | b[1];
    return raw * 0.125f - 40.0f;
}

/* CTA frequency counter reading (3 bytes little-endian as original) */
uint32_t HY3131_Read_Frequency(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_CTA0, 3);
    /* original returned (b[2]<<16)|(b[1]<<8)|b[0]; preserve same order */
    return (b[2] << 16) | (b[1] << 8) | b[0];
}

/* ---- Range tables and auto-range logic (kept original behavior) ---- */
typedef struct {
    float max_voltage;
    float divider;
    uint8_t reg20[20];
    const char *name;
} HY_DCV_RANGE_t;

static HY_DCV_RANGE_t dcv_ranges[] =
{
    { 0.6f,    1.0f,     {0},  "0.6V"   },
    { 6.0f,    10.0f,    {0},  "6V"     },
    { 60.0f,   100.0f,   {0},  "60V"    },
    { 600.0f,  1000.0f,  {0},  "600V"   },
};
static uint8_t dcv_range_index = 0;

typedef struct {
    float max_rms;
    float divider;
    uint8_t reg20[20];
    const char *name;
} HY_ACV_RANGE_t;

static HY_ACV_RANGE_t acv_ranges[] =
{
    { 0.4f,    1.0f,     {0},  "0.4V AC"   },
    { 4.0f,    10.0f,    {0},  "4V AC"     },
    { 40.0f,   100.0f,   {0},  "40V AC"    },
    { 400.0f,  1000.0f,  {0},  "400V AC"   },
};
static uint8_t acv_range_index = 0;

typedef struct {
    float max_ohm;
    float current;
    uint8_t reg20[20];
    const char *name;
} HY_OHM_RANGE_t;

static HY_OHM_RANGE_t ohm_ranges[] =
{
    { 600.0f,      0.001f,     {0},  "600O"    },
    { 6000.0f,     0.0001f,    {0},  "6kO"     },
    { 60000.0f,    0.00001f,   {0},  "60kO"    },
    { 600000.0f,   0.000001f,  {0},  "600kO"   },
};
static uint8_t ohm_range_index = 0;

/* generic auto-range helper: pass limit table and pointer to current range */
static uint8_t AutoRange_Generic(float value,
                                 float *limit_table,
                                 uint8_t *range,
                                 uint8_t max_range)
{
    uint8_t r = *range;
    if (value > limit_table[r] * 0.90f)
    {
        if (r < (max_range - 1))
        {
            *range = r + 1;
            return 1;
        }
    }
    if (value < limit_table[r] * 0.10f)
    {
        if (r > 0)
        {
            *range = r - 1;
            return 1;
        }
    }
    return 0;
}

static void HY3131_Apply_R20(uint8_t *reg20)
{
    HY3131_WriteData(reg20, 0x20, 20);
    hy_delay_us(50);
}

void HY3131_AutoRange_DCV(float voltage)
{
    float limits[] = {0.6f, 6.0f, 60.0f, 600.0f};
    if (AutoRange_Generic(voltage, limits, &dcv_range_index, 4))
        HY3131_Apply_R20(dcv_ranges[dcv_range_index].reg20);
}

void HY3131_AutoRange_ACV(float vrms)
{
    float limits[] = {0.4f, 4.0f, 40.0f, 400.0f};
    if (AutoRange_Generic(vrms, limits, &acv_range_index, 4))
        HY3131_Apply_R20(acv_ranges[acv_range_index].reg20);
}

void HY3131_AutoRange_OHM(float R)
{
    float limits[] = {600.0f, 6000.0f, 60000.0f, 600000.0f};
    if (AutoRange_Generic(R, limits, &ohm_range_index, 4))
        HY3131_Apply_R20(ohm_ranges[ohm_range_index].reg20);
}

/* conversion helpers */
float HY3131_Convert_DCV(int32_t raw)
{
    float voltage = (float)raw / 50000.0f;
    voltage *= dcv_ranges[dcv_range_index].divider;
    return voltage;
}

float HY3131_Convert_ACV(uint32_t rms_raw)
{
    float v = (float)rms_raw / 50000.0f;
    v *= acv_ranges[acv_range_index].divider;
    return v;
}

float HY3131_Convert_OHM(int32_t raw)
{
    if (raw <= 0) return -1.0f;
    float voltage = (float)raw / 50000.0f;
    float Iex = ohm_ranges[ohm_range_index].current;
    return voltage / Iex;
}

/* AC/DC detection */
MEASURE_MODE HY3131_Detect_AC_DC(int32_t raw_dc, uint32_t raw_rms)
{
    float vdc = (float)raw_dc / 50000.0f;
    float vac = (float)raw_rms / 50000.0f;
    if (vac > vdc * 0.05f)
        return MODE_ACV;
    else
        return MODE_DCV;
}

/* diode and continuity helpers */
float HY3131_Measure_Diode(void)
{
    int32_t raw = HY3131_Read_AD1();
    float voltage = (float)raw / 50000.0f;
    return voltage;
}

uint8_t HY3131_Measure_Continuity(float *R)
{
    int32_t raw = HY3131_Read_AD1();
    float voltage = (float)raw / 50000.0f;
    float ohms = voltage / ohm_ranges[ohm_range_index].current;
    *R = ohms;
    return (ohms < 50.0f) ? 1 : 0;
}

/* Unified measurement engine (keeps original behavior and naming) */
HY_Result_t HY3131_Measure(MEASURE_MODE reqMode)
{
    HY_Result_t ret;
    memset(&ret, 0, sizeof(ret));
    switch (reqMode)
    {
        case MODE_DCV:
        {
            int32_t raw = HY3131_Read_AD1();
            float voltage = HY3131_Convert_DCV(raw);
            HY3131_AutoRange_DCV(voltage);
            ret.value = voltage;
            ret.range = dcv_range_index;
            ret.mode  = MODE_DCV;
            break;
        }
        case MODE_ACV:
        {
            uint32_t raw = HY3131_Read_AD2_RMS();
            float vrms = HY3131_Convert_ACV(raw);
            HY3131_AutoRange_ACV(vrms);
            ret.value = vrms;
            ret.range = acv_range_index;
            ret.mode  = MODE_ACV;
            break;
        }
        case MODE_OHM:
        {
            int32_t raw = HY3131_Read_AD1();
            float R = HY3131_Convert_OHM(raw);
            HY3131_AutoRange_OHM(R);
            ret.value = R;
            ret.range = ohm_range_index;
            ret.mode  = MODE_OHM;
            break;
        }
        case MODE_FREQ:
        {
            uint32_t cnt = HY3131_Read_Frequency();
            ret.value = (float)cnt;
            ret.mode  = MODE_FREQ;
            break;
        }
        case MODE_DIODE:
        {
            ret.value = HY3131_Measure_Diode();
            ret.mode  = MODE_DIODE;
            break;
        }
        case MODE_CONTINUITY:
        {
            float R;
            uint8_t closed = HY3131_Measure_Continuity(&R);
            ret.value = R;
            ret.value2 = closed;
            ret.mode  = MODE_CONTINUITY;
            break;
        }
        default:
            break;
    }
    return ret;
}

HY_Result_t HY3131_Measure_AC_DC(void)
{
    int32_t raw_dc = HY3131_Read_AD1();
    uint32_t raw_rms = HY3131_Read_AD2_RMS();
    MEASURE_MODE m = HY3131_Detect_AC_DC(raw_dc, raw_rms);
    if (m == MODE_ACV) return HY3131_Measure(MODE_ACV);
    return HY3131_Measure(MODE_DCV);
}

uint8_t  SPI_Data[64];
uint8_t  SPI_DataOut[4];
void SPI_3131_Read(void)
{
    // Read all needed registers in one burst starting from AD1_DATA0
    HY3131_ReadData((uint8_t*)SPI_Data, HY3131_AD1_DATA0, 52);
    // Clear all event flags
    SPI_DataOut[0] = 0x00;
    HY3131_WriteData((uint8_t*)SPI_DataOut, HY3131_INTF, 1);

}
/* FreeRTOS measurement task (keeps original structure) */
void HY3131_Task(void *argument)
{
    HY3131_SPI_Init();
    MEASURE_MODE mode = MODE_DCV;
    for (;;)
    {
        HY_Result_t res;
        switch (mode)
        {
            case MODE_DCV:
            case MODE_ACV:
                res = HY3131_Measure_AC_DC();
                break;
            case MODE_OHM:
            case MODE_FREQ:
            case MODE_DIODE:
            case MODE_CONTINUITY:
                res = HY3131_Measure(mode);
                break;
            default:
                res = HY3131_Measure(MODE_DCV);
                break;
        }

        /* optionally handle or log results here (kept commented) */
        switch (res.mode)
        {
            /* Mapping B:
             *  - test[200] = DCV (V * 100) as uint16
             *  - test[201] = ACV (V RMS * 100) as uint16
             *  - test[202] = OHM (Ohms * 100) as uint16
             *  - test[203] = FREQ (counter) as uint16 (truncated)
             *  - test[204] = DIODE (V * 1000) as uint16 (mV)
             *  - test[205] = CONTINUITY R (Ohms * 100) as uint16
             * Note: continuity closed flag (res.value2) is NOT stored here.
             */
            case MODE_DCV:
                test[200] = (uint16_t)(res.value * 100.0f);
                break;

            case MODE_ACV:
                test[201] = (uint16_t)(res.value * 100.0f);
                break;

            case MODE_OHM:
                test[202] = (uint16_t)(res.value * 100.0f);
                break;

            case MODE_FREQ:
                test[203] = (uint16_t)(res.value);
                break;

            case MODE_DIODE:
                test[204] = (uint16_t)(res.value * 1000.0f);
                break;

            case MODE_CONTINUITY:
                test[205] = (uint16_t)(res.value * 100.0f);
                break;

            default:
                break;
        }

        /* delay ~200ms (busy wait) */
        //hy_delay_us(200000);
				delay_ms(200);
    }
}
#endif

#if 0
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <string.h>
#include "hy3131_spi.h"
#include "FreeRTOS.h"
#include "task.h"


/***************************************************************
 *  Pin Mapping (Fixed)
 *  HY3131 ---- STM32F103
 *   CS   -> PA4
 *   SCK  -> PA5
 *   MISO -> PA6
 *   MOSI -> PA7
 ***************************************************************/

#define HY3131_CS_LOW()   (GPIOA->BRR  = GPIO_BRR_BR4)   // PA4 = 0
#define HY3131_CS_HIGH()  (GPIOA->BSRR = GPIO_BSRR_BS4)  // PA4 = 1

/***************************************************************
 *  Short delay for HY3131 analog settling (us-level)
 ***************************************************************/
static inline void hy_delay_us(uint32_t us)
{
    uint32_t ticks = (SystemCoreClock / 8000000) * us;
    while (ticks--) __NOP();
}

/***************************************************************
 *  SPI1 Initialization (Polling Mode)
 ***************************************************************/
void HY3131_SPI_Init(void)
{
    // Enable GPIOA + SPI1 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    /**************** GPIO CONFIG ****************
     * PA4 = CS        ? Output push-pull
     * PA5 = SPI1_SCK  ? AF Push Pull
     * PA6 = SPI1_MISO ? Input floating
     * PA7 = SPI1_MOSI ? AF Push Pull
     ************************************************/

		// PA4 = CS
		GPIOA->CRL &= ~(0xF << 16);
		GPIOA->CRL |=  (0x3 << 16);  // MODE=11 (50MHz), CNF=00 (PP)

		// PA5 = SCK
		GPIOA->CRL &= ~(0xF << 20);
		GPIOA->CRL |=  (0xB << 20);  // MODE=11, CNF=10 (AF PP)

		// PA6 = MISO
		GPIOA->CRL &= ~(0xF << 24);
		GPIOA->CRL |=  (0x4 << 24);  // MODE=00, CNF=01 (Input floating)

		// PA7 = MOSI
		GPIOA->CRL &= ~(0xF << 28);
		GPIOA->CRL |=  (0xB << 28);  // MODE=11, CNF=10 (AF PP)

    /**************** SPI1 CONFIG ****************
     * Mode 0: CPOL=0, CPHA=0
     * Master mode
     * Baud = Fpclk/16 (�4.5 MHz ? safe for HY3131)
     ************************************************/

    SPI1->CR1 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR;          // Master mode
    SPI1->CR1 |= SPI_CR1_BR_1;          // Baudrate = Fpclk/16
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // Software CS
    SPI1->CR1 |= 0;                      // CPOL=0 CPHA=0
    SPI1->CR1 |= SPI_CR1_SPE;            // Enable SPI

    // CS idle HIGH
    HY3131_CS_HIGH();
}

/***************************************************************
 *  SPI Polling Send/Receive (8-bit)
 ***************************************************************/
static uint8_t HY_SPI_SendRecv(uint8_t data)
{
    // Wait TXE
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;

    // Wait RXNE
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

/***************************************************************
 *  HY3131 Register Commands
 ***************************************************************/
#define HY_WRITE_CMD(reg)   (((reg) << 1) | 0x00)
#define HY_READ_CMD(reg)    (((reg) << 1) | 0x01)

/***************************************************************
 *  Low-level Write Register
 ***************************************************************/
void HY3131_WriteReg(uint8_t reg, uint8_t value)
{
    HY3131_CS_LOW();
    HY_SPI_SendRecv(HY_WRITE_CMD(reg));
    HY_SPI_SendRecv(value);
    HY3131_CS_HIGH();
    hy_delay_us(1);
}

/***************************************************************
 *  Multi-byte Write
 ***************************************************************/
void HY3131_WriteData(uint8_t *buf, uint8_t reg, uint8_t len)
{
    HY3131_CS_LOW();

    HY_SPI_SendRecv(HY_WRITE_CMD(reg));
    for (uint8_t i = 0; i < len; i++)
        HY_SPI_SendRecv(buf[i]);

    HY3131_CS_HIGH();
    hy_delay_us(1);
}

/***************************************************************
 *  Multi-byte Read (dummy first byte)
 ***************************************************************/
void HY3131_ReadData(uint8_t *buf, uint8_t reg, uint8_t len)
{
    HY3131_CS_LOW();

    HY_SPI_SendRecv(HY_READ_CMD(reg));   // send command

    HY_SPI_SendRecv(0x00);               // dummy byte (ignored)

    for (uint8_t i = 0; i < len; i++)
        buf[i] = HY_SPI_SendRecv(0x00);

    HY3131_CS_HIGH();
}

/***************************************************************
 *  HY3131 Register Map Definitions
 ***************************************************************/
#define HY_REG_AD1_H       0x40
#define HY_REG_AD1_M       0x41
#define HY_REG_AD1_L       0x42

#define HY_REG_AD2_H       0x43
#define HY_REG_AD2_M       0x44
#define HY_REG_AD2_L       0x45

#define HY_REG_AD3_H       0x46
#define HY_REG_AD3_M       0x47
#define HY_REG_AD3_L       0x48

#define HY_REG_TEMP_H      0x50
#define HY_REG_TEMP_L      0x51

#define HY_REG_CTA0        0x30
#define HY_REG_CTA1        0x31
#define HY_REG_CTA2        0x32

/***************************************************************
 *  AD1 Reading (24-bit signed)
 ***************************************************************/
int32_t HY3131_Read_AD1(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD1_H, 3);

    int32_t raw = (b[0] << 16) | (b[1] << 8) | b[2];
    if (raw & 0x800000) raw |= 0xFF000000; // sign extend

    return raw;
}

/***************************************************************
 *  AD2 RMS Reading
 ***************************************************************/
uint32_t HY3131_Read_AD2_RMS(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD2_H, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/***************************************************************
 *  AD3 Peak Reading
 ***************************************************************/
uint32_t HY3131_Read_AD3_Peak(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_AD3_H, 3);
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/***************************************************************
 *  Temperature Reading
 ***************************************************************/
float HY3131_Read_Temperature(void)
{
    uint8_t b[2];
    HY3131_ReadData(b, HY_REG_TEMP_H, 2);

    uint16_t raw = (b[0] << 8) | b[1];
    return raw * 0.125f - 40.0f;
}

/***************************************************************
 *  CTA Frequency Counter Reading
 ***************************************************************/
uint32_t HY3131_Read_Frequency(void)
{
    uint8_t b[3];
    HY3131_ReadData(b, HY_REG_CTA0, 3);
    return (b[2] << 16) | (b[1] << 8) | b[0];
}

/***************************************************************
 *  HY3131 Driver (Part 2/3)
 *  This section includes:
 *    - Measurement mode enum
 *    - Range tables for DCV / ACV / OHM / Diode
 *    - Auto-range algorithms
 *    - AC/DC mode detection
 ***************************************************************/

/***************************************************************
 *  -------------------------
 *   DC VOLTAGE RANGE TABLE
 *  -------------------------
 *  Max Voltage | Divider
 *    0.6V      | x1
 *    6V        | x10
 *    60V       | x100
 *    600V      | x1000
 ***************************************************************/

typedef struct {
    float max_voltage;
    float divider;
    uint8_t reg20[20];        // HY3131 function config block
    const char *name;
} HY_DCV_RANGE_t;

static HY_DCV_RANGE_t dcv_ranges[] =
{
    { 0.6f,    1.0f,     {0},  "0.6V"   },
    { 6.0f,    10.0f,    {0},  "6V"     },
    { 60.0f,   100.0f,   {0},  "60V"    },
    { 600.0f,  1000.0f,  {0},  "600V"   },
};

static uint8_t dcv_range_index = 0;

/***************************************************************
 *  -------------------------
 *   AC VOLTAGE RANGE TABLE
 *  -------------------------
 *  Same as DCV (industry standard)
 ***************************************************************/
typedef struct {
    float max_rms;
    float divider;
    uint8_t reg20[20];
    const char *name;
} HY_ACV_RANGE_t;

static HY_ACV_RANGE_t acv_ranges[] =
{
    { 0.4f,    1.0f,     {0},  "0.4V AC"   },
    { 4.0f,    10.0f,    {0},  "4V AC"     },
    { 40.0f,   100.0f,   {0},  "40V AC"    },
    { 400.0f,  1000.0f,  {0},  "400V AC"   },
};

static uint8_t acv_range_index = 0;

/***************************************************************
 *  -------------------------
 *      OHM RANGE TABLE
 *  -------------------------
 *  600O   @ 1mA
 *  6kO    @ 100uA
 *  60kO   @ 10uA
 *  600kO  @ 1uA
 ***************************************************************/

typedef struct {
    float max_ohm;
    float current;        // excitation current
    uint8_t reg20[20];
    const char *name;
} HY_OHM_RANGE_t;

static HY_OHM_RANGE_t ohm_ranges[] =
{
    { 600.0f,      0.001f,     {0},  "600O"    },
    { 6000.0f,     0.0001f,    {0},  "6kO"     },
    { 60000.0f,    0.00001f,   {0},  "60kO"    },
    { 600000.0f,   0.000001f,  {0},  "600kO"   },
};

static uint8_t ohm_range_index = 0;

/***************************************************************
 *  -------------------------
 *  AUTO RANGE CORE ALGORITHM
 *  -------------------------
 ***************************************************************/
static uint8_t AutoRange_Generic(float value,
                                 float *limit_table,
                                 uint8_t *range,
                                 uint8_t max_range)
{
    uint8_t r = *range;

    // Increase range when close to upper limit (90%)
    if (value > limit_table[r] * 0.90f)
    {
        if (r < max_range - 1)
        {
            *range = r + 1;
            return 1;
        }
    }

    // Decrease range when too small (<10%)
    if (value < limit_table[r] * 0.10f)
    {
        if (r > 0)
        {
            *range = r - 1;
            return 1;
        }
    }

    return 0; // no change
}

/***************************************************************
 *  Write updated R20 block after range change
 ***************************************************************/
static void HY3131_Apply_R20(uint8_t *reg20)
{
    HY3131_WriteData(reg20, 0x20, 20);  // HY_REG_R20 = 0x20
    hy_delay_us(50);
}

/***************************************************************
 *  DC VOLTAGE auto-range
 ***************************************************************/
void HY3131_AutoRange_DCV(float voltage)
{
    float limits[] = {0.6f, 6.0f, 60.0f, 600.0f};

    uint8_t changed = AutoRange_Generic(voltage,
                                        limits,
                                        &dcv_range_index,
                                        4);

    if (changed)
    {
        HY3131_Apply_R20(dcv_ranges[dcv_range_index].reg20);
    }
}

/***************************************************************
 *  AC VOLTAGE (RMS) auto-range
 ***************************************************************/
void HY3131_AutoRange_ACV(float vrms)
{
    float limits[] = {0.4f, 4.0f, 40.0f, 400.0f};

    uint8_t changed = AutoRange_Generic(vrms,
                                        limits,
                                        &acv_range_index,
                                        4);

    if (changed)
    {
        HY3131_Apply_R20(acv_ranges[acv_range_index].reg20);
    }
}

/***************************************************************
 *  RESISTANCE auto-range
 ***************************************************************/
void HY3131_AutoRange_OHM(float R)
{
    float limits[] = {600.0f, 6000.0f, 60000.0f, 600000.0f};

    uint8_t changed = AutoRange_Generic(R,
                                        limits,
                                        &ohm_range_index,
                                        4);

    if (changed)
    {
        HY3131_Apply_R20(ohm_ranges[ohm_range_index].reg20);
    }
}

/***************************************************************
 *  Convert RAW AD1 reading to actual voltage (DC mode)
 ***************************************************************/
float HY3131_Convert_DCV(int32_t raw)
{
    float voltage = (float)raw / 50000.0f;              // convert to volts
    voltage *= dcv_ranges[dcv_range_index].divider;     // apply divider
    return voltage;
}

/***************************************************************
 *  Convert AD2 RMS reading to AC voltage
 ***************************************************************/
float HY3131_Convert_ACV(uint32_t rms_raw)
{
    float v = (float)rms_raw / 50000.0f;
    v *= acv_ranges[acv_range_index].divider;
    return v;
}

/***************************************************************
 *  Convert AD1 to Resistance (OHM)
 ***************************************************************/
float HY3131_Convert_OHM(int32_t raw)
{
    if (raw <= 0)
        return -1.0f;

    float voltage = (float)raw / 50000.0f;
    float Iex = ohm_ranges[ohm_range_index].current;
    return voltage / Iex;
}

/***************************************************************
 *  AC/DC AUTO-DETECTION
 *  Rule:
 *     if RMS >= 5% of DC magnitude ? AC
 *     else ? DC
 ***************************************************************/
MEASURE_MODE HY3131_Detect_AC_DC(int32_t raw_dc, uint32_t raw_rms)
{
    float vdc = (float)raw_dc / 50000.0f;
    float vac = (float)raw_rms / 50000.0f;

    if (vac > vdc * 0.05f)
        return MODE_ACV;
    else
        return MODE_DCV;
}



/***************************************************************
 *  --- Diode Measurement ---
 *  Rule:
 *    - Use AD1
 *    - Voltage drop < 0.2V = short
 *    - 0.2V~0.9V = typical diode drop
 *    - >1.0V = open
 ***************************************************************/
float HY3131_Measure_Diode(void)
{
    int32_t raw = HY3131_Read_AD1();
    float voltage = (float)raw / 50000.0f;
    return voltage; // direct reading (no divider)
}

/***************************************************************
 *  --- Continuity ---
 *  Rule:
 *    - Measure ohms
 *    - If R < 50O ? buzzer ON
 *    - If R > 100O ? buzzer OFF
 ***************************************************************/
uint8_t HY3131_Measure_Continuity(float *R)
{
    int32_t raw = HY3131_Read_AD1();
    float voltage = raw / 50000.0f;
    float ohms = voltage / ohm_ranges[ohm_range_index].current;

    *R = ohms;

    if (ohms < 50.0f)
        return 1;   // continuity
    else
        return 0;   // no continuity
}

/***************************************************************
 *  Unified Measurement Engine
 ***************************************************************/

HY_Result_t HY3131_Measure(MEASURE_MODE reqMode)
{
    HY_Result_t ret;
    memset(&ret, 0, sizeof(ret));

    switch (reqMode)
    {
        /***********************************************************
         * DC VOLTAGE
         ***********************************************************/
        case MODE_DCV:
        {
            int32_t raw = HY3131_Read_AD1();
            float voltage = HY3131_Convert_DCV(raw);

            HY3131_AutoRange_DCV(voltage);

            ret.value = voltage;
            ret.range = dcv_range_index;
            ret.mode  = MODE_DCV;
            break;
        }

        /***********************************************************
         * AC VOLTAGE (RMS)
         ***********************************************************/
        case MODE_ACV:
        {
            uint32_t raw = HY3131_Read_AD2_RMS();
            float vrms = HY3131_Convert_ACV(raw);

            HY3131_AutoRange_ACV(vrms);

            ret.value = vrms;
            ret.range = acv_range_index;
            ret.mode  = MODE_ACV;
            break;
        }

        /***********************************************************
         * OHM Measurement
         ***********************************************************/
        case MODE_OHM:
        {
            int32_t raw = HY3131_Read_AD1();
            float R = HY3131_Convert_OHM(raw);

            HY3131_AutoRange_OHM(R);

            ret.value = R;
            ret.range = ohm_range_index;
            ret.mode  = MODE_OHM;
            break;
        }

        /***********************************************************
         * Frequency (CTA counter)
         ***********************************************************/
        case MODE_FREQ:
        {
            uint32_t cnt = HY3131_Read_Frequency();
            ret.value = (float)cnt;   // user converts to Hz
            ret.mode  = MODE_FREQ;
            break;
        }

        /***********************************************************
         * Diode Mode
         ***********************************************************/
        case MODE_DIODE:
        {
            ret.value = HY3131_Measure_Diode();
            ret.mode  = MODE_DIODE;
            break;
        }

        /***********************************************************
         * Continuity Mode
         ***********************************************************/
        case MODE_CONTINUITY:
        {
            float R;
            uint8_t closed = HY3131_Measure_Continuity(&R);

            ret.value = R;
            ret.value2 = closed;
            ret.mode  = MODE_CONTINUITY;
            break;
        }
    }

    return ret;
}

/***************************************************************
 *  Auto AC/DC Measurement
 ***************************************************************/
HY_Result_t HY3131_Measure_AC_DC(void)
{
    int32_t raw_dc = HY3131_Read_AD1();
    uint32_t raw_rms = HY3131_Read_AD2_RMS();

    MEASURE_MODE m = HY3131_Detect_AC_DC(raw_dc, raw_rms);

    if (m == MODE_ACV)
        return HY3131_Measure(MODE_ACV);

    return HY3131_Measure(MODE_DCV);
}

/***************************************************************
 *  FreeRTOS Measurement Task
 ***************************************************************/
void HY3131_Task(void *argument)
{
    HY3131_SPI_Init();

    MEASURE_MODE mode = MODE_DCV;  // default

    for (;;)
    {
        HY_Result_t res;

        switch (mode)
        {
            case MODE_DCV:
            case MODE_ACV:
                // auto AC/DC
                res = HY3131_Measure_AC_DC();
                break;

            case MODE_OHM:
            case MODE_FREQ:
            case MODE_DIODE:
            case MODE_CONTINUITY:
                res = HY3131_Measure(mode);
                break;
        }

        /*****************************************************
         * Print debug data (you can remove or replace)
         *****************************************************/
        switch (res.mode)
        {
            case MODE_DCV:
                //printf("DCV = %.3f V (Range %d)\n", res.value, res.range);
                break;

            case MODE_ACV:
                //printf("ACV = %.3f V RMS (Range %d)\n", res.value, res.range);
                break;

            case MODE_OHM:
               // printf("OHM = %.2f O (Range %d)\n", res.value, res.range);
                break;

            case MODE_FREQ:
                //printf("FREQ counter = %.0f\n", res.value);
                break;

            case MODE_DIODE:
                //printf("DIODE = %.3f V\n", res.value);
                break;

            case MODE_CONTINUITY:
                //printf("CONTINUITY R=%.2f O  %s\n",
               //        res.value,
               //        (res.value2 ? "CLOSED" : "OPEN"));
                break;
        }

        //vTaskDelay(pdMS_TO_TICKS(200));
				hy_delay_us(200000);
    }
}


#endif
