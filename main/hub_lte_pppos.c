#include "hub_lte_pppos.h"

#include <stdio.h>
#include <string.h>

#include "a7608.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if HUB_LTE_PPPOS_ENABLE
#include "esp_event.h"
#include "esp_modem_api.h"
#include "esp_netif.h"
#endif

static const char *TAG = "hub_lte_pppos";

typedef struct {
    hub_ppp_state_t current_state;
    hub_ppp_state_t previous_state;
    esp_err_t last_error;
    TickType_t state_enter_tick;
} hub_lte_pppos_runtime_t;

static hub_lte_pppos_config_t s_lte_config;
static hub_lte_pppos_status_t s_lte_status = {
    .state = HUB_PPP_STATE_IDLE,
    .uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS,
};
static hub_lte_pppos_runtime_t s_lte_runtime = {
    .current_state = HUB_PPP_STATE_IDLE,
    .previous_state = HUB_PPP_STATE_IDLE,
    .last_error = ESP_OK,
};

#if HUB_LTE_PPPOS_ENABLE
static esp_modem_dte_config_t s_lte_dte_config;
static esp_modem_dce_t *s_lte_dce;
static esp_netif_t *s_lte_ppp_netif;

static void hub_lte_pppos_ppp_event_handler(void *handler_arg,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data)
{
    (void)handler_arg;
    (void)event_base;
    (void)event_id;
    (void)event_data;
}

static void hub_lte_pppos_ip_event_handler(void *handler_arg,
                                           esp_event_base_t event_base,
                                           int32_t event_id,
                                           void *event_data)
{
    (void)handler_arg;
    (void)event_base;
    (void)event_id;
    (void)event_data;
}

