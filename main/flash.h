#ifndef __FLASH_H
#define __FLASH_H

#include "esp_err.h"

#define STORAGE_NAMESPACE "storage"
#define FLASH_SSID_INFO		"SSID_INFO"
#define FLASH_MODBUS_ID		"MODBUS_ID"
#define FLASH_BAUD_RATE		"BAUD_RATE"
#define FLASH_INPUT_INFO	"INPUT_INFO"
#define FLASH_INPUT_FLAG	"INPUT_FLAG"

extern esp_err_t read_default_from_flash(void);
extern esp_err_t save_wifi_info(void);
extern esp_err_t save_uint8_to_flash(const char* key, uint8_t value);
extern esp_err_t save_uint16_to_flash(const char* key, uint16_t value);
extern esp_err_t read_uint8_from_falsh(const char* key, uint8_t* value);
extern esp_err_t read_uint16_from_falsh(const char* key, uint16_t* value);
extern esp_err_t read_blob_info(const char* key, const void* pValue, size_t length);
extern esp_err_t save_blob_info(const char* key, const void* pValue, size_t length);

#endif
