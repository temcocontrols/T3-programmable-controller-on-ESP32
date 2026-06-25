#ifndef MOCK_DRIVER_I2C_H
#define MOCK_DRIVER_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_NUM_0 0
#define I2C_NUM_1 1

typedef int i2c_port_t;
typedef void* i2c_cmd_handle_t;
typedef int i2c_mode_t;

#define I2C_MASTER_WRITE 0
#define I2C_MASTER_READ  1

#define I2C_MODE_MASTER  1
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLUP_DISABLE 0

#define I2C_MASTER_ACK 0
#define I2C_MASTER_NACK 1
#define I2C_MASTER_LAST_NACK 2

typedef struct {
    i2c_mode_t mode;
    int sda_io_num;
    int scl_io_num;
    gpio_pullup_t sda_pullup_en;
    gpio_pullup_t scl_pullup_en;
    struct {
        uint32_t clk_speed;
    } master;
} i2c_config_t;

static inline esp_err_t i2c_driver_install(i2c_port_t i2c_num, int mode, size_t slv_rx_buf_len, size_t slv_tx_buf_len, int intr_alloc_flags)
{
    (void)i2c_num; (void)mode; (void)slv_rx_buf_len; (void)slv_tx_buf_len; (void)intr_alloc_flags;
    return ESP_OK;
}

i2c_cmd_handle_t i2c_cmd_link_create(void);
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en);
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd, const uint8_t *data, size_t data_len, bool ack_en);
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd, uint8_t *data, int ack_val);
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd, uint8_t *data, size_t data_len, int ack_val);
esp_err_t i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd, TickType_t ticks_to_wait);
esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf);

#ifdef __cplusplus
}
#endif

#endif // MOCK_DRIVER_I2C_H
