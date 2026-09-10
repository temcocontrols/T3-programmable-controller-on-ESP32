#include "hub_lte_pppos.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "a7608.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "hub_network_manager.h"

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME
#include "esp_event.h"
#include "esp_modem_api.h"
#include "esp_modem_c_api_types.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_ppp.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#endif

static const char *TAG = "hub_lte_pppos";

#define HUB_LTE_PPPOS_STARTING_TIMEOUT_MS 60000U

#define HUB_LTE_PPPOS_MODEM_REFRESH_MS 10000U
#define HUB_LTE_PPPOS_MODEM_COPS_REFRESH_MS 60000U
#define HUB_LTE_PPPOS_MODEM_STATUS_FRESH_MS 15000U
#define HUB_LTE_PPPOS_MODEM_AT_PROBE_MS 5000U
#define HUB_LTE_PPPOS_REFRESH_DEFER_LOG_MS 10000U
#define HUB_LTE_PPPOS_CELL_NORMAL_WAIT_MS 90000U
#define HUB_LTE_PPPOS_CELL_CFUN_WAIT_MS 30000U
#define HUB_LTE_PPPOS_CELL_COPS_WAIT_MS 60000U
#define HUB_LTE_PPPOS_CELL_RADIO_OFF_WAIT_MS 3000U
#define HUB_LTE_PPPOS_CELL_RADIO_ON_MIN_WAIT_MS 8000U
#define HUB_LTE_PPPOS_CELL_RADIO_ON_WAIT_MS 30000U
#define HUB_LTE_PPPOS_CELL_HARD_RESET_QUIET_MS 8000U
#define HUB_LTE_PPPOS_CELL_HARD_RESET_WAIT_MS 30000U
#define HUB_LTE_PPPOS_CELL_AT_DEAD_WAIT_MS 15000U
#define HUB_LTE_PPPOS_CELL_SIM_MISSING_WAIT_MS 3000U
#define HUB_LTE_PPPOS_CELL_BACKOFF_MS 60000U
#define HUB_LTE_PPPOS_BOTH_SIM_NETWORK_FAIL_MS 300000U
#define HUB_LTE_PPPOS_RECONNECT_STABLE_MS 60000U
#define HUB_LTE_PPPOS_RECONNECT_DELAY_1_MS 5000U
#define HUB_LTE_PPPOS_RECONNECT_DELAY_2_MS 10000U
#define HUB_LTE_PPPOS_RECONNECT_DELAY_N_MS 30000U

#ifndef HUB_LTE_PPPOS_NET_TEST_ENABLE
#define HUB_LTE_PPPOS_NET_TEST_ENABLE 1
#endif

#ifndef HUB_LTE_PPPOS_NET_TEST_PUBLIC_IP
#define HUB_LTE_PPPOS_NET_TEST_PUBLIC_IP "114.114.114.114"
#endif

#ifndef HUB_LTE_PPPOS_NET_TEST_DNS_NAME
#define HUB_LTE_PPPOS_NET_TEST_DNS_NAME "example.com"
#endif

#ifndef HUB_LTE_PPPOS_NET_TEST_UDP_ENABLE
#define HUB_LTE_PPPOS_NET_TEST_UDP_ENABLE 1
#endif

#define HUB_LTE_PPPOS_NET_TEST_PING_TIMEOUT_MS 3000U
#define HUB_LTE_PPPOS_NET_TEST_SETTLE_MS 1000U
#define HUB_LTE_PPPOS_NET_TEST_SOCKET_TIMEOUT_MS 5000U
#define HUB_LTE_PPPOS_NET_TEST_TASK_STACK 4096U
#define HUB_LTE_PPPOS_NET_TEST_TASK_PRIO 4U
#define HUB_LTE_PPPOS_NET_TEST_TCP_HTTP_PORT 80U
#define HUB_LTE_PPPOS_NET_TEST_DNS_PORT 53U
#define HUB_LTE_PPPOS_NET_TEST_DNS_QUERY_MAX_LEN 96U
#define HUB_LTE_PPPOS_NET_TEST_DNS_RESPONSE_MAX_LEN 256U
#define HUB_LTE_PPPOS_NET_TEST_HTTP_RESPONSE_MAX_LEN 96U
#define HUB_LTE_PPPOS_PING_DONE_BIT BIT0

typedef struct {
    hub_ppp_state_t current_state;
    hub_ppp_state_t previous_state;
    esp_err_t last_error;
    TickType_t state_enter_tick;
} hub_lte_pppos_lifecycle_t;

static hub_lte_pppos_config_t s_lte_config;
static bool s_lte_config_saved;
static bool s_lte_apn_auto = true;
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
static char s_lte_last_preflight_log_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN];
static char s_lte_last_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN] = "No PPPoS lifecycle error";

typedef enum {
    CELL_RECOVERY_IDLE = 0,
    CELL_RECOVERY_WAIT_REGISTRATION,
    CELL_RECOVERY_CHECK_CFUN,
    CELL_RECOVERY_SET_CFUN_1,
    CELL_RECOVERY_REQUEST_AUTO_OPERATOR,
    CELL_RECOVERY_RADIO_OFF,
    CELL_RECOVERY_RADIO_ON,
    CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART,
    CELL_RECOVERY_HARD_RESET,
    CELL_RECOVERY_WAIT_AFTER_HARD_RESET,
    CELL_RECOVERY_BACKOFF,
} hub_lte_cell_recovery_stage_t;

typedef struct {
    hub_lte_cell_recovery_stage_t stage;
    TickType_t stage_enter_tick;
    TickType_t last_at_probe_tick;
    TickType_t last_full_refresh_tick;
    TickType_t last_operator_refresh_tick;
    TickType_t last_status_defer_log_tick;
    TickType_t last_preflight_defer_log_tick;
    TickType_t last_cell_recovery_defer_log_tick;
    TickType_t last_wait_log_tick;
    hub_lte_cell_recovery_stage_t last_status_defer_stage;
    hub_lte_cell_recovery_stage_t last_preflight_defer_stage;
    hub_lte_cell_recovery_stage_t last_cell_recovery_defer_stage;
    TickType_t next_retry_tick;
    TickType_t reconnect_due_tick;
    TickType_t running_since_tick;
    uint32_t recovery_attempt;
    uint32_t hardware_reset_count;
    uint32_t reconnect_attempt;
    esp_err_t last_at_probe_result;
    esp_err_t last_full_refresh_result;
    bool command_sent;
    bool cleanup_pending;
    bool cleanup_in_progress;
    bool cleanup_done;
    bool reconnect_scheduled;
    bool reconnect_after_cleanup;
    bool status_refresh_in_progress;
    bool radio_restart_context;
    bool radio_restart_at_ready;
    bool radio_restart_sim_ready;
    bool hard_reset_at_ready;
    bool hard_reset_sim_ready;
    bool sim_mismatch_tried;
    TickType_t slot_connect_start_tick;
    char last_status_defer_requester[24];
    char cleanup_reason[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN];
} hub_lte_cell_recovery_t;

static hub_lte_cell_recovery_t s_lte_cell_recovery;
static portMUX_TYPE s_lte_cleanup_lock = portMUX_INITIALIZER_UNLOCKED;

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
static TaskHandle_t s_lte_net_test_task;
static bool s_lte_net_test_running;
static portMUX_TYPE s_lte_net_test_lock = portMUX_INITIALIZER_UNLOCKED;
#endif

static esp_err_t hub_lte_pppos_create_netif(void);
static esp_err_t hub_lte_pppos_create_modem(void);
static esp_err_t hub_lte_pppos_enter_data_mode(void);
static esp_err_t hub_lte_pppos_start_ppp(void);
static esp_err_t hub_lte_pppos_stop_ppp(void);
static esp_err_t hub_lte_pppos_destroy_runtime(void);
static esp_err_t hub_lte_pppos_handle_starting_timeout(void);
static void hub_lte_pppos_set_last_result(esp_err_t error, const char *reason);
static const char *hub_lte_pppos_uart_owner_name(hub_lte_pppos_uart_owner_t owner);
static const char *hub_lte_cell_recovery_stage_name(hub_lte_cell_recovery_stage_t stage);
static void hub_lte_pppos_cell_recovery_set_stage(hub_lte_cell_recovery_stage_t stage, const char *reason);
static bool hub_lte_pppos_can_use_at_status_uart(void);
static void hub_lte_pppos_format_status_age(const a7608_status_t *status, char *buf, size_t buf_len);
static esp_err_t hub_lte_pppos_probe_at_if_due(const char *requester, bool *probe_sent);
static esp_err_t hub_lte_pppos_request_status_refresh(const char *requester);
static bool hub_lte_pppos_modem_registered_fresh(void);
static void hub_lte_pppos_process_radio_restart_wait(void);
static bool hub_lte_pppos_switch_sim_slot_on_missing(const char *reason);
static bool hub_lte_pppos_switch_sim_on_network_fail(const char *reason);
static void hub_lte_pppos_both_sim_network_failover_process(void);
static void hub_lte_pppos_cell_recovery_process(void);
static bool hub_lte_pppos_request_async_cleanup(const char *reason);
static esp_err_t hub_lte_pppos_cleanup_after_loss(const char *reason);
static void hub_lte_pppos_schedule_reconnect(void);
static void hub_lte_pppos_reconnect_process(void);

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

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME && HUB_LTE_PPPOS_NET_TEST_ENABLE
typedef struct {
    esp_ip4_addr_t ip;
    esp_ip4_addr_t gateway;
    char ip_text[HUB_LTE_PPPOS_IP_ADDR_LEN];
    char gateway_text[HUB_LTE_PPPOS_IP_ADDR_LEN];
} hub_lte_pppos_net_test_args_t;

typedef struct {
    EventGroupHandle_t event_group;
    bool success;
    uint32_t elapsed_ms;
    uint32_t recv_size;
    uint8_t ttl;
} hub_lte_pppos_ping_result_t;

static void hub_lte_pppos_start_net_tests(const ip_event_got_ip_t *event);
static void hub_lte_pppos_net_test_task(void *arg);
static const char *hub_lte_pppos_ping_ipv4(const char *label, const ip_addr_t *target_addr);
static bool hub_lte_pppos_resolve_dns_name(const char *name, struct sockaddr_in *resolved_addr, char *addr_text, size_t addr_text_len);
static bool hub_lte_pppos_tcp_http_roundtrip(const char *name, const struct sockaddr_in *remote_addr, const char *remote_ip);
static const char *hub_lte_pppos_udp_dns_roundtrip(const char *name);
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

    hub_ppp_state_t ppp_state = hub_lte_pppos_get_state();
    bool ppp_data_session = (ppp_state == HUB_PPP_STATE_STARTING) ||
                            (ppp_state == HUB_PPP_STATE_RUNNING);

    ESP_LOGW(TAG,
             "esp_modem terminal error: code=%d name=%s ppp_state=%s dtr_level=%d after_data=%d first3s=%d unexpected_count=%lu",
             error,
             hub_lte_pppos_terminal_error_name(error),
             hub_lte_pppos_state_name(ppp_state),
             a7608_get_dtr_level(),
             after_data_mode,
             in_first_three_seconds,
             (unsigned long)s_lte_terminal_unexpected_flow_count);

    /* A7608 often reports UART break during PPP data. Do not tear down a live session. */
    if ((error == ESP_MODEM_TERMINAL_UNEXPECTED_CONTROL_FLOW) && ppp_data_session) {
        ESP_LOGW(TAG, "Ignoring UART break/unexpected_control_flow while PPP is %s",
                 hub_lte_pppos_state_name(ppp_state));
        return;
    }

    if (after_data_mode && hub_lte_pppos_request_async_cleanup("esp_modem terminal error")) {
        hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "esp_modem terminal error after data mode");
        if (hub_lte_pppos_get_state() != HUB_PPP_STATE_STOPPING) {
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
        }
    }
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

#if HUB_LTE_PPPOS_NET_TEST_ENABLE
static const char *hub_lte_pppos_test_result(bool pass)
{
    return pass ? "PASS" : "FAIL";
}

static bool hub_lte_pppos_ipv4_valid(const esp_ip4_addr_t *addr)
{
    return (addr != NULL) && (addr->addr != 0);
}

static bool hub_lte_pppos_net_test_ppp_up(const hub_lte_pppos_net_test_args_t *test_args)
{
    return (test_args != NULL) &&
           hub_lte_pppos_ipv4_valid(&test_args->ip) &&
           s_lte_status.connected &&
           (hub_lte_pppos_get_state() == HUB_PPP_STATE_RUNNING);
}

static bool hub_lte_pppos_net_test_try_mark_running(void)
{
    bool marked = false;

    taskENTER_CRITICAL(&s_lte_net_test_lock);
    if (!s_lte_net_test_running) {
        s_lte_net_test_running = true;
        marked = true;
    }
    taskEXIT_CRITICAL(&s_lte_net_test_lock);

    return marked;
}

static void hub_lte_pppos_net_test_clear_running(void)
{
    taskENTER_CRITICAL(&s_lte_net_test_lock);
    s_lte_net_test_running = false;
    s_lte_net_test_task = NULL;
    taskEXIT_CRITICAL(&s_lte_net_test_lock);
}

static void hub_lte_pppos_ping_success(esp_ping_handle_t handle, void *args)
{
    hub_lte_pppos_ping_result_t *result = (hub_lte_pppos_ping_result_t *)args;
    if (result == NULL) {
        return;
    }

    result->success = true;
    (void)esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &result->elapsed_ms, sizeof(result->elapsed_ms));
    (void)esp_ping_get_profile(handle, ESP_PING_PROF_SIZE, &result->recv_size, sizeof(result->recv_size));
    (void)esp_ping_get_profile(handle, ESP_PING_PROF_TTL, &result->ttl, sizeof(result->ttl));
}

