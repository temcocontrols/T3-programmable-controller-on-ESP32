#ifndef ESP_SYSTEM_H
#define ESP_SYSTEM_H

#include "esp_partition.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void esp_restart(void);
uint32_t esp_get_free_heap_size(void);
uint32_t esp_get_minimum_free_heap_size(void);

#ifdef __cplusplus
}
#endif

#endif // ESP_SYSTEM_H
