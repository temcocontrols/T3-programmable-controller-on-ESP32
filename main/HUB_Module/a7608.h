#ifndef A7608_H
#define A7608_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define A7608_DEFAULT_UART_NUM             UART_NUM_1
#define A7608_DEFAULT_BAUD_RATE            115200
#define A7608_DEFAULT_RX_BUFFER_SIZE       2048

#define A7608_DEFAULT_MODEM_TX_PIN         GPIO_NUM_17
#define A7608_DEFAULT_MODEM_RX_PIN         GPIO_NUM_18
#define A7608_DEFAULT_PWRKEY_PIN           GPIO_NUM_15
#define A7608_DEFAULT_RESET_PIN            GPIO_NUM_16
#define A7608_DEFAULT_RING_PIN             GPIO_NUM_6
#define A7608_DEFAULT_DTR_PIN              GPIO_NUM_7

#define A7608_OPERATOR_LEN                 32
#define A7608_IP_ADDR_LEN                  40
#define A7608_LAST_ERROR_LEN               64
#define A7608_GNSS_UTC_LEN                 24
#define A7608_GNSS_COORD_LEN               20

typedef enum {
    A7608_STATE_OFF = 0,
    A7608_STATE_BOOTING,
    A7608_STATE_AT_READY,
    A7608_STATE_SIM_READY,
    A7608_STATE_REGISTERED,
    A7608_STATE_ATTACHED,
    A7608_STATE_CONNECTED,
    A7608_STATE_ERROR,
} a7608_state_t;

typedef enum
{
    A7608_SERVICE_RUNNING = 0,
    A7608_SERVICE_PAUSE_REQUESTED,
    A7608_SERVICE_PAUSED,
    A7608_SERVICE_RESUME_REQUESTED,
    A7608_SERVICE_ERROR,
} a7608_service_state_t;

typedef struct {
    uart_port_t uart_num;
    int baud_rate;
    int rx_buffer_size;

    gpio_num_t modem_tx_pin;
    gpio_num_t modem_rx_pin;
    gpio_num_t pwrkey_pin;
    gpio_num_t reset_pin;
    gpio_num_t ring_pin;
    gpio_num_t dtr_pin;

    int pwrkey_active_level;
    int reset_active_level;
    int dtr_active_level;

    bool reset_uart_driver;
    bool configure_control_pins;
} a7608_config_t;

typedef struct {
    a7608_state_t state;
    bool at_ready;
    bool sim_ready;
    bool registered_home;
    bool registered_roaming;
    bool attached;
    bool connected;
    int csq;
    int rssi_dbm;
    char operator_name[A7608_OPERATOR_LEN];
    char ip_addr[A7608_IP_ADDR_LEN];
    bool gnss_powered;
    bool gnss_fix;
    char gnss_utc[A7608_GNSS_UTC_LEN];
    char gnss_latitude[A7608_GNSS_COORD_LEN];
    char gnss_ns;
    char gnss_longitude[A7608_GNSS_COORD_LEN];
    char gnss_ew;
    char last_error[A7608_LAST_ERROR_LEN];
} a7608_status_t;

void a7608_get_default_config(a7608_config_t *config);
esp_err_t a7608_init(const a7608_config_t *config);
esp_err_t a7608_deinit(void);

esp_err_t a7608_power_on(uint32_t pulse_ms, uint32_t boot_wait_ms);
esp_err_t a7608_hard_reset(uint32_t pulse_ms, uint32_t boot_wait_ms);
esp_err_t a7608_set_dtr(bool active);
esp_err_t a7608_set_dtr_level(int level);
int a7608_get_dtr_level(void);

esp_err_t a7608_send_command(const char *cmd,
                             const char *expected,
                             uint32_t timeout_ms,
                             char *response,
                             size_t response_len);
esp_err_t a7608_probe(void);
esp_err_t a7608_refresh_status(void);
esp_err_t a7608_gnss_enable(void);
esp_err_t a7608_gnss_disable(void);
esp_err_t a7608_refresh_gnss(void);

void a7608_at_debug_task(void *pvParameters);

const a7608_status_t *a7608_get_status(void);
const char *a7608_state_name(a7608_state_t state);
esp_err_t a7608_request_pause(void);
esp_err_t a7608_request_resume(void);
bool a7608_is_pause_requested(void);
bool a7608_is_paused(void);
a7608_service_state_t a7608_get_service_state(void);
const char *a7608_service_state_name(a7608_service_state_t state);

#ifdef __cplusplus
}
#endif

#endif
