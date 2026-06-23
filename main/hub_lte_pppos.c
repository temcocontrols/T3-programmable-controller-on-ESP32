#include "hub_lte_pppos.h"

#include <stdio.h>
#include <string.h>

#include "a7608.h"
#include "esp_log.h"

static const char *TAG = "hub_lte_pppos";

static hub_lte_pppos_config_t s_lte_config;
static hub_lte_pppos_status_t s_lte_status = {
    .state = HUB_LTE_PPPOS_STATE_IDLE,
};

static void hub_lte_pppos_copy_string(char *dest, size_t dest_len, const char *src)
{
    if ((dest == NULL) || (dest_len == 0)) {
        return;
    }

    snprintf(dest, dest_len, "%s", (src != NULL) ? src : "");
}

static esp_err_t hub_lte_pppos_validate_config(const hub_lte_pppos_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((config->uart_num < UART_NUM_0) || (config->uart_num >= UART_NUM_MAX)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (config->baud_rate <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((config->rx_buffer_size <= 0) || (config->tx_buffer_size < 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((config->tx_io_num == GPIO_NUM_NC) || (config->rx_io_num == GPIO_NUM_NC)) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

void hub_lte_pppos_get_default_config(hub_lte_pppos_config_t *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->uart_num = A7608_DEFAULT_UART_NUM;
    config->baud_rate = A7608_DEFAULT_BAUD_RATE;
    config->rx_buffer_size = A7608_DEFAULT_RX_BUFFER_SIZE;
    config->tx_buffer_size = A7608_DEFAULT_RX_BUFFER_SIZE;
    config->tx_io_num = A7608_DEFAULT_MODEM_TX_PIN;
    config->rx_io_num = A7608_DEFAULT_MODEM_RX_PIN;
    config->rts_io_num = GPIO_NUM_NC;
    config->cts_io_num = GPIO_NUM_NC;
    hub_lte_pppos_copy_string(config->apn, sizeof(config->apn), "3GNET");
}

esp_err_t hub_lte_pppos_init(void)
{
    hub_lte_pppos_config_t config;

    hub_lte_pppos_get_default_config(&config);
    return hub_lte_pppos_init_with_config(&config);
}

esp_err_t hub_lte_pppos_init_with_config(const hub_lte_pppos_config_t *config)
{
    esp_err_t ret = hub_lte_pppos_validate_config(config);
    if (ret != ESP_OK) {
        return ret;
    }

    s_lte_config = *config;
    memset(&s_lte_status, 0, sizeof(s_lte_status));
    s_lte_status.initialized = true;
    s_lte_status.state = HUB_LTE_PPPOS_STATE_READY;

    ESP_LOGI(TAG,
             "LTE PPPoS framework ready: enabled=%d uart=%d baud=%d tx=%d rx=%d apn=%s",
             HUB_LTE_PPPOS_ENABLE,
             s_lte_config.uart_num,
             s_lte_config.baud_rate,
             s_lte_config.tx_io_num,
             s_lte_config.rx_io_num,
             s_lte_config.apn);

    return ESP_OK;
}

esp_err_t hub_lte_pppos_set_uart_config(const hub_lte_pppos_config_t *config)
{
    esp_err_t ret = hub_lte_pppos_validate_config(config);
    if (ret != ESP_OK) {
        return ret;
    }
    if (s_lte_status.start_requested || s_lte_status.connected) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_config = *config;
    return ESP_OK;
}

esp_err_t hub_lte_pppos_start(void)
{
    if (!s_lte_status.initialized) {
        esp_err_t ret = hub_lte_pppos_init();
        if (ret != ESP_OK) {
            return ret;
        }
    }

#if HUB_LTE_PPPOS_ENABLE
    s_lte_status.start_requested = true;
    s_lte_status.state = HUB_LTE_PPPOS_STATE_STARTING;
    ESP_LOGW(TAG, "PPP backend is enabled but not implemented yet");
    return ESP_ERR_NOT_SUPPORTED;
#else
    ESP_LOGI(TAG, "PPP start skipped because HUB_LTE_PPPOS_ENABLE is 0");
    s_lte_status.start_requested = false;
    s_lte_status.state = HUB_LTE_PPPOS_STATE_DISCONNECTED;
    return ESP_OK;
#endif
}

esp_err_t hub_lte_pppos_stop(void)
{
    if (!s_lte_status.initialized) {
        return ESP_OK;
    }

    s_lte_status.start_requested = false;
    s_lte_status.connected = false;
    s_lte_status.state = HUB_LTE_PPPOS_STATE_DISCONNECTED;
    s_lte_status.ip_addr[0] = '\0';
    return ESP_OK;
}

bool hub_lte_pppos_is_connected(void)
{
    return s_lte_status.connected;
}

esp_err_t hub_lte_pppos_set_connected(bool connected, const char *ip_addr)
{
    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_status.connected = connected;
    s_lte_status.start_requested = connected;
    s_lte_status.state = connected ? HUB_LTE_PPPOS_STATE_CONNECTED : HUB_LTE_PPPOS_STATE_DISCONNECTED;
    hub_lte_pppos_copy_string(s_lte_status.ip_addr, sizeof(s_lte_status.ip_addr), connected ? ip_addr : "");
    return ESP_OK;
}

esp_err_t hub_lte_pppos_get_status(hub_lte_pppos_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *status = s_lte_status;
    return ESP_OK;
}

const char *hub_lte_pppos_get_ip_addr(void)
{
    return s_lte_status.ip_addr;
}

const char *hub_lte_pppos_state_name(hub_lte_pppos_state_t state)
{
    switch (state) {
    case HUB_LTE_PPPOS_STATE_IDLE:
        return "idle";
    case HUB_LTE_PPPOS_STATE_READY:
        return "ready";
    case HUB_LTE_PPPOS_STATE_STARTING:
        return "starting";
    case HUB_LTE_PPPOS_STATE_CONNECTED:
        return "connected";
    case HUB_LTE_PPPOS_STATE_DISCONNECTED:
        return "disconnected";
    case HUB_LTE_PPPOS_STATE_STOPPING:
        return "stopping";
    case HUB_LTE_PPPOS_STATE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}
