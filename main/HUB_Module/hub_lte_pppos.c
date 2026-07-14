#include "hub_lte_pppos.h"

#include <stdio.h>
#include <string.h>

#include "a7608.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hub_network_manager.h"

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
#include "esp_event.h"
#include "esp_modem_api.h"
#include "esp_modem_c_api_types.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_ppp.h"
#endif

static const char *TAG = "hub_lte_pppos";

#define HUB_LTE_PPPOS_STARTING_TIMEOUT_MS 60000U

typedef struct {
    hub_ppp_state_t current_state;
    hub_ppp_state_t previous_state;
    esp_err_t last_error;
    TickType_t state_enter_tick;
} hub_lte_pppos_lifecycle_t;

static hub_lte_pppos_config_t s_lte_config;
static bool s_lte_config_saved;
static hub_lte_pppos_status_t s_lte_status = {
    .state = HUB_PPP_STATE_IDLE,
    .uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS,
};
static hub_lte_pppos_lifecycle_t s_lte_lifecycle = {
    .current_state = HUB_PPP_STATE_IDLE,
    .previous_state = HUB_PPP_STATE_IDLE,
    .last_error = ESP_OK,
};
static hub_lte_pppos_runtime_t s_lte_runtime;
static char s_lte_preflight_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN] = "Preflight not run";
static char s_lte_last_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN] = "No PPPoS lifecycle error";

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
static esp_modem_dte_config_t s_lte_dte_config;
static esp_modem_dce_t *s_lte_dce;
static esp_netif_t *s_lte_ppp_netif;
static bool s_lte_ip_handler_registered;
static bool s_lte_ppp_handler_registered;
static TickType_t s_lte_data_mode_enter_tick;
static uint32_t s_lte_terminal_unexpected_flow_count;
static bool s_lte_terminal_break_after_data;
static bool s_lte_timeout_cleanup_active;
#endif

static esp_err_t hub_lte_pppos_create_netif(void);
static esp_err_t hub_lte_pppos_create_modem(void);
static esp_err_t hub_lte_pppos_enter_data_mode(void);
static esp_err_t hub_lte_pppos_start_ppp(void);
static esp_err_t hub_lte_pppos_stop_ppp(void);
static esp_err_t hub_lte_pppos_destroy_runtime(void);
static esp_err_t hub_lte_pppos_handle_starting_timeout(void);
static void hub_lte_pppos_set_last_result(esp_err_t error, const char *reason);

static bool hub_lte_pppos_real_runtime_allowed(void)
{
    return (HUB_LTE_PPPOS_ENABLE != 0) &&
           (HUB_LTE_PPPOS_TEST_MODE != 0) &&
           (HUB_LTE_PPPOS_REAL_RUNTIME != 0) &&
           (HUB_LTE_PPPOS_MANUAL_TEST != 0);
}

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
static void hub_lte_pppos_ppp_event_handler(void *handler_arg,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data);
static void hub_lte_pppos_ip_event_handler(void *handler_arg,
                                           esp_event_base_t event_base,
                                           int32_t event_id,
                                           void *event_data);
#endif

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
static const char *hub_lte_pppos_ip_event_name(int32_t event_id)
{
    switch (event_id) {
    case IP_EVENT_PPP_GOT_IP:
        return "IP_EVENT_PPP_GOT_IP";
    case IP_EVENT_PPP_LOST_IP:
        return "IP_EVENT_PPP_LOST_IP";
    default:
        return "IP_EVENT_OTHER";
    }
}

