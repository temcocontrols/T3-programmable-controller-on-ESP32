#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "flash.h"
#include "wifi.h"
#include "deviceparams.h"


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
		debug_info("The value is not initialized yet!\n");
	if (err != ESP_OK) return err;
	err = nvs_get_u8(my_handle,FLASH_MODBUS_ID, &holding_reg_params.modbus_address);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		holding_reg_params.modbus_address = 1;
		nvs_set_u8(my_handle, FLASH_MODBUS_ID, holding_reg_params.modbus_address);
	}
	err = nvs_get_u8(my_handle, FLASH_BAUD_RATE, &holding_reg_params.baud_rate);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		holding_reg_params.baud_rate = 4;
		nvs_set_u8(my_handle, FLASH_BAUD_RATE, holding_reg_params.baud_rate);
	}
#if 1
	err = nvs_get_u16(my_handle, FLASH_SERIAL_NUM_LO, &holding_reg_params.serial_number_lo);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		holding_reg_params.serial_number_lo = 26;
		nvs_set_u16(my_handle, FLASH_SERIAL_NUM_LO, holding_reg_params.serial_number_lo);
	}
	err = nvs_get_u16(my_handle, FLASH_SERIAL_NUM_HI, &holding_reg_params.serial_number_hi);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		holding_reg_params.serial_number_hi = 0;
		nvs_set_u16(my_handle, FLASH_SERIAL_NUM_HI, holding_reg_params.serial_number_hi);
	}
#endif
	err = nvs_get_u16(my_handle, FLASH_SOUND_TRIGGER_VALUE, &sound_trigger.trigger);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		sound_trigger.trigger = 80;
		nvs_set_u16(my_handle, FLASH_SOUND_TRIGGER_VALUE, sound_trigger.trigger);
	}
	err = nvs_get_u16(my_handle, FLASH_SOUND_TRIGGER_TIMER, &sound_trigger.timer);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		sound_trigger.timer = 1;
		nvs_set_u16(my_handle, FLASH_SOUND_TRIGGER_TIMER, sound_trigger.timer);
	}
	err = nvs_get_u16(my_handle, FLASH_LIGHT_TRIGGER_VALUE, &light_trigger.trigger);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		light_trigger.trigger = 1000;
		nvs_set_u16(my_handle, FLASH_LIGHT_TRIGGER_VALUE, light_trigger.trigger);
	}
	err = nvs_get_u16(my_handle, FLASH_LIGHT_TRIGGER_TIMER, &light_trigger.timer);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		light_trigger.timer = 1;
		nvs_set_u16(my_handle, FLASH_LIGHT_TRIGGER_TIMER, light_trigger.timer);
	}
	err = nvs_get_u16(my_handle, FLASH_CO2_TRIGGER_VALUE, &co2_trigger.trigger);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		co2_trigger.trigger = 2000;
		nvs_set_u16(my_handle, FLASH_CO2_TRIGGER_VALUE, co2_trigger.trigger);
	}
	err = nvs_get_u16(my_handle, FLASH_CO2_TRIGGER_TIMER, &co2_trigger.timer);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		co2_trigger.timer = 1;
		nvs_set_u16(my_handle, FLASH_CO2_TRIGGER_TIMER, co2_trigger.timer);
	}
	err = nvs_get_u16(my_handle, FLASH_OCC_TRIGGER_VALUE, &occ_trigger.trigger);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		occ_trigger.trigger = 200;
		nvs_set_u16(my_handle, FLASH_OCC_TRIGGER_VALUE, occ_trigger.trigger);
	}
	err = nvs_get_u16(my_handle, FLASH_OCC_TRIGGER_TIMER, &occ_trigger.timer);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		occ_trigger.timer = 1;
		nvs_set_u16(my_handle, FLASH_OCC_TRIGGER_TIMER, occ_trigger.timer);
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

	debug_info("save_wifi_info\n");
	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;


	err = nvs_set_blob(my_handle, FLASH_SSID_INFO, (const void*)(&SSID_Info), sizeof(STR_SSID));
	if (err != ESP_OK) return err;
	debug_info("nvs_set_blob\n");

	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;
	debug_info("nvs_commit\n");

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
