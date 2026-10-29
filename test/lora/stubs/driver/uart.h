#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#define UART_NUM_1 1
#define UART_DATA_8_BITS 8
#define UART_PARITY_DISABLE 0
#define UART_STOP_BITS_1 1
#define UART_HW_FLOWCTRL_DISABLE 0
#define UART_SCLK_DEFAULT 0
#define UART_PIN_NO_CHANGE -1
#define UART_MODE_UART 0
typedef struct {
    int baud_rate, data_bits, parity, stop_bits, flow_ctrl, source_clk;
} uart_config_t;
bool uart_is_driver_installed(int port);
esp_err_t uart_driver_delete(int port);
esp_err_t uart_param_config(int port, const uart_config_t *config);
esp_err_t uart_set_pin(int port, int tx, int rx, int rts, int cts);
esp_err_t uart_driver_install(int port, int rx, int tx, int count, void *queue, int flags);
esp_err_t uart_set_mode(int port, int mode);
int uart_write_bytes(int port, const void *data, size_t len);
int uart_read_bytes(int port, void *data, size_t len, TickType_t timeout);