static const char *hub_lte_pppos_ppp_status_name(int32_t event_id)
{
    switch (event_id) {
    case NETIF_PPP_ERRORNONE:
        return "NETIF_PPP_ERRORNONE";
    case NETIF_PPP_ERRORPARAM:
        return "NETIF_PPP_ERRORPARAM";
    case NETIF_PPP_ERROROPEN:
        return "NETIF_PPP_ERROROPEN";
    case NETIF_PPP_ERRORDEVICE:
        return "NETIF_PPP_ERRORDEVICE";
    case NETIF_PPP_ERRORALLOC:
        return "NETIF_PPP_ERRORALLOC";
    case NETIF_PPP_ERRORUSER:
        return "NETIF_PPP_ERRORUSER";
    case NETIF_PPP_ERRORCONNECT:
        return "NETIF_PPP_ERRORCONNECT";
    case NETIF_PPP_ERRORAUTHFAIL:
        return "NETIF_PPP_ERRORAUTHFAIL";
    case NETIF_PPP_ERRORPROTOCOL:
        return "NETIF_PPP_ERRORPROTOCOL";
    case NETIF_PPP_ERRORPEERDEAD:
        return "NETIF_PPP_ERRORPEERDEAD";
    case NETIF_PPP_ERRORIDLETIMEOUT:
        return "NETIF_PPP_ERRORIDLETIMEOUT";
    case NETIF_PPP_ERRORCONNECTTIME:
        return "NETIF_PPP_ERRORCONNECTTIME";
    case NETIF_PPP_ERRORLOOPBACK:
        return "NETIF_PPP_ERRORLOOPBACK";
    case NETIF_PPP_PHASE_DEAD:
        return "NETIF_PPP_PHASE_DEAD";
    case NETIF_PPP_PHASE_MASTER:
        return "NETIF_PPP_PHASE_MASTER";
    case NETIF_PPP_PHASE_HOLDOFF:
        return "NETIF_PPP_PHASE_HOLDOFF";
    case NETIF_PPP_PHASE_INITIALIZE:
        return "NETIF_PPP_PHASE_INITIALIZE";
    case NETIF_PPP_PHASE_SERIALCONN:
        return "NETIF_PPP_PHASE_SERIALCONN";
    case NETIF_PPP_PHASE_DORMANT:
        return "NETIF_PPP_PHASE_DORMANT";
    case NETIF_PPP_PHASE_ESTABLISH:
        return "NETIF_PPP_PHASE_ESTABLISH";
    case NETIF_PPP_PHASE_AUTHENTICATE:
        return "NETIF_PPP_PHASE_AUTHENTICATE";
    case NETIF_PPP_PHASE_CALLBACK:
        return "NETIF_PPP_PHASE_CALLBACK";
    case NETIF_PPP_PHASE_NETWORK:
        return "NETIF_PPP_PHASE_NETWORK";
    case NETIF_PPP_PHASE_RUNNING:
        return "NETIF_PPP_PHASE_RUNNING";
    case NETIF_PPP_PHASE_TERMINATE:
        return "NETIF_PPP_PHASE_TERMINATE";
    case NETIF_PPP_PHASE_DISCONNECT:
        return "NETIF_PPP_PHASE_DISCONNECT";
    case NETIF_PPP_CONNECT_FAILED:
        return "NETIF_PPP_CONNECT_FAILED";
    default:
        if ((event_id > NETIF_PPP_ERRORNONE) && (event_id < NETIF_PP_PHASE_OFFSET)) {
            return "NETIF_PPP_ERROR";
        }
        if (event_id >= NETIF_PP_PHASE_OFFSET) {
            return "NETIF_PPP_PHASE";
        }
        return "NETIF_PPP_STATUS";
    }
}

static const char *hub_lte_pppos_dce_device_name(esp_modem_dce_device_t device)
{
    switch (device) {
    case ESP_MODEM_DCE_GENERIC:
        return "generic";
    case ESP_MODEM_DCE_SIM7600:
        return "sim7600";
    case ESP_MODEM_DCE_SIM7070:
        return "sim7070";
    case ESP_MODEM_DCE_SIM7000:
        return "sim7000";
    case ESP_MODEM_DCE_BG96:
        return "bg96";
    case ESP_MODEM_DCE_EC20:
        return "ec20";
    case ESP_MODEM_DCE_SIM800:
        return "sim800";
    case ESP_MODEM_DCE_SQNGM02S:
        return "sqngm02s";
    case ESP_MODEM_DCE_CUSTOM:
        return "custom";
    default:
        return "unknown";
    }
}

static const char *hub_lte_pppos_flow_control_name(esp_modem_flow_ctrl_t flow_control)
{
    switch (flow_control) {
    case ESP_MODEM_FLOW_CONTROL_NONE:
        return "none";
    case ESP_MODEM_FLOW_CONTROL_SW:
        return "software";
    case ESP_MODEM_FLOW_CONTROL_HW:
        return "hardware";
    default:
        return "unknown";
    }
}

static const char *hub_lte_pppos_terminal_error_name(esp_modem_terminal_error_t error)
{
    switch (error) {
    case ESP_MODEM_TERMINAL_BUFFER_OVERFLOW:
        return "buffer_overflow";
    case ESP_MODEM_TERMINAL_CHECKSUM_ERROR:
        return "checksum_error";
    case ESP_MODEM_TERMINAL_UNEXPECTED_CONTROL_FLOW:
        return "unexpected_control_flow";
    case ESP_MODEM_TERMINAL_DEVICE_GONE:
        return "device_gone";
    case ESP_MODEM_TERMINAL_UNKNOWN_ERROR:
        return "unknown_error";
    default:
        return "unknown";
    }
}

static void hub_lte_pppos_terminal_error_handler(esp_modem_terminal_error_t error)
{
    TickType_t now = xTaskGetTickCount();
    bool after_data_mode = s_lte_runtime.data_mode_entered || (s_lte_data_mode_enter_tick != 0);
    bool in_first_three_seconds = after_data_mode &&
                                  ((now - s_lte_data_mode_enter_tick) <= pdMS_TO_TICKS(3000));

    if ((error == ESP_MODEM_TERMINAL_UNEXPECTED_CONTROL_FLOW) && in_first_three_seconds) {
        s_lte_terminal_unexpected_flow_count++;
        if (s_lte_terminal_unexpected_flow_count >= 2U) {
            s_lte_terminal_break_after_data = true;
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_RESPONSE,
                                          "PPP UART break/unexpected_control_flow after data mode");
        }
    }

    ESP_LOGW(TAG,
             "esp_modem terminal error: code=%d name=%s ppp_state=%s dtr_level=%d after_data=%d first3s=%d unexpected_count=%lu",
             error,
             hub_lte_pppos_terminal_error_name(error),
             hub_lte_pppos_state_name(hub_lte_pppos_get_state()),
             a7608_get_dtr_level(),
             after_data_mode,
             in_first_three_seconds,
             (unsigned long)s_lte_terminal_unexpected_flow_count);
}