static void hub_lte_pppos_ping_timeout(esp_ping_handle_t handle, void *args)
{
    (void)handle;
    hub_lte_pppos_ping_result_t *result = (hub_lte_pppos_ping_result_t *)args;
    if (result != NULL) {
        result->success = false;
    }
}

static void hub_lte_pppos_ping_end(esp_ping_handle_t handle, void *args)
{
    (void)handle;
    hub_lte_pppos_ping_result_t *result = (hub_lte_pppos_ping_result_t *)args;
    if ((result != NULL) && (result->event_group != NULL)) {
        xEventGroupSetBits(result->event_group, HUB_LTE_PPPOS_PING_DONE_BIT);
    }
}

static const char *hub_lte_pppos_ping_ipv4(const char *label, const ip_addr_t *target_addr)
{
    char target_text[IPADDR_STRLEN_MAX] = {0};
    ipaddr_ntoa_r(target_addr, target_text, sizeof(target_text));

    hub_lte_pppos_ping_result_t result = {0};
    result.event_group = xEventGroupCreate();
    if (result.event_group == NULL) {
        ESP_LOGE(TAG, "PPP traffic test ping %s: INFO result=ERROR reason=event_group_alloc", label);
        return "ERROR";
    }

    int ppp_if_index = esp_netif_get_netif_impl_index(s_lte_ppp_netif);
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = *target_addr;
    ping_config.count = 1;
    ping_config.timeout_ms = HUB_LTE_PPPOS_NET_TEST_PING_TIMEOUT_MS;
    ping_config.interface = (ppp_if_index > 0) ? (uint32_t)ppp_if_index : 0U;

    esp_ping_callbacks_t callbacks = {
        .on_ping_success = hub_lte_pppos_ping_success,
        .on_ping_timeout = hub_lte_pppos_ping_timeout,
        .on_ping_end = hub_lte_pppos_ping_end,
        .cb_args = &result,
    };

    esp_ping_handle_t ping = NULL;
    esp_err_t ret = esp_ping_new_session(&ping_config, &callbacks, &ping);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG,
                 "PPP traffic test ping %s: INFO result=ERROR target=%s create_ret=%s ppp_if_index=%ld route_unchanged=1",
                 label,
                 target_text,
                 esp_err_to_name(ret),
                 (long)ppp_if_index);
        vEventGroupDelete(result.event_group);
        return "ERROR";
    }

    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG,
                 "PPP traffic test ping %s: INFO result=ERROR target=%s start_ret=%s ppp_if_index=%ld route_unchanged=1",
                 label,
                 target_text,
                 esp_err_to_name(ret),
                 (long)ppp_if_index);
        (void)esp_ping_delete_session(ping);
        vEventGroupDelete(result.event_group);
        return "ERROR";
    }

    EventBits_t bits = xEventGroupWaitBits(result.event_group,
                                           HUB_LTE_PPPOS_PING_DONE_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(HUB_LTE_PPPOS_NET_TEST_PING_TIMEOUT_MS + 2000U));
    bool done = (bits & HUB_LTE_PPPOS_PING_DONE_BIT) != 0;
    if (!done) {
        (void)esp_ping_stop(ping);
    }

    (void)esp_ping_delete_session(ping);
    vEventGroupDelete(result.event_group);

    if (done && result.success) {
        ESP_LOGI(TAG,
                 "PPP traffic test ping %s: INFO result=PASS target=%s time=%lu ms size=%lu ttl=%u ppp_if_index=%ld route_unchanged=1",
                 label,
                 target_text,
                 (unsigned long)result.elapsed_ms,
                 (unsigned long)result.recv_size,
                 result.ttl,
                 (long)ppp_if_index);
        return "PASS";
    }

    ESP_LOGI(TAG,
             "PPP traffic test ping %s: INFO result=NO_REPLY target=%s timeout_ms=%u icmp_may_be_filtered=1 ppp_if_index=%ld route_unchanged=1",
             label,
             target_text,
             HUB_LTE_PPPOS_NET_TEST_PING_TIMEOUT_MS,
             (long)ppp_if_index);
    return "NO_REPLY";
}

static bool hub_lte_pppos_resolve_dns_name(const char *name, struct sockaddr_in *resolved_addr, char *addr_text, size_t addr_text_len)
{
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
    };
    struct addrinfo *result = NULL;

    int err = getaddrinfo(name, NULL, &hints, &result);
    if (err != 0) {
        ESP_LOGW(TAG, "PPP traffic test DNS resolve: FAIL name=%s err=%d", name, err);
        return false;
    }

    bool valid = false;
    if ((result != NULL) &&
        (result->ai_addr != NULL) &&
        (result->ai_addrlen >= sizeof(struct sockaddr_in))) {
        struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
        if (addr->sin_addr.s_addr != 0) {
            valid = true;
            if (resolved_addr != NULL) {
                *resolved_addr = *addr;
            }
            if ((addr_text != NULL) && (addr_text_len > 0)) {
                inet_ntoa_r(addr->sin_addr, addr_text, addr_text_len);
            }
        }
    }
    freeaddrinfo(result);

    if (valid) {
        ESP_LOGI(TAG, "PPP traffic test DNS resolve: PASS name=%s addr=%s", name, ((addr_text != NULL) && (addr_text[0] != '\0')) ? addr_text : "-");
        return true;
    }

    ESP_LOGW(TAG, "PPP traffic test DNS resolve: FAIL name=%s reason=no_valid_ipv4_addr", name);
    return false;
}

static void hub_lte_pppos_set_socket_timeout(int sock, uint32_t timeout_ms)
{
    struct timeval timeout = {
        .tv_sec = (long)(timeout_ms / 1000U),
        .tv_usec = (long)((timeout_ms % 1000U) * 1000U),
    };
    (void)setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

static bool hub_lte_pppos_wait_tcp_connect(int sock, const struct sockaddr_in *remote_addr, int *connect_errno)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    int ret = connect(sock, (const struct sockaddr *)remote_addr, sizeof(*remote_addr));
    if (ret == 0) {
        if (flags >= 0) {
            (void)fcntl(sock, F_SETFL, flags);
        }
        return true;
    }

    if (errno != EINPROGRESS) {
        if (connect_errno != NULL) {
            *connect_errno = errno;
        }
        return false;
    }

    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET(sock, &write_set);
    struct timeval timeout = {
        .tv_sec = (long)(HUB_LTE_PPPOS_NET_TEST_SOCKET_TIMEOUT_MS / 1000U),
        .tv_usec = (long)((HUB_LTE_PPPOS_NET_TEST_SOCKET_TIMEOUT_MS % 1000U) * 1000U),
    };

    ret = select(sock + 1, NULL, &write_set, NULL, &timeout);
    if (ret <= 0) {
        if (connect_errno != NULL) {
            *connect_errno = (ret == 0) ? ETIMEDOUT : errno;
        }
        return false;
    }

    int so_error = 0;
    socklen_t so_error_len = sizeof(so_error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) != 0) {
        if (connect_errno != NULL) {
            *connect_errno = errno;
        }
        return false;
    }
    if (so_error != 0) {
        if (connect_errno != NULL) {
            *connect_errno = so_error;
        }
        return false;
    }

    if (flags >= 0) {
        (void)fcntl(sock, F_SETFL, flags);
    }
    return true;
}

static void hub_lte_pppos_copy_status_line(char *dest, size_t dest_len, const char *src, int src_len)
{
    if ((dest == NULL) || (dest_len == 0)) {
        return;
    }
    dest[0] = '\0';
    if ((src == NULL) || (src_len <= 0)) {
        return;
    }

    size_t copy_len = 0;
    while ((copy_len < (dest_len - 1U)) &&
           (copy_len < (size_t)src_len) &&
           (src[copy_len] != '\r') &&
           (src[copy_len] != '\n')) {
        dest[copy_len] = src[copy_len];
        copy_len++;
    }
    dest[copy_len] = '\0';
}

static bool hub_lte_pppos_tcp_http_roundtrip(const char *name, const struct sockaddr_in *remote_addr, const char *remote_ip)
{
    if ((remote_addr == NULL) || (remote_ip == NULL) || (remote_ip[0] == '\0')) {
        ESP_LOGW(TAG, "PPP traffic test TCP HTTP: SKIP reason=no_valid_dns_result");
        return false;
    }

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGW(TAG, "PPP traffic test TCP HTTP: FAIL remote=%s connect=0 sent=0 received=0 errno=%d", remote_ip, errno);
        return false;
    }

    hub_lte_pppos_set_socket_timeout(sock, HUB_LTE_PPPOS_NET_TEST_SOCKET_TIMEOUT_MS);

    struct sockaddr_in connect_addr = *remote_addr;
    connect_addr.sin_port = htons(HUB_LTE_PPPOS_NET_TEST_TCP_HTTP_PORT);
    int connect_errno = 0;
    bool connected = hub_lte_pppos_wait_tcp_connect(sock, &connect_addr, &connect_errno);
    if (!connected) {
        ESP_LOGW(TAG,
                 "PPP traffic test TCP HTTP: FAIL remote=%s connect=0 sent=0 received=0 errno=%d",
                 remote_ip,
                 connect_errno);
        close(sock);
        return false;
    }

    char request[128];
    int request_len = snprintf(request,
                               sizeof(request),
                               "HEAD / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                               name);
    int sent = -1;
    int received = -1;
    char response[HUB_LTE_PPPOS_NET_TEST_HTTP_RESPONSE_MAX_LEN] = {0};
    char status_line[48] = {0};
    if ((request_len > 0) && (request_len < (int)sizeof(request))) {
        sent = send(sock, request, (size_t)request_len, 0);
        if (sent > 0) {
            received = recv(sock, response, sizeof(response) - 1U, 0);
            if (received > 0) {
                response[received] = '\0';
                hub_lte_pppos_copy_status_line(status_line, sizeof(status_line), response, received);
            }
        }
    }

    int saved_errno = errno;
    (void)shutdown(sock, SHUT_RDWR);
    close(sock);

    if ((sent > 0) && (received > 0)) {
        ESP_LOGI(TAG,
                 "PPP traffic test TCP HTTP: PASS remote=%s connect=1 sent=%d received=%d status=%s",
                 remote_ip,
                 sent,
                 received,
                 status_line[0] != '\0' ? status_line : "-");
        return true;
    }

    ESP_LOGW(TAG,
             "PPP traffic test TCP HTTP: FAIL remote=%s connect=1 sent=%d received=%d status=%s errno=%d",
             remote_ip,
             sent,
             received,
             status_line[0] != '\0' ? status_line : "-",
             saved_errno);
    return false;
}

