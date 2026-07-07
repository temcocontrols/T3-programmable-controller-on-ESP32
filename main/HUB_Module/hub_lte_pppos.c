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
static bool s_lte_config_saved;
static hub_lte_pppos_status_t s_lte_status = {
    .state = HUB_PPP_STATE_IDLE,
    .uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS,
};
static hub_lte_pppos_runtime_t s_lte_runtime = {
    .current_state = HUB_PPP_STATE_IDLE,
    .previous_state = HUB_PPP_STATE_IDLE,
    .last_error = ESP_OK,
};
static char s_lte_preflight_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN] = "Preflight not run";
static char s_lte_last_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN] = "No PPPoS lifecycle error";

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

static bool hub_lte_pppos_apn_is_valid(const char *apn)
{
    return (apn != NULL) && (apn[0] != '\0');
}

static void hub_lte_pppos_set_preflight_reason(hub_lte_pppos_preflight_t *preflight, const char *reason)
{
    hub_lte_pppos_copy_string(s_lte_preflight_reason, sizeof(s_lte_preflight_reason), reason);
    if (preflight != NULL) {
        hub_lte_pppos_copy_string(preflight->reason, sizeof(preflight->reason), s_lte_preflight_reason);
    }
}

static void hub_lte_pppos_set_last_result(esp_err_t error, const char *reason)
{
    s_lte_runtime.last_error = error;
    hub_lte_pppos_copy_string(s_lte_last_reason, sizeof(s_lte_last_reason), reason);
}

static esp_err_t hub_lte_pppos_preflight_error(const hub_lte_pppos_preflight_t *preflight)
{
    if ((preflight == NULL) || preflight->ready_to_start) {
        return ESP_OK;
    }
    if (!preflight->has_apn || !preflight->config_valid) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_ERR_INVALID_STATE;
}

static esp_err_t hub_lte_pppos_run_start_preflight(hub_lte_pppos_preflight_t *preflight)
{
    esp_err_t ret = hub_lte_pppos_preflight_check(preflight);
    if (ret != ESP_OK) {
        hub_lte_pppos_set_last_result(ret, "PPPoS preflight check failed");
        return ret;
    }
    if (!preflight->ready_to_start) {
        ret = hub_lte_pppos_preflight_error(preflight);
        hub_lte_pppos_set_last_result(ret, preflight->reason);
        return ret;
    }

    hub_lte_pppos_set_last_result(ESP_OK, preflight->reason);
    return ESP_OK;
}

