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
    hub_network_manager_select_active_interface();
    return hub_lte_pppos_process();
}

const char *hub_module_active_interface_name(void)
{
    return hub_network_manager_interface_name((hub_network_interface_t)hub_network_manager_get_active_interface());
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

esp_err_t hub_module_dump_status(void)
{
    hub_module_status_t status;
    esp_err_t ret = hub_module_get_status(&status);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hub_module_get_status failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG,
             "status: initialized=%d eth_link_up=%d eth_has_ip=%d lte_connected=%d lte_ip=%s active_interface=%s pppos_enabled=%d pppos_running=%d pppos_state=%s uart_owner=%d pppos_start_requested=%d pppos_stop_requested=%d pppos_last_error=%s pppos_last_reason=%s",
             status.initialized,
             status.eth_link_up,
             status.eth_has_ip,
             status.lte_connected,
             status.lte_ip[0] != '\0' ? status.lte_ip : "-",
             hub_network_manager_interface_name((hub_network_interface_t)status.active_interface),
             status.pppos_enabled,
             status.pppos_running,
             hub_lte_pppos_state_name((hub_ppp_state_t)status.pppos_state),
             status.uart_owner,
             hub_lte_pppos_start_requested(),
             hub_lte_pppos_stop_requested(),
             esp_err_to_name(hub_lte_pppos_get_last_error()),
             hub_lte_pppos_get_last_reason());

    hub_lte_pppos_preflight_t preflight;
    ret = hub_lte_pppos_preflight_check(&preflight);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "hub_lte_pppos_preflight_check failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG,
             "pppos_preflight: ready=%d reason=%s config_valid=%d pppos_enabled=%d test_mode=%d uart_available=%d uart_owner=%d modem_status_known=%d sim_ready=%d registered=%d has_signal=%d rssi=%d has_apn=%d apn=%s",
             preflight.ready_to_start,
             preflight.reason,
             preflight.config_valid,
             preflight.pppos_enabled,
             preflight.test_mode_enabled,
             preflight.uart_available,
             preflight.uart_owner,
             preflight.modem_status_known,
             preflight.sim_ready,
             preflight.registered_to_network,
             preflight.has_signal,
             preflight.rssi,
             preflight.has_apn,
             preflight.apn[0] != '\0' ? preflight.apn : "-");

    return ESP_OK;
}
