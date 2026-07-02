#ifndef HUB_MODULE_H
#define HUB_MODULE_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUB_MODULE_IP_ADDR_LEN 40

typedef struct {
    bool initialized;

    bool eth_link_up;
    bool eth_has_ip;

    bool lte_connected;
    char lte_ip[HUB_MODULE_IP_ADDR_LEN];

    int active_interface;

    bool pppos_enabled;
    bool pppos_running;
    int pppos_state;
    int uart_owner;
} hub_module_status_t;

esp_err_t hub_module_init(void);
esp_err_t hub_module_process(void);
esp_err_t hub_module_get_status(hub_module_status_t *status);

#ifdef __cplusplus
}
#endif

#endif