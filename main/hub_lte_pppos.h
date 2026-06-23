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

#define HUB_LTE_PPPOS_APN_LEN          64
#define HUB_LTE_PPPOS_IP_ADDR_LEN      40

typedef enum {
    HUB_LTE_PPPOS_STATE_IDLE = 0,
    HUB_LTE_PPPOS_STATE_READY,
    HUB_LTE_PPPOS_STATE_STARTING,
    HUB_LTE_PPPOS_STATE_CONNECTED,
    HUB_LTE_PPPOS_STATE_DISCONNECTED,
    HUB_LTE_PPPOS_STATE_STOPPING,
    HUB_LTE_PPPOS_STATE_ERROR,
} hub_lte_pppos_state_t;

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
    bool connected;
    hub_lte_pppos_state_t state;
    char ip_addr[HUB_LTE_PPPOS_IP_ADDR_LEN];
} hub_lte_pppos_status_t;

void hub_lte_pppos_get_default_config(hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_init(void);
esp_err_t hub_lte_pppos_init_with_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_set_uart_config(const hub_lte_pppos_config_t *config);
esp_err_t hub_lte_pppos_start(void);
esp_err_t hub_lte_pppos_stop(void);
bool hub_lte_pppos_is_connected(void);
esp_err_t hub_lte_pppos_set_connected(bool connected, const char *ip_addr);
esp_err_t hub_lte_pppos_get_status(hub_lte_pppos_status_t *status);
const char *hub_lte_pppos_get_ip_addr(void);
const char *hub_lte_pppos_state_name(hub_lte_pppos_state_t state);

#ifdef __cplusplus
}
#endif

#endif