static bool hub_lte_pppos_get_ppp_dns_server(esp_ip4_addr_t *dns_addr, char *dns_text, size_t dns_text_len)
{
    const int dns_types[] = {
        ESP_NETIF_DNS_MAIN,
        ESP_NETIF_DNS_BACKUP,
    };

    for (size_t index = 0; index < (sizeof(dns_types) / sizeof(dns_types[0])); index++) {
        esp_netif_dns_info_t dns_info = {0};
        esp_err_t ret = esp_netif_get_dns_info(s_lte_ppp_netif, dns_types[index], &dns_info);
        if ((ret == ESP_OK) &&
            (dns_info.ip.type == ESP_IPADDR_TYPE_V4) &&
            hub_lte_pppos_ipv4_valid(&dns_info.ip.u_addr.ip4)) {
            if (dns_addr != NULL) {
                *dns_addr = dns_info.ip.u_addr.ip4;
            }
            if ((dns_text != NULL) && (dns_text_len > 0)) {
                snprintf(dns_text, dns_text_len, IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
            }
            return true;
        }
    }

    return false;
}

static size_t hub_lte_pppos_build_dns_query(uint8_t *query, size_t query_len, uint16_t transaction_id, const char *name)
{
    if ((query == NULL) || (name == NULL) || (query_len < 18U)) {
        return 0;
    }

    memset(query, 0, query_len);
    query[0] = (uint8_t)(transaction_id >> 8);
    query[1] = (uint8_t)(transaction_id & 0xFFU);
    query[2] = 0x01;
    query[5] = 0x01;

    size_t offset = 12U;
    const char *label = name;
    while (*label != '\0') {
        const char *dot = strchr(label, '.');
        size_t label_len = (dot != NULL) ? (size_t)(dot - label) : strlen(label);
        if ((label_len == 0U) || (label_len > 63U) || ((offset + 1U + label_len + 5U) > query_len)) {
            return 0;
        }
        query[offset++] = (uint8_t)label_len;
        memcpy(&query[offset], label, label_len);
        offset += label_len;
        if (dot == NULL) {
            break;
        }
        label = dot + 1;
    }

    query[offset++] = 0x00;
    query[offset++] = 0x00;
    query[offset++] = 0x01;
    query[offset++] = 0x00;
    query[offset++] = 0x01;
    return offset;
}

static const char *hub_lte_pppos_udp_dns_roundtrip(const char *name)
{
    esp_ip4_addr_t dns_addr = {0};
    char dns_text[HUB_LTE_PPPOS_IP_ADDR_LEN] = {0};
    if (!hub_lte_pppos_get_ppp_dns_server(&dns_addr, dns_text, sizeof(dns_text))) {
        ESP_LOGI(TAG, "PPP traffic test UDP DNS: SKIP reason=no_valid_ppp_dns_server");
        return "SKIP";
    }

    uint8_t query[HUB_LTE_PPPOS_NET_TEST_DNS_QUERY_MAX_LEN] = {0};
    uint16_t transaction_id = (uint16_t)(((uint32_t)xTaskGetTickCount()) ^ dns_addr.addr ^ (uint32_t)s_lte_status.connected);
    size_t query_len = hub_lte_pppos_build_dns_query(query, sizeof(query), transaction_id, name);
    if (query_len == 0U) {
        ESP_LOGW(TAG, "PPP traffic test UDP DNS: FAIL server=%s reason=query_build_failed", dns_text);
        return "FAIL";
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGW(TAG, "PPP traffic test UDP DNS: FAIL server=%s sent=0 received=0 errno=%d", dns_text, errno);
        return "FAIL";
    }

    hub_lte_pppos_set_socket_timeout(sock, HUB_LTE_PPPOS_NET_TEST_SOCKET_TIMEOUT_MS);

    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(HUB_LTE_PPPOS_NET_TEST_DNS_PORT);
    dest_addr.sin_addr.s_addr = dns_addr.addr;

    int sent = sendto(sock,
                      query,
                      query_len,
                      0,
                      (struct sockaddr *)&dest_addr,
                      sizeof(dest_addr));
    if (sent != (int)query_len) {
        int saved_errno = errno;
        ESP_LOGW(TAG, "PPP traffic test UDP DNS: FAIL server=%s sent=%d received=0 errno=%d", dns_text, sent, saved_errno);
        close(sock);
        return "FAIL";
    }

    uint8_t response[HUB_LTE_PPPOS_NET_TEST_DNS_RESPONSE_MAX_LEN] = {0};
    int received = recvfrom(sock, response, sizeof(response), 0, NULL, NULL);
    int saved_errno = errno;
    close(sock);

    bool enough = received >= 12;
    bool id_match = enough && (response[0] == query[0]) && (response[1] == query[1]);
    bool is_response = enough && ((response[2] & 0x80U) != 0);
    uint8_t rcode = enough ? (response[3] & 0x0FU) : 0xFFU;
    bool pass = enough && id_match && is_response;
    if (pass) {
        ESP_LOGI(TAG,
                 "PPP traffic test UDP DNS: PASS server=%s sent=%d received=%d id_match=%d rcode=%u errno=0",
                 dns_text,
                 sent,
                 received,
                 id_match,
                 rcode);
        return "PASS";
    }

    ESP_LOGW(TAG,
             "PPP traffic test UDP DNS: FAIL server=%s sent=%d received=%d id_match=%d rcode=%u errno=%d",
             dns_text,
             sent,
             received,
             id_match,
             rcode,
             saved_errno);
    return "FAIL";
}

static void hub_lte_pppos_log_net_test_summary(const char *ppp_result,
                                               const char *dns_result,
                                               const char *tcp_result,
                                               const char *udp_result,
                                               const char *gateway_result,
                                               const char *public_result,
                                               const char *network_result)
{
    ESP_LOGI(TAG,
             "PPP traffic test result: ppp_up=%s dns=%s tcp=%s udp_roundtrip=%s gateway_ping=%s public_ping=%s network_usable=%s",
             ppp_result,
             dns_result,
             tcp_result,
             udp_result,
             gateway_result,
             public_result,
             network_result);
}

static void hub_lte_pppos_net_test_task(void *arg)
{
    hub_lte_pppos_net_test_args_t *test_args = (hub_lte_pppos_net_test_args_t *)arg;
    const char *gateway_result = "SKIP";
    const char *public_result = "SKIP";
    const char *dns_result = "SKIP";
    const char *udp_result = "SKIP";
    const char *tcp_result = "SKIP";
    const char *ppp_result = "FAIL";
    const char *network_result = "FAIL";
    bool dns_pass = false;
    bool tcp_pass = false;
    struct sockaddr_in resolved_addr = {0};
    char resolved_ip[INET_ADDRSTRLEN] = {0};

    if (test_args == NULL) {
        hub_lte_pppos_net_test_clear_running();
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG,
             "PPP traffic test begin: ppp_ip=%s gateway=%s public_ip=%s dns_name=%s udp=%d settle_ms=%u",
             test_args->ip_text,
             test_args->gateway_text,
             HUB_LTE_PPPOS_NET_TEST_PUBLIC_IP,
             HUB_LTE_PPPOS_NET_TEST_DNS_NAME,
             HUB_LTE_PPPOS_NET_TEST_UDP_ENABLE,
             HUB_LTE_PPPOS_NET_TEST_SETTLE_MS);

    vTaskDelay(pdMS_TO_TICKS(HUB_LTE_PPPOS_NET_TEST_SETTLE_MS));

    if (!hub_lte_pppos_net_test_ppp_up(test_args)) {
        goto finish;
    }

    if (test_args->gateway.addr != 0) {
        ip_addr_t gateway_addr;
        IP_ADDR4(&gateway_addr,
                 esp_ip4_addr1(&test_args->gateway),
                 esp_ip4_addr2(&test_args->gateway),
                 esp_ip4_addr3(&test_args->gateway),
                 esp_ip4_addr4(&test_args->gateway));
        gateway_result = hub_lte_pppos_ping_ipv4("gateway", &gateway_addr);
    } else {
        ESP_LOGI(TAG, "PPP traffic test ping gateway: INFO result=SKIP reason=empty_gateway");
    }

    if (!hub_lte_pppos_net_test_ppp_up(test_args)) {
        goto finish;
    }

    ip_addr_t public_addr;
    if (ipaddr_aton(HUB_LTE_PPPOS_NET_TEST_PUBLIC_IP, &public_addr) && IP_IS_V4(&public_addr)) {
        public_result = hub_lte_pppos_ping_ipv4("public", &public_addr);
    } else {
        ESP_LOGI(TAG, "PPP traffic test ping public: INFO result=SKIP invalid_target=%s", HUB_LTE_PPPOS_NET_TEST_PUBLIC_IP);
    }

    if (!hub_lte_pppos_net_test_ppp_up(test_args)) {
        goto finish;
    }

    dns_pass = hub_lte_pppos_resolve_dns_name(HUB_LTE_PPPOS_NET_TEST_DNS_NAME, &resolved_addr, resolved_ip, sizeof(resolved_ip));
    dns_result = hub_lte_pppos_test_result(dns_pass);

    if (!hub_lte_pppos_net_test_ppp_up(test_args)) {
        goto finish;
    }

    tcp_pass = hub_lte_pppos_tcp_http_roundtrip(HUB_LTE_PPPOS_NET_TEST_DNS_NAME,
                                                dns_pass ? &resolved_addr : NULL,
                                                dns_pass ? resolved_ip : NULL);
    tcp_result = hub_lte_pppos_test_result(tcp_pass);

    if (!hub_lte_pppos_net_test_ppp_up(test_args)) {
        goto finish;
    }

#if HUB_LTE_PPPOS_NET_TEST_UDP_ENABLE
    udp_result = hub_lte_pppos_udp_dns_roundtrip(HUB_LTE_PPPOS_NET_TEST_DNS_NAME);
#else
    ESP_LOGI(TAG, "PPP traffic test UDP DNS: SKIP reason=disabled");
#endif

finish:
    bool ppp_up = hub_lte_pppos_net_test_ppp_up(test_args);
    ppp_result = hub_lte_pppos_test_result(ppp_up);
    network_result = hub_lte_pppos_test_result(ppp_up && dns_pass && tcp_pass);
    hub_lte_pppos_log_net_test_summary(ppp_result,
                                       dns_result,
                                       tcp_result,
                                       udp_result,
                                       gateway_result,
                                       public_result,
                                       network_result);

    free(test_args);
    hub_lte_pppos_net_test_clear_running();
    vTaskDelete(NULL);
}

static void hub_lte_pppos_start_net_tests(const ip_event_got_ip_t *event)
{
    if (event == NULL) {
        return;
    }
    if (!hub_lte_pppos_net_test_try_mark_running()) {
        ESP_LOGW(TAG, "PPP traffic test already running; skip duplicate got-IP trigger");
        return;
    }

    hub_lte_pppos_net_test_args_t *test_args = (hub_lte_pppos_net_test_args_t *)calloc(1, sizeof(*test_args));
    if (test_args == NULL) {
        ESP_LOGE(TAG, "PPP traffic test start failed: no memory");
        hub_lte_pppos_net_test_clear_running();
        return;
    }

    test_args->ip = event->ip_info.ip;
    test_args->gateway = event->ip_info.gw;
    snprintf(test_args->ip_text, sizeof(test_args->ip_text), IPSTR, IP2STR(&event->ip_info.ip));
    snprintf(test_args->gateway_text, sizeof(test_args->gateway_text), IPSTR, IP2STR(&event->ip_info.gw));

    BaseType_t created = xTaskCreate(hub_lte_pppos_net_test_task,
                                     "pppos_net_test",
                                     HUB_LTE_PPPOS_NET_TEST_TASK_STACK,
                                     test_args,
                                     HUB_LTE_PPPOS_NET_TEST_TASK_PRIO,
                                     &s_lte_net_test_task);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "PPP traffic test start failed: xTaskCreate returned %ld", (long)created);
        free(test_args);
        hub_lte_pppos_net_test_clear_running();
    }
}
#endif
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

typedef struct {
    const char *plmn;
    const char *apn;
} hub_lte_plmn_apn_t;

