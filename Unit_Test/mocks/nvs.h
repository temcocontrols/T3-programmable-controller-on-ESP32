#ifndef NVS_H
#define NVS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t nvs_handle_t;
typedef enum {
    NVS_READWRITE,
    NVS_READONLY,
} nvs_open_mode_t;

#define ESP_ERR_NVS_NOT_FOUND          0x1101
#define ESP_ERR_NVS_NO_FREE_PAGES      0x1102
#define ESP_ERR_NVS_NEW_VERSION_FOUND  0x1103

int nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle);
int nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value);
int nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value);
int nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value);
int nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value);
int nvs_get_i16(nvs_handle_t handle, const char* key, int16_t* out_value);
int nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value);
int nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value);
int nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value);
int nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length);
int nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length);
int nvs_commit(nvs_handle_t handle);
void nvs_close(nvs_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // NVS_H
