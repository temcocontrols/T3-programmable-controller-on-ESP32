#ifndef HUB_NETWORK_MANAGER_H
#define HUB_NETWORK_MANAGER_H

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUB_NETWORK_MANAGER_IP_ADDR_LEN 40

typedef enum {
    HUB_NETWORK_INTERFACE_NONE = 0,
    HUB_NETWORK_INTERFACE_ETHERNET,
    HUB_NETWORK_INTERFACE_LTE,
} hub_network_interface_t;

typedef struct {
    bool initialized;
    bool eth_link_up;
    bool eth_has_ip;
    bool lte_connected;
    char lte_ip_addr[HUB_NETWORK_MANAGER_IP_ADDR_LEN];
    hub_network_interface_t active_interface;
} hub_network_manager_status_t;

esp_err_t hub_network_manager_init(void);
void hub_network_manager_set_eth_status(bool link_up, bool has_ip);
void hub_network_manager_set_lte_status(bool connected, const char *ip_addr);
void hub_network_manager_select_active_interface(void);
int hub_network_manager_get_active_interface(void);
bool hub_network_manager_has_usable_network(void);
esp_err_t hub_network_manager_get_status(hub_network_manager_status_t *status);
const char *hub_network_manager_interface_name(int interface_id);

#ifdef __cplusplus
}
#endif

#endif