static const hub_lte_plmn_apn_t s_lte_plmn_apn[] = {
    /* India - Jio */
    {"40584", "jionet"},
    {"40585", "jionet"},
    {"40586", "jionet"},
    {"40587", "jionet"},
    /* India - Airtel */
    {"40402", "airtelgprs.com"},
    {"40403", "airtelgprs.com"},
    {"40406", "airtelgprs.com"},
    {"40410", "airtelgprs.com"},
    {"40416", "airtelgprs.com"},
    {"40431", "airtelgprs.com"},
    {"40440", "airtelgprs.com"},
    {"40445", "airtelgprs.com"},
    {"40449", "airtelgprs.com"},
    {"40470", "airtelgprs.com"},
    {"40490", "airtelgprs.com"},
    {"40492", "airtelgprs.com"},
    {"40493", "airtelgprs.com"},
    {"40494", "airtelgprs.com"},
    {"40495", "airtelgprs.com"},
    {"40496", "airtelgprs.com"},
    {"40497", "airtelgprs.com"},
    {"40498", "airtelgprs.com"},
    {"40551", "airtelgprs.com"},
    {"40552", "airtelgprs.com"},
    {"40553", "airtelgprs.com"},
    {"40554", "airtelgprs.com"},
    {"40555", "airtelgprs.com"},
    {"40556", "airtelgprs.com"},
    /* India - Vi (Vodafone Idea) */
    {"40401", "www"},
    {"40405", "www"},
    {"40411", "www"},
    {"40413", "www"},
    {"40415", "www"},
    {"40420", "www"},
    {"40427", "www"},
    {"40430", "www"},
    {"40443", "www"},
    {"40446", "www"},
    {"40460", "www"},
    {"40484", "www"},
    {"40486", "www"},
    {"40488", "www"},
    {"40567", "www"},
    {"40570", "www"},
    {"405750", "www"},
    {"405751", "www"},
    {"405752", "www"},
    {"405753", "www"},
    {"405754", "www"},
    {"405755", "www"},
    {"405756", "www"},
    /* India - BSNL / MTNL */
    {"40434", "bsnlnet"},
    {"40438", "bsnlnet"},
    {"40451", "bsnlnet"},
    {"40453", "bsnlnet"},
    {"40454", "bsnlnet"},
    {"40455", "bsnlnet"},
    {"40457", "bsnlnet"},
    {"40458", "bsnlnet"},
    {"40459", "bsnlnet"},
    {"40462", "bsnlnet"},
    {"40464", "bsnlnet"},
    {"40466", "bsnlnet"},
    {"40471", "bsnlnet"},
    {"40472", "bsnlnet"},
    {"40473", "bsnlnet"},
    {"40474", "bsnlnet"},
    {"40475", "bsnlnet"},
    {"40476", "bsnlnet"},
    {"40477", "bsnlnet"},
    {"40480", "bsnlnet"},
    {"40481", "bsnlnet"},
    {"40468", "mtnl.net"},
    {"40469", "mtnl.net"},
    /* China */
    {"46000", "CMNET"},
    {"46002", "CMNET"},
    {"46004", "CMNET"},
    {"46007", "CMNET"},
    {"46008", "CMNET"},
    {"46013", "CMNET"},
    {"46001", "3GNET"},
    {"46006", "3GNET"},
    {"46009", "3GNET"},
    {"46003", "CTNET"},
    {"46005", "CTNET"},
    {"46011", "CTNET"},
    {"46012", "CTNET"},
    /* Pakistan */
    {"41001", "jazzconnect.mobilinkworld.com"},
    {"41003", "ufone.internet"},
    {"41004", "zonginternet"},
    {"41006", "internet"},
    {"41007", "jazzconnect.mobilinkworld.com"},
    /* Bangladesh */
    {"47001", "gpinternet"},
    {"47002", "internet"},
    {"47003", "blweb"},
    {"47004", "internet"},
    /* Sri Lanka / Nepal / Myanmar */
    {"41301", "mobitel"},
    {"41302", "dialogbb"},
    {"41305", "airtel"},
    {"42901", "ntcnet"},
    {"42902", "internet"},
    {"41401", "internet"},
    {"41405", "internet"},
    {"41406", "internet"},
    /* Indonesia */
    {"51010", "internet"},
    {"51011", "indosatgprs"},
    {"51008", "3gprs"},
    {"51001", "indosatgprs"},
    {"51009", "smartfren"},
    {"51021", "internet"},
    {"51089", "3gprs"},
    /* Malaysia / Singapore / Thailand / Vietnam / Philippines */
    {"50212", "unet"},
    {"50213", "celcom.net.my"},
    {"50216", "diginet"},
    {"50218", "my3g"},
    {"50219", "celcom.net.my"},
    {"52501", "internet"},
    {"52503", "internet"},
    {"52505", "shinternet"},
    {"52000", "internet"},
    {"52001", "internet"},
    {"52003", "internet"},
    {"52004", "internet"},
    {"52005", "www.dtac.co.th"},
    {"52015", "internet"},
    {"52018", "internet"},
    {"52099", "internet"},
    {"45201", "m-wap"},
    {"45202", "m-wap"},
    {"45204", "v-internet"},
    {"45205", "internet"},
    {"45207", "internet"},
    {"51502", "internet.globe.com.ph"},
    {"51503", "internet"},
    {"51505", "minternet"},
    {"51566", "internet.dito.ph"},
    /* Hong Kong / Macau / Taiwan / Japan / Korea */
    {"45400", "internet"},
    {"45402", "internet"},
    {"45403", "mobile.three.com.hk"},
    {"45404", "internet"},
    {"45406", "internet"},
    {"45412", "internet"},
    {"45416", "internet"},
    {"45419", "internet"},
    {"45500", "internet"},
    {"45501", "ctm-mobile"},
    {"45503", "internet"},
    {"45504", "ctm-mobile"},
    {"45507", "internet"},
    {"46601", "internet"},
    {"46605", "internet"},
    {"46688", "internet"},
    {"46689", "internet"},
    {"46692", "internet"},
    {"46693", "internet"},
    {"46697", "internet"},
    {"46699", "internet"},
    {"44010", "spmode.ne.jp"},
    {"44020", "plus.4g"},
    {"44050", "plus.4g"},
    {"44051", "plus.4g"},
    {"44000", "au.au-net.ne.jp"},
    {"44011", "rakuten.jp"},
    {"45005", "lte.sktelecom.com"},
    {"45008", "lte.ktfwing.com"},
    {"45006", "internet.lguplus.co.kr"},
    /* Australia / New Zealand */
    {"50501", "telstra.internet"},
    {"50502", "internet"},
    {"50503", "telstra.internet"},
    {"50506", "live.vodafone.com"},
    {"50571", "telstra.internet"},
    {"50572", "telstra.internet"},
    {"53001", "internet"},
    {"53005", "internet"},
    {"53024", "internet"},
    /* Middle East */
    {"42001", "jawalnet.com.sa"},
    {"42003", "web1"},
    {"42004", "zain"},
    {"42402", "etisalat.ae"},
    {"42403", "du"},
    {"42202", "internet"},
    {"42203", "internet"},
    {"42701", "web.vodafone.com.qa"},
    {"42702", "web.vodafone.com.qa"},
    {"41902", "internet"},
    {"41903", "action.wataniya.com"},
    {"41904", "internet.stc.com.kw"},
    {"42601", "internet"},
    {"42602", "internet"},
    {"42604", "internet"},
    {"42501", "internet"},
    {"42502", "sphone"},
    {"42503", "internet"},
    {"42507", "internet"},
    {"42508", "internet"},
    {"28601", "internet"},
    {"28602", "internet"},
    {"28603", "internet"},
    {"60201", "internet.vodafone.net"},
    {"60202", "internet"},
    {"60203", "internet"},
    {"41601", "internet"},
    {"41603", "internet"},
    {"41677", "internet"},
    {"41805", "internet"},
    {"41820", "internet"},
    {"41830", "internet"},
    {"41840", "internet"},
    {"43211", "mcinet"},
    {"43235", "mtnirancell"},
    /* Africa */
    {"65501", "internet"},
    {"65502", "internet"},
    {"65507", "internet"},
    {"65510", "internet"},
    {"62120", "web.gprs.mtnnigeria.net"},
    {"62130", "gloflat"},
    {"62140", "etisalat"},
    {"62150", "internet.ng.airtel.com"},
    {"62160", "internet.ng.airtel.com"},
    {"63902", "safaricom"},
    {"63903", "internet"},
    {"63907", "internet"},
    {"64002", "internet"},
    {"64004", "internet"},
    {"64005", "internet"},
    {"64101", "internet"},
    {"64110", "internet"},
    {"64114", "internet"},
    {"62001", "internet"},
    {"62002", "internet"},
    {"62003", "internet"},
    {"62006", "internet"},
    {"64601", "internet"},
    {"64602", "internet"},
    {"64604", "internet"},
    {"65001", "internet"},
    {"65010", "internet"},
    {"63401", "internet"},
    {"63402", "internet"},
    /* USA */
    {"310260", "fast.t-mobile.com"},
    {"310240", "fast.t-mobile.com"},
    {"310250", "fast.t-mobile.com"},
    {"310310", "fast.t-mobile.com"},
    {"310660", "fast.t-mobile.com"},
    {"310800", "fast.t-mobile.com"},
    {"310200", "fast.t-mobile.com"},
    {"311490", "fast.t-mobile.com"},
    {"311870", "fast.t-mobile.com"},
    {"311882", "fast.t-mobile.com"},
    {"310410", "broadband"},
    {"310150", "broadband"},
    {"310170", "broadband"},
    {"310380", "broadband"},
    {"310560", "broadband"},
    {"310680", "broadband"},
    {"310070", "broadband"},
    {"310090", "broadband"},
    {"311480", "vzwinternet"},
    {"310012", "vzwinternet"},
    {"310004", "vzwinternet"},
    {"311270", "vzwinternet"},
    {"311271", "vzwinternet"},
    {"311272", "vzwinternet"},
    {"311273", "vzwinternet"},
    {"311274", "vzwinternet"},
    {"311275", "vzwinternet"},
    {"311276", "vzwinternet"},
    {"311277", "vzwinternet"},
    {"311278", "vzwinternet"},
    {"311279", "vzwinternet"},
    {"311280", "vzwinternet"},
    {"311281", "vzwinternet"},
    {"311282", "vzwinternet"},
    {"311283", "vzwinternet"},
    {"311284", "vzwinternet"},
    {"311285", "vzwinternet"},
    {"311286", "vzwinternet"},
    {"311287", "vzwinternet"},
    {"311288", "vzwinternet"},
    {"311289", "vzwinternet"},
    {"311390", "vzwinternet"},
    {"311110", "vzwinternet"},
    {"311580", "usccinternet"},
    /* Canada */
    {"302720", "ltemobile.apn"},
    {"302370", "ltemobile.apn"},
    {"302320", "ltemobile.apn"},
    {"302490", "internet.freedommobile.ca"},
    {"302500", "vlmobile.com"},
    {"302510", "vlmobile.com"},
    {"302610", "pda.bell.ca"},
    {"302640", "pda.bell.ca"},
    {"302220", "isp.telus.com"},
    {"302270", "isp.telus.com"},
    {"302360", "isp.telus.com"},
    {"302760", "isp.telus.com"},
    /* Mexico / Brazil / LatAm */
    {"334020", "internet.itelcel.com"},
    {"33403", "internet.movistar.mx"},
    {"334050", "internet.attmex.mx"},
    {"334090", "internet.attmex.mx"},
    {"72406", "zap.vivo.com.br"},
    {"72410", "zap.vivo.com.br"},
    {"72411", "zap.vivo.com.br"},
    {"72423", "zap.vivo.com.br"},
    {"72405", "claro.com.br"},
    {"72432", "timbrasil.br"},
    {"72433", "timbrasil.br"},
    {"72434", "timbrasil.br"},
    {"732101", "internet.comcel.com.co"},
    {"732123", "internet.movistar.com.co"},
    {"732103", "internet.tigo.com.co"},
    {"71606", "movistar.pe"},
    {"71610", "claro.pe"},
    {"71617", "entel.pe"},
    {"73001", "bam.entelpcs.cl"},
    {"73002", "wap.tmovil.cl"},
    {"73003", "bam.clarochile.cl"},
    {"72207", "internet.movistar.com.ar"},
    {"722310", "igprs.claro.com.ar"},
    {"72234", "internet.personal.com"},
    /* UK / Ireland */
    {"23410", "mobile.o2.co.uk"},
    {"23411", "mobile.o2.co.uk"},
    {"23402", "mobile.o2.co.uk"},
    {"23415", "vodafone"},
    {"23420", "eeinternet"},
    {"23430", "eeinternet"},
    {"23433", "eeinternet"},
    {"23486", "eeinternet"},
    {"23408", "internet"},
    {"23426", "three.co.uk"},
    {"27201", "live.vodafone.com"},
    {"27202", "internet"},
    {"27203", "3ireland.ie"},
    {"27205", "3ireland.ie"},
    /* Germany / France / Italy / Spain / Netherlands / Belgium */
    {"26201", "internet.telekom"},
    {"26202", "web.vodafone.de"},
    {"26203", "internet"},
    {"26207", "internet.telekom"},
    {"26208", "internet"},
    {"26209", "web.vodafone.de"},
    {"20801", "orange"},
    {"20802", "orange"},
    {"20809", "orange"},
    {"20810", "sl2sfr"},
    {"20811", "sl2sfr"},
    {"20813", "sl2sfr"},
    {"20815", "internet"},
    {"20816", "internet"},
    {"20820", "ebouyges"},
    {"20826", "ebouyges"},
    {"22201", "ibox.tim.it"},
    {"22210", "mobile.vodafone.it"},
    {"22288", "internet.wind"},
    {"22299", "internet.wind"},
    {"21401", "telefonica.es"},
    {"21403", "orangeworld"},
    {"21404", "internet"},
    {"21405", "orangeworld"},
    {"21406", "internet"},
    {"21407", "movistar.es"},
    {"20404", "internet"},
    {"20408", "portalmmm.nl"},
    {"20412", "internet"},
    {"20416", "internet"},
    {"20601", "internet.proximus.be"},
    {"20605", "internet"},
    {"20610", "internet.be"},
    {"20620", "internet.be"},
    /* Nordics / DACH / CEE */
    {"24001", "online.telia.se"},
    {"24002", "4g.tele2.se"},
    {"24004", "data.tre.se"},
    {"24007", "internet.tele2.se"},
    {"24008", "internet.telenor.se"},
    {"24201", "telenor"},
    {"24202", "internet.public"},
    {"24205", "internet"},
    {"24405", "internet"},
    {"24412", "internet"},
    {"24491", "internet"},
    {"23801", "internet"},
    {"23802", "internet"},
    {"23806", "data.tre.dk"},
    {"23820", "internet"},
    {"23201", "a1.net"},
    {"23203", "internet"},
    {"23205", "drei.at"},
    {"22801", "gprs.swisscom.ch"},
    {"22802", "internet"},
    {"22803", "internet"},
    {"26001", "plus"},
    {"26002", "internet"},
    {"26003", "internet"},
    {"26006", "internet"},
    {"23101", "internet"},
    {"23102", "internet"},
    {"23106", "o2internet"},
    {"23001", "internet"},
    {"23002", "internet"},
    {"23003", "internet"},
    {"21601", "internet"},
    {"21630", "internet"},
    {"21670", "internet"},
    {"22601", "internet"},
    {"22603", "vodafone"},
    {"22605", "internet"},
    {"22610", "internet"},
    {"21901", "web.htgprs"},
    {"21902", "internet"},
    {"21910", "internet.tele2.hr"},
    {"22001", "internet"},
    {"22003", "internet"},
    {"22005", "internet"},
    {"20201", "internet"},
    {"20205", "internet"},
    {"20209", "gint.b-online.gr"},
    {"20210", "internet"},
    /* Russia / Ukraine / CIS */
    {"25001", "internet.mts.ru"},
    {"25002", "internet.beeline.ru"},
    {"25020", "internet"},
    {"25035", "internet.tele2.ru"},
    {"25099", "internet"},
    {"25501", "www.kyivstar.net"},
    {"25502", "internet"},
    {"25503", "internet"},
    {"25506", "internet"},
    {"25507", "internet"},
    {"40101", "internet"},
    {"40102", "internet.beeline.kz"},
    {"40177", "internet"},
    {"43404", "internet.beeline.uz"},
    {"43405", "internet"},
    {"43407", "net.ucell.uz"},
};

static const char *hub_lte_pppos_apn_for_plmn(const char *plmn)
{
    const char *best_apn = NULL;
    size_t best_len = 0;

    if ((plmn == NULL) || (plmn[0] == '\0')) {
        return NULL;
    }

    for (size_t i = 0; i < sizeof(s_lte_plmn_apn) / sizeof(s_lte_plmn_apn[0]); i++) {
        size_t plmn_len = strlen(s_lte_plmn_apn[i].plmn);
        if ((plmn_len > best_len) && (strncmp(plmn, s_lte_plmn_apn[i].plmn, plmn_len) == 0)) {
            best_apn = s_lte_plmn_apn[i].apn;
            best_len = plmn_len;
        }
    }
    return best_apn;
}

static bool hub_lte_pppos_copy_apn_if_valid(const char *src, const char *source)
{
    if (!hub_lte_pppos_apn_is_valid(src)) {
        return false;
    }

    bool changed = (strcmp(s_lte_config.apn, src) != 0);
    hub_lte_pppos_copy_string(s_lte_config.apn, sizeof(s_lte_config.apn), src);
    if (changed) {
        ESP_LOGI(TAG, "APN selected from %s: %s", source != NULL ? source : "unknown", s_lte_config.apn);
    }
    return true;
}