static esp_err_t hub_lte_pppos_pre_data_at_check_one(const char *cmd)
{
    char response[512] = {0};
    esp_err_t ret = esp_modem_at(s_lte_dce, cmd, response, 3000);
    ESP_LOGI(TAG,
             "PPPoS pre-data AT check: cmd=%s ret=%s raw=[%s]",
             cmd,
             esp_err_to_name(ret),
             response[0] != '\0' ? response : "-");
    return ret;
}

static esp_err_t hub_lte_pppos_pre_data_at_check(void)
{
#if HUB_LTE_PPPOS_TEST_MODE && HUB_LTE_PPPOS_MANUAL_TEST
    esp_err_t first_error = ESP_OK;
    esp_err_t ret = hub_lte_pppos_pre_data_at_check_one("AT");
    if (ret != ESP_OK) {
        first_error = ret;
    }
    ret = hub_lte_pppos_pre_data_at_check_one("AT+CGDCONT?");
    if ((first_error == ESP_OK) && (ret != ESP_OK)) {
        first_error = ret;
    }
    ret = hub_lte_pppos_pre_data_at_check_one("AT+CGATT?");
    if ((first_error == ESP_OK) && (ret != ESP_OK)) {
        first_error = ret;
    }
    return first_error;
#else
    return ESP_OK;
#endif
}

static void hub_lte_pppos_log_dns_info(int dns_type, const char *name)
{
    esp_netif_dns_info_t dns_info = {0};
    esp_err_t ret = esp_netif_get_dns_info(s_lte_ppp_netif, dns_type, &dns_info);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "PPP DNS %s read failed: %s", name, esp_err_to_name(ret));
        return;
    }

    if (dns_info.ip.type == ESP_IPADDR_TYPE_V4) {
        ESP_LOGI(TAG, "PPP DNS %s: " IPSTR, name, IP2STR(&dns_info.ip.u_addr.ip4));
    } else {
        ESP_LOGI(TAG, "PPP DNS %s: non-IPv4 type=%d", name, dns_info.ip.type);
    }
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
    s_lte_lifecycle.last_error = error;
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

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
static void hub_lte_pppos_ppp_event_handler(void *handler_arg,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data)
{
    (void)handler_arg;

    ESP_LOGW(TAG,
             "PPP status event: base=%s id=%ld status_code=%ld error_code=%ld phase_code=%ld name=%s data=%p state=%s",
             event_base,
             (long)event_id,
             (long)event_id,
             ((event_id > NETIF_PPP_ERRORNONE) && (event_id < NETIF_PP_PHASE_OFFSET)) ? (long)event_id : -1L,
             (event_id >= NETIF_PP_PHASE_OFFSET) ? (long)(event_id - NETIF_PP_PHASE_OFFSET) : -1L,
             hub_lte_pppos_ppp_status_name(event_id),
             event_data,
             hub_lte_pppos_state_name(hub_lte_pppos_get_state()));
    if (event_id == NETIF_PPP_ERRORUSER) {
        esp_netif_t **event_netif = (esp_netif_t **)event_data;
        if ((event_netif == NULL) || (*event_netif == s_lte_ppp_netif)) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP stopped by user event");
            if (hub_lte_pppos_get_state() != HUB_PPP_STATE_STOPPING) {
                (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
            }
        }
    }
}

static void hub_lte_pppos_ip_event_handler(void *handler_arg,
                                           esp_event_base_t event_base,
                                           int32_t event_id,
                                           void *event_data)
{
    (void)handler_arg;

    ESP_LOGI(TAG,
             "PPP IP event: base=%s id=%ld name=%s data=%p state=%s",
             event_base,
             (long)event_id,
             hub_lte_pppos_ip_event_name(event_id),
             event_data,
             hub_lte_pppos_state_name(hub_lte_pppos_get_state()));

    if (event_id == IP_EVENT_PPP_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        if ((event == NULL) || (event->esp_netif != s_lte_ppp_netif)) {
            ESP_LOGW(TAG, "PPP got IP ignored: event=%p netif=%p expected=%p", event, event != NULL ? event->esp_netif : NULL, s_lte_ppp_netif);
            return;
        }

        snprintf(s_lte_status.ip_addr,
                 sizeof(s_lte_status.ip_addr),
                 IPSTR,
                 IP2STR(&event->ip_info.ip));
        s_lte_status.connected = true;
        hub_network_manager_set_lte_status(true, s_lte_status.ip_addr);
        hub_lte_pppos_set_last_result(ESP_OK, "PPP got IP event");
        (void)hub_lte_pppos_set_state(HUB_PPP_STATE_RUNNING, ESP_OK);
        ESP_LOGI(TAG,
                 "PPP got IP: ip=" IPSTR " netmask=" IPSTR " gw=" IPSTR,
                 IP2STR(&event->ip_info.ip),
                 IP2STR(&event->ip_info.netmask),
                 IP2STR(&event->ip_info.gw));
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_MAIN, "main");
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_BACKUP, "backup");
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_FALLBACK, "fallback");
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        s_lte_status.connected = false;
        s_lte_status.ip_addr[0] = '\0';
        hub_network_manager_set_lte_status(false, NULL);
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP lost IP event");
        ESP_LOGW(TAG,
                 "PPP lost IP: reason=%s state=%s",
                 hub_lte_pppos_get_last_reason(),
                 hub_lte_pppos_state_name(hub_lte_pppos_get_state()));
        if (hub_lte_pppos_get_state() != HUB_PPP_STATE_STOPPING) {
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
        }
    }
}
#endif

