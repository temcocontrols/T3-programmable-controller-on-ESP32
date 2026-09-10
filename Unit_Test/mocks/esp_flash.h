#ifndef ESP_FLASH_H
#define ESP_FLASH_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_flash_t esp_flash_t;

esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size);

#ifdef __cplusplus
}
#endif

#endif // ESP_FLASH_H
