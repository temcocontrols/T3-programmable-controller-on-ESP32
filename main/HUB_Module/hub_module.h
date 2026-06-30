#ifndef HUB_MODULE_H
#define HUB_MODULE_H

#include "esp_err.h"

#include "a7608.h"
#include "hub_lte_pppos.h"
#include "hub_network_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    a7608_status_t a7608;
    hub_lte_pppos_status_t lte_pppos;
    hub_network_manager_status_t network;
} hub_module_status_t;

esp_err_t hub_module_init(void);
esp_err_t hub_module_process(void);
esp_err_t hub_module_get_status(void *status);

#ifdef __cplusplus
}
#endif

#endif