static esp_err_t hub_lte_pppos_resolve_apn(void)
{
    char imsi[A7608_IMSI_LEN] = {0};
    const a7608_status_t *status = a7608_get_status();
    const char *lookup_apn;

    if (hub_lte_pppos_apn_is_valid(s_lte_config.apn)) {
        return ESP_OK;
    }
    if (!a7608_startup_probe_complete()) {
        return ESP_ERR_INVALID_STATE;
    }

    if ((status != NULL) && hub_lte_pppos_copy_apn_if_valid(status->apn, "modem CGDCONT")) {
        return ESP_OK;
    }

    if (!a7608_status_is_registered()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (a7608_read_pdp_apn(s_lte_config.apn, sizeof(s_lte_config.apn)) == ESP_OK) {
        ESP_LOGI(TAG, "APN selected from modem CGDCONT query: %s", s_lte_config.apn);
        return ESP_OK;
    }
    s_lte_config.apn[0] = '\0';

    if (a7608_read_imsi(imsi, sizeof(imsi)) == ESP_OK) {
        lookup_apn = hub_lte_pppos_apn_for_plmn(imsi);
        if (hub_lte_pppos_copy_apn_if_valid(lookup_apn, "IMSI PLMN")) {
            return ESP_OK;
        }
        ESP_LOGW(TAG, "No APN mapping for IMSI PLMN prefix %s", imsi);
    }

    if ((status != NULL) && (status->operator_name[0] != '\0')) {
        lookup_apn = hub_lte_pppos_apn_for_plmn(status->operator_name);
        if (hub_lte_pppos_copy_apn_if_valid(lookup_apn, "COPS PLMN")) {
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "APN auto-select waiting: SIM/network has not provided an APN yet");
    return ESP_ERR_NOT_FOUND;
}

static const char *hub_lte_cell_recovery_stage_name(hub_lte_cell_recovery_stage_t stage)
{
    switch (stage) {
    case CELL_RECOVERY_IDLE:
        return "IDLE";
    case CELL_RECOVERY_WAIT_REGISTRATION:
        return "WAIT_REGISTRATION";
    case CELL_RECOVERY_CHECK_CFUN:
        return "CHECK_CFUN";
    case CELL_RECOVERY_SET_CFUN_1:
        return "SET_CFUN_1";
    case CELL_RECOVERY_REQUEST_AUTO_OPERATOR:
        return "REQUEST_AUTO_OPERATOR";
    case CELL_RECOVERY_RADIO_OFF:
        return "RADIO_OFF";
    case CELL_RECOVERY_RADIO_ON:
        return "RADIO_ON";
    case CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART:
        return "WAIT_AFTER_RADIO_RESTART";
    case CELL_RECOVERY_HARD_RESET:
        return "HARD_RESET";
    case CELL_RECOVERY_WAIT_AFTER_HARD_RESET:
        return "WAIT_AFTER_HARD_RESET";
    case CELL_RECOVERY_BACKOFF:
        return "BACKOFF";
    default:
        return "UNKNOWN";
    }
}

static void hub_lte_pppos_cell_recovery_set_stage(hub_lte_cell_recovery_stage_t stage, const char *reason)
{
    if (s_lte_cell_recovery.stage == stage) {
        return;
    }

    hub_lte_cell_recovery_stage_t previous_stage = s_lte_cell_recovery.stage;
    ESP_LOGW(TAG,
             "Cell recovery: %s -> %s reason=%s",
             hub_lte_cell_recovery_stage_name(previous_stage),
             hub_lte_cell_recovery_stage_name(stage),
             reason != NULL ? reason : "-");
    s_lte_cell_recovery.stage = stage;
    s_lte_cell_recovery.stage_enter_tick = xTaskGetTickCount();
    s_lte_cell_recovery.command_sent = false;
    s_lte_cell_recovery.last_wait_log_tick = 0;

    if (stage == CELL_RECOVERY_IDLE) {
        s_lte_cell_recovery.sim_mismatch_tried = false;
    }

    if (stage == CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) {
        s_lte_cell_recovery.radio_restart_context = true;
        s_lte_cell_recovery.radio_restart_at_ready = false;
        s_lte_cell_recovery.radio_restart_sim_ready = false;
        s_lte_cell_recovery.last_at_probe_tick = 0;
    } else if (previous_stage == CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) {
        s_lte_cell_recovery.radio_restart_context = false;
    }

    if (stage == CELL_RECOVERY_WAIT_AFTER_HARD_RESET) {
        s_lte_cell_recovery.hard_reset_at_ready = false;
        s_lte_cell_recovery.hard_reset_sim_ready = false;
        s_lte_cell_recovery.last_at_probe_tick = 0;
    }
}

static bool hub_lte_pppos_hard_reset_in_progress(void)
{
    return (s_lte_cell_recovery.stage == CELL_RECOVERY_HARD_RESET) ||
           (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_HARD_RESET);
}

static bool hub_lte_pppos_can_use_at_status_uart(void)
{
    hub_ppp_state_t state = hub_lte_pppos_get_state();
    return s_lte_status.initialized &&
           (s_lte_status.uart_owner == HUB_LTE_PPPOS_UART_OWNER_AT_STATUS) &&
           (a7608_get_service_state() == A7608_SERVICE_RUNNING) &&
           (state != HUB_PPP_STATE_WAIT_UART) &&
           (state != HUB_PPP_STATE_MODEM_READY) &&
           (state != HUB_PPP_STATE_STARTING) &&
           (state != HUB_PPP_STATE_RUNNING) &&
           (state != HUB_PPP_STATE_STOPPING);
}

static void hub_lte_pppos_format_status_age(const a7608_status_t *status, char *buf, size_t buf_len)
{
    if ((buf == NULL) || (buf_len == 0)) {
        return;
    }
    if ((status == NULL) || !status->status_valid) {
        snprintf(buf, buf_len, "invalid");
        return;
    }
    snprintf(buf, buf_len, "%lu", (unsigned long)a7608_status_age_ms());
}

static esp_err_t hub_lte_pppos_probe_at_if_due(const char *requester, bool *probe_sent)
{
    TickType_t now = xTaskGetTickCount();
    if (probe_sent != NULL) {
        *probe_sent = false;
    }

    if ((s_lte_cell_recovery.last_at_probe_tick != 0) &&
        ((now - s_lte_cell_recovery.last_at_probe_tick) < pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_AT_PROBE_MS))) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = a7608_probe();
    s_lte_cell_recovery.last_at_probe_tick = xTaskGetTickCount();
    s_lte_cell_recovery.last_at_probe_result = ret;
    if (probe_sent != NULL) {
        *probe_sent = true;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG,
                 "A7608 AT probe: ret=%s next_retry_ms=0 requester=%s",
                 esp_err_to_name(ret),
                 requester != NULL ? requester : "-");
    } else {
        ESP_LOGW(TAG,
                 "A7608 AT probe: ret=%s next_retry_ms=%lu requester=%s",
                 esp_err_to_name(ret),
                 (unsigned long)HUB_LTE_PPPOS_MODEM_AT_PROBE_MS,
                 requester != NULL ? requester : "-");
    }
    return ret;
}

static esp_err_t hub_lte_pppos_request_status_refresh(const char *requester)
{
    if (!hub_lte_pppos_can_use_at_status_uart()) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!a7608_startup_probe_complete()) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_lte_cell_recovery.status_refresh_in_progress) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) ||
        hub_lte_pppos_hard_reset_in_progress()) {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t now = xTaskGetTickCount();

    const a7608_status_t *status = a7608_get_status();
    if ((s_lte_cell_recovery.last_full_refresh_tick == 0) &&
        (status != NULL) &&
        (status->last_refresh_tick != 0)) {
        s_lte_cell_recovery.last_full_refresh_tick = status->last_refresh_tick;
        s_lte_cell_recovery.last_full_refresh_result = status->last_refresh_result;
        if (status->operator_name[0] != '\0') {
            s_lte_cell_recovery.last_operator_refresh_tick = status->last_refresh_tick;
        }
    }
    bool need_at_probe = (status == NULL) ||
                         !status->at_ready ||
                         !status->status_valid ||
                         (status->last_refresh_result != ESP_OK);
    if (need_at_probe) {
        bool probe_sent = false;
        esp_err_t probe_ret = hub_lte_pppos_probe_at_if_due(requester, &probe_sent);
        if (probe_ret != ESP_OK) {
            return probe_ret;
        }
        now = xTaskGetTickCount();
    }

    bool status_due = (s_lte_cell_recovery.last_full_refresh_tick == 0) ||
                      ((now - s_lte_cell_recovery.last_full_refresh_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_REFRESH_MS));
    if (!status_due) {
        const char *requester_text = requester != NULL ? requester : "-";
        TickType_t *last_log_tick = &s_lte_cell_recovery.last_status_defer_log_tick;
        hub_lte_cell_recovery_stage_t *last_log_stage = &s_lte_cell_recovery.last_status_defer_stage;
        if (strcmp(requester_text, "preflight") == 0) {
            last_log_tick = &s_lte_cell_recovery.last_preflight_defer_log_tick;
            last_log_stage = &s_lte_cell_recovery.last_preflight_defer_stage;
        } else if (strcmp(requester_text, "cell_recovery") == 0) {
            last_log_tick = &s_lte_cell_recovery.last_cell_recovery_defer_log_tick;
            last_log_stage = &s_lte_cell_recovery.last_cell_recovery_defer_stage;
        }
        bool stage_changed = *last_log_stage != s_lte_cell_recovery.stage;
        if ((*last_log_tick == 0) ||
            stage_changed ||
            ((now - *last_log_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_REFRESH_DEFER_LOG_MS))) {
            uint32_t elapsed_ms = (uint32_t)((now - s_lte_cell_recovery.last_full_refresh_tick) * portTICK_PERIOD_MS);
            uint32_t remaining_ms = elapsed_ms >= HUB_LTE_PPPOS_MODEM_REFRESH_MS ? 0U : HUB_LTE_PPPOS_MODEM_REFRESH_MS - elapsed_ms;
            ESP_LOGI(TAG,
                     "A7608 full status refresh deferred: requester=%s remaining_ms=%lu min_interval_ms=%lu",
                     requester_text,
                     (unsigned long)remaining_ms,
                     (unsigned long)HUB_LTE_PPPOS_MODEM_REFRESH_MS);
            *last_log_tick = now;
            *last_log_stage = s_lte_cell_recovery.stage;
            hub_lte_pppos_copy_string(s_lte_cell_recovery.last_status_defer_requester,
                                      sizeof(s_lte_cell_recovery.last_status_defer_requester),
                                      requester_text);
        }
        return ESP_OK;
    }

    bool include_operator = (s_lte_cell_recovery.last_operator_refresh_tick == 0) ||
                            ((now - s_lte_cell_recovery.last_operator_refresh_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_COPS_REFRESH_MS));
    s_lte_cell_recovery.status_refresh_in_progress = true;
    esp_err_t ret = a7608_refresh_status_ex(include_operator);
    s_lte_cell_recovery.status_refresh_in_progress = false;
    s_lte_cell_recovery.last_full_refresh_tick = xTaskGetTickCount();
    s_lte_cell_recovery.last_full_refresh_result = ret;
    s_lte_cell_recovery.last_status_defer_log_tick = 0;
    s_lte_cell_recovery.last_preflight_defer_log_tick = 0;
    s_lte_cell_recovery.last_cell_recovery_defer_log_tick = 0;
    s_lte_cell_recovery.last_status_defer_requester[0] = '\0';
    if (include_operator) {
        s_lte_cell_recovery.last_operator_refresh_tick = s_lte_cell_recovery.last_full_refresh_tick;
    }

    status = a7608_get_status();
    char age_text[16];
    hub_lte_pppos_format_status_age(status, age_text, sizeof(age_text));
    ESP_LOGI(TAG,
             "A7608 preflight status: valid=%d age_ms=%s refresh=%s at=%d sim=%d csq=%d rssi_valid=%d creg=%d cereg=%d attached=%d cfun=%d registered=%d slot=%s sim1=%d sim2=%d requester=%s",
             status != NULL ? status->status_valid : 0,
             age_text,
             esp_err_to_name(ret),
             status != NULL ? status->at_ready : 0,
             status != NULL ? status->sim_ready : 0,
             status != NULL ? status->csq : 99,
             status != NULL ? status->rssi_valid : 0,
             status != NULL ? status->creg_stat : -1,
             status != NULL ? status->cereg_stat : -1,
             status != NULL ? status->attached : 0,
             status != NULL ? status->cfun : -1,
             a7608_status_is_registered(),
             a7608_sim_slot_name(status != NULL ? status->active_sim_slot : A7608_SIM_SLOT_NONE),
             status != NULL ? status->sim1_present : 0,
             status != NULL ? status->sim2_present : 0,
             requester != NULL ? requester : "-");
    return ret;
}

static bool hub_lte_pppos_modem_registered_fresh(void)
{
    const a7608_status_t *status = a7608_get_status();
    return (status != NULL) &&
           a7608_status_is_fresh(HUB_LTE_PPPOS_MODEM_STATUS_FRESH_MS) &&
           status->at_ready &&
           status->sim_ready &&
           a7608_status_is_registered() &&
           (status->cfun == 1);
}

static void hub_lte_pppos_process_radio_restart_wait(void)
{
    TickType_t now = xTaskGetTickCount();
    TickType_t elapsed_ticks = now - s_lte_cell_recovery.stage_enter_tick;
    uint32_t elapsed_ms = (uint32_t)(elapsed_ticks * portTICK_PERIOD_MS);

    if (!s_lte_cell_recovery.radio_restart_context) {
        s_lte_cell_recovery.radio_restart_context = true;
        s_lte_cell_recovery.radio_restart_at_ready = false;
        s_lte_cell_recovery.radio_restart_sim_ready = false;
        s_lte_cell_recovery.last_at_probe_tick = 0;
    }

    bool probe_sent = false;
    esp_err_t at_ret = hub_lte_pppos_probe_at_if_due("radio_restart_wait", &probe_sent);
    if (probe_sent && (at_ret == ESP_OK)) {
        bool sim_ready = false;
        esp_err_t sim_ret = a7608_check_sim_ready(&sim_ready);
        s_lte_cell_recovery.radio_restart_at_ready = true;
        s_lte_cell_recovery.radio_restart_sim_ready = (sim_ret == ESP_OK) && sim_ready;
    } else if (probe_sent) {
        s_lte_cell_recovery.radio_restart_at_ready = false;
        s_lte_cell_recovery.radio_restart_sim_ready = false;
    }

    if ((s_lte_cell_recovery.last_wait_log_tick == 0) ||
        ((now - s_lte_cell_recovery.last_wait_log_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_AT_PROBE_MS))) {
        ESP_LOGI(TAG,
                 "Cell recovery wait: elapsed_ms=%lu at=%d sim=%d",
                 (unsigned long)elapsed_ms,
                 s_lte_cell_recovery.radio_restart_at_ready,
                 s_lte_cell_recovery.radio_restart_sim_ready);
        s_lte_cell_recovery.last_wait_log_tick = now;
    }

    if ((elapsed_ticks >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_RADIO_ON_MIN_WAIT_MS)) &&
        s_lte_cell_recovery.radio_restart_at_ready &&
        s_lte_cell_recovery.radio_restart_sim_ready) {
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "at_and_sim_ready");
        return;
    }

    if (elapsed_ticks >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_RADIO_ON_WAIT_MS)) {
        if (s_lte_cell_recovery.radio_restart_at_ready &&
            !s_lte_cell_recovery.radio_restart_sim_ready) {
            if (!hub_lte_pppos_switch_sim_slot_on_missing("sim_missing_after_radio_on")) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION,
                                                      "sim_missing_other_empty");
            }
            return;
        }
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_HARD_RESET,
                                              s_lte_cell_recovery.radio_restart_at_ready ? "sim_recovery_timeout" : "at_recovery_timeout");
    }
}