esp_err_t hub_lte_pppos_validate_config(const hub_lte_pppos_config_t *config)
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
    if (!GPIO_IS_VALID_GPIO(config->tx_io_num) || !GPIO_IS_VALID_GPIO(config->rx_io_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((config->rts_io_num != GPIO_NUM_NC) && !GPIO_IS_VALID_GPIO(config->rts_io_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((config->cts_io_num != GPIO_NUM_NC) && !GPIO_IS_VALID_GPIO(config->cts_io_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!hub_lte_pppos_apn_is_valid(config->apn)) {
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

    if (s_lte_config_saved) {
        config = s_lte_config;
    } else {
        hub_lte_pppos_get_default_config(&config);
    }
    return hub_lte_pppos_init_with_config(&config);
}

esp_err_t hub_lte_pppos_init_with_config(const hub_lte_pppos_config_t *config)
{
    esp_err_t ret = hub_lte_pppos_validate_config(config);
    if (ret != ESP_OK) {
        return ret;
    }

    s_lte_config = *config;
    s_lte_config_saved = true;
    memset(&s_lte_status, 0, sizeof(s_lte_status));
    s_lte_status.initialized = true;
    s_lte_status.uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS;
    s_lte_runtime.current_state = HUB_PPP_STATE_IDLE;
    s_lte_runtime.previous_state = HUB_PPP_STATE_IDLE;
    s_lte_runtime.last_error = ESP_OK;
    s_lte_runtime.state_enter_tick = xTaskGetTickCount();
    hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle initialized");
    (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);

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
    return hub_lte_pppos_set_config(config);
}

esp_err_t hub_lte_pppos_get_config(hub_lte_pppos_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_lte_config_saved) {
        hub_lte_pppos_get_default_config(config);
        return ESP_OK;
    }

    *config = s_lte_config;
    return ESP_OK;
}

esp_err_t hub_lte_pppos_set_config(const hub_lte_pppos_config_t *config)
{
    esp_err_t ret = hub_lte_pppos_validate_config(config);
    if (ret != ESP_OK) {
        return ret;
    }
    if (s_lte_status.start_requested || s_lte_status.connected) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_config = *config;
    s_lte_config_saved = true;
    return ESP_OK;
}

esp_err_t hub_lte_pppos_request_start(void)
{
    if (!s_lte_status.initialized) {
        esp_err_t ret = hub_lte_pppos_init();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPPoS init failed");
            return ret;
        }
    }

    if (!hub_lte_pppos_is_enabled()) {
        s_lte_status.start_requested = false;
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS disabled by build config");
        ESP_LOGW(TAG, "PPP start request blocked: %s", hub_lte_pppos_get_last_reason());
        return ESP_ERR_INVALID_STATE;
    }

    hub_ppp_state_t state = hub_lte_pppos_get_state();
    if ((state != HUB_PPP_STATE_IDLE) &&
        (state != HUB_PPP_STATE_WAIT_UART) &&
        (state != HUB_PPP_STATE_MODEM_READY)) {
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS lifecycle is not idle");
        ESP_LOGW(TAG, "PPP start request blocked: state=%s", hub_lte_pppos_state_name(state));
        return ESP_ERR_INVALID_STATE;
    }

    hub_lte_pppos_preflight_t preflight;
    esp_err_t ret = hub_lte_pppos_run_start_preflight(&preflight);
    if (ret != ESP_OK) {
        s_lte_status.start_requested = false;
        ESP_LOGW(TAG, "PPP start request blocked: %s", hub_lte_pppos_get_last_reason());
        return ret;
    }

    s_lte_status.start_requested = true;
    ESP_LOGI(TAG, "PPP start requested: %s", preflight.reason);
    return ESP_OK;
}

esp_err_t hub_lte_pppos_request_stop(void)
{
    s_lte_status.stop_requested = true;
    hub_lte_pppos_set_last_result(ESP_OK, "PPPoS stop requested");
    ESP_LOGI(TAG, "PPP stop requested");
    return ESP_OK;
}

bool hub_lte_pppos_start_requested(void)
{
    return s_lte_status.start_requested;
}

bool hub_lte_pppos_stop_requested(void)
{
    return s_lte_status.stop_requested;
}

esp_err_t hub_lte_pppos_start(void)
{
    esp_err_t ret = hub_lte_pppos_request_start();
    if (ret != ESP_OK) {
        return ret;
    }
    return hub_lte_pppos_process();
}

esp_err_t hub_lte_pppos_stop(void)
{
    if (!s_lte_status.initialized) {
        return ESP_OK;
    }

    return hub_lte_pppos_request_stop();
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
        s_lte_status.stop_requested = false;
        s_lte_status.connected = false;
        s_lte_status.ip_addr[0] = '\0';
        if (hub_lte_pppos_get_state() != HUB_PPP_STATE_IDLE) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        s_lte_status.state = HUB_PPP_STATE_IDLE;
        return ESP_OK;
    }

    if (!s_lte_status.initialized) {
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS framework not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_lte_status.stop_requested && (hub_lte_pppos_get_state() != HUB_PPP_STATE_STOPPING)) {
        return hub_lte_pppos_set_state(HUB_PPP_STATE_STOPPING, ESP_OK);
    }

    switch (hub_lte_pppos_get_state()) {
    case HUB_PPP_STATE_IDLE:
        if (s_lte_status.start_requested) {
            hub_lte_pppos_preflight_t preflight;
            esp_err_t ret = hub_lte_pppos_run_start_preflight(&preflight);
            if (ret != ESP_OK) {
                s_lte_status.start_requested = false;
                (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
                return ret;
            }
            return hub_lte_pppos_set_state(HUB_PPP_STATE_WAIT_UART, ESP_OK);
        }
        break;

    case HUB_PPP_STATE_WAIT_UART:
        if (!s_lte_status.start_requested) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        if (HUB_LTE_PPPOS_TEST_MODE == 0) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS test mode disabled");
            return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
        }
        if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS) {
            hub_lte_pppos_set_last_result(ESP_OK, "UART available for PPPoS placeholder");
            return hub_lte_pppos_set_state(HUB_PPP_STATE_MODEM_READY, ESP_OK);
        }
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "UART not available for PPPoS");
        break;

    case HUB_PPP_STATE_MODEM_READY:
        if (!s_lte_status.start_requested) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        hub_lte_pppos_set_last_result(ESP_OK, "PPPoS modem-ready placeholder; dialing not started");
        break;

    case HUB_PPP_STATE_STARTING:
    case HUB_PPP_STATE_RUNNING:
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP runtime state is not implemented");
        return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);

    case HUB_PPP_STATE_STOPPING:
        s_lte_status.start_requested = false;
        s_lte_status.stop_requested = false;
        hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle stopped");
        return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);

    case HUB_PPP_STATE_ERROR:
        break;

    default:
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "Invalid PPPoS lifecycle state");
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
    hub_lte_pppos_copy_string(s_lte_status.ip_addr, sizeof(s_lte_status.ip_addr), connected ? ip_addr : "");
    if (!connected && (hub_lte_pppos_get_state() == HUB_PPP_STATE_RUNNING)) {
        (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
    }
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

esp_err_t hub_lte_pppos_preflight_check(hub_lte_pppos_preflight_t *preflight)
{
    if (preflight == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(preflight, 0, sizeof(*preflight));

    hub_lte_pppos_config_t config;
    esp_err_t config_ret = hub_lte_pppos_get_config(&config);
    if (config_ret == ESP_OK) {
        preflight->config_valid = (hub_lte_pppos_validate_config(&config) == ESP_OK);
        preflight->has_apn = hub_lte_pppos_apn_is_valid(config.apn);
        hub_lte_pppos_copy_string(preflight->apn, sizeof(preflight->apn), config.apn);
    }

    preflight->test_mode_enabled = HUB_LTE_PPPOS_TEST_MODE != 0;
    preflight->pppos_enabled = hub_lte_pppos_is_enabled();
    preflight->uart_owner = (int)s_lte_status.uart_owner;
    preflight->uart_available = s_lte_status.initialized &&
                                preflight->test_mode_enabled &&
                                (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS);

    const a7608_status_t *modem_status = a7608_get_status();
    if (modem_status != NULL) {
        preflight->modem_status_known = true;
        preflight->sim_ready = modem_status->sim_ready;
        preflight->registered_to_network = modem_status->registered_home || modem_status->registered_roaming;
        preflight->has_signal = (modem_status->csq > 0) || (modem_status->rssi_dbm < 0);
        preflight->rssi = modem_status->rssi_dbm;
    }

    if (!preflight->pppos_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS disabled by build config");
    } else if (!preflight->test_mode_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS test mode disabled");
    } else if (config_ret != ESP_OK) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS config unavailable");
    } else if (!preflight->has_apn) {
        hub_lte_pppos_set_preflight_reason(preflight, "APN is empty");
    } else if (!preflight->config_valid) {
        hub_lte_pppos_set_preflight_reason(preflight, "Invalid PPPoS config");
    } else if (!s_lte_status.initialized) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS framework not initialized");
    } else if (!preflight->uart_available) {
        hub_lte_pppos_set_preflight_reason(preflight, "UART not available for PPPoS");
    } else if (!preflight->modem_status_known) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status unavailable");
    } else if (!preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "SIM not ready");
    } else if (!preflight->registered_to_network) {
        hub_lte_pppos_set_preflight_reason(preflight, "Modem not registered to network");
    } else if (!preflight->has_signal) {
        hub_lte_pppos_set_preflight_reason(preflight, "No LTE signal");
    } else {
        preflight->ready_to_start = true;
        hub_lte_pppos_set_preflight_reason(preflight, "Ready to start PPPoS");
    }

    return ESP_OK;
}

const char *hub_lte_pppos_preflight_reason(void)
{
    return s_lte_preflight_reason;
}

esp_err_t hub_lte_pppos_get_last_error(void)
{
    return s_lte_runtime.last_error;
}

const char *hub_lte_pppos_get_last_reason(void)
{
    return s_lte_last_reason;
}
