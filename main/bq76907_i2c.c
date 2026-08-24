#include "bq76907_i2c.h"
#include "bq76907.h"

#include <string.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Share bus with existing ESP-IDF sensors: I2C1, SDA=GPIO4, SCL=GPIO14 */
#define BQ76907_I2C_PORT        I2C_NUM_1
#define BQ76907_I2C_SDA_GPIO    GPIO_NUM_4
#define BQ76907_I2C_SCL_GPIO    GPIO_NUM_14
#define BQ76907_I2C_FREQ_HZ     100000
#define BQ76907_I2C_TIMEOUT_MS  200

static uint8_t s_i2c_addr_7bit = BQ76907_I2C_ADDR_7BIT;
static uint8_t s_i2c_last_error = 0;
static uint8_t s_i2c_ready = 0;

uint8_t BQ76907_I2C_GetLastError(void)
{
	return s_i2c_last_error;
}

static esp_err_t bq_i2c_reinstall(void)
{
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = BQ76907_I2C_SDA_GPIO,
		.scl_io_num = BQ76907_I2C_SCL_GPIO,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = BQ76907_I2C_FREQ_HZ,
	};
	esp_err_t err = i2c_param_config(BQ76907_I2C_PORT, &conf);
	if (err != ESP_OK) {
		return err;
	}
	err = i2c_driver_install(BQ76907_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
	if (err == ESP_ERR_INVALID_STATE) {
		/* Already installed by i2c_master_init / another driver — reuse. */
		return ESP_OK;
	}
	return err;
}

void BQ76907_I2C_Init(void)
{
	s_i2c_last_error = 0;
	if (bq_i2c_reinstall() == ESP_OK) {
		s_i2c_ready = 1;
	} else {
		s_i2c_ready = 0;
		s_i2c_last_error = 0xFE;
	}
}

void BQ76907_I2C_BusRecovery(void)
{
	int i;

	s_i2c_last_error = 0;
	(void)i2c_driver_delete(BQ76907_I2C_PORT);
	s_i2c_ready = 0;

	gpio_set_direction(BQ76907_I2C_SCL_GPIO, GPIO_MODE_OUTPUT_OD);
	gpio_set_direction(BQ76907_I2C_SDA_GPIO, GPIO_MODE_OUTPUT_OD);
	gpio_set_level(BQ76907_I2C_SDA_GPIO, 1);
	for (i = 0; i < 9; i++) {
		gpio_set_level(BQ76907_I2C_SCL_GPIO, 1);
		esp_rom_delay_us(5);
		gpio_set_level(BQ76907_I2C_SCL_GPIO, 0);
		esp_rom_delay_us(5);
	}
	/* STOP: SDA low → SCL high → SDA high */
	gpio_set_level(BQ76907_I2C_SDA_GPIO, 0);
	esp_rom_delay_us(2);
	gpio_set_level(BQ76907_I2C_SCL_GPIO, 1);
	esp_rom_delay_us(5);
	gpio_set_level(BQ76907_I2C_SDA_GPIO, 1);
	esp_rom_delay_us(5);

	if (bq_i2c_reinstall() == ESP_OK) {
		s_i2c_ready = 1;
	}
}

static uint8_t bq_i2c_probe(uint8_t addr_7bit)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	esp_err_t ret;

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr_7bit << 1) | I2C_MASTER_WRITE, true);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(BQ76907_I2C_PORT, cmd, pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	i2c_cmd_link_delete(cmd);
	return (ret == ESP_OK) ? 0 : 1;
}

uint8_t BQ76907_I2C_Scan(uint8_t *addr_7bit)
{
	static const uint8_t addr_list[] = {0x08, 0x09, 0x0A, 0x0B};
	uint8_t i;

	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	for (i = 0; i < sizeof(addr_list); i++) {
		if (bq_i2c_probe(addr_list[i]) == 0) {
			s_i2c_addr_7bit = addr_list[i];
			if (addr_7bit != 0) {
				*addr_7bit = addr_list[i];
			}
			return 0;
		}
	}

	if (addr_7bit != 0) {
		*addr_7bit = 0xFF;
	}
	return 1;
}

uint8_t BQ76907_I2C_ReadReg(uint16_t reg, uint16_t *value)
{
	uint8_t reg_addr = (uint8_t)(reg & 0xFF);
	uint8_t data[2];
	esp_err_t ret;

	s_i2c_last_error = 0;
	if (value == 0) {
		return 1;
	}
	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	ret = i2c_master_write_read_device(BQ76907_I2C_PORT, s_i2c_addr_7bit,
					  &reg_addr, 1, data, 2,
					  pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	if (ret != ESP_OK) {
		s_i2c_last_error = 1;
		return 1;
	}
	*value = ((uint16_t)data[1] << 8) | data[0];
	return 0;
}

uint8_t BQ76907_I2C_ReadBlock(uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
	esp_err_t ret;

	s_i2c_last_error = 0;
	if (buf == 0 || len == 0) {
		return 1;
	}
	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	ret = i2c_master_write_read_device(BQ76907_I2C_PORT, s_i2c_addr_7bit,
					  &reg_addr, 1, buf, len,
					  pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	if (ret != ESP_OK) {
		s_i2c_last_error = 4;
		return 1;
	}
	return 0;
}

uint8_t BQ76907_I2C_WriteReg(uint16_t reg, uint16_t value)
{
	uint8_t buf[3];
	esp_err_t ret;

	s_i2c_last_error = 0;
	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	buf[0] = (uint8_t)(reg & 0xFF);
	buf[1] = (uint8_t)(value & 0xFF);
	buf[2] = (uint8_t)(value >> 8);
	ret = i2c_master_write_to_device(BQ76907_I2C_PORT, s_i2c_addr_7bit,
					buf, sizeof(buf),
					pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	if (ret != ESP_OK) {
		s_i2c_last_error = 5;
		return 1;
	}
	return 0;
}

uint8_t BQ76907_I2C_WriteByte(uint8_t reg_addr, uint8_t value)
{
	uint8_t buf[2];
	esp_err_t ret;

	s_i2c_last_error = 0;
	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	buf[0] = reg_addr;
	buf[1] = value;
	ret = i2c_master_write_to_device(BQ76907_I2C_PORT, s_i2c_addr_7bit,
					buf, sizeof(buf),
					pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	if (ret != ESP_OK) {
		s_i2c_last_error = 5;
		return 1;
	}
	return 0;
}

uint8_t BQ76907_I2C_WriteBlock(uint8_t reg_addr, const uint8_t *buf, uint8_t len)
{
	uint8_t tmp[33];
	esp_err_t ret;

	s_i2c_last_error = 0;
	if (buf == 0 || len == 0 || len > 32) {
		return 1;
	}
	if (!s_i2c_ready) {
		BQ76907_I2C_Init();
	}

	tmp[0] = reg_addr;
	memcpy(tmp + 1, buf, len);
	ret = i2c_master_write_to_device(BQ76907_I2C_PORT, s_i2c_addr_7bit,
					tmp, (size_t)len + 1,
					pdMS_TO_TICKS(BQ76907_I2C_TIMEOUT_MS));
	if (ret != ESP_OK) {
		s_i2c_last_error = 5;
		return 1;
	}
	return 0;
}