static esp_err_t hub_lte_pppos_create_netif(void)
{
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    if (s_lte_ppp_netif != NULL) {
        s_lte_runtime.ppp_netif_created = true;
        return ESP_OK;
    }

    esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
    s_lte_ppp_netif = esp_netif_new(&netif_ppp_config);
    if (s_lte_ppp_netif == NULL) {
        return ESP_ERR_NO_MEM;
    }
    s_lte_runtime.ppp_netif_created = true;
    ESP_LOGI(TAG, "PPP netif created");

    esp_netif_ppp_config_t ppp_config = {
        .ppp_phase_event_enabled = true,
        .ppp_error_event_enabled = true,
    };
    esp_err_t ret = esp_netif_ppp_set_params(s_lte_ppp_netif, &ppp_config);
    ESP_LOGI(TAG,
             "PPP netif params: phase_events=%d error_events=%d set_ret=%s",
             ppp_config.ppp_phase_event_enabled,
             ppp_config.ppp_error_event_enabled,
             esp_err_to_name(ret));
    if (ret != ESP_OK) {
        esp_netif_destroy(s_lte_ppp_netif);
        s_lte_ppp_netif = NULL;
        s_lte_runtime.ppp_netif_created = false;
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT,
                                     ESP_EVENT_ANY_ID,
                                     hub_lte_pppos_ip_event_handler,
                                     NULL);
    if (ret != ESP_OK) {
        esp_netif_destroy(s_lte_ppp_netif);
        s_lte_ppp_netif = NULL;
        s_lte_runtime.ppp_netif_created = false;
        return ret;
    }
    s_lte_ip_handler_registered = true;
    ESP_LOGI(TAG, "PPP IP event handler registered for ESP_EVENT_ANY_ID");

    ret = esp_event_handler_register(NETIF_PPP_STATUS,
                                     ESP_EVENT_ANY_ID,
                                     hub_lte_pppos_ppp_event_handler,
                                     NULL);
    if (ret != ESP_OK) {
        (void)esp_event_handler_unregister(IP_EVENT,
                                           ESP_EVENT_ANY_ID,
                                           hub_lte_pppos_ip_event_handler);
        s_lte_ip_handler_registered = false;
        esp_netif_destroy(s_lte_ppp_netif);
        s_lte_ppp_netif = NULL;
        s_lte_runtime.ppp_netif_created = false;
        return ret;
    }
    s_lte_ppp_handler_registered = true;
    ESP_LOGI(TAG, "PPP status event handler registered for ESP_EVENT_ANY_ID");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t hub_lte_pppos_create_modem(void)
{
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    if (s_lte_dce != NULL) {
        s_lte_runtime.modem_created = true;
        return ESP_OK;
    }
    if (s_lte_ppp_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_lte_status.uart_owner != HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
        esp_err_t ret = hub_lte_pppos_request_uart_owner();
        if (ret != ESP_OK) {
            return ret;
        }
    }

    s_lte_dte_config = (esp_modem_dte_config_t)ESP_MODEM_DTE_DEFAULT_CONFIG();
    s_lte_dte_config.uart_config.port_num = s_lte_config.uart_num;
    s_lte_dte_config.uart_config.baud_rate = s_lte_config.baud_rate;
    s_lte_dte_config.uart_config.tx_io_num = s_lte_config.tx_io_num;
    s_lte_dte_config.uart_config.rx_io_num = s_lte_config.rx_io_num;
    s_lte_dte_config.uart_config.rts_io_num = s_lte_config.rts_io_num;
    s_lte_dte_config.uart_config.cts_io_num = s_lte_config.cts_io_num;
#if HUB_LTE_PPPOS_TEST_MODE && HUB_LTE_PPPOS_MANUAL_TEST
    s_lte_dte_config.uart_config.rts_io_num = UART_PIN_NO_CHANGE;
    s_lte_dte_config.uart_config.cts_io_num = UART_PIN_NO_CHANGE;
    s_lte_dte_config.uart_config.flow_control = ESP_MODEM_FLOW_CONTROL_NONE;
#else
    s_lte_dte_config.uart_config.flow_control = ((s_lte_config.rts_io_num != GPIO_NUM_NC) &&
                                                 (s_lte_config.cts_io_num != GPIO_NUM_NC))
                                                    ? ESP_MODEM_FLOW_CONTROL_HW
                                                    : ESP_MODEM_FLOW_CONTROL_NONE;
#endif
    s_lte_dte_config.uart_config.rx_buffer_size = s_lte_config.rx_buffer_size;
    s_lte_dte_config.uart_config.tx_buffer_size = s_lte_config.tx_buffer_size;
    if (s_lte_config.rx_buffer_size > 0) {
        s_lte_dte_config.dte_buffer_size = (size_t)s_lte_config.rx_buffer_size / 2;
    }

    ESP_LOGI(TAG,
             "PPPoS DTE UART config: port=%d tx=%d rx=%d rts=%d cts=%d baud=%d flow_control=%s(%d) rx_buf=%d tx_buf=%d dte_buf=%u task_stack=%lu task_prio=%u",
             s_lte_dte_config.uart_config.port_num,
             s_lte_dte_config.uart_config.tx_io_num,
             s_lte_dte_config.uart_config.rx_io_num,
             s_lte_dte_config.uart_config.rts_io_num,
             s_lte_dte_config.uart_config.cts_io_num,
             s_lte_dte_config.uart_config.baud_rate,
             hub_lte_pppos_flow_control_name(s_lte_dte_config.uart_config.flow_control),
             s_lte_dte_config.uart_config.flow_control,
             s_lte_dte_config.uart_config.rx_buffer_size,
             s_lte_dte_config.uart_config.tx_buffer_size,
             (unsigned)s_lte_dte_config.dte_buffer_size,
             (unsigned long)s_lte_dte_config.task_stack_size,
             s_lte_dte_config.task_priority);

    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(s_lte_config.apn);
#if HUB_LTE_PPPOS_USE_SIM7600_DCE
    esp_modem_dce_device_t dce_device = ESP_MODEM_DCE_SIM7600;
#else
    esp_modem_dce_device_t dce_device = ESP_MODEM_DCE_GENERIC;
#endif
    ESP_LOGI(TAG,
             "PPPoS DCE config: device=%s(%d) apn=%s pdp_context=esp_modem_default data_cmd=ATD*99# alt_cmd=%s",
             hub_lte_pppos_dce_device_name(dce_device),
             dce_device,
             s_lte_config.apn,
             HUB_LTE_PPPOS_ALT_DIAL_CMD);
    s_lte_dce = esp_modem_new_dev(dce_device, &s_lte_dte_config, &dce_config, s_lte_ppp_netif);
    if (s_lte_dce == NULL) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_modem created: device=%s", hub_lte_pppos_dce_device_name(dce_device));

    esp_err_t err_cb_ret = esp_modem_set_error_cb(s_lte_dce, hub_lte_pppos_terminal_error_handler);
    ESP_LOGI(TAG, "esp_modem_set_error_cb returned %s", esp_err_to_name(err_cb_ret));

    esp_err_t ret = esp_modem_set_apn(s_lte_dce, s_lte_config.apn);
    ESP_LOGI(TAG, "esp_modem_set_apn(%s) returned %s", s_lte_config.apn, esp_err_to_name(ret));
    if (ret != ESP_OK) {
        esp_modem_destroy(s_lte_dce);
        s_lte_dce = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "APN set: %s", s_lte_config.apn);

    s_lte_runtime.modem_created = true;
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t hub_lte_pppos_enter_data_mode(void)
{
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    if (s_lte_runtime.data_mode_entered) {
        return ESP_OK;
    }
    if (s_lte_dce == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t precheck_ret = hub_lte_pppos_pre_data_at_check();
    ESP_LOGI(TAG, "PPPoS pre-data AT checks complete: ret=%s", esp_err_to_name(precheck_ret));
    if (precheck_ret != ESP_OK) {
        return precheck_ret;
    }

    ESP_LOGI(TAG, "enter data mode: dtr_level_before=%d dtr_level_macro=%d dial_mode=%d", a7608_get_dtr_level(), HUB_LTE_PPPOS_DTR_LEVEL_BEFORE_DATA, HUB_LTE_PPPOS_DIAL_MODE);
#if HUB_LTE_PPPOS_DTR_LEVEL_BEFORE_DATA == 0
    esp_err_t dtr_ret = a7608_set_dtr_level(0);
    ESP_LOGI(TAG, "PPPoS DTR drive low before data mode: ret=%s level=%d", esp_err_to_name(dtr_ret), a7608_get_dtr_level());
#elif HUB_LTE_PPPOS_DTR_LEVEL_BEFORE_DATA == 1
    esp_err_t dtr_ret = a7608_set_dtr_level(1);
    ESP_LOGI(TAG, "PPPoS DTR drive high before data mode: ret=%s level=%d", esp_err_to_name(dtr_ret), a7608_get_dtr_level());
#else
    ESP_LOGI(TAG, "PPPoS DTR unchanged before data mode: level=%d", a7608_get_dtr_level());
#endif
    s_lte_data_mode_enter_tick = xTaskGetTickCount();
    s_lte_terminal_unexpected_flow_count = 0;
    s_lte_terminal_break_after_data = false;
    esp_err_t ret = esp_modem_set_mode(s_lte_dce, ESP_MODEM_MODE_DATA);
    ESP_LOGI(TAG, "esp_modem_set_mode(DATA) returned %s dtr_level_after=%d", esp_err_to_name(ret), a7608_get_dtr_level());
    if (ret != ESP_OK) {
        return ret;
    }

    s_lte_runtime.data_mode_entered = true;
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t hub_lte_pppos_start_ppp(void)
{
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    if (!s_lte_runtime.data_mode_entered) {
        return ESP_ERR_INVALID_STATE;
    }

    s_lte_runtime.ppp_started = true;
    hub_lte_pppos_set_last_result(ESP_OK, "PPP started; waiting for IP event");
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t hub_lte_pppos_stop_ppp(void)
{
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    esp_err_t first_error = ESP_OK;

    if ((s_lte_dce != NULL) && s_lte_runtime.data_mode_entered) {
#if HUB_LTE_PPPOS_SKIP_COMMAND_MODE_ON_TIMEOUT
        if (s_lte_timeout_cleanup_active) {
            ESP_LOGW(TAG, "Skip esp_modem_set_mode(COMMAND) during PPP timeout cleanup; destroy modem directly");
        } else
#endif
        {
        esp_err_t ret = esp_modem_set_mode(s_lte_dce, ESP_MODEM_MODE_COMMAND);
        ESP_LOGI(TAG, "esp_modem_set_mode(COMMAND) returned %s", esp_err_to_name(ret));
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "COMMAND mode exit failed; continuing runtime destroy and UART release");
            first_error = ret;
        }
        }
    }

    s_lte_runtime.ppp_started = false;
    s_lte_runtime.data_mode_entered = false;
    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    hub_network_manager_set_lte_status(false, NULL);
    return first_error;
#else
    s_lte_runtime.ppp_started = false;
    s_lte_runtime.data_mode_entered = false;
    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    return ESP_OK;
#endif
}

static esp_err_t hub_lte_pppos_destroy_runtime(void)
{
    esp_err_t first_error = hub_lte_pppos_stop_ppp();

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
    if (s_lte_dce != NULL) {
        esp_modem_destroy(s_lte_dce);
        s_lte_dce = NULL;
    }
    s_lte_runtime.modem_created = false;

    if (s_lte_ppp_handler_registered) {
        esp_err_t ret = esp_event_handler_unregister(NETIF_PPP_STATUS,
                                                     ESP_EVENT_ANY_ID,
                                                     hub_lte_pppos_ppp_event_handler);
        if ((first_error == ESP_OK) && (ret != ESP_OK)) {
            first_error = ret;
        }
        s_lte_ppp_handler_registered = false;
    }
    if (s_lte_ip_handler_registered) {
        esp_err_t ret = esp_event_handler_unregister(IP_EVENT,
                                                     ESP_EVENT_ANY_ID,
                                                     hub_lte_pppos_ip_event_handler);
        if ((first_error == ESP_OK) && (ret != ESP_OK)) {
            first_error = ret;
        }
        s_lte_ip_handler_registered = false;
    }
    if (s_lte_ppp_netif != NULL) {
        esp_netif_destroy(s_lte_ppp_netif);
        s_lte_ppp_netif = NULL;
    }
    s_lte_data_mode_enter_tick = 0;
    s_lte_terminal_unexpected_flow_count = 0;
    s_lte_terminal_break_after_data = false;
#endif

    s_lte_runtime.ppp_netif_created = false;
    if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
        esp_err_t ret = hub_lte_pppos_release_uart_owner();
        if ((first_error == ESP_OK) && (ret != ESP_OK)) {
            first_error = ret;
        }
    }
    return first_error;
}

static esp_err_t hub_lte_pppos_handle_starting_timeout(void)
{
    hub_lte_pppos_set_last_result(ESP_ERR_TIMEOUT, "PPP got IP timeout");
    ESP_LOGE(TAG, "PPP got IP timeout after %u ms; stopping PPPoS runtime", HUB_LTE_PPPOS_STARTING_TIMEOUT_MS);

    s_lte_timeout_cleanup_active = true;
    esp_err_t first_error = hub_lte_pppos_destroy_runtime();
    s_lte_timeout_cleanup_active = false;
    if (first_error != ESP_OK) {
        ESP_LOGW(TAG, "PPPoS runtime destroy during timeout returned: %s", esp_err_to_name(first_error));
    }

    esp_err_t resume_ret = a7608_request_resume();
    if ((first_error == ESP_OK) && (resume_ret != ESP_OK)) {
        first_error = resume_ret;
    }
    if (resume_ret != ESP_OK) {
        ESP_LOGW(TAG, "A7608 resume request after PPP timeout failed: %s", esp_err_to_name(resume_ret));
    }

#if HUB_LTE_PPPOS_RESET_A7608_ON_TIMEOUT
    esp_err_t reset_ret = a7608_hard_reset(100, 3000);
    ESP_LOGW(TAG, "A7608 hard reset after PPP timeout: %s", esp_err_to_name(reset_ret));
#endif

    s_lte_status.start_requested = false;
    s_lte_status.stop_requested = false;
    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    hub_network_manager_set_lte_status(false, NULL);

    (void)first_error;
    return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_TIMEOUT);
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
    s_lte_lifecycle.current_state = HUB_PPP_STATE_IDLE;
    s_lte_lifecycle.previous_state = HUB_PPP_STATE_IDLE;
    s_lte_lifecycle.last_error = ESP_OK;
    s_lte_lifecycle.state_enter_tick = xTaskGetTickCount();
    (void)hub_lte_pppos_reset_runtime();
    hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle initialized");
    (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);

    ESP_LOGI(TAG,
             "LTE PPPoS framework ready: HUB_LTE_PPPOS_ENABLE=%d HUB_LTE_PPPOS_TEST_MODE=%d HUB_LTE_PPPOS_REAL_RUNTIME=%d HUB_LTE_PPPOS_MANUAL_TEST=%d uart=%d owner=%s baud=%d tx=%d rx=%d apn=%s",
             HUB_LTE_PPPOS_ENABLE,
             HUB_LTE_PPPOS_TEST_MODE,
             HUB_LTE_PPPOS_REAL_RUNTIME,
             HUB_LTE_PPPOS_MANUAL_TEST,
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
    if (HUB_LTE_PPPOS_MANUAL_TEST == 0) {
        s_lte_status.start_requested = false;
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS manual test disabled");
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
    ESP_LOGI(TAG, "PPPoS test start requested: %s", preflight.reason);
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

bool hub_lte_pppos_real_runtime_enabled(void)
{
    return hub_lte_pppos_real_runtime_allowed();
}

bool hub_lte_pppos_is_running(void)
{
    hub_ppp_state_t state = hub_lte_pppos_get_state();
    return s_lte_status.start_requested ||
           (state == HUB_PPP_STATE_WAIT_UART) ||
           (state == HUB_PPP_STATE_MODEM_READY) ||
           (state == HUB_PPP_STATE_STARTING) ||
           (state == HUB_PPP_STATE_RUNNING) ||
           (state == HUB_PPP_STATE_STOPPING);
}

bool hub_lte_pppos_is_connected(void)
{
    return (hub_lte_pppos_get_state() == HUB_PPP_STATE_RUNNING) && s_lte_status.connected;
}

bool hub_lte_pppos_can_take_uart(void)
{
    return s_lte_status.initialized &&
           (HUB_LTE_PPPOS_TEST_MODE != 0) &&
            (HUB_LTE_PPPOS_MANUAL_TEST != 0) &&
           a7608_is_paused() &&
           (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS);
}

esp_err_t hub_lte_pppos_request_uart_owner(void)
{
    if (!s_lte_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((HUB_LTE_PPPOS_TEST_MODE == 0) || (HUB_LTE_PPPOS_MANUAL_TEST == 0)) {
        ESP_LOGW(TAG, "PPP UART request blocked: manual test mode is disabled");
        return ESP_ERR_INVALID_STATE;
    }
    if (!a7608_is_paused()) {
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "A7608 AT service is not paused");
        ESP_LOGW(TAG, "PPP UART request blocked: A7608 service is not paused");
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
    ESP_LOGI(TAG, "PPPoS UART owner acquired");
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
    ESP_LOGI(TAG, "PPPoS UART owner released");
    return ESP_OK;
}

hub_ppp_state_t hub_lte_pppos_get_state(void)
{
    return s_lte_lifecycle.current_state;
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

    s_lte_lifecycle.last_error = reason;
    if (s_lte_lifecycle.current_state == new_state) {
        s_lte_status.state = new_state;
        return ESP_OK;
    }

    s_lte_lifecycle.previous_state = s_lte_lifecycle.current_state;
    s_lte_lifecycle.current_state = new_state;
    s_lte_lifecycle.state_enter_tick = xTaskGetTickCount();
    s_lte_status.state = new_state;

    ESP_LOGI(TAG,
             "PPPoS state: %s -> %s",
             hub_lte_pppos_state_name(s_lte_lifecycle.previous_state),
             hub_lte_pppos_state_name(s_lte_lifecycle.current_state));
    if (new_state == HUB_PPP_STATE_ERROR) {
        ESP_LOGE(TAG, "PPPoS error: %s", hub_lte_pppos_get_last_reason());
    }

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

    esp_err_t ret;

    switch (hub_lte_pppos_get_state()) {
    case HUB_PPP_STATE_IDLE:
        if (s_lte_status.start_requested) {
            hub_lte_pppos_preflight_t preflight;
            ret = hub_lte_pppos_run_start_preflight(&preflight);
            if (ret != ESP_OK) {
                s_lte_status.start_requested = false;
                (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
                return ret;
            }
            ret = a7608_request_pause();
            if (ret != ESP_OK) {
                hub_lte_pppos_set_last_result(ret, "A7608 pause request failed");
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
        if ((HUB_LTE_PPPOS_TEST_MODE == 0) || (HUB_LTE_PPPOS_MANUAL_TEST == 0)) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS manual test mode disabled");
            return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
        }
        if (!a7608_is_paused()) {
            hub_lte_pppos_set_last_result(ESP_OK, "A7608 AT service still running");
            break;
        }
        if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS) {
            ret = hub_lte_pppos_request_uart_owner();
            if (ret != ESP_OK) {
                return ret;
            }
            hub_lte_pppos_set_last_result(ESP_OK, "A7608 service paused; PPPoS UART ready");
            return hub_lte_pppos_set_state(HUB_PPP_STATE_MODEM_READY, ESP_OK);
        }
        if (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_PPPOS) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_MODEM_READY, ESP_OK);
        }
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "UART not available for PPPoS");
        break;

    case HUB_PPP_STATE_MODEM_READY:
        if (!s_lte_status.start_requested) {
            return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }

        if (!hub_lte_pppos_real_runtime_allowed()) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPPoS real runtime manual test disabled");
            return hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
        }

        ret = hub_lte_pppos_create_netif();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP netif create failed");
            (void)hub_lte_pppos_destroy_runtime();
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_create_modem();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP modem create failed");
            (void)hub_lte_pppos_destroy_runtime();
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_enter_data_mode();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP data mode failed");
            (void)hub_lte_pppos_destroy_runtime();
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_start_ppp();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP start failed");
            (void)hub_lte_pppos_destroy_runtime();
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        return hub_lte_pppos_set_state(HUB_PPP_STATE_STARTING, ESP_OK);

    case HUB_PPP_STATE_STARTING:
    {
        TickType_t elapsed_ticks = xTaskGetTickCount() - s_lte_lifecycle.state_enter_tick;
        if (elapsed_ticks >= pdMS_TO_TICKS(HUB_LTE_PPPOS_STARTING_TIMEOUT_MS)) {
            return hub_lte_pppos_handle_starting_timeout();
        }
        if (!s_lte_terminal_break_after_data) {
            hub_lte_pppos_set_last_result(ESP_OK, "Waiting for PPP/IP event");
        }
        break;
    }

    case HUB_PPP_STATE_RUNNING:
        hub_lte_pppos_set_last_result(ESP_OK, "PPP running from IP event");
        break;

    case HUB_PPP_STATE_STOPPING:
        ret = hub_lte_pppos_destroy_runtime();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPPoS runtime destroy failed");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = a7608_request_resume();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "A7608 resume request failed");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        s_lte_status.start_requested = false;
        s_lte_status.stop_requested = false;
        hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle stopped");
        ESP_LOGI(TAG, "PPPoS stopped");
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

    hub_ppp_state_t state = hub_lte_pppos_get_state();
    if (hub_lte_pppos_real_runtime_allowed() &&
        ((state == HUB_PPP_STATE_STARTING) ||
         (state == HUB_PPP_STATE_RUNNING) ||
         (state == HUB_PPP_STATE_STOPPING))) {
        return ESP_OK;
    }

    s_lte_status.connected = connected;
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

esp_err_t hub_lte_pppos_get_runtime(hub_lte_pppos_runtime_t *runtime)
{
    if (runtime == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *runtime = s_lte_runtime;
    return ESP_OK;
}

esp_err_t hub_lte_pppos_reset_runtime(void)
{
    memset(&s_lte_runtime, 0, sizeof(s_lte_runtime));
    return ESP_OK;
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
                                (HUB_LTE_PPPOS_MANUAL_TEST != 0) &&
                                a7608_is_paused() &&
                                (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS);

    const a7608_status_t *modem_status = a7608_get_status();
    if (modem_status != NULL) {
        preflight->modem_status_known = modem_status->at_ready;
        preflight->sim_ready = modem_status->sim_ready;
        preflight->registered_to_network = modem_status->registered_home || modem_status->registered_roaming;
        preflight->has_signal = ((modem_status->csq > 0) && (modem_status->csq <= 31)) ||
                                (modem_status->rssi_dbm < 0);
        preflight->rssi = modem_status->rssi_dbm;
    }

    if (!preflight->pppos_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS disabled by build config");
    } else if (!preflight->test_mode_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS test mode disabled");
    } else if (HUB_LTE_PPPOS_REAL_RUNTIME == 0) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS real runtime disabled");
    } else if (HUB_LTE_PPPOS_MANUAL_TEST == 0) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS manual test disabled");
    } else if (config_ret != ESP_OK) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS config unavailable");
    } else if (!preflight->has_apn) {
        hub_lte_pppos_set_preflight_reason(preflight, "APN is empty");
    } else if (!preflight->config_valid) {
        hub_lte_pppos_set_preflight_reason(preflight, "Invalid PPPoS config");
    } else if (!s_lte_status.initialized) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS framework not initialized");
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
        hub_lte_pppos_set_preflight_reason(preflight, "Ready to start PPPoS lifecycle");
    }

    return ESP_OK;
}

const char *hub_lte_pppos_preflight_reason(void)
{
    return s_lte_preflight_reason;
}

esp_err_t hub_lte_pppos_get_last_error(void)
{
    return s_lte_lifecycle.last_error;
}

const char *hub_lte_pppos_get_last_reason(void)
{
    return s_lte_last_reason;
}
