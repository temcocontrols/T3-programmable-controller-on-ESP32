#ifndef HUB_LTE_PPPOS_H
#define HUB_LTE_PPPOS_H

#include <stdbool.h>
#include <stddef.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HUB_LTE_PPPOS_ENABLE
#define HUB_LTE_PPPOS_ENABLE 0
#endif

#ifndef HUB_LTE_PPPOS_TEST_MODE
#define HUB_LTE_PPPOS_TEST_MODE 0
#endif

#define HUB_LTE_PPPOS_APN_LEN          64
#define HUB_LTE_PPPOS_IP_ADDR_LEN      40
#define HUB_LTE_PPPOS_PREFLIGHT_APN_LEN 32
#define HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN 96

typedef enum
{
    HUB_PPP_STATE_IDLE = 0,
    HUB_PPP_STATE_WAIT_UART,
    HUB_PPP_STATE_MODEM_READY,
    HUB_PPP_STATE_STARTING,
    HUB_PPP_STATE_RUNNING,
    HUB_PPP_STATE_STOPPING,
    HUB_PPP_STATE_ERROR,
} hub_ppp_state_t;

typedef hub_ppp_state_t hub_lte_pppos_state_t;

#define HUB_LTE_PPPOS_STATE_IDLE         HUB_PPP_STATE_IDLE
#define HUB_LTE_PPPOS_STATE_READY        HUB_PPP_STATE_MODEM_READY
#define HUB_LTE_PPPOS_STATE_STARTING     HUB_PPP_STATE_STARTING
#define HUB_LTE_PPPOS_STATE_CONNECTED    HUB_PPP_STATE_RUNNING
#define HUB_LTE_PPPOS_STATE_DISCONNECTED HUB_PPP_STATE_IDLE
#define HUB_LTE_PPPOS_STATE_STOPPING     HUB_PPP_STATE_STOPPING
#define HUB_LTE_PPPOS_STATE_ERROR        HUB_PPP_STATE_ERROR

typedef enum {
    HUB_LTE_PPPOS_UART_OWNER_IDLE = 0,
    HUB_LTE_PPPOS_UART_OWNER_AT_STATUS,
    HUB_LTE_PPPOS_UART_OWNER_PPPOS,
} hub_lte_pppos_uart_owner_t;

typedef struct {
    uart_port_t uart_num;
    int baud_rate;
    int rx_buffer_size;
    int tx_buffer_size;
    gpio_num_t tx_io_num;
    gpio_num_t rx_io_num;
    gpio_num_t rts_io_num;
    gpio_num_t cts_io_num;
    char apn[HUB_LTE_PPPOS_APN_LEN];
} hub_lte_pppos_config_t;

typedef struct {
    bool initialized;
    bool start_requested;
    bool stop_requested;
    bool connected;
    hub_lte_pppos_state_t state;
    hub_lte_pppos_uart_owner_t uart_owner;
    char ip_addr[HUB_LTE_PPPOS_IP_ADDR_LEN];
} hub_lte_pppos_status_t;

typedef struct {
    bool config_valid;
    bool test_mode_enabled;
    bool pppos_enabled;

    bool uart_available;
    int uart_owner;

    bool modem_status_known;
    bool sim_ready;
    bool registered_to_network;
    bool has_signal;
    int rssi;

    bool has_apn;
    char apn[HUB_LTE_PPPOS_PREFLIGHT_APN_LEN];

    bool ready_to_start;
    char reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN];
} hub_lte_pppos_preflight_t;

typedef struct
{
    bool ppp_netif_created;
    bool modem_created;
    bool data_mode_entered;
    bool ppp_started;
} hub_lte_pppos_runtime_t;

void hub_lte_pppos_get_default_config(hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_init(void);
esp_err_t hub_lte_pppos_init_with_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_validate_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_get_config(hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_set_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_set_uart_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_request_start(void);
esp_err_t hub_lte_pppos_request_stop(void);
bool hub_lte_pppos_start_requested(void);
bool hub_lte_pppos_stop_requested(void);
esp_err_t hub_lte_pppos_start(void);
esp_err_t hub_lte_pppos_stop(void);
bool hub_lte_pppos_is_enabled(void);
bool hub_lte_pppos_is_running(void);
bool hub_lte_pppos_is_connected(void);
bool hub_lte_pppos_can_take_uart(void);
hub_ppp_state_t hub_lte_pppos_get_state(void);
const char *hub_lte_pppos_state_name(hub_ppp_state_t state);
esp_err_t hub_lte_pppos_set_state(hub_ppp_state_t new_state, esp_err_t reason);
esp_err_t hub_lte_pppos_process(void);
esp_err_t hub_lte_pppos_request_uart_owner(void);
esp_err_t hub_lte_pppos_release_uart_owner(void);
esp_err_t hub_lte_pppos_set_connected(bool connected, const char *ip_addr);
esp_err_t hub_lte_pppos_get_status(hub_lte_pppos_status_t *status);
const char *hub_lte_pppos_get_ip_addr(void);
esp_err_t hub_lte_pppos_preflight_check(hub_lte_pppos_preflight_t *preflight);
const char *hub_lte_pppos_preflight_reason(void);
esp_err_t hub_lte_pppos_get_runtime(hub_lte_pppos_runtime_t *runtime);
esp_err_t hub_lte_pppos_reset_runtime(void);
esp_err_t hub_lte_pppos_get_last_error(void);
const char *hub_lte_pppos_get_last_reason(void);

#ifdef __cplusplus
}
#endif

#endif
