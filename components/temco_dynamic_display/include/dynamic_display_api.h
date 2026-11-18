#ifndef TEMCO_DYNAMIC_DISPLAY_API_H
#define TEMCO_DYNAMIC_DISPLAY_API_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts the Dynamic Display REST API HTTP Server.
 *
 * Initializes screen flash storage, mounts SPIFFS screen_data partition,
 * seeds default screen JSONs from firmware if not present, and starts
 * the REST API server endpoints under /api/eez-device/.
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t dynamic_display_api_start(void);

/**
 * @brief Stops the Dynamic Display REST API server.
 */
void dynamic_display_api_stop(void);

/**
 * @brief Resets screen storage back to factory defaults.
 * Can be called from Modbus/BACnet command handlers.
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t dynamic_display_reset_to_defaults(void);

#ifdef __cplusplus
}
#endif

#endif // TEMCO_DYNAMIC_DISPLAY_API_H
