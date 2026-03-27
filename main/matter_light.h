/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Matter Light
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t matter_light_init(void);

/**
 * @brief Get Matter Light On/Off state
 *
 * @return true if light is on, false if off
 */
bool matter_light_get_power(void);

/**
 * @brief Set Matter Light On/Off state
 *
 * Override this function in your application to control the actual light hardware.
 * Default implementation just stores the value.
 *
 * @param power true to turn on, false to turn off
 */
void matter_light_set_power(bool power);

/**
 * @brief Get Matter Light brightness level (0-254)
 *
 * @return brightness level
 */
uint8_t matter_light_get_brightness(void);

/**
 * @brief Set Matter Light brightness level
 *
 * Override this function in your application to control the actual light brightness.
 * Default implementation just stores the value.
 *
 * @param level brightness level (0-254, where 254 is maximum)
 */
void matter_light_set_brightness(uint8_t level);

/**
 * @brief Update Matter Light On/Off attribute from application
 *
 * Call this from your application when the light power state changes
 * (e.g., from physical button press)
 *
 * @param power true to turn on, false to turn off
 * @return ESP_OK on success
 */
esp_err_t matter_light_update_power(bool power);

/**
 * @brief Update Matter Light brightness attribute from application
 *
 * Call this from your application when the light brightness changes
 * (e.g., from physical dimmer)
 *
 * @param level brightness level (0-254)
 * @return ESP_OK on success
 */
esp_err_t matter_light_update_brightness(uint8_t level);

#ifdef __cplusplus
}
#endif
