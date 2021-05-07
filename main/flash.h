#ifndef __FLASH_H
#define __FLASH_H

#include "esp_err.h"

#define STORAGE_NAMESPACE "storage"
#define FLASH_SSID_INFO		"SSID_INFO"
#define FLASH_MODBUS_ID		"MODBUS_ID"
#define FLASH_BAUD_RATE		"BAUD_RATE"
#define FLASH_INPUT_INFO	"INPUT_INFO"
#define FLASH_INPUT_FLAG	"INPUT_FLAG"
#define FLASH_OUTPUT_INFO	"OUTPUT_INFO"
#define FLASH_OUTPUT_FLAG	"OUTPUT_FLAG"
#define FLASH_SERIAL_NUM_LO	"SERIAL_NUM_LO"
#define FLASH_SERIAL_NUM_HI	"SERIAL_NUM_HI"
#define FLASH_SOUND_TRIGGER_VALUE	"SOUND_TRIGGER_VALUE"
#define FLASH_SOUND_TRIGGER_TIMER	"SOUND_TRIGGER_TIMER"
#define FLASH_LIGHT_TRIGGER_VALUE	"LIGHT_TRIGGER_VALUE"
#define FLASH_LIGHT_TRIGGER_TIMER	"LIGHT_TRIGGER_TIMER"
#define FLASH_CO2_TRIGGER_VALUE	"CO2_TRIGGER_VALUE"
#define FLASH_CO2_TRIGGER_TIMER	"CO2_TRIGGER_TIMER"
#define FLASH_OCC_TRIGGER_VALUE	"OCC_TRIGGER_VALUE"
#define FLASH_OCC_TRIGGER_TIMER	"OCC_TRIGGER_TIMER"

extern esp_err_t read_default_from_flash(void);
extern esp_err_t save_wifi_info(void);
extern esp_err_t save_uint8_to_flash(const char* key, uint8_t value);
extern esp_err_t save_uint16_to_flash(const char* key, uint16_t value);
extern esp_err_t read_uint8_from_falsh(const char* key, uint8_t* value);
extern esp_err_t read_uint16_from_falsh(const char* key, uint16_t* value);
extern esp_err_t read_blob_info(const char* key, const void* pValue, size_t length);
extern esp_err_t save_blob_info(const char* key, const void* pValue, size_t length);

#endif
