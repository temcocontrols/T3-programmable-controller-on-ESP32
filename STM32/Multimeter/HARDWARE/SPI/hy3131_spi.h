#ifndef _HY3131_SPI_H_
#define _HY3131_SPI_H_

#include <stdint.h>

/* Mode */
typedef enum {
    MODE_DCV,
    MODE_ACV,
    MODE_OHM,
    MODE_DIODE,
    MODE_CONT,
    MODE_FREQ
} HY_MODE;

typedef struct {
    HY_MODE mode;
    float value;
    float extra;
} HY_Result;

/* Init */
void HY_SPI_Init(void);

/* Set mode */
void HY_Set_Mode(HY_MODE mode);

/* Measurement */
HY_Result HY_Measure(HY_MODE mode);

/* FreeRTOS task */
void HY3131_Task(void *argument);

#endif

#if 0
#ifndef _HY3131_SPI_H_
#define _HY3131_SPI_H_

#include <stdint.h>
#include "stm32f10x.h"

/***************************************************************
 *  HY3131 Measurement Modes
 ***************************************************************/
typedef enum {
    MODE_DCV = 0,
    MODE_ACV,
    MODE_OHM,
    MODE_FREQ,
    MODE_DIODE,
    MODE_CONTINUITY
} MEASURE_MODE;

/***************************************************************
 *  HY3131 Measurement Result Structure
 ***************************************************************/
typedef struct {
    float value;        // primary measurement value
    float value2;       // secondary (AC/DC detect, continuity flag etc)
    uint8_t range;      // selected range index
    MEASURE_MODE mode;  // final measurement mode used
} HY_Result_t;

/***************************************************************
 *  Public API (provided by hy3131_spi.c)
 ***************************************************************/

/* Initialize SPI1 + GPIO (PA4~PA7) for HY3131 */
void HY3131_SPI_Init(void);

/* Low-level ADC readings */
int32_t  HY3131_Read_AD1(void);
uint32_t HY3131_Read_AD2_RMS(void);
uint32_t HY3131_Read_AD3_Peak(void);
float    HY3131_Read_Temperature(void);
uint32_t HY3131_Read_Frequency(void);

/* Mode detection (AC vs DC) */
MEASURE_MODE HY3131_Detect_AC_DC(int32_t raw_dc, uint32_t raw_rms);

/* Conversions */
float HY3131_Convert_DCV(int32_t raw);
float HY3131_Convert_ACV(uint32_t rms_raw);
float HY3131_Convert_OHM(int32_t raw);

/* Auto-range functions */
void HY3131_AutoRange_DCV(float voltage);
void HY3131_AutoRange_ACV(float vrms);
void HY3131_AutoRange_OHM(float R);

/* Measurement functions */
HY_Result_t HY3131_Measure(MEASURE_MODE mode);
HY_Result_t HY3131_Measure_AC_DC(void);

/* Diode / Continuity */
float HY3131_Measure_Diode(void);
uint8_t HY3131_Measure_Continuity(float *R);

/* FreeRTOS measurement task */
void HY3131_Task(void *argument);

#endif
#endif