#ifndef MOCK_DRIVER_UART_H
#define MOCK_DRIVER_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define UART_NUM_0 0
#define UART_NUM_1 1
#define UART_NUM_2 2

/* ── EEPROM address map — missing in host build ── */
#ifndef EEP_UART0_STOPBIT
#define EEP_UART0_STOPBIT       0x00   /* replace with actual address if needed */
#endif

#ifndef EEP_UART2_STOPBIT
#define EEP_UART2_STOPBIT       0x01
#endif

#ifndef EEP_MAX_MASTER
#define EEP_MAX_MASTER          0x02
#endif

#define UART_PIN_NO_CHANGE -1

typedef enum {
    UART_DATA_5_BITS = 0x0,
    UART_DATA_6_BITS = 0x1,
    UART_DATA_7_BITS = 0x2,
    UART_DATA_8_BITS = 0x3,
    UART_DATA_BITS_MAX,
} uart_word_length_t;

typedef enum {
    UART_STOP_BITS_1   = 0x1,
    UART_STOP_BITS_1_5 = 0x2,
    UART_STOP_BITS_2   = 0x3,
    UART_STOP_BITS_MAX,
} uart_stop_bits_t;

typedef enum {
    UART_PARITY_DISABLE = 0x0,
    UART_PARITY_EVEN    = 0x2,
    UART_PARITY_ODD     = 0x3,
} uart_parity_t;

typedef enum {
    UART_HW_FLOWCTRL_DISABLE = 0x0,
    UART_HW_FLOWCTRL_RTS     = 0x1,
    UART_HW_FLOWCTRL_CTS     = 0x2,
    UART_HW_FLOWCTRL_CTS_RTS = 0x3,
    UART_HW_FLOWCTRL_MAX,
} uart_hw_flowcontrol_t;

typedef enum {
    UART_MODE_UART = 0x00,
    UART_MODE_RS485_HALF_DUPLEX = 0x01,
    UART_MODE_RS485_COLLISION_DETECT = 0x02,
    UART_MODE_RS485_APP_CTRL = 0x03,
} uart_mode_t;

typedef struct {
    int baud_rate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;
    uint8_t rx_flow_ctrl_thresh;
    uint32_t source_clk;
} uart_config_t;

typedef enum {
    UART_DATA,
    UART_BREAK,
    UART_BUFFER_FULL,
    UART_FIFO_OVF,
    UART_FRAME_ERR,
    UART_PARITY_ERR,
    UART_DATA_BREAK,
    UART_PATTERN_DET,
    UART_EVENT_MAX,
} uart_event_type_t;

typedef struct {
    uart_event_type_t type;
    size_t size;
} uart_event_t;

static inline esp_err_t uart_param_config(int uart_num, const uart_config_t *config) { (void)uart_num; (void)config; return ESP_OK; }
static inline esp_err_t uart_set_pin(int uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num) { (void)uart_num; (void)tx_io_num; (void)rx_io_num; (void)rts_io_num; (void)cts_io_num; return ESP_OK; }
static inline esp_err_t uart_driver_install(int uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, void *uart_queue, int intr_alloc_flags) { (void)uart_num; (void)rx_buffer_size; (void)tx_buffer_size; (void)queue_size; (void)uart_queue; (void)intr_alloc_flags; return ESP_OK; }
static inline esp_err_t uart_set_mode(int uart_num, uart_mode_t mode) { (void)uart_num; (void)mode; return ESP_OK; }
static inline int uart_read_bytes(int uart_num, void *buf, uint32_t length, TickType_t ticks_to_wait) { (void)uart_num; (void)buf; (void)length; (void)ticks_to_wait; return 0; }
static inline int uart_write_bytes(int uart_num, const void *src, size_t size) { (void)uart_num; (void)src; (void)size; return size; }

#define UART_SCLK_DEFAULT 0

static inline bool uart_is_driver_installed(int uart_num) { (void)uart_num; return true; }
static inline esp_err_t uart_driver_delete(int uart_num) { (void)uart_num; return ESP_OK; }
static inline esp_err_t uart_flush(int uart_num) { (void)uart_num; return ESP_OK; }

#endif // MOCK_DRIVER_UART_H
