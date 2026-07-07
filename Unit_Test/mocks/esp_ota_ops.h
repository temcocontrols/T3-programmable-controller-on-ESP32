#ifndef ESP_OTA_OPS_H
#define ESP_OTA_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

// Dummy types for compilation
typedef void* esp_ota_handle_t;
typedef struct {
    char label[16];
    uint32_t address;
    uint32_t size;
} esp_partition_t;

const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t* start_from);
int esp_ota_begin(const esp_partition_t* partition, size_t image_size, esp_ota_handle_t* out_handle);
int esp_ota_write(esp_ota_handle_t handle, const void* data, size_t size);
int esp_ota_end(esp_ota_handle_t handle);
int esp_ota_set_boot_partition(const esp_partition_t* partition);

#ifdef __cplusplus
}
#endif

#endif // ESP_OTA_OPS_H
