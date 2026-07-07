#ifndef ESP_HTTPS_OTA_H
#define ESP_HTTPS_OTA_H

#include "esp_http_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const esp_http_client_config_t *http_config;
} esp_https_ota_config_t;

int esp_https_ota(const esp_https_ota_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // ESP_HTTPS_OTA_H
