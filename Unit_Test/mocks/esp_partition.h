#ifndef ESP_PARTITION_H
#define ESP_PARTITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_ota_ops.h" // for esp_partition_t

typedef enum {
    ESP_PARTITION_TYPE_APP = 0x00,
    ESP_PARTITION_TYPE_DATA = 0x01,
} esp_partition_type_t;

typedef enum {
    ESP_PARTITION_SUBTYPE_APP_FACTORY = 0x00,
    ESP_PARTITION_SUBTYPE_ANY = 0xff,
} esp_partition_subtype_t;

const esp_partition_t* esp_partition_find_first(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* label);
esp_err_t esp_partition_erase_range(const esp_partition_t* partition, uint32_t offset, uint32_t size);
esp_err_t esp_partition_write(const esp_partition_t* partition, uint32_t dst_offset, const void* src, uint32_t size);
esp_err_t esp_partition_read(const esp_partition_t* partition, uint32_t src_offset, void* dst, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // ESP_PARTITION_H
