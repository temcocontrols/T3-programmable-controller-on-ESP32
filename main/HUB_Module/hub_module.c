#include "hub_module.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "hub_module";

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

    return first_error;
}

esp_err_t hub_module_process(void)
{
    return hub_lte_pppos_process();
}

esp_err_t hub_module_get_status(void *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    hub_module_status_t *hub_status = (hub_module_status_t *)status;
    memset(hub_status, 0, sizeof(*hub_status));

    const a7608_status_t *a7608_status = a7608_get_status();
    if (a7608_status != NULL) {
        hub_status->a7608 = *a7608_status;
    }

    esp_err_t first_error = hub_lte_pppos_get_status(&hub_status->lte_pppos);
    esp_err_t ret = hub_network_manager_get_status(&hub_status->network);
    if ((first_error == ESP_OK) && (ret != ESP_OK)) {
        first_error = ret;
    }

    return first_error;
}