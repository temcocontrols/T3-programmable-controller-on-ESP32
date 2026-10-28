#ifndef SCREEN_FLASH_STORE_H
#define SCREEN_FLASH_STORE_H

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SPIFFS partition screen_data and seed default screens if needed.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t screen_store_init(void);

/**
 * @brief Check if SPIFFS storage is mounted and initialized.
 */
bool screen_store_is_ready(void);

/**
 * @brief Read single screen JSON content.
 * 
 * @param name Screen identifier (e.g. "home_screen")
 * @param out_buf Pointer to allocated buffer output (caller must free(*out_buf))
 * @param out_size Pointer to output buffer size
 * @return esp_err_t ESP_OK if found and read
 */
esp_err_t screen_store_get_screen(const char *name, char **out_buf, size_t *out_size);

/**
 * @brief Save single screen JSON content to SPIFFS.
 * 
 * @param name Screen identifier (e.g. "home_screen")
 * @param json_data JSON string payload
 * @param len Payload length (or 0 for strlen)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t screen_store_save_screen(const char *name, const char *json_data, size_t len);

/**
 * @brief Delete single screen from SPIFFS.
 * 
 * @param name Screen identifier
 * @return esp_err_t ESP_OK on success
 */
esp_err_t screen_store_delete_screen(const char *name);

/**
 * @brief Reset all screens and images back to default compile-time defaults.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t screen_store_reset_to_defaults(void);

/**
 * @brief Get screen count and list of names.
 * 
 * @param names Array of char buffers to fill
 * @param max_count Maximum names to store
 * @param out_count Actual count returned
 * @return esp_err_t ESP_OK on success
 */
esp_err_t screen_store_list_screens(char names[][64], size_t max_count, size_t *out_count);

/**
 * @brief Image asset storage APIs.
 */
esp_err_t screen_store_save_image(const char *name, const char *json_payload);
esp_err_t screen_store_get_image(const char *name, char **out_buf, size_t *out_size);
esp_err_t screen_store_delete_image(const char *name);
esp_err_t screen_store_list_images(char names[][64], size_t max_count, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_FLASH_STORE_H
