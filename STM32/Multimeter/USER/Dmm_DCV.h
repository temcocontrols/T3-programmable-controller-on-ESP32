/******************************************************************************/
/* DMM_DCV.h                                                                  */
/* DC Voltage measurement interfaces (header)                                 */
/*                                                                            */
/* Provides prototypes for AD1-based DCV processing, zero calibration and     */
/* DCV module initialization.                                                  */
/******************************************************************************/

#ifndef DMM_DCV_H
#define DMM_DCV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Process AD1 buffer value for DC voltage measurement */
void AD1_DCV_PROC(signed int AD1Data_buff);

/* Acquire zero reference for 6V range */
void Get_Volt6V_Zero(void);

/* Initialize DCV measurement subsystem */
void DCV_initial(void);

#ifdef __cplusplus
}
#endif

#endif /* DMM_DCV_H */