static void hub_lte_pppos_process_hard_reset(void)
{
    s_lte_cell_recovery.recovery_attempt++;
    s_lte_cell_recovery.hardware_reset_count++;
    ESP_LOGE(TAG,
             "Cell recovery: hardware reset attempt=%lu",
             (unsigned long)s_lte_cell_recovery.hardware_reset_count);
    esp_err_t ret = a7608_hard_reset(A7608_HARD_RESET_PULSE_MS, 0);
    ESP_LOGE(TAG,
             "Cell recovery: hardware reset result=%s",
             esp_err_to_name(ret));
    hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_AFTER_HARD_RESET,
                                          "hardware_reset_executed");
}

static void hub_lte_pppos_process_hard_reset_wait(void)
{
    TickType_t now = xTaskGetTickCount();
    TickType_t elapsed_ticks = now - s_lte_cell_recovery.stage_enter_tick;
    uint32_t elapsed_ms = (uint32_t)(elapsed_ticks * portTICK_PERIOD_MS);

    if ((elapsed_ticks >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_HARD_RESET_QUIET_MS)) &&
        hub_lte_pppos_can_use_at_status_uart() &&
        a7608_startup_probe_complete()) {
        bool probe_sent = false;
        esp_err_t at_ret = hub_lte_pppos_probe_at_if_due("hard_reset_wait", &probe_sent);
        if (probe_sent && (at_ret == ESP_OK)) {
            bool sim_ready = false;
            esp_err_t sim_ret = a7608_check_sim_ready(&sim_ready);
            s_lte_cell_recovery.hard_reset_at_ready = true;
            s_lte_cell_recovery.hard_reset_sim_ready = (sim_ret == ESP_OK) && sim_ready;
        } else if (probe_sent) {
            s_lte_cell_recovery.hard_reset_at_ready = false;
            s_lte_cell_recovery.hard_reset_sim_ready = false;
        }
    }

    if ((s_lte_cell_recovery.last_wait_log_tick == 0) ||
        ((now - s_lte_cell_recovery.last_wait_log_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_AT_PROBE_MS))) {
        ESP_LOGI(TAG,
                 "Cell recovery hard reset wait: elapsed_ms=%lu at=%d sim=%d",
                 (unsigned long)elapsed_ms,
                 s_lte_cell_recovery.hard_reset_at_ready,
                 s_lte_cell_recovery.hard_reset_sim_ready);
        s_lte_cell_recovery.last_wait_log_tick = now;
    }

    if (s_lte_cell_recovery.hard_reset_at_ready &&
        s_lte_cell_recovery.hard_reset_sim_ready) {
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION,
                                              "AT and SIM recovered");
        return;
    }

    if (elapsed_ticks >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_HARD_RESET_WAIT_MS)) {
        if (s_lte_cell_recovery.hard_reset_at_ready &&
            !s_lte_cell_recovery.hard_reset_sim_ready) {
            if (!hub_lte_pppos_switch_sim_slot_on_missing("sim_missing_after_hard_reset")) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION,
                                                      "sim_missing_other_empty");
            }
            return;
        }
        s_lte_cell_recovery.next_retry_tick = now + pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_BACKOFF_MS);
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_BACKOFF,
                                              "hard_reset_recovery_timeout");
    }
}

static bool hub_lte_pppos_request_async_cleanup(const char *reason)
{
    const char *cleanup_reason = reason != NULL ? reason : "PPP connection lost";
    char cleanup_reason_copy[HUB_LTE_PPPOS_PREFLIGHT_REASON_LEN];
    hub_ppp_state_t state = hub_lte_pppos_get_state();
    bool cleanup_pending;
    bool cleanup_in_progress;
    bool cleanup_done;

    hub_lte_pppos_copy_string(cleanup_reason_copy,
                              sizeof(cleanup_reason_copy),
                              cleanup_reason);
    portENTER_CRITICAL(&s_lte_cleanup_lock);
    cleanup_pending = s_lte_cell_recovery.cleanup_pending;
    cleanup_in_progress = s_lte_cell_recovery.cleanup_in_progress;
    cleanup_done = s_lte_cell_recovery.cleanup_done;
    if ((state == HUB_PPP_STATE_STOPPING) ||
        cleanup_pending ||
        cleanup_in_progress ||
        cleanup_done) {
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        ESP_LOGW(TAG,
                 "PPPoS duplicate cleanup ignored: reason=%s state=%s pending=%d in_progress=%d done=%d",
                 cleanup_reason,
                 hub_lte_pppos_state_name(state),
                 cleanup_pending,
                 cleanup_in_progress,
                 cleanup_done);
        return false;
    }

        memcpy(s_lte_cell_recovery.cleanup_reason,
            cleanup_reason_copy,
            sizeof(s_lte_cell_recovery.cleanup_reason));
    s_lte_cell_recovery.cleanup_pending = true;
    s_lte_cell_recovery.reconnect_after_cleanup = true;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);

    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    hub_network_manager_set_lte_status(false, NULL);
    ESP_LOGW(TAG, "PPPoS cleanup requested: reason=%s", cleanup_reason_copy);
    return true;
}

static esp_err_t hub_lte_pppos_cleanup_after_loss(const char *reason)
{
    bool cleanup_in_progress;
    bool cleanup_done;

    portENTER_CRITICAL(&s_lte_cleanup_lock);
    cleanup_in_progress = s_lte_cell_recovery.cleanup_in_progress;
    cleanup_done = s_lte_cell_recovery.cleanup_done;
    if (cleanup_in_progress || cleanup_done) {
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        ESP_LOGW(TAG,
                 "PPPoS duplicate cleanup ignored: reason=%s in_progress=%d done=%d",
                 reason != NULL ? reason : "-",
                 cleanup_in_progress,
                 cleanup_done);
        return ESP_OK;
    }
    s_lte_cell_recovery.cleanup_pending = false;
    s_lte_cell_recovery.cleanup_in_progress = true;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);

    ESP_LOGW(TAG, "PPPoS cleanup started: reason=%s", reason != NULL ? reason : "-");
    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    s_lte_status.start_requested = false;
    s_lte_status.stop_requested = false;
    hub_network_manager_set_lte_status(false, NULL);

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_REAL_RUNTIME && HUB_LTE_PPPOS_NET_TEST_ENABLE
    if (s_lte_net_test_running) {
        ESP_LOGW(TAG, "PPPoS cleanup: traffic-test task still running; it will abort when PPP state is not RUNNING");
    }
#endif

    esp_err_t cleanup_ret = hub_lte_pppos_destroy_runtime();
    if (cleanup_ret != ESP_OK) {
        ESP_LOGW(TAG, "PPPoS cleanup runtime destroy returned: %s", esp_err_to_name(cleanup_ret));
    }

    esp_err_t dtr_ret = a7608_set_dtr(false);
    ESP_LOGI(TAG, "PPPoS cleanup DTR command-state request: ret=%s gpio_readback=%d", esp_err_to_name(dtr_ret), a7608_get_dtr_level());

    esp_err_t resume_ret = a7608_request_resume();
    if (resume_ret != ESP_OK) {
        ESP_LOGW(TAG, "PPPoS cleanup A7608 resume request failed: %s", esp_err_to_name(resume_ret));
    }

    a7608_clear_stale_ready();
    if (a7608_try_exit_data_mode() != ESP_OK) {
        ESP_LOGW(TAG, "PPPoS cleanup PPP escape did not restore AT; issuing hardware reset");
        esp_err_t reset_ret = a7608_hard_reset(A7608_HARD_RESET_PULSE_MS, A7608_HARD_RESET_QUIET_MS);
        ESP_LOGW(TAG, "PPPoS cleanup hardware reset: %s", esp_err_to_name(reset_ret));
        (void)a7608_probe();
    } else {
        ESP_LOGI(TAG, "PPPoS cleanup PPP escape restored AT");
    }

    s_lte_cell_recovery.last_at_probe_tick = 0;
    s_lte_cell_recovery.last_full_refresh_tick = 0;
    s_lte_cell_recovery.last_status_defer_log_tick = 0;
    s_lte_cell_recovery.last_preflight_defer_log_tick = 0;
    s_lte_cell_recovery.last_cell_recovery_defer_log_tick = 0;
    s_lte_cell_recovery.last_status_defer_stage = CELL_RECOVERY_IDLE;
    s_lte_cell_recovery.last_preflight_defer_stage = CELL_RECOVERY_IDLE;
    s_lte_cell_recovery.last_cell_recovery_defer_stage = CELL_RECOVERY_IDLE;
    s_lte_cell_recovery.last_status_defer_requester[0] = '\0';
    s_lte_cell_recovery.last_wait_log_tick = 0;
    s_lte_cell_recovery.running_since_tick = 0;
    s_lte_cell_recovery.radio_restart_context = false;
    s_lte_cell_recovery.radio_restart_at_ready = false;
    s_lte_cell_recovery.radio_restart_sim_ready = false;
    portENTER_CRITICAL(&s_lte_cleanup_lock);
    s_lte_cell_recovery.cleanup_in_progress = false;
    s_lte_cell_recovery.cleanup_done = true;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);
    hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "PPP cleanup complete");
    ESP_LOGI(TAG,
             "PPPoS cleanup complete: uart_owner=%s at_resume_requested=%d cleanup_ret=%s",
             hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner),
             resume_ret == ESP_OK,
             esp_err_to_name(cleanup_ret));
    return cleanup_ret != ESP_OK ? cleanup_ret : resume_ret;
}

static void hub_lte_pppos_schedule_reconnect(void)
{
    uint32_t delay_ms;

    portENTER_CRITICAL(&s_lte_cleanup_lock);
    if (s_lte_cell_recovery.reconnect_scheduled) {
        uint32_t attempt = s_lte_cell_recovery.reconnect_attempt;
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        ESP_LOGI(TAG,
                 "PPPoS reconnect already scheduled: attempt=%lu",
                 (unsigned long)attempt);
        return;
    }

    s_lte_cell_recovery.reconnect_attempt++;
    if (s_lte_cell_recovery.reconnect_attempt == 1) {
        delay_ms = HUB_LTE_PPPOS_RECONNECT_DELAY_1_MS;
    } else if (s_lte_cell_recovery.reconnect_attempt == 2) {
        delay_ms = HUB_LTE_PPPOS_RECONNECT_DELAY_2_MS;
    } else {
        delay_ms = HUB_LTE_PPPOS_RECONNECT_DELAY_N_MS;
    }
    s_lte_cell_recovery.reconnect_due_tick = xTaskGetTickCount() + pdMS_TO_TICKS(delay_ms);
    s_lte_cell_recovery.reconnect_scheduled = true;
    uint32_t attempt = s_lte_cell_recovery.reconnect_attempt;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);
    ESP_LOGI(TAG,
             "PPPoS reconnect scheduled: attempt=%lu delay_ms=%lu",
             (unsigned long)attempt,
             (unsigned long)delay_ms);
}

static void hub_lte_pppos_reconnect_process(void)
{
    if (!s_lte_cell_recovery.reconnect_scheduled) {
        return;
    }
    if ((int32_t)(xTaskGetTickCount() - s_lte_cell_recovery.reconnect_due_tick) < 0) {
        return;
    }
    if (!hub_lte_pppos_modem_registered_fresh()) {
        ESP_LOGW(TAG, "PPPoS reconnect deferred: registration_fresh=0");
        s_lte_cell_recovery.reconnect_scheduled = false;
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "registration lost before reconnect");
        return;
    }

    ESP_LOGI(TAG,
             "PPPoS reconnect starting: registration_fresh=1 attempt=%lu",
             (unsigned long)s_lte_cell_recovery.reconnect_attempt);
    portENTER_CRITICAL(&s_lte_cleanup_lock);
    s_lte_cell_recovery.reconnect_scheduled = false;
    s_lte_cell_recovery.reconnect_after_cleanup = false;
    s_lte_cell_recovery.cleanup_done = false;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);
    s_lte_status.start_requested = true;
}

static bool hub_lte_pppos_switch_sim_slot_on_missing(const char *reason)
{
    a7608_sim_slot_t current = a7608_get_active_sim_slot();
    a7608_sim_slot_t next = a7608_alternate_sim_slot();
    const char *why = (reason != NULL) ? reason : "sim_not_inserted";
    bool current_present = a7608_sim_slot_detected(current);
    bool other_present = a7608_other_sim_slot_detected();
    bool physical_other = other_present && !current_present;
    bool both_present = other_present && current_present;
    bool gpio_cpin_mismatch = current_present && !other_present;

    if (physical_other || both_present) {
        ESP_LOGW(TAG,
                 "SIM missing on %s; switching to %s detect other=%d current=%d (%s)",
                 a7608_sim_slot_name(current),
                 a7608_sim_slot_name(next),
                 other_present,
                 current_present,
                 why);
    } else if (gpio_cpin_mismatch && !s_lte_cell_recovery.sim_mismatch_tried) {
        s_lte_cell_recovery.sim_mismatch_tried = true;
        ESP_LOGW(TAG,
                 "GPIO/CPIN mismatch on %s (gpio present, modem not inserted); trying %s (%s)",
                 a7608_sim_slot_name(current),
                 a7608_sim_slot_name(next),
                 why);
    } else {
        TickType_t now = xTaskGetTickCount();
        if ((s_lte_cell_recovery.last_wait_log_tick == 0) ||
            ((now - s_lte_cell_recovery.last_wait_log_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_AT_PROBE_MS))) {
            ESP_LOGW(TAG,
                     "SIM missing on %s; %s empty, staying (%s)",
                     a7608_sim_slot_name(current),
                     a7608_sim_slot_name(next),
                     why);
            s_lte_cell_recovery.last_wait_log_tick = now;
        }
        return false;
    }

    if (a7608_switch_sim_slot(next) != ESP_OK) {
        ESP_LOGE(TAG, "SIM slot switch to %s failed", a7608_sim_slot_name(next));
        return false;
    }
    s_lte_cell_recovery.slot_connect_start_tick = xTaskGetTickCount();
    hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_RADIO_OFF, why);
    return true;
}

static bool hub_lte_pppos_switch_sim_on_network_fail(const char *reason)
{
    a7608_sim_slot_t current = a7608_get_active_sim_slot();
    a7608_sim_slot_t next = a7608_alternate_sim_slot();
    const char *why = (reason != NULL) ? reason : "network_fail_both_sims";

    if (!a7608_sim_slot_detected(A7608_SIM_SLOT_1) ||
        !a7608_sim_slot_detected(A7608_SIM_SLOT_2)) {
        return false;
    }

    ESP_LOGW(TAG,
             "Both SIMs present; no network on %s for %u ms; switching to %s (%s)",
             a7608_sim_slot_name(current),
             HUB_LTE_PPPOS_BOTH_SIM_NETWORK_FAIL_MS,
             a7608_sim_slot_name(next),
             why);
    if (a7608_switch_sim_slot(next) != ESP_OK) {
        ESP_LOGE(TAG, "SIM slot switch to %s failed", a7608_sim_slot_name(next));
        return false;
    }
    s_lte_cell_recovery.slot_connect_start_tick = xTaskGetTickCount();
    hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_RADIO_OFF, why);
    return true;
}

static void hub_lte_pppos_both_sim_network_failover_process(void)
{
    TickType_t now = xTaskGetTickCount();
    hub_ppp_state_t state = hub_lte_pppos_get_state();

    if (state == HUB_PPP_STATE_RUNNING) {
        s_lte_cell_recovery.slot_connect_start_tick = 0;
        return;
    }
    if ((state != HUB_PPP_STATE_IDLE) && (state != HUB_PPP_STATE_ERROR)) {
        return;
    }
    if (!a7608_startup_probe_complete()) {
        return;
    }
    if (!a7608_sim_slot_detected(A7608_SIM_SLOT_1) ||
        !a7608_sim_slot_detected(A7608_SIM_SLOT_2)) {
        return;
    }

    switch (s_lte_cell_recovery.stage) {
    case CELL_RECOVERY_RADIO_OFF:
    case CELL_RECOVERY_RADIO_ON:
    case CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART:
    case CELL_RECOVERY_HARD_RESET:
    case CELL_RECOVERY_WAIT_AFTER_HARD_RESET:
        return;
    default:
        break;
    }

    if (s_lte_cell_recovery.slot_connect_start_tick == 0) {
        s_lte_cell_recovery.slot_connect_start_tick = now;
        return;
    }

    TickType_t elapsed = now - s_lte_cell_recovery.slot_connect_start_tick;
    if (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_BOTH_SIM_NETWORK_FAIL_MS)) {
        (void)hub_lte_pppos_switch_sim_on_network_fail("both_sim_no_network_timeout");
    }
}

static void hub_lte_pppos_cell_recovery_process(void)
{
    if (s_lte_cell_recovery.stage == CELL_RECOVERY_HARD_RESET) {
        hub_lte_pppos_process_hard_reset();
        return;
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_HARD_RESET) {
        hub_lte_pppos_process_hard_reset_wait();
        return;
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_BACKOFF) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - s_lte_cell_recovery.next_retry_tick) >= 0) {
            ESP_LOGW(TAG,
                     "Cell recovery: backoff complete attempt=%lu",
                     (unsigned long)s_lte_cell_recovery.recovery_attempt);
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "backoff_complete");
        }
        return;
    }

    if (!hub_lte_pppos_can_use_at_status_uart()) {
        return;
    }
    if (!a7608_startup_probe_complete()) {
        return;
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) {
        hub_lte_pppos_process_radio_restart_wait();
        return;
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_RADIO_OFF) {
        TickType_t elapsed = xTaskGetTickCount() - s_lte_cell_recovery.stage_enter_tick;
        char response[128];
        if (!s_lte_cell_recovery.command_sent) {
            esp_err_t ret = a7608_send_command("AT+CFUN=0", "OK", 5000, response, sizeof(response));
            s_lte_cell_recovery.command_sent = true;
            ESP_LOGW(TAG, "Cell recovery command: AT+CFUN=0 ret=%s", esp_err_to_name(ret));
        } else if (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_RADIO_OFF_WAIT_MS)) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_RADIO_ON, "radio_off_wait_done");
        }
        return;
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_RADIO_ON) {
        char response[128];
        esp_err_t ret = a7608_send_command("AT+CFUN=1", "OK", 5000, response, sizeof(response));
        s_lte_cell_recovery.command_sent = true;
        ESP_LOGW(TAG, "Cell recovery command: AT+CFUN=1 ret=%s", esp_err_to_name(ret));
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART,
                                              ret == ESP_OK ? "radio_on_sent" : "radio_on_failed_wait_recovery");
        return;
    }

    (void)hub_lte_pppos_request_status_refresh("cell_recovery");
    const a7608_status_t *status = a7608_get_status();
    if (hub_lte_pppos_modem_registered_fresh()) {
        if (s_lte_cell_recovery.stage != CELL_RECOVERY_IDLE) {
            ESP_LOGI(TAG,
                     "Cell recovery: registration restored creg=%d cereg=%d csq=%d",
                     status->creg_stat,
                     status->cereg_stat,
                     status->csq);
        }
        s_lte_cell_recovery.recovery_attempt = 0;
        s_lte_cell_recovery.hardware_reset_count = 0;
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_IDLE, "registration restored");
        if (s_lte_cell_recovery.reconnect_after_cleanup &&
            !s_lte_status.start_requested &&
            !hub_lte_pppos_is_running() &&
            !s_lte_cell_recovery.reconnect_scheduled) {
            hub_lte_pppos_schedule_reconnect();
        }
        return;
    }

    TickType_t now = xTaskGetTickCount();
    TickType_t elapsed = now - s_lte_cell_recovery.stage_enter_tick;

    if (!status->at_ready || !status->sim_ready) {
        if (s_lte_cell_recovery.stage == CELL_RECOVERY_IDLE) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "waiting for AT/SIM ready");
            return;
        }
        if (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_REGISTRATION) {
            if (!status->at_ready &&
                (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_AT_DEAD_WAIT_MS))) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_HARD_RESET, "AT dead after PPP cleanup");
                return;
            }
            if (status->at_ready &&
                (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_SIM_MISSING_WAIT_MS))) {
                bool current_present = a7608_sim_slot_detected(a7608_get_active_sim_slot());
                if (!status->sim_ready || !current_present) {
                    (void)hub_lte_pppos_switch_sim_slot_on_missing("sim_not_inserted_switch_slot");
                    return;
                }
            }
            if (!status->at_ready &&
                (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_NORMAL_WAIT_MS))) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_HARD_RESET,
                                                      "AT/SIM status timeout");
            }
            return;
        }
        if (status->at_ready && !status->sim_ready) {
            if (!hub_lte_pppos_switch_sim_slot_on_missing("sim_not_inserted_abort_cops")) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION,
                                                      "sim_missing_other_empty");
            }
            return;
        }
        if ((s_lte_cell_recovery.last_wait_log_tick == 0) ||
            ((now - s_lte_cell_recovery.last_wait_log_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_MODEM_AT_PROBE_MS))) {
            ESP_LOGW(TAG,
                     "Cell recovery transient status ignored for stage progress: stage=%s at=%d sim=%d refresh=%s",
                     hub_lte_cell_recovery_stage_name(s_lte_cell_recovery.stage),
                     status->at_ready,
                     status->sim_ready,
                     esp_err_to_name(status->last_refresh_result));
            s_lte_cell_recovery.last_wait_log_tick = now;
        }
    }

    if ((s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_REGISTRATION) &&
        (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_SIM_MISSING_WAIT_MS)) &&
        !a7608_sim_slot_detected(a7608_get_active_sim_slot())) {
        if (hub_lte_pppos_switch_sim_slot_on_missing("gpio_active_slot_empty")) {
            return;
        }
    }

    char response[128];

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_IDLE) {
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "unregistered fresh status");
        return;
    }

    switch (s_lte_cell_recovery.stage) {
    case CELL_RECOVERY_WAIT_REGISTRATION:
        if (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_NORMAL_WAIT_MS)) {
            if ((status == NULL) || !status->at_ready) {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_HARD_RESET, "registration_timeout_at_dead");
            } else if (!status->sim_ready) {
                (void)hub_lte_pppos_switch_sim_slot_on_missing("registration_timeout_sim_missing");
            } else {
                hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_CHECK_CFUN, "registration_timeout");
            }
        }
        break;

    case CELL_RECOVERY_CHECK_CFUN:
        if ((status == NULL) || !status->at_ready) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_HARD_RESET, "cfun_check_at_dead");
        } else if (!status->sim_ready) {
            (void)hub_lte_pppos_switch_sim_slot_on_missing("cfun_check_sim_missing");
        } else if (status->cfun != 1) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_SET_CFUN_1, "cfun_not_1");
        } else {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_REQUEST_AUTO_OPERATOR, "cfun_ok_unregistered");
        }
        break;

    case CELL_RECOVERY_SET_CFUN_1:
        if (!s_lte_cell_recovery.command_sent) {
            esp_err_t ret = a7608_send_command("AT+CFUN=1", "OK", 5000, response, sizeof(response));
            s_lte_cell_recovery.command_sent = true;
            ESP_LOGW(TAG, "Cell recovery command: AT+CFUN=1 ret=%s", esp_err_to_name(ret));
        } else if (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_CFUN_WAIT_MS)) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_REQUEST_AUTO_OPERATOR, "cfun_wait_done");
        }
        break;

    case CELL_RECOVERY_REQUEST_AUTO_OPERATOR:
        if (!s_lte_cell_recovery.command_sent) {
            esp_err_t ret = a7608_send_command("AT+COPS=0", "OK", 10000, response, sizeof(response));
            s_lte_cell_recovery.command_sent = true;
            ESP_LOGW(TAG, "Cell recovery command: AT+COPS=0 ret=%s", esp_err_to_name(ret));
        } else if (elapsed >= pdMS_TO_TICKS(HUB_LTE_PPPOS_CELL_COPS_WAIT_MS)) {
            hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_RADIO_OFF, "operator_wait_done");
        }
        break;

    case CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART:
        hub_lte_pppos_process_radio_restart_wait();
        break;

    case CELL_RECOVERY_HARD_RESET:
        hub_lte_pppos_process_hard_reset();
        break;

    case CELL_RECOVERY_WAIT_AFTER_HARD_RESET:
        hub_lte_pppos_process_hard_reset_wait();
        break;

    case CELL_RECOVERY_BACKOFF:
        break;

    default:
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "invalid_stage");
        break;
    }
}

static void hub_lte_pppos_set_preflight_reason(hub_lte_pppos_preflight_t *preflight, const char *reason)
{
    hub_lte_pppos_copy_string(s_lte_preflight_reason, sizeof(s_lte_preflight_reason), reason);
    if (preflight != NULL) {
        hub_lte_pppos_copy_string(preflight->reason, sizeof(preflight->reason), reason);
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

    esp_log_level_t log_level = (event_id == NETIF_PPP_ERRORNONE) || (event_id >= NETIF_PP_PHASE_OFFSET) ? ESP_LOG_INFO : ESP_LOG_WARN;
    ESP_LOG_LEVEL(log_level,
                  TAG,
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
            if (hub_lte_pppos_request_async_cleanup("PPP stopped by user event")) {
                hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP stopped by user event");
                (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
            }
        }
    } else if ((event_id > NETIF_PPP_ERRORNONE) && (event_id < NETIF_PP_PHASE_OFFSET)) {
        if (hub_lte_pppos_request_async_cleanup(hub_lte_pppos_ppp_status_name(event_id))) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP status error event");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ESP_ERR_INVALID_STATE);
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
        portENTER_CRITICAL(&s_lte_cleanup_lock);
        s_lte_cell_recovery.reconnect_scheduled = false;
        s_lte_cell_recovery.reconnect_after_cleanup = false;
        s_lte_cell_recovery.cleanup_done = false;
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        s_lte_cell_recovery.running_since_tick = xTaskGetTickCount();
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_IDLE, "PPP got IP");
        ESP_LOGI(TAG,
                 "PPP got IP: ip=" IPSTR " netmask=" IPSTR " gw=" IPSTR,
                 IP2STR(&event->ip_info.ip),
                 IP2STR(&event->ip_info.netmask),
                 IP2STR(&event->ip_info.gw));
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_MAIN, "main");
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_BACKUP, "backup");
        hub_lte_pppos_log_dns_info(ESP_NETIF_DNS_FALLBACK, "fallback");
    #if HUB_LTE_PPPOS_NET_TEST_ENABLE
        hub_lte_pppos_start_net_tests(event);
    #endif
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        s_lte_status.connected = false;
        s_lte_status.ip_addr[0] = '\0';
        hub_network_manager_set_lte_status(false, NULL);
        if (hub_lte_pppos_request_async_cleanup("PPP lost IP event")) {
            hub_lte_pppos_set_last_result(ESP_ERR_INVALID_STATE, "PPP lost IP event");
            ESP_LOGW(TAG,
                     "PPP lost IP: reason=%s state=%s",
                     hub_lte_pppos_get_last_reason(),
                     hub_lte_pppos_state_name(hub_lte_pppos_get_state()));
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
    ESP_LOGI(TAG, "PPPoS DTR request before data mode: requested_level=0 gpio_readback=%d ret=%s", a7608_get_dtr_level(), esp_err_to_name(dtr_ret));
#elif HUB_LTE_PPPOS_DTR_LEVEL_BEFORE_DATA == 1
    esp_err_t dtr_ret = a7608_set_dtr_level(1);
    ESP_LOGI(TAG, "PPPoS DTR request before data mode: requested_level=1 gpio_readback=%d ret=%s", a7608_get_dtr_level(), esp_err_to_name(dtr_ret));
#else
    ESP_LOGI(TAG, "PPPoS DTR unchanged before data mode: requested_level=-1 gpio_readback=%d", a7608_get_dtr_level());
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

    portENTER_CRITICAL(&s_lte_cleanup_lock);
    s_lte_cell_recovery.cleanup_pending = false;
    s_lte_cell_recovery.cleanup_in_progress = true;
    s_lte_cell_recovery.cleanup_done = false;
    s_lte_cell_recovery.reconnect_after_cleanup = true;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);
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
    esp_err_t reset_ret = a7608_hard_reset(A7608_HARD_RESET_PULSE_MS, A7608_HARD_RESET_QUIET_MS);
    ESP_LOGW(TAG, "A7608 hard reset after PPP timeout: %s", esp_err_to_name(reset_ret));
