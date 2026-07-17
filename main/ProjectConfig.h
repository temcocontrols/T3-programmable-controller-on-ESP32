#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

/**
 * @file ProjectConfig.h
 * @brief Main project configuration file for setting compile-time feature toggles.
 *
 * This file contains macros to enable or disable optional features like the mmWave
 * Radar sensor, Vital Signs sensor, and their associated background tasks and web server.
 */

/**
 * @brief Enable/Disable the mmWave Radar Sensor.
 *
 * Set this macro to 1 to compile and enable the mmWave Radar parser task (UART1),
 * or set it to 0 to disable and exclude it from running.
 */
#define ENABLE_MM_WAVE_RADAR       1

#if ENABLE_MM_WAVE_RADAR
/**
 * @brief UART configuration for the mmWave Radar Sensor (LD6001).
 *
 * These macros define the UART number, TX pin, RX pin, and baud rate for the
 * mmWave Radar sensor. Adjust these values according to your hardware setup.
 */
#define RADAR_UART_NUM           UART_NUM_1
#define RADAR_TX_PIN             3
#define RADAR_RX_PIN             4

#endif

/**
 * @brief Enable/Disable the Vital Signs Sensor.
 *
 * Set this macro to 1 to compile and enable the Vital Signs tracker task (UART2),
 * or set it to 0 to disable and exclude it from running.
 */
#define ENABLE_VITAL_SIGNS         1

#if ENABLE_VITAL_SIGNS

/**
 * @brief UART configuration for the Vital Signs Sensor (LD6002).
 *
 * These macros define the UART number, TX pin, RX pin, and baud rate for the
 * Vital Signs sensor. Adjust these values according to your hardware setup.
 */
#define LD6002_UART_NUM  UART_NUM_2
#define LD6002_TX_PIN    1
#define LD6002_RX_PIN    2

#endif

/**
 * @brief Enable/Disable the Web Server for mmWave Sensors.
 *
 * Set this macro to 1 to compile and enable the WebSocket/HTTP web server that serves
 * data from the mmWave Radar and Vital Signs sensors, or set it to 0 to disable it.
 */
#define ENABLE_WEB_SERVER_RADAR       1

/**
 * @brief Include the mmWave radar initialization helper header if either sensor is enabled.
 *
 * This header provides the function to initialize the mmWave Radar and Vital Signs sensors
 * based on the configuration macros defined above.
 */
#if ENABLE_VITAL_SIGNS || ENABLE_MM_WAVE_RADAR
	// Initialize mmWave Radar and/or Vital Signs sensors based on ProjectConfig.h
	#include "mm_wave_radar_init.h"
#endif

#endif // PROJECT_CONFIG_H
