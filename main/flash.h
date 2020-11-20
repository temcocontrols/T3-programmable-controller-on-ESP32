#ifndef __FLASH_H
#define __FLASH_H

#define STORAGE_NAMESPACE "storage"
#define FLASH_SSID_INFO		"SSID_INFO"
#define FLASH_MODBUS_ID		"MODBUS_ID"
#define FLASH_BAUD_RATE		"BAUD_RATE"

extern esp_err_t read_default_from_flash(void);
extern esp_err_t save_wifi_info(void);
extern esp_err_t save_uint8_to_flash(const char* key, uint8_t value);

#endif