static esp_err_t hub_lte_pppos_prepare_stage2(void)
{
    memset(&s_lte_dte_config, 0, sizeof(s_lte_dte_config));
    s_lte_dce = NULL;
    s_lte_ppp_netif = NULL;

    (void)hub_lte_pppos_ppp_event_handler;
    (void)hub_lte_pppos_ip_event_handler;

    /* Future PPPoS handoff flow:
     * stop AT debug/status
     * release UART
     * create PPP netif
     * create esp_modem DTE/DCE
     * enter data mode
     * start PPP
     */
    ESP_LOGW(TAG, "PPP backend Stage 2 placeholder prepared; dialing is not implemented yet");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif

static const char *hub_lte_pppos_uart_owner_name(hub_lte_pppos_uart_owner_t owner)
{
    switch (owner) {
    case HUB_LTE_PPPOS_UART_OWNER_IDLE:
        return "idle";
    case HUB_LTE_PPPOS_UART_OWNER_AT_STATUS:
        return "at_debug_status";
    case HUB_LTE_PPPOS_UART_OWNER_PPPOS:
        return "pppos";
    default:
        return "unknown";
    }
}

static bool hub_lte_pppos_state_is_valid(hub_ppp_state_t state)
{
    return (state >= HUB_PPP_STATE_IDLE) && (state <= HUB_PPP_STATE_ERROR);
}

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
    s_lte_status.uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS;
    s_lte_runtime.current_state = HUB_PPP_STATE_IDLE;
    s_lte_runtime.previous_state = HUB_PPP_STATE_IDLE;
    s_lte_runtime.last_error = ESP_OK;
    s_lte_runtime.state_enter_tick = xTaskGetTickCount();
    (void)hub_lte_pppos_set_state(HUB_LTE_PPPOS_ENABLE ? HUB_PPP_STATE_WAIT_UART : HUB_PPP_STATE_IDLE, ESP_OK);

    ESP_LOGI(TAG,
             "LTE PPPoS framework ready: enabled=%d uart=%d owner=%s baud=%d tx=%d rx=%d apn=%s",
             HUB_LTE_PPPOS_ENABLE,
             s_lte_config.uart_num,
             hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner),
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

    if (HUB_LTE_PPPOS_TEST_MODE == 0) {
        ESP_LOGW(TAG, "PPP start blocked: test mode is disabled");
        (void)hub_lte_pppos_set_state(HUB_PPP_STATE_WAIT_UART, ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS) {
        ESP_LOGI(TAG, "PPP test-mode start waiting: UART%d is owned by A7608 AT debug/status flow", s_lte_config.uart_num);
        return hub_lte_pppos_set_state(HUB_PPP_STATE_WAIT_UART, ESP_OK);
    }
    if (s_lte_status.uart_owner != HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
        ESP_LOGW(TAG,
                 "PPP start blocked: UART%d owner is %s",
                 s_lte_config.uart_num,
                 hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner));
        (void)hub_lte_pppos_set_state(HUB_PPP_STATE_WAIT_UART, ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }

    return hub_lte_pppos_process();
#else
    ESP_LOGI(TAG, "PPP start skipped because HUB_LTE_PPPOS_ENABLE is 0");
    s_lte_status.start_requested = false;
    s_lte_status.connected = false;
    (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
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
    if (hub_lte_pppos_get_state() != HUB_PPP_STATE_IDLE) {
        (void)hub_lte_pppos_set_state(HUB_PPP_STATE_STOPPING, ESP_OK);
    }
    if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
        (void)hub_lte_pppos_release_uart_owner();
    }
    s_lte_status.ip_addr[0] = '\0';
    (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
    return ESP_OK;
}

bool hub_lte_pppos_is_enabled(void)
{
    return HUB_LTE_PPPOS_ENABLE != 0;
}

bool hub_lte_pppos_is_running(void)
{
    return s_lte_status.start_requested || s_lte_status.connected;
}

bool hub_lte_pppos_is_connected(void)
{
    return s_lte_status.connected;
}

bool hub_lte_pppos_can_take_uart(void)
{
    return s_lte_status.initialized &&
           (HUB_LTE_PPPOS_TEST_MODE != 0) &&
           (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS);
}

esp_err_t hub_lte_pppos_request_uart_owner(void)
{
    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (HUB_LTE_PPPOS_TEST_MODE == 0) {
        ESP_LOGW(TAG, "PPP UART request blocked: test mode is disabled");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lte_status.uart_owner != HUB_LTE_PPPOS_UART_OWNER_AT_STATUS) {
        ESP_LOGW(TAG,
                 "PPP UART request blocked: UART%d owner is %s",
                 s_lte_config.uart_num,
                 hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner));
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_status.uart_owner = HUB_LTE_PPPOS_UART_OWNER_PPPOS;
    ESP_LOGW(TAG, "PPP test-mode UART owner: AT debug/status -> PPPoS placeholder");
    return ESP_OK;
}

esp_err_t hub_lte_pppos_release_uart_owner(void)
{
    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lte_status.uart_owner != HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
        ESP_LOGW(TAG,
                 "PPP UART release blocked: UART%d owner is %s",
                 s_lte_config.uart_num,
                 hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner));
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_status.uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS;
    ESP_LOGW(TAG, "PPP test-mode UART owner: PPPoS placeholder -> AT debug/status");
    return ESP_OK;
}

hub_ppp_state_t hub_lte_pppos_get_state(void)
{
    return s_lte_runtime.current_state;
}

const char *hub_lte_pppos_state_name(hub_ppp_state_t state)
{
    switch (state) {
    case HUB_PPP_STATE_IDLE:
        return "IDLE";
    case HUB_PPP_STATE_WAIT_UART:
        return "WAIT_UART";
    case HUB_PPP_STATE_MODEM_READY:
        return "MODEM_READY";
    case HUB_PPP_STATE_STARTING:
        return "STARTING";
    case HUB_PPP_STATE_RUNNING:
        return "RUNNING";
    case HUB_PPP_STATE_STOPPING:
        return "STOPPING";
    case HUB_PPP_STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

esp_err_t hub_lte_pppos_set_state(hub_ppp_state_t new_state, esp_err_t reason)
{
    if (!hub_lte_pppos_state_is_valid(new_state)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_lte_runtime.last_error = reason;
    if (s_lte_runtime.current_state == new_state) {
        s_lte_status.state = new_state;
        return ESP_OK;
    }

    s_lte_runtime.previous_state = s_lte_runtime.current_state;
    s_lte_runtime.current_state = new_state;
    s_lte_runtime.state_enter_tick = xTaskGetTickCount();
    s_lte_status.state = new_state;

    ESP_LOGI(TAG,
             "PPPoS state: %s -> %s",
             hub_lte_pppos_state_name(s_lte_runtime.previous_state),
             hub_lte_pppos_state_name(s_lte_runtime.current_state));

    return ESP_OK;
}

esp_err_t hub_lte_pppos_process(void)
{
    if (!hub_lte_pppos_is_enabled()) {
        s_lte_status.start_requested = false;
        s_lte_status.connected = false;
        return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
    }

    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    switch (hub_lte_pppos_get_state()) {
    case HUB_PPP_STATE_IDLE:
        if (s_lte_status.start_requested) {
            return hub_lte_pppos_set_state((s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_PPPOS) ?
                                               HUB_PPP_STATE_MODEM_READY : HUB_PPP_STATE_WAIT_UART,
                                           ESP_OK);
        }
        break;

    case HUB_PPP_STATE_WAIT_UART:
        if (!s_lte_status.start_requested) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_MODEM_READY, ESP_OK);
        }
        break;

    case HUB_PPP_STATE_MODEM_READY:
        if (!s_lte_status.start_requested) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        break;

    case HUB_PPP_STATE_STARTING:
    case HUB_PPP_STATE_RUNNING:
    case HUB_PPP_STATE_STOPPING:
    case HUB_PPP_STATE_ERROR:
        break;

    default:
        return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
    }

    return ESP_OK;
}

esp_err_t hub_lte_pppos_set_connected(bool connected, const char *ip_addr)
{
    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_status.connected = connected;
    s_lte_status.start_requested = connected;
    (void)hub_lte_pppos_set_state(connected ? HUB_PPP_STATE_RUNNING : HUB_PPP_STATE_IDLE, ESP_OK);
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
