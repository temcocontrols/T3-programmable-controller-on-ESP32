#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "flash.h"
#include "wifi.h"
#include "define.h"


esp_err_t save_uint8_to_flash(const char* key, uint8_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;
	// Open

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_u8(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_uint16_to_flash(const char* key, uint16_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_u16(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_int16_to_flash(const char* key, int16_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_i16(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_uint8_from_falsh(const char* key, uint8_t* value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	err = nvs_get_u8(my_handle, key, value);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		return ESP_ERR_NVS_NOT_FOUND;
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_uint16_from_falsh(const char* key, uint16_t* value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	err = nvs_get_u16(my_handle, key, value);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		return ESP_ERR_NVS_NOT_FOUND;
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_default_from_flash(void)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	// Open

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	debug_info("read_default_from_flash nvs_open\n");

	uint32_t len = sizeof(STR_SSID);
	err = nvs_get_blob(my_handle, FLASH_SSID_INFO, &SSID_Info, &len);

	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		//init_ssid_info();
		debug_info("The value is not initialized yet!\n");
	}
	if (err != ESP_OK) return err;
	err = nvs_get_u8(my_handle,FLASH_MODBUS_ID, &Modbus.address);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.address = 1;
		nvs_set_u8(my_handle, FLASH_MODBUS_ID, Modbus.address);
	}
	err = nvs_get_u8(my_handle, FLASH_BAUD_RATE, &Modbus.baudrate);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.baudrate = 4;
		nvs_set_u8(my_handle, FLASH_BAUD_RATE, Modbus.baudrate);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM1, &Modbus.serialNum[0]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[0] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM1, Modbus.serialNum[0]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM2, &Modbus.serialNum[1]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[1] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM2, Modbus.serialNum[1]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM3, &Modbus.serialNum[2]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[0] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM3, Modbus.serialNum[2]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM4, &Modbus.serialNum[3]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[1] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM4, Modbus.serialNum[3]);
	}

	debug_info("nvs_get_blob");
	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_wifi_info(void)
{
	nvs_handle_t my_handle;
	esp_err_t err;
	Test[13] = 1;
	debug_info("save_wifi_info\n");
	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	Test[13] = 2;
	err = nvs_set_blob(my_handle, FLASH_SSID_INFO, (const void*)(&SSID_Info), sizeof(STR_SSID));
	if (err != ESP_OK) return err;
	debug_info("nvs_set_blob\n");

	Test[13] = 3;
	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;
	debug_info("nvs_commit\n");
	Test[13] = 4;
	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_blob_info(const char* key, const void* pValue, size_t length)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_blob(my_handle, key, pValue, length);
	if (err != ESP_OK) return err;

	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_blob_info(const char* key, const void* pValue, size_t length)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_get_blob(my_handle, key, pValue, &length);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}
