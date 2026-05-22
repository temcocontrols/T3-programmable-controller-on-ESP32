
/**
 * @file  touch_drv.h
 * @brief Touch Driver Implementation for the LCD Subsystem.
 * Detailed description:
 * - This module implements the touch driver for the LCD subsystem.
 * Responsibilities:
 * - Initialize the touch driver.
 * - Handle I2C communication with the touch controller.
 * Implement touch event reading and processing.
 *
 * @author  Bhavik Panchal
 * @date    22-12-2025
 * @version 1.0
 *
 */

#ifndef TOUCH_DRV_H
#define TOUCH_DRV_H

#include <stdbool.h>
#include <stdint.h>


/*******************************************************************************
  * Register      : P1_XL
  * Address       : 0x8150U
  * Bit Group Name: First Touch X Position
  * Permission    : R
  * Default value : 0x00U
  *******************************************************************************/
#define   GT911_P1_XL_TP_BIT_MASK        0xFFU
#define   GT911_P1_XL_TP_BIT_POSITION    0U

/*******************************************************************************
  * Register      : P1_XH
  * Address       : 0x8151U
  * Bit Group Name: First Touch X Position
  * Permission    : R
  * Default value : 0x00U
  *******************************************************************************/
#define   GT911_P1_XH_TP_BIT_MASK        0x0FU
#define   GT911_P1_XH_TP_BIT_POSITION    1U

/*******************************************************************************
  * Register      : P1_YL
  * Address       : 0x8152U
  * Bit Group Name: First Touch Y Position
  * Permission    : R
  * Default value : 0x00U
  *******************************************************************************/
#define   GT911_P1_YL_TP_BIT_MASK        0xFFU
#define   GT911_P1_YL_TP_BIT_POSITION    2U

/*******************************************************************************
  * Register      : P1_YH
  * Address       : 0x8153U
  * Bit Group Name: First Touch Y Position
  * Permission    : R
  * Default value : 0x00U
  *******************************************************************************/
#define   GT911_P1_YH_TP_BIT_MASK        0x0FU
#define   GT911_P1_YH_TP_BIT_POSITION    3U


void Touch_Drv_Init(void);
bool TouchGetInputs(uint16_t *x, uint16_t *y);

#endif // TOUCH_DRV_H