#endif

    s_lte_status.start_requested = false;
    s_lte_status.stop_requested = false;
    s_lte_status.connected = false;
    s_lte_status.ip_addr[0] = '\0';
    hub_network_manager_set_lte_status(false, NULL);
    portENTER_CRITICAL(&s_lte_cleanup_lock);
    s_lte_cell_recovery.cleanup_in_progress = false;
    s_lte_cell_recovery.cleanup_done = true;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);

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
    config->apn[0] = '\0';
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
    s_lte_apn_auto = !hub_lte_pppos_apn_is_valid(config->apn);
    memset(&s_lte_status, 0, sizeof(s_lte_status));
    s_lte_status.initialized = true;
    s_lte_status.uart_owner = HUB_LTE_PPPOS_UART_OWNER_AT_STATUS;
    s_lte_lifecycle.current_state = HUB_PPP_STATE_IDLE;
    s_lte_lifecycle.previous_state = HUB_PPP_STATE_IDLE;
    s_lte_lifecycle.last_error = ESP_OK;
    s_lte_lifecycle.state_enter_tick = xTaskGetTickCount();
    memset(&s_lte_cell_recovery, 0, sizeof(s_lte_cell_recovery));
    s_lte_cell_recovery.stage = CELL_RECOVERY_IDLE;
    s_lte_cell_recovery.stage_enter_tick = xTaskGetTickCount();
    (void)hub_lte_pppos_reset_runtime();
    hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle initialized");
    (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);

    ESP_LOGI(TAG,
             "LTE PPPoS framework ready: HUB_LTE_PPPOS_ENABLE=%d HUB_LTE_PPPOS_TEST_MODE=%d HUB_LTE_PPPOS_REAL_RUNTIME=%d HUB_LTE_PPPOS_MANUAL_TEST=%d uart=%d owner=%s baud=%d tx=%d rx=%d apn=%s auto=%d",
             HUB_LTE_PPPOS_ENABLE,
             HUB_LTE_PPPOS_TEST_MODE,
             HUB_LTE_PPPOS_REAL_RUNTIME,
             HUB_LTE_PPPOS_MANUAL_TEST,
             s_lte_config.uart_num,
             hub_lte_pppos_uart_owner_name(s_lte_status.uart_owner),
             s_lte_config.baud_rate,
             s_lte_config.tx_io_num,
             s_lte_config.rx_io_num,
             s_lte_config.apn[0] != '\0' ? s_lte_config.apn : "(from SIM)",
             s_lte_apn_auto);

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
    s_lte_apn_auto = !hub_lte_pppos_apn_is_valid(config->apn);
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

    portENTER_CRITICAL(&s_lte_cleanup_lock);
    s_lte_cell_recovery.cleanup_done = false;
    portEXIT_CRITICAL(&s_lte_cleanup_lock);
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

    if (s_lte_cell_recovery.cleanup_pending) {
        (void)hub_lte_pppos_cleanup_after_loss(s_lte_cell_recovery.cleanup_reason);
        if (hub_lte_pppos_get_state() == HUB_PPP_STATE_ERROR) {
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
        return ESP_OK;
    }

    if ((hub_lte_pppos_get_state() == HUB_PPP_STATE_IDLE) ||
        (hub_lte_pppos_get_state() == HUB_PPP_STATE_ERROR)) {
        hub_lte_pppos_both_sim_network_failover_process();
        hub_lte_pppos_cell_recovery_process();
        hub_lte_pppos_reconnect_process();
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
            (void)hub_lte_pppos_request_async_cleanup("PPP netif create failed");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_create_modem();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP modem create failed");
            (void)hub_lte_pppos_request_async_cleanup("PPP modem create failed");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_enter_data_mode();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP data mode failed");
            (void)hub_lte_pppos_request_async_cleanup("PPP data mode failed");
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_ERROR, ret);
            return ret;
        }
        ret = hub_lte_pppos_start_ppp();
        if (ret != ESP_OK) {
            hub_lte_pppos_set_last_result(ret, "PPP start failed");
            (void)hub_lte_pppos_request_async_cleanup("PPP start failed");
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
        if (s_lte_cell_recovery.running_since_tick == 0) {
            s_lte_cell_recovery.running_since_tick = xTaskGetTickCount();
        } else if ((xTaskGetTickCount() - s_lte_cell_recovery.running_since_tick) >= pdMS_TO_TICKS(HUB_LTE_PPPOS_RECONNECT_STABLE_MS)) {
            s_lte_cell_recovery.reconnect_attempt = 0;
        }
        hub_lte_pppos_set_last_result(ESP_OK, "PPP running from IP event");
        break;

    case HUB_PPP_STATE_STOPPING:
        portENTER_CRITICAL(&s_lte_cleanup_lock);
        s_lte_cell_recovery.cleanup_pending = false;
        s_lte_cell_recovery.cleanup_in_progress = true;
        s_lte_cell_recovery.cleanup_done = false;
        s_lte_cell_recovery.reconnect_scheduled = false;
        s_lte_cell_recovery.reconnect_after_cleanup = false;
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        ret = hub_lte_pppos_destroy_runtime();
        portENTER_CRITICAL(&s_lte_cleanup_lock);
        s_lte_cell_recovery.cleanup_in_progress = false;
        s_lte_cell_recovery.cleanup_done = true;
        portEXIT_CRITICAL(&s_lte_cleanup_lock);
        s_lte_status.start_requested = false;
        s_lte_status.stop_requested = false;
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
        hub_lte_pppos_set_last_result(ESP_OK, "PPPoS lifecycle stopped");
        ESP_LOGI(TAG, "PPPoS stopped");
        hub_lte_pppos_cell_recovery_set_stage(CELL_RECOVERY_WAIT_REGISTRATION, "PPPoS stopped");
        return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);

    case HUB_PPP_STATE_ERROR:
        if (!s_lte_cell_recovery.cleanup_pending) {
            if (s_lte_cell_recovery.cleanup_done) {
                return hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
            }
            if (!s_lte_status.stop_requested) {
                portENTER_CRITICAL(&s_lte_cleanup_lock);
                s_lte_cell_recovery.reconnect_after_cleanup = true;
                portEXIT_CRITICAL(&s_lte_cleanup_lock);
            }
            (void)hub_lte_pppos_cleanup_after_loss(hub_lte_pppos_get_last_reason());
            (void)hub_lte_pppos_set_state(HUB_PPP_STATE_IDLE, ESP_OK);
        }
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
    hub_lte_pppos_set_preflight_reason(preflight, "Preflight check started");

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

    bool hard_reset_in_progress = hub_lte_pppos_hard_reset_in_progress();
    if (!hard_reset_in_progress &&
        hub_lte_pppos_can_use_at_status_uart() &&
        a7608_startup_probe_complete() &&
        (s_lte_cell_recovery.stage != CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) &&
        (s_lte_cell_recovery.stage != CELL_RECOVERY_WAIT_REGISTRATION)) {
        (void)hub_lte_pppos_request_status_refresh("preflight");
    }

    const a7608_status_t *modem_status = a7608_get_status();
    if (modem_status != NULL) {
        preflight->status_age_ms = a7608_status_age_ms();
        preflight->status_fresh = a7608_status_is_fresh(HUB_LTE_PPPOS_MODEM_STATUS_FRESH_MS);
        preflight->modem_status_known = modem_status->status_valid;
        preflight->sim_ready = modem_status->sim_ready;
        preflight->registered_to_network = a7608_status_is_registered();
        preflight->has_signal = modem_status->rssi_valid;
        preflight->attached = modem_status->attached;
        preflight->rssi = modem_status->rssi_dbm;
        preflight->csq = modem_status->csq;
        preflight->creg_stat = modem_status->creg_stat;
        preflight->cereg_stat = modem_status->cereg_stat;
        preflight->cfun = modem_status->cfun;
    }

    if ((config_ret == ESP_OK) && preflight->sim_ready && a7608_startup_probe_complete()) {
        if (hub_lte_pppos_resolve_apn() == ESP_OK) {
            preflight->has_apn = true;
            hub_lte_pppos_copy_string(preflight->apn, sizeof(preflight->apn), s_lte_config.apn);
        } else if (s_lte_apn_auto) {
            preflight->has_apn = false;
            preflight->apn[0] = '\0';
        }
    }

    if (s_lte_cell_recovery.stage == CELL_RECOVERY_HARD_RESET) {
        hub_lte_pppos_set_preflight_reason(preflight, "Cellular recovery in progress");
    } else if (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_HARD_RESET) {
        hub_lte_pppos_set_preflight_reason(preflight, "Waiting after hardware reset");
    } else if (!preflight->pppos_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS disabled by build config");
    } else if (!preflight->test_mode_enabled) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS test mode disabled");
    } else if (HUB_LTE_PPPOS_REAL_RUNTIME == 0) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS real runtime disabled");
    } else if (HUB_LTE_PPPOS_MANUAL_TEST == 0) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS manual test disabled");
    } else if (config_ret != ESP_OK) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS config unavailable");
    } else if (!preflight->config_valid) {
        hub_lte_pppos_set_preflight_reason(preflight, "Invalid PPPoS config");
    } else if (!s_lte_status.initialized) {
        hub_lte_pppos_set_preflight_reason(preflight, "PPPoS framework not initialized");
    } else if (!a7608_startup_probe_complete()) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 startup probe in progress");
    } else if (s_lte_cell_recovery.reconnect_scheduled) {
        hub_lte_pppos_set_preflight_reason(preflight, "Reconnect delay active");
    } else if (s_lte_cell_recovery.stage == CELL_RECOVERY_WAIT_AFTER_RADIO_RESTART) {
        hub_lte_pppos_set_preflight_reason(preflight, "Waiting after radio restart");
    } else if (s_lte_cell_recovery.status_refresh_in_progress) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status refresh in progress");
    } else if (!hub_lte_pppos_can_use_at_status_uart()) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 AT interface unavailable");
    } else if ((s_lte_cell_recovery.stage != CELL_RECOVERY_IDLE) &&
               (s_lte_cell_recovery.stage != CELL_RECOVERY_WAIT_REGISTRATION)) {
        hub_lte_pppos_set_preflight_reason(preflight, "Cellular recovery in progress");
    } else if ((modem_status != NULL) && !modem_status->at_ready &&
               (modem_status->last_refresh_result != ESP_OK) && !preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 AT interface unavailable");
    } else if ((modem_status != NULL) && !modem_status->status_valid &&
               (modem_status->last_refresh_result != ESP_OK) && !preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status refresh incomplete");
    } else if (!preflight->status_fresh && !preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status stale");
    } else if (!preflight->modem_status_known && !preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status unavailable");
    } else if (!preflight->sim_ready) {
        hub_lte_pppos_set_preflight_reason(preflight, "SIM not ready");
    } else if (!preflight->registered_to_network) {
        hub_lte_pppos_set_preflight_reason(preflight, "Modem not registered to network");
    } else if (!preflight->has_apn) {
        hub_lte_pppos_set_preflight_reason(preflight, "APN not provided by SIM");
    } else if (preflight->cfun != 1) {
        hub_lte_pppos_set_preflight_reason(preflight, "Modem radio not ready");
    } else {
        preflight->ready_to_start = true;
        hub_lte_pppos_set_preflight_reason(preflight, "Ready to start PPPoS lifecycle");
    }

    if (!preflight->ready_to_start && (preflight->reason[0] == '\0')) {
        hub_lte_pppos_set_preflight_reason(preflight, "A7608 status unavailable");
    }

    if (strcmp(s_lte_last_preflight_log_reason, preflight->reason) != 0) {
        hub_lte_pppos_copy_string(s_lte_last_preflight_log_reason,
                                  sizeof(s_lte_last_preflight_log_reason),
                                  preflight->reason);
        ESP_LOGI(TAG, "PPPoS preflight: ready=%d sim=%d apn=%s reason=%s",
                 preflight->ready_to_start,
                 preflight->sim_ready,
                 preflight->apn[0] != '\0' ? preflight->apn : "-",
                 preflight->reason);
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
