#include "hub_module.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "hub_lte_pppos.h"
#include "hub_network_manager.h"

static const char *TAG = "hub_module";

static bool s_hub_module_initialized;

esp_err_t hub_module_init(void)
{
    esp_err_t first_error = ESP_OK;

    esp_err_t ret = hub_network_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hub_network_manager_init failed: %s", esp_err_to_name(ret));
        first_error = ret;
    }

    ret = hub_lte_pppos_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hub_lte_pppos_init failed: %s", esp_err_to_name(ret));
        if (first_error == ESP_OK) {
            first_error = ret;
        }
    }

    s_hub_module_initialized = (first_error == ESP_OK);
    return first_error;
}

esp_err_t hub_module_process(void)
{
    return hub_lte_pppos_process();
}

esp_err_t hub_module_get_status(hub_module_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(status, 0, sizeof(*status));
    status->initialized = s_hub_module_initialized;

    hub_lte_pppos_status_t lte_pppos_status;
    esp_err_t first_error = hub_lte_pppos_get_status(&lte_pppos_status);
    if (first_error == ESP_OK) {
        status->pppos_running = hub_lte_pppos_is_running();
        status->pppos_state = (int)lte_pppos_status.state;
        status->uart_owner = (int)lte_pppos_status.uart_owner;
    }
    status->pppos_enabled = hub_lte_pppos_is_enabled();

    hub_network_manager_status_t network_status;
    esp_err_t ret = hub_network_manager_get_status(&network_status);
    if ((first_error == ESP_OK) && (ret != ESP_OK)) {
        first_error = ret;
    }
    if (ret == ESP_OK) {
        status->eth_link_up = network_status.eth_link_up;
        status->eth_has_ip = network_status.eth_has_ip;
        status->lte_connected = network_status.lte_connected;
        status->active_interface = (int)network_status.active_interface;
        snprintf(status->lte_ip, sizeof(status->lte_ip), "%s", network_status.lte_ip_addr);
    }

    return first_error;
}