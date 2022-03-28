#ifndef __FLASH_H
#define __FLASH_H

#include "esp_err.h"


extern uint8_t ChangeFlash;
extern uint16_t count_write_Flash;

enum
{
	FLASH_BLOCK1_SSID,
	FLASH_BLOCK2_PN,
}E_BLOCK;
#define STORAGE_NAMESPACE   "storage"
#define FLASH_SSID_INFO		"SSID_INFO"
#define FLASH_MODBUS_ID		"MODBUS_ID"
#define FLASH_BAUD_RATE		"BAUD_RATE"
#define FLASH_BAUD_RATE2	"BAUD_RATE2"
#define FLASH_INPUT_INFO	"INPUT_INFO"
#define FLASH_INPUT_FLAG	"INPUT_FLAG"
#define FLASH_OUTPUT_INFO	"OUTPUT_INFO"
#define FLASH_OUTPUT_FLAG	"OUTPUT_FLAG"
#define FLASH_SERIAL_NUM1	"SERIAL_NUM_1"
#define FLASH_SERIAL_NUM2	"SERIAL_NUM_2"
#define FLASH_SERIAL_NUM3	"SERIAL_NUM_3"
#define FLASH_SERIAL_NUM4	"SERIAL_NUM_4"
#define FLASH_PANEL_NAME	"PANEL_NAME"
#define FLASH_UART2_CONFIG	"UART2_CONFIG"
#define FLASH_UART_CONFIG	"UART0_CONFIG"
#define FLASH_SN_WRITE		"SN_WRITE"
#define FLASH_MINI_TYPE		"MINI_TYPE"


#define FLASH_POINT_OUT		"POINT_OUT"
#define FLASH_POINT_IN		"POINT_IN"
#define FLASH_POINT_VAR		"POINT_VAR"
#define FLASH_POINT_CON		"POINT_CON"
#define FLASH_POINT_WR		"POINT_WR"
#define FLASH_POINT_AR		"POINT_AR"
#define FLASH_POINT_PRG		"POINT_PRG"
#define FLASH_POINT_TZ		"POINT_TZ"
#define FLASH_POINT_AMON	"POINT_AMON"
#define FLASH_POINT_GRP		"POINT_GRP"

#define FLASH_POINT_ARRAY			"POINT_ARRAY"
#define FLASH_POINT_ALARMM			"POINT_ALARMM"
#define FLASH_POINT_UNIT			"POINT_UNIT"
#define FLASH_POINT_USER_NAME		"POINT_USER_NAME"
#define FLASH_POINT_ALARM_SET		"POINT_ALARM_SET"
#define FLASH_POINT_WR_TIME			"POINT_WR_TIME"
#define FLASH_POINT_AR_DATA			"POINT_AR_DATA"
#define FLASH_POINT_GRP_POINT		"POINT_GRP_POINT"
#define FLASH_POINT_TBL				"POINT_TBL"
#define FLASH_POINT_ID_ROUTION		"POINT_ID_ROUTION"

/*#define FLASH_SOUND_TRIGGER_VALUE	"SOUND_TRIGGER_VALUE"
#define FLASH_SOUND_TRIGGER_TIMER	"SOUND_TRIGGER_TIMER"
#define FLASH_LIGHT_TRIGGER_VALUE	"LIGHT_TRIGGER_VALUE"
#define FLASH_LIGHT_TRIGGER_TIMER	"LIGHT_TRIGGER_TIMER"
#define FLASH_CO2_TRIGGER_VALUE	"CO2_TRIGGER_VALUE"
#define FLASH_CO2_TRIGGER_TIMER	"CO2_TRIGGER_TIMER"
#define FLASH_OCC_TRIGGER_VALUE	"OCC_TRIGGER_VALUE"
#define FLASH_OCC_TRIGGER_TIMER	"OCC_TRIGGER_TIMER"
#define FLASH_SHT31_TEMP_OFFSET "SHT31_TEMP_OFFSET"
#define FLASH_10K_TEMP_OFFSET	"10K_TEMP_OFFSET"
#define FLASH_AMBIENT_TEMP_OFFSET	"AMBIENT_TEMP_OFFSET"
#define FLASH_OBJECT_TEMP_OFFSET	"OBJECT_TEMP_OFFSET"*/

extern esp_err_t save_block(uint8_t key);
extern esp_err_t read_default_from_flash(void);
extern esp_err_t save_wifi_info(void);
extern esp_err_t save_point_info(uint8_t point_type);
extern esp_err_t save_uint8_to_flash(const char* key, uint8_t value);
extern esp_err_t save_uint16_to_flash(const char* key, uint16_t value);
extern esp_err_t read_uint8_from_falsh(const char* key, uint8_t* value);
extern esp_err_t read_uint16_from_falsh(const char* key, uint16_t* value);
extern esp_err_t read_blob_info(const char* key, const void* pValue, size_t length);
extern esp_err_t save_blob_info(const char* key, const void* pValue, size_t length);
extern esp_err_t save_int16_to_flash(const char* key, int16_t value);

extern void Flash_Inital(void);
extern void read_point_info(void);
#endif
