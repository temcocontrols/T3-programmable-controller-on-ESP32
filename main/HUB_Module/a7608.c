#include "a7608.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "hub_lte_pppos.h"
#include "hub_network_manager.h"

#define A7608_LOG(fmt, ...) do { printf("[A7608] " fmt "\n", ##__VA_ARGS__); fflush(stdout); } while (0)

static a7608_config_t a7608_cfg;
static a7608_status_t a7608_status;
static bool a7608_initialized;
static bool a7608_uart_driver_owned;
static a7608_service_state_t a7608_service_state = A7608_SERVICE_RUNNING;
static SemaphoreHandle_t a7608_uart_lock;
static bool a7608_startup_probe_started_flag = true;
static bool a7608_startup_probe_complete_flag;
static esp_err_t a7608_startup_probe_ret = ESP_ERR_INVALID_STATE;
static bool a7608_startup_probe_state_log_printed;
static bool a7608_startup_probe_started_log_printed;
static bool a7608_cpin_failure_raw_logged;
static bool a7608_cpin_success_raw_logged;

#define A7608_STATUS_INVALID_STAT       (-1)
#define A7608_STATUS_INVALID_CFUN       (-1)
#define A7608_UART_LOCK_WAIT_MS         5000U
#define A7608_STARTUP_INITIAL_PROBES    3U
#define A7608_STARTUP_AT_STABLE_COUNT   3U
#define A7608_STARTUP_AT_TIMEOUT_MS     60000U
#define A7608_STARTUP_SIM_TIMEOUT_MS    60000U
#define A7608_STARTUP_AT_FAILURE_LIMIT  3U
#define A7608_STARTUP_PROBE_INTERVAL_MS 1500U
#define A7608_STARTUP_SIM_INTERVAL_MS   3000U
#define A7608_STARTUP_PWRKEY_PULSE_MS   1000U
#define A7608_STARTUP_BOOT_QUIET_MS     8000U
#define A7608_STARTUP_RESET_PULSE_MS    A7608_HARD_RESET_PULSE_MS
#define A7608_STARTUP_RESET_QUIET_MS    A7608_HARD_RESET_QUIET_MS
#define A7608_STARTUP_RESET_AT_TIMEOUT_MS 15000U
#define A7608_STARTUP_REG_TIMEOUT_MS    45000U
#define A7608_STARTUP_BOTH_SIM_REG_TIMEOUT_MS 300000U
#define A7608_STARTUP_RADIO_OFF_WAIT_MS 3000U
#define A7608_STARTUP_RADIO_ON_WAIT_MS  3000U
#define A7608_STARTUP_SIM_NOT_INSERTED_GRACE_MS 10000U
#define A7608_PPP_ESCAPE_GUARD_MS       1000U
#define A7608_CONTROL_IDLE_SETTLE_MS    100U
#define A7608_SIM_SWITCH_SETTLE_MS      500U

extern int hub_usb_serial_read(uint8_t *buf, uint32_t length, uint32_t timeout_ms);
extern int hub_usb_serial_write(const uint8_t *buf, size_t length, uint32_t timeout_ms);

static bool pin_is_valid(gpio_num_t pin);
static int inactive_level(int active_level);
static bool a7608_ip_is_valid(const char *ip_addr);
static bool a7608_registered_from_stat(int stat);
static bool a7608_take_uart_lock(uint32_t timeout_ms);
static void a7608_give_uart_lock(void);
static void a7608_startup_probe_mark_started(void);
static void a7608_startup_probe_mark_complete(esp_err_t ret);
static void a7608_startup_log_phase(const char *phase, bool cold_boot);
static bool a7608_startup_detect_running_modem(void);
static bool a7608_startup_try_exit_data_mode(bool *uart_silent);
static void a7608_refresh_sim_detect(void);
static esp_err_t a7608_select_sim_slot(a7608_sim_slot_t slot);
static a7608_sim_slot_t a7608_preferred_sim_slot(void);
static bool a7608_startup_wait_at_stable(bool cold_boot,
                                         bool first_response_seen,
                                         const char *action,
                                         uint32_t timeout_ms);
typedef enum {
    A7608_CPIN_READY = 0,
    A7608_CPIN_NOT_READY,
    A7608_CPIN_SIM_BUSY,
    A7608_CPIN_SIM_FAILURE,
    A7608_CPIN_SIM_NOT_INSERTED,
    A7608_CPIN_CME_ERROR,
    A7608_CPIN_QUERY_FAILED,
} a7608_cpin_type_t;
typedef struct {
    a7608_cpin_type_t type;
    int cme_code;
    esp_err_t command_ret;
} a7608_cpin_result_t;
typedef enum {
    A7608_STARTUP_SIM_READY = 0,
    A7608_STARTUP_SIM_TIMEOUT,
    A7608_STARTUP_SIM_AT_LOST,
    A7608_STARTUP_SIM_PERSISTENT_FAILURE,
    A7608_STARTUP_SIM_NOT_INSERTED,
} a7608_startup_sim_result_t;
static const char *a7608_cpin_type_name(a7608_cpin_type_t type);
static a7608_cpin_result_t a7608_query_cpin(bool *sim_ready);
static a7608_startup_sim_result_t a7608_startup_wait_sim_ready(bool cold_boot,
                                                               bool after_radio_restart);
static bool a7608_startup_wait_network_registered(bool cold_boot);
static a7608_startup_sim_result_t a7608_startup_try_sim_with_fallback(bool cold_boot);
static bool a7608_startup_finish_ready(bool cold_boot,
                                       const char *action,
                                       esp_err_t *status_ret);
static bool a7608_startup_confirm_ready(bool cold_boot,
                                        bool first_response_seen,
                                        const char *action,
                                        bool *at_stable,
                                        a7608_startup_sim_result_t *sim_result,
                                        esp_err_t *status_ret,
                                        uint32_t at_timeout_ms);
static bool a7608_startup_radio_restart(void);
static bool a7608_debug_run_status_snapshot(void);
static esp_err_t a7608_debug_print_parsed_status(void);
static void a7608_sync_network_status(void);
static void a7608_parse_snapshot_response(const char *cmd, const char *response);
static esp_err_t a7608_reinstall_uart_driver(void);
static esp_err_t a7608_pause_service(void);
static esp_err_t a7608_resume_service(void);
static void parse_gnss_info(const char *response);
static void parse_gps_info(const char *response);
static void parse_cgnsinf(const char *response);
static esp_err_t a7608_read_gnss_info(char *response, size_t response_len, const char **command_used);

static void a7608_debug_write(const char *text)
{
    if (text == NULL) {
        return;
    }
    hub_usb_serial_write((const uint8_t *)text, strlen(text), 50);
}

static void a7608_debug_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len <= 0) {
        return;
    }
    if (len >= (int)sizeof(buf)) {
        len = sizeof(buf) - 1;
    }
    hub_usb_serial_write((const uint8_t *)buf, (size_t)len, 50);
}

static void a7608_debug_write_modem_bytes(const uint8_t *buf, int len)
{
    char out[384];
    size_t out_len = 0;

    for (int i = 0; i < len; i++) {
        uint8_t ch = buf[i];
        if ((ch == '\r') || (ch == '\n') || (ch == '\t') || ((ch >= 0x20) && (ch <= 0x7e))) {
            if (out_len >= sizeof(out) - 1) {
                hub_usb_serial_write((const uint8_t *)out, out_len, 50);
                out_len = 0;
            }
            out[out_len++] = (char)ch;
        } else {
            if (out_len >= sizeof(out) - 5) {
                hub_usb_serial_write((const uint8_t *)out, out_len, 50);
                out_len = 0;
            }
            out_len += snprintf(&out[out_len], sizeof(out) - out_len, "\\x%02X", ch);
        }
    }

    if (out_len > 0) {
        hub_usb_serial_write((const uint8_t *)out, out_len, 50);
    }
}

static void a7608_debug_write_response_block(const char *title, const char *response)
{
    a7608_debug_printf("[%s RAW]\r\n", title);
    if ((response != NULL) && (response[0] != '\0')) {
        a7608_debug_write(response);
        size_t len = strlen(response);
        if ((len > 0) && (response[len - 1] != '\n')) {
            a7608_debug_write("\r\n");
        }
    } else {
        a7608_debug_write("-\r\n");
    }
    a7608_debug_printf("[%s RAW END]\r\n", title);
}

static bool a7608_take_uart_lock(uint32_t timeout_ms)
{
    if (a7608_uart_lock == NULL) {
        return true;
    }
    return xSemaphoreTake(a7608_uart_lock, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void a7608_give_uart_lock(void)
{
    if (a7608_uart_lock != NULL) {
        xSemaphoreGive(a7608_uart_lock);
    }
}

static void a7608_startup_probe_mark_started(void)
{
    if (!a7608_startup_probe_started_log_printed) {
        ESP_LOGI("A7608", "A7608 startup probe: started");
        a7608_startup_probe_started_log_printed = true;
    }
    a7608_startup_probe_started_flag = true;
    a7608_startup_probe_complete_flag = false;
    a7608_startup_probe_ret = ESP_ERR_INVALID_STATE;
}

static void a7608_startup_probe_mark_complete(esp_err_t ret)
{
    if (!a7608_startup_probe_complete_flag) {
        ESP_LOGI("A7608", "A7608 startup probe: complete ret=%s", esp_err_to_name(ret));
    }
    a7608_startup_probe_started_flag = true;
    a7608_startup_probe_complete_flag = true;
    a7608_startup_probe_ret = ret;
}

static void a7608_startup_log_phase(const char *phase, bool cold_boot)
{
    ESP_LOGI("A7608", "A7608 startup phase: %s cold_boot=%d", phase, cold_boot);
}

static bool a7608_startup_detect_running_modem(void)
{
    for (uint32_t attempt = 1; attempt <= A7608_STARTUP_INITIAL_PROBES; attempt++) {
        esp_err_t ret = a7608_probe();
        ESP_LOGI("A7608",
                 "A7608 startup AT probe: attempt=%lu/%u ret=%s",
                 (unsigned long)attempt,
                 A7608_STARTUP_INITIAL_PROBES,
                 esp_err_to_name(ret));
        if (ret == ESP_OK) {
            ESP_LOGI("A7608", "A7608 startup AT first response after action=INITIAL_PROBE");
            return true;
        }
        if (attempt < A7608_STARTUP_INITIAL_PROBES) {
            vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_PROBE_INTERVAL_MS));
        }
    }
    return false;
}

static bool a7608_startup_try_exit_data_mode(bool *uart_silent)
{
    uint8_t buf[64];
    char response[96];
    size_t used = 0;

    if (uart_silent != NULL) {
        *uart_silent = true;
    }

    ESP_LOGW("A7608", "A7608 recovery stage=PPP_ESCAPE");
    if (!a7608_take_uart_lock(A7608_UART_LOCK_WAIT_MS)) {
        ESP_LOGW("A7608", "A7608 PPP escape skipped: UART busy");
        return false;
    }

    (void)uart_flush_input(a7608_cfg.uart_num);
    vTaskDelay(pdMS_TO_TICKS(A7608_PPP_ESCAPE_GUARD_MS));
    int wrote = uart_write_bytes(a7608_cfg.uart_num, "+++", 3);
    if (wrote < 3) {
        ESP_LOGW("A7608", "A7608 PPP escape write failed");
        a7608_give_uart_lock();
        return false;
    }

    response[0] = '\0';
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(A7608_PPP_ESCAPE_GUARD_MS + 1500U)) {
        int len = uart_read_bytes(a7608_cfg.uart_num, buf, sizeof(buf) - 1, pdMS_TO_TICKS(50));
        if (len <= 0) {
            continue;
        }
        if (uart_silent != NULL) {
            *uart_silent = false;
        }
        size_t copy_len = (size_t)len;
        if (copy_len > (sizeof(response) - used - 1)) {
            copy_len = sizeof(response) - used - 1;
        }
        if (copy_len > 0) {
            memcpy(response + used, buf, copy_len);
            used += copy_len;
            response[used] = '\0';
        }
        if (strstr(response, "OK") != NULL) {
            break;
        }
    }
    a7608_give_uart_lock();

    ESP_LOGI("A7608",
             "A7608 PPP escape response=\"%s\" uart_silent=%d",
             response[0] != '\0' ? response : "-",
             (uart_silent != NULL) && *uart_silent);

    vTaskDelay(pdMS_TO_TICKS(200));
    esp_err_t ret = a7608_probe();
    ESP_LOGI("A7608", "A7608 startup AT probe after PPP escape: ret=%s", esp_err_to_name(ret));
    if (ret == ESP_OK) {
        if (uart_silent != NULL) {
            *uart_silent = false;
        }
        return true;
    }
    return false;
}

static bool a7608_startup_wait_at_stable(bool cold_boot,
                                         bool first_response_seen,
                                         const char *action,
                                         uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    uint32_t attempt = 0;
    uint32_t consecutive_ok = 0;
    bool unstable_logged = false;

    a7608_startup_log_phase("WAIT_AT_STABLE", cold_boot);
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        esp_err_t ret = a7608_probe();
        attempt++;
        ESP_LOGI("A7608",
                 "A7608 startup AT probe: attempt=%lu ret=%s",
                 (unsigned long)attempt,
                 esp_err_to_name(ret));
        if (ret == ESP_OK) {
            if (!first_response_seen) {
                ESP_LOGI("A7608",
                         "A7608 startup AT first response after action=%s",
                         action != NULL ? action : "NONE");
                first_response_seen = true;
            }
            consecutive_ok++;
            ESP_LOGI("A7608",
                     "A7608 startup AT stable confirmation %lu/%u",
                     (unsigned long)consecutive_ok,
                     A7608_STARTUP_AT_STABLE_COUNT);
            if (consecutive_ok >= A7608_STARTUP_AT_STABLE_COUNT) {
                ESP_LOGI("A7608", "A7608 startup AT stable");
                (void)a7608_set_dtr(false);
                ESP_LOGI("A7608", "A7608 startup DTR wake: gpio=%d level=%d",
                         a7608_cfg.dtr_pin, a7608_get_dtr_level());
                char csclk_rsp[64];
                esp_err_t csclk_ret = a7608_send_command("AT+CSCLK=0", "OK", 2000, csclk_rsp, sizeof(csclk_rsp));
                ESP_LOGI("A7608", "A7608 AT+CSCLK=0 ret=%s", esp_err_to_name(csclk_ret));
                return true;
            }
        } else {
            if (first_response_seen && !unstable_logged) {
                ESP_LOGW("A7608", "A7608 startup AT unstable after first response");
                unstable_logged = true;
            }
            consecutive_ok = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_PROBE_INTERVAL_MS));
    }

    ESP_LOGE("A7608",
             "A7608 startup timeout: phase=WAIT_AT_STABLE cold_boot=%d action=%s first_response=%d",
             cold_boot,
             action != NULL ? action : "NONE",
             first_response_seen);
    return false;
}

static a7608_startup_sim_result_t a7608_startup_wait_sim_ready(bool cold_boot,
                                                               bool after_radio_restart)
{
    TickType_t start = xTaskGetTickCount();
    uint32_t attempt = 0;
    uint32_t consecutive_at_failures = 0;
    uint32_t sim_failure_count = 0;
    bool only_sim_failure = true;

    a7608_startup_log_phase("WAIT_SIM_READY", cold_boot);
    if (after_radio_restart) {
        ESP_LOGW("A7608",
                 "A7608 CPIN after radio restart: begin timeout_ms=%u",
                 A7608_STARTUP_SIM_TIMEOUT_MS);
    }
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(A7608_STARTUP_SIM_TIMEOUT_MS)) {
        bool sim_ready = false;
        a7608_cpin_result_t cpin_result = a7608_query_cpin(&sim_ready);
        esp_err_t at_ret = ESP_OK;
        bool at_alive = cpin_result.type != A7608_CPIN_QUERY_FAILED;
        attempt++;
        if (!at_alive) {
            at_ret = a7608_probe();
            at_alive = at_ret == ESP_OK;
        }

        uint32_t elapsed_ms = (uint32_t)((xTaskGetTickCount() - start) * portTICK_PERIOD_MS);
        ESP_LOGI("A7608",
                 "A7608 CPIN result: type=%s cme=%d at_alive=%d attempt=%lu elapsed_ms=%lu",
                 a7608_cpin_type_name(cpin_result.type),
                 cpin_result.cme_code,
                 at_alive,
                 (unsigned long)attempt,
                 (unsigned long)elapsed_ms);

        if (cpin_result.type == A7608_CPIN_READY) {
            ESP_LOGI("A7608", "A7608 startup SIM READY");
            return A7608_STARTUP_SIM_READY;
        }

        if (cpin_result.type == A7608_CPIN_SIM_NOT_INSERTED) {
            if (elapsed_ms < A7608_STARTUP_SIM_NOT_INSERTED_GRACE_MS) {
                ESP_LOGW("A7608",
                         "A7608 SIM not inserted (CME=10); retrying until grace_ms=%u",
                         A7608_STARTUP_SIM_NOT_INSERTED_GRACE_MS);
                vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_SIM_INTERVAL_MS));
                continue;
            }
            ESP_LOGE("A7608", "A7608 SIM not inserted reported: CME=10");
            return A7608_STARTUP_SIM_NOT_INSERTED;
        }

        if (cpin_result.type == A7608_CPIN_SIM_FAILURE) {
            sim_failure_count++;
            ESP_LOGE("A7608", "A7608 SIM failure reported: CME=13");
            ESP_LOGW("A7608",
                     "A7608 SIM failure count=%lu elapsed_ms=%lu",
                     (unsigned long)sim_failure_count,
                     (unsigned long)elapsed_ms);
        } else {
            only_sim_failure = false;
        }

        if (cpin_result.type == A7608_CPIN_SIM_BUSY) {
            ESP_LOGW("A7608", "A7608 SIM busy reported: CME=14; retrying CPIN");
        } else if (cpin_result.type == A7608_CPIN_NOT_READY) {
            ESP_LOGW("A7608", "A7608 SIM reports a valid non-ready CPIN state; retrying CPIN");
        } else if (cpin_result.type == A7608_CPIN_QUERY_FAILED) {
            ESP_LOGW("A7608",
                     "A7608 startup SIM query failed: attempt=%lu cpin_ret=%s at_probe_ret=%s",
                     (unsigned long)attempt,
                     esp_err_to_name(cpin_result.command_ret),
                     esp_err_to_name(at_ret));
            if (at_alive) {
                consecutive_at_failures = 0;
                ESP_LOGW("A7608", "A7608 startup SIM status unknown, retrying CPIN");
            } else {
                consecutive_at_failures++;
                ESP_LOGW("A7608",
                         "A7608 startup AT failure while waiting SIM: %lu/%u",
                         (unsigned long)consecutive_at_failures,
                         A7608_STARTUP_AT_FAILURE_LIMIT);
                if (consecutive_at_failures >= A7608_STARTUP_AT_FAILURE_LIMIT) {
                    ESP_LOGE("A7608", "A7608 startup AT interface lost while waiting SIM");
                    return A7608_STARTUP_SIM_AT_LOST;
                }
            }
        } else {
            consecutive_at_failures = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_SIM_INTERVAL_MS));
    }

    uint32_t elapsed_ms = (uint32_t)((xTaskGetTickCount() - start) * portTICK_PERIOD_MS);
    if (only_sim_failure && (sim_failure_count > 0)) {
        ESP_LOGE("A7608",
                 "A7608 SIM failure persistent: count=%lu elapsed_ms=%lu",
                 (unsigned long)sim_failure_count,
                 (unsigned long)elapsed_ms);
        return A7608_STARTUP_SIM_PERSISTENT_FAILURE;
    }

    ESP_LOGE("A7608", "A7608 startup timeout: phase=WAIT_SIM_READY cold_boot=%d", cold_boot);
    return A7608_STARTUP_SIM_TIMEOUT;
}

static bool a7608_startup_wait_network_registered(bool cold_boot)
{
    TickType_t start = xTaskGetTickCount();
    uint32_t attempt = 0;
    a7608_refresh_sim_detect();
    uint32_t timeout_ms = (a7608_status.sim1_present && a7608_status.sim2_present) ?
                          A7608_STARTUP_BOTH_SIM_REG_TIMEOUT_MS :
                          A7608_STARTUP_REG_TIMEOUT_MS;

    a7608_startup_log_phase("WAIT_REGISTRATION", cold_boot);
    ESP_LOGI("A7608",
             "A7608 registration wait: timeout_ms=%lu both_sims=%d slot=%s",
             (unsigned long)timeout_ms,
             a7608_status.sim1_present && a7608_status.sim2_present,
             a7608_sim_slot_name(a7608_status.active_sim_slot));
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        attempt++;
        (void)a7608_refresh_status_ex(false);
        bool registered = a7608_status_is_registered();
        ESP_LOGI("A7608",
                 "A7608 registration wait: attempt=%lu slot=%s creg=%d cereg=%d registered=%d",
                 (unsigned long)attempt,
                 a7608_sim_slot_name(a7608_status.active_sim_slot),
                 a7608_status.creg_stat,
                 a7608_status.cereg_stat,
                 registered);
        if (registered) {
            ESP_LOGI("A7608",
                     "A7608 startup registered on %s",
                     a7608_sim_slot_name(a7608_status.active_sim_slot));
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_SIM_INTERVAL_MS));
    }
    ESP_LOGW("A7608",
             "A7608 startup registration timeout on %s",
             a7608_sim_slot_name(a7608_status.active_sim_slot));
    return false;
}

static a7608_startup_sim_result_t a7608_startup_try_sim_with_fallback(bool cold_boot)
{
    a7608_sim_slot_t first = a7608_preferred_sim_slot();
    (void)a7608_select_sim_slot(first);
    ESP_LOGI("A7608",
             "A7608 SIM startup: first_slot=%s priority=SIM1 sim1_present=%d sim2_present=%d",
             a7608_sim_slot_name(first),
             a7608_status.sim1_present,
             a7608_status.sim2_present);

    a7608_startup_sim_result_t result = a7608_startup_wait_sim_ready(cold_boot, false);
    bool registered = false;
    if (result == A7608_STARTUP_SIM_READY) {
        registered = a7608_startup_wait_network_registered(cold_boot);
        if (registered || (first != A7608_SIM_SLOT_1)) {
            return A7608_STARTUP_SIM_READY;
        }
        ESP_LOGW("A7608", "A7608 SIM1 CPIN ready but not registered; falling back to SIM2");
    }

    if (result == A7608_STARTUP_SIM_AT_LOST) {
        return result;
    }

    a7608_sim_slot_t next = a7608_alternate_sim_slot();
    a7608_refresh_sim_detect();
    ESP_LOGW("A7608",
             "A7608 SIM fallback: %s -> %s reason=%d sim1_present=%d sim2_present=%d",
             a7608_sim_slot_name(first),
             a7608_sim_slot_name(next),
             (int)result,
             a7608_status.sim1_present,
             a7608_status.sim2_present);
    if (a7608_select_sim_slot(next) != ESP_OK) {
        return result;
    }
    (void)a7608_startup_radio_restart();
    result = a7608_startup_wait_sim_ready(cold_boot, true);
    if (result == A7608_STARTUP_SIM_READY) {
        (void)a7608_startup_wait_network_registered(cold_boot);
        ESP_LOGI("A7608",
                 "A7608 SIM fallback complete: active=%s cpin_ready=1",
                 a7608_sim_slot_name(a7608_status.active_sim_slot));
        return A7608_STARTUP_SIM_READY;
    }
    ESP_LOGE("A7608",
             "A7608 SIM fallback to %s failed; restoring %s",
             a7608_sim_slot_name(next),
             a7608_sim_slot_name(first));
    (void)a7608_select_sim_slot(first);
    return result;
}

static bool a7608_startup_finish_ready(bool cold_boot,
                                       const char *action,
                                       esp_err_t *status_ret)
{
    a7608_startup_log_phase("CHECK_RADIO_REGISTRATION", cold_boot);
    for (uint32_t attempt = 1; attempt <= 3; attempt++) {
        if (a7608_debug_run_status_snapshot()) {
            if (status_ret != NULL) {
                *status_ret = a7608_debug_print_parsed_status();
            }
            return true;
        }
        ESP_LOGW("A7608",
                 "A7608 startup AT snapshot retry %lu/3 after action=%s",
                 (unsigned long)attempt,
                 action != NULL ? action : "NONE");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGW("A7608",
             "A7608 startup AT unstable during status snapshot after action=%s",
             action != NULL ? action : "NONE");
    return false;
}

static bool a7608_startup_confirm_ready(bool cold_boot,
                                        bool first_response_seen,
                                        const char *action,
                                        bool *at_stable,
                                        a7608_startup_sim_result_t *sim_result,
                                        esp_err_t *status_ret,
                                        uint32_t at_timeout_ms)
{
    bool stable = a7608_startup_wait_at_stable(cold_boot,
                                               first_response_seen,
                                               action,
                                               at_timeout_ms);
    if (at_stable != NULL) {
        *at_stable = stable;
    }
    if (!stable) {
        return false;
    }

    a7608_startup_sim_result_t result = a7608_startup_try_sim_with_fallback(cold_boot);
    if (sim_result != NULL) {
        *sim_result = result;
    }
    if (result != A7608_STARTUP_SIM_READY) {
        if ((result == A7608_STARTUP_SIM_AT_LOST) && (at_stable != NULL)) {
            *at_stable = false;
        }
        return false;
    }

    return a7608_startup_finish_ready(cold_boot, action, status_ret);
}

static bool a7608_startup_radio_restart(void)
{
    char response[128];

    ESP_LOGW("A7608", "A7608 SIM recovery action=RADIO_RESTART");
    esp_err_t off_ret = a7608_send_command("AT+CFUN=0", "OK", 5000, response, sizeof(response));
    ESP_LOGW("A7608", "A7608 AT+CFUN=0 ret=%s", esp_err_to_name(off_ret));
    vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_RADIO_OFF_WAIT_MS));

    esp_err_t on_ret = a7608_send_command("AT+CFUN=1", "OK", 5000, response, sizeof(response));
    ESP_LOGW("A7608", "A7608 AT+CFUN=1 ret=%s", esp_err_to_name(on_ret));
    vTaskDelay(pdMS_TO_TICKS(A7608_STARTUP_RADIO_ON_WAIT_MS));

    return on_ret == ESP_OK;
}

static bool a7608_debug_send_snapshot_command(const char *cmd, uint32_t timeout_ms, bool uart_already_locked)
{
    char response[768];
    size_t used = 0;
    bool received = false;
    bool command_ok = false;
    uint8_t buf[128];

    if (!uart_already_locked && !a7608_take_uart_lock(A7608_UART_LOCK_WAIT_MS)) {
        a7608_debug_printf("UART busy for %s\r\n", cmd);
        return false;
    }

    a7608_debug_printf("\r\n>>> %s\r\n", cmd);
    (void)uart_flush_input(a7608_cfg.uart_num);
    int wrote = uart_write_bytes(a7608_cfg.uart_num, cmd, strlen(cmd));
    int wrote_crlf = uart_write_bytes(a7608_cfg.uart_num, "\r\n", 2);
    if ((wrote < 0) || (wrote_crlf < 0)) {
        a7608_debug_printf("UART write failed for %s\r\n", cmd);
        if (!uart_already_locked) {
            a7608_give_uart_lock();
        }
        return false;
    }

    response[0] = '\0';
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    while ((xTaskGetTickCount() - start) < timeout_ticks) {
        int len = uart_read_bytes(a7608_cfg.uart_num, buf, sizeof(buf), pdMS_TO_TICKS(50));
        if (len <= 0) {
            continue;
        }

        received = true;
        a7608_debug_write_modem_bytes(buf, len);

        for (int i = 0; (i < len) && (used < sizeof(response) - 1); i++) {
            if ((buf[i] == '\r') || (buf[i] == '\n') || (buf[i] == '\t') || ((buf[i] >= 0x20) && (buf[i] <= 0x7e))) {
                response[used++] = (char)buf[i];
            }
        }
        response[used] = '\0';

        if (strstr(response, "OK") != NULL) {
            command_ok = true;
            break;
        }
        if (strstr(response, "ERROR") != NULL) {
            break;
        }
    }

    if (!received) {
        a7608_debug_printf("No response for %s\r\n", cmd);
    }
    if (command_ok) {
        a7608_parse_snapshot_response(cmd, response);
    }
    if (!uart_already_locked) {
        a7608_give_uart_lock();
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    return command_ok;
}

static bool a7608_debug_run_status_snapshot(void)
{
    static const struct {
        const char *cmd;
        uint32_t timeout_ms;
    } commands[] = {
        {"AT", 3000},
        {"AT+CPIN?", 3000},
        {"AT+CSQ", 3000},
        {"AT+CFUN?", 3000},
        {"AT+CEREG?", 5000},
        {"AT+CREG?", 5000},
    };

    bool prev_at_ready = a7608_status.at_ready;
    bool prev_sim_ready = a7608_status.sim_ready;
    char prev_apn[A7608_APN_LEN];
    snprintf(prev_apn, sizeof(prev_apn), "%s", a7608_status.apn);

    if (!a7608_take_uart_lock(A7608_UART_LOCK_WAIT_MS)) {
        ESP_LOGW("A7608", "A7608 status snapshot skipped: UART busy");
        return false;
    }

    esp_err_t first_error = ESP_OK;
    a7608_status.status_valid = false;
    a7608_status.at_ready = false;
    a7608_status.sim_ready = false;
    a7608_status.registered_home = false;
    a7608_status.registered_roaming = false;
    a7608_status.attached = false;
    a7608_status.connected = false;
    a7608_status.csq = 99;
    a7608_status.rssi_valid = false;
    a7608_status.rssi_dbm = 0;
    a7608_status.creg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cereg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cfun = A7608_STATUS_INVALID_CFUN;
    a7608_status.status_age_ms = UINT32_MAX;
    a7608_status.apn[0] = '\0';
    a7608_status.ip_addr[0] = '\0';

    a7608_debug_write("\r\n[A7608 STATUS SNAPSHOT]\r\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        bool command_ok = a7608_debug_send_snapshot_command(commands[i].cmd, commands[i].timeout_ms, true);
        bool critical_command = (strcmp(commands[i].cmd, "AT") == 0) ||
                                (strcmp(commands[i].cmd, "AT+CPIN?") == 0) ||
                                (strcmp(commands[i].cmd, "AT+CFUN?") == 0);
        if (!command_ok && critical_command) {
            first_error = ESP_FAIL;
        }
    }
    a7608_give_uart_lock();

    bool snapshot_ok = (first_error == ESP_OK) && a7608_status.at_ready && a7608_status.sim_ready;
    if (!snapshot_ok && prev_at_ready && prev_sim_ready && a7608_status.at_ready) {
        ESP_LOGW("A7608", "A7608 status snapshot failed; keeping prior SIM ready state");
        a7608_status.sim_ready = true;
        if ((a7608_status.apn[0] == '\0') && (prev_apn[0] != '\0')) {
            snprintf(a7608_status.apn, sizeof(a7608_status.apn), "%s", prev_apn);
        }
        snapshot_ok = true;
        first_error = ESP_OK;
    }

    a7608_status.status_valid = snapshot_ok;
    a7608_status.last_refresh_tick = xTaskGetTickCount();
    a7608_status.status_age_ms = a7608_status.status_valid ? 0 : UINT32_MAX;
    a7608_status.last_refresh_result = first_error;
    a7608_sync_network_status();
    a7608_debug_write("\r\n[A7608 STATUS SNAPSHOT END]\r\n");
    return snapshot_ok;
}

static bool pin_is_valid(gpio_num_t pin)
{
    return pin >= 0;
}

static int inactive_level(int active_level)
{
    return active_level ? 0 : 1;
}

static void set_last_error(const char *text)
{
    if (text == NULL) {
        a7608_status.last_error[0] = '\0';
        return;
    }
    snprintf(a7608_status.last_error, sizeof(a7608_status.last_error), "%s", text);
}

static esp_err_t configure_output_pin(gpio_num_t pin, int inactive)
{
    if (!pin_is_valid(pin)) {
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = (inactive != 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (inactive == 0) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        ret = gpio_set_level(pin, inactive);
    }
    return ret;
}

static esp_err_t configure_input_pin(gpio_num_t pin)
{
    if (!pin_is_valid(pin)) {
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&io_conf);
}

const char *a7608_sim_slot_name(a7608_sim_slot_t slot)
{
    switch (slot) {
    case A7608_SIM_SLOT_1:
        return "SIM1";
    case A7608_SIM_SLOT_2:
        return "SIM2";
    default:
        return "NONE";
    }
}

bool a7608_sim_slot_detected(a7608_sim_slot_t slot)
{
    gpio_num_t pin = GPIO_NUM_NC;
    if (slot == A7608_SIM_SLOT_1) {
        pin = a7608_cfg.sim1_det_pin;
    } else if (slot == A7608_SIM_SLOT_2) {
        pin = a7608_cfg.sim2_det_pin;
    }
    if (!pin_is_valid(pin)) {
        return false;
    }
    return gpio_get_level(pin) == A7608_SIM_DET_INSERTED_LEVEL;
}

a7608_sim_slot_t a7608_get_active_sim_slot(void)
{
    return a7608_status.active_sim_slot;
}

a7608_sim_slot_t a7608_alternate_sim_slot(void)
{
    return (a7608_status.active_sim_slot == A7608_SIM_SLOT_2) ?
           A7608_SIM_SLOT_1 : A7608_SIM_SLOT_2;
}

bool a7608_other_sim_slot_detected(void)
{
    a7608_refresh_sim_detect();
    return a7608_sim_slot_detected(a7608_alternate_sim_slot());
}

esp_err_t a7608_switch_sim_slot(a7608_sim_slot_t slot)
{
    a7608_sim_slot_t current = a7608_status.active_sim_slot;
    if (slot == current) {
        return ESP_OK;
    }
    ESP_LOGW("A7608",
             "A7608 SIM switch: %s -> %s",
             a7608_sim_slot_name(current),
             a7608_sim_slot_name(slot));
    return a7608_select_sim_slot(slot);
}

static void a7608_refresh_sim_detect(void)
{
    bool sim1 = a7608_sim_slot_detected(A7608_SIM_SLOT_1);
    bool sim2 = a7608_sim_slot_detected(A7608_SIM_SLOT_2);
    int gpio1 = pin_is_valid(a7608_cfg.sim1_det_pin) ? gpio_get_level(a7608_cfg.sim1_det_pin) : -1;
    int gpio2 = pin_is_valid(a7608_cfg.sim2_det_pin) ? gpio_get_level(a7608_cfg.sim2_det_pin) : -1;

    if ((sim1 != a7608_status.sim1_present) || (sim2 != a7608_status.sim2_present)) {
        ESP_LOGI("A7608",
                 "A7608 SIM detect: sim1=%d sim2=%d gpio1=%d gpio2=%d active=%s",
                 sim1,
                 sim2,
                 gpio1,
                 gpio2,
                 a7608_sim_slot_name(a7608_status.active_sim_slot));
    }
    a7608_status.sim1_present = sim1;
    a7608_status.sim2_present = sim2;
}

static esp_err_t a7608_select_sim_slot(a7608_sim_slot_t slot)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.sim_sel_pin)) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((slot != A7608_SIM_SLOT_1) && (slot != A7608_SIM_SLOT_2)) {
        return ESP_ERR_INVALID_ARG;
    }

    int level = (slot == A7608_SIM_SLOT_2) ? A7608_SIM2_SELECT_LEVEL : A7608_SIM1_SELECT_LEVEL;
    ESP_LOGI("A7608",
             "A7608 SIM select: slot=%s gpio=%d level=%d",
             a7608_sim_slot_name(slot),
             a7608_cfg.sim_sel_pin,
             level);
    esp_err_t ret = gpio_set_level(a7608_cfg.sim_sel_pin, level);
    if (ret != ESP_OK) {
        return ret;
    }
    a7608_status.active_sim_slot = slot;
    a7608_status.sim_ready = false;
    vTaskDelay(pdMS_TO_TICKS(A7608_SIM_SWITCH_SETTLE_MS));
    a7608_refresh_sim_detect();
    return ESP_OK;
}

static a7608_sim_slot_t a7608_preferred_sim_slot(void)
{
    a7608_refresh_sim_detect();
    if (a7608_status.sim1_present) {
        return A7608_SIM_SLOT_1;
    }
    if (a7608_status.sim2_present) {
        return A7608_SIM_SLOT_2;
    }
    return A7608_SIM_SLOT_1;
}

static bool response_has_registered(const char *response)
{
    bool is_cereg = strstr(response, "+CEREG:") != NULL;
    const char *line = is_cereg ? strstr(response, "+CEREG:") : strstr(response, "+CREG:");
    if (line == NULL) {
        return false;
    }

    int n = 0;
    int stat = A7608_STATUS_INVALID_STAT;
    if (sscanf(line, "%*[^:]: %d,%d", &n, &stat) != 2) {
        if (sscanf(line, "%*[^:]: %d", &stat) != 1) {
            return false;
        }
    }

    if (is_cereg) {
        a7608_status.cereg_stat = stat;
    } else {
        a7608_status.creg_stat = stat;
    }
    a7608_status.registered_home = a7608_registered_from_stat(a7608_status.creg_stat) ||
                                   a7608_registered_from_stat(a7608_status.cereg_stat);
    a7608_status.registered_roaming = (a7608_status.creg_stat == 5) ||
                                      (a7608_status.cereg_stat == 5);
    return a7608_registered_from_stat(stat);
}

static void parse_csq(const char *response)
{
    const char *line = strstr(response, "+CSQ:");
    int csq = 99;
    int ber = 99;
    if ((line != NULL) && (sscanf(line, "%*[^:]: %d,%d", &csq, &ber) == 2)) {
        a7608_status.csq = csq;
        a7608_status.rssi_valid = (csq >= 0) && (csq <= 31);
        a7608_status.rssi_dbm = a7608_status.rssi_valid ? (-113 + (2 * csq)) : 0;
    }
}

static void parse_cfun(const char *response)
{
    const char *line = strstr(response, "+CFUN:");
    int cfun = A7608_STATUS_INVALID_CFUN;
    if ((line != NULL) && (sscanf(line, "%*[^:]: %d", &cfun) == 1)) {
        a7608_status.cfun = cfun;
    }
}

static bool a7608_registered_from_stat(int stat)
{
    return (stat == 1) || (stat == 5);
}

static void parse_operator(const char *response)
{
    const char *first_quote = strchr(response, '"');
    if (first_quote == NULL) {
        return;
    }
    const char *second_quote = strchr(first_quote + 1, '"');
    if (second_quote == NULL) {
        return;
    }

    size_t len = (size_t)(second_quote - first_quote - 1);
    if (len >= sizeof(a7608_status.operator_name)) {
        len = sizeof(a7608_status.operator_name) - 1;
    }
    memcpy(a7608_status.operator_name, first_quote + 1, len);
    a7608_status.operator_name[len] = '\0';
}

static bool a7608_extract_quoted_field(const char **cursor, char *out, size_t out_len)
{
    const char *p = *cursor;
    if ((p == NULL) || (out == NULL) || (out_len == 0)) {
        return false;
    }

    while ((*p == ' ') || (*p == '\t')) {
        p++;
    }
    if (*p != '"') {
        return false;
    }
    p++;

    const char *end = strchr(p, '"');
    if (end == NULL) {
        return false;
    }

    size_t len = (size_t)(end - p);
    if (len >= out_len) {
        len = out_len - 1;
    }
    memcpy(out, p, len);
    out[len] = '\0';
    *cursor = end + 1;
    return true;
}

static void parse_cgdcont(const char *response)
{
    const char *line = response;
    char cid1_apn[A7608_APN_LEN] = {0};
    char first_apn[A7608_APN_LEN] = {0};

    while ((line = strstr(line, "+CGDCONT:")) != NULL) {
        const char *p = line + strlen("+CGDCONT:");
        int cid = 0;
        char pdp_type[16] = {0};
        char apn[A7608_APN_LEN] = {0};

        while ((*p == ' ') || (*p == '\t')) {
            p++;
        }
        if (sscanf(p, "%d", &cid) != 1) {
            line++;
            continue;
        }
        p = strchr(p, ',');
        if (p == NULL) {
            line++;
            continue;
        }
        p++;
        if (!a7608_extract_quoted_field(&p, pdp_type, sizeof(pdp_type))) {
            line++;
            continue;
        }
        if (*p == ',') {
            p++;
        }
        if (!a7608_extract_quoted_field(&p, apn, sizeof(apn))) {
            line++;
            continue;
        }

        if (apn[0] != '\0') {
            if (first_apn[0] == '\0') {
                snprintf(first_apn, sizeof(first_apn), "%s", apn);
            }
            if (cid == 1) {
                snprintf(cid1_apn, sizeof(cid1_apn), "%s", apn);
                break;
            }
        }
        line++;
    }

    if (cid1_apn[0] != '\0') {
        snprintf(a7608_status.apn, sizeof(a7608_status.apn), "%s", cid1_apn);
    } else if (first_apn[0] != '\0') {
        snprintf(a7608_status.apn, sizeof(a7608_status.apn), "%s", first_apn);
    }
}

static void parse_ip_addr(const char *response)
{
    a7608_status.ip_addr[0] = '\0';

    const char *cgpaddr = strstr(response, "+CGPADDR:");
    if (cgpaddr != NULL) {
        const char *addr = strchr(cgpaddr, ',');
        if (addr == NULL) {
            return;
        }
        addr++;
        while ((*addr == ' ') || (*addr == '\t') || (*addr == '"')) {
            addr++;
        }

        size_t len = strcspn(addr, ",\r\n\"");
        if (len >= sizeof(a7608_status.ip_addr)) {
            len = sizeof(a7608_status.ip_addr) - 1;
        }
        if (len > 0) {
            memcpy(a7608_status.ip_addr, addr, len);
            a7608_status.ip_addr[len] = '\0';
        }
        return;
    }

    const char *quote = strchr(response, '"');
    if (quote != NULL) {
        const char *end_quote = strchr(quote + 1, '"');
        if (end_quote != NULL) {
            size_t len = (size_t)(end_quote - quote - 1);
            if (len >= sizeof(a7608_status.ip_addr)) {
                len = sizeof(a7608_status.ip_addr) - 1;
            }
            memcpy(a7608_status.ip_addr, quote + 1, len);
            a7608_status.ip_addr[len] = '\0';
            return;
        }
    }

    const char *colon = strchr(response, ':');
    if (colon == NULL) {
        return;
    }
    while ((*colon == ':') || (*colon == ' ') || (*colon == '\t') || (*colon == ',')) {
        colon++;
    }
    snprintf(a7608_status.ip_addr, sizeof(a7608_status.ip_addr), "%s", colon);
    char *end = strpbrk(a7608_status.ip_addr, "\r\n,");
    if (end != NULL) {
        *end = '\0';
    }
}

static void a7608_parse_snapshot_response(const char *cmd, const char *response)
{
    if ((cmd == NULL) || (response == NULL)) {
        return;
    }
    if (strcmp(cmd, "AT") == 0) {
        a7608_status.at_ready = strstr(response, "OK") != NULL;
        if (a7608_status.at_ready) {
            a7608_status.state = A7608_STATE_AT_READY;
        }
    } else if (strcmp(cmd, "AT+CPIN?") == 0) {
        a7608_status.sim_ready = strstr(response, "READY") != NULL;
        if (a7608_status.sim_ready) {
            a7608_status.state = A7608_STATE_SIM_READY;
        }
    } else if (strcmp(cmd, "AT+CSQ") == 0) {
        parse_csq(response);
    } else if (strcmp(cmd, "AT+CFUN?") == 0) {
        parse_cfun(response);
    } else if ((strcmp(cmd, "AT+CEREG?") == 0) || (strcmp(cmd, "AT+CREG?") == 0)) {
        (void)response_has_registered(response);
    } else if (strcmp(cmd, "AT+CGATT?") == 0) {
        a7608_status.attached = strstr(response, "+CGATT: 1") != NULL;
        if (a7608_status.attached) {
            a7608_status.state = A7608_STATE_ATTACHED;
        }
    } else if (strcmp(cmd, "AT+COPS?") == 0) {
        parse_operator(response);
    } else if (strcmp(cmd, "AT+CGDCONT?") == 0) {
        parse_cgdcont(response);
    } else if (strcmp(cmd, "AT+CGPADDR") == 0) {
        parse_ip_addr(response);
        a7608_status.connected = a7608_ip_is_valid(a7608_status.ip_addr);
        if (a7608_status.connected) {
            a7608_status.state = A7608_STATE_CONNECTED;
        }
    }
}

static bool a7608_ip_is_valid(const char *ip_addr)
{
    return (ip_addr != NULL) && (ip_addr[0] != '\0') && (strcmp(ip_addr, "0.0.0.0") != 0);
}

static void a7608_sync_network_status(void)
{
    bool connected = a7608_status.connected && a7608_ip_is_valid(a7608_status.ip_addr);
    const char *ip_addr = connected ? a7608_status.ip_addr : NULL;

#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_TEST_MODE && HUB_LTE_PPPOS_REAL_RUNTIME && HUB_LTE_PPPOS_MANUAL_TEST
    if (connected) {
        ESP_LOGI("A7608", "PPPoS manual test: ignore CGPADDR IP %s for Network Manager; waiting for PPP got IP", ip_addr);
    }
    (void)hub_lte_pppos_set_connected(false, NULL);
    hub_network_manager_set_lte_status(false, NULL);
#else
    (void)hub_lte_pppos_set_connected(connected, ip_addr);
    hub_network_manager_set_lte_status(connected, ip_addr);
#endif
}

static esp_err_t a7608_reinstall_uart_driver(void)
{
    if (uart_is_driver_installed(a7608_cfg.uart_num)) {
        a7608_uart_driver_owned = true;
        return ESP_OK;
    }

    uart_config_t uart_config = {
        .baud_rate = a7608_cfg.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_param_config(a7608_cfg.uart_num, &uart_config);
    if (ret != ESP_OK) {
        set_last_error("uart_param_config resume failed");
        return ret;
    }

    ret = uart_set_pin(a7608_cfg.uart_num,
                       a7608_cfg.modem_tx_pin,
                       a7608_cfg.modem_rx_pin,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        set_last_error("uart_set_pin resume failed");
        return ret;
    }

    ret = uart_driver_install(a7608_cfg.uart_num, a7608_cfg.rx_buffer_size, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        set_last_error("uart_driver_install resume failed");
        return ret;
    }

    ret = uart_set_mode(a7608_cfg.uart_num, UART_MODE_UART);
    if (ret != ESP_OK) {
        (void)uart_driver_delete(a7608_cfg.uart_num);
        set_last_error("uart_set_mode resume failed");
        return ret;
    }

    a7608_uart_driver_owned = true;
    return ESP_OK;
}

static esp_err_t a7608_pause_service(void)
{
    if (a7608_service_state == A7608_SERVICE_PAUSED) {
        return ESP_OK;
    }
    if (a7608_service_state != A7608_SERVICE_PAUSE_REQUESTED) {
        return ESP_ERR_INVALID_STATE;
    }

    if (a7608_initialized && uart_is_driver_installed(a7608_cfg.uart_num)) {
        (void)uart_wait_tx_done(a7608_cfg.uart_num, pdMS_TO_TICKS(200));
        (void)uart_flush_input(a7608_cfg.uart_num);
        if (a7608_uart_driver_owned) {
            esp_err_t ret = uart_driver_delete(a7608_cfg.uart_num);
            if (ret != ESP_OK) {
                a7608_service_state = A7608_SERVICE_ERROR;
                set_last_error("UART driver release for PPPoS failed");
                ESP_LOGE("A7608", "PPPoS pause failed: uart_driver_delete UART%d %s", a7608_cfg.uart_num, esp_err_to_name(ret));
                return ret;
            }
            a7608_uart_driver_owned = false;
        }
    }

    a7608_service_state = A7608_SERVICE_PAUSED;
    ESP_LOGI("A7608", "A7608 service paused");
    return ESP_OK;
}

static esp_err_t a7608_resume_service(void)
{
    if (a7608_service_state != A7608_SERVICE_RESUME_REQUESTED) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_OK;
    if (a7608_initialized) {
        ret = a7608_reinstall_uart_driver();
    }
    if (ret != ESP_OK) {
        a7608_service_state = A7608_SERVICE_ERROR;
        ESP_LOGE("A7608", "A7608 AT service resume failed: %s", esp_err_to_name(ret));
        return ret;
    }

    a7608_service_state = A7608_SERVICE_RUNNING;
    ESP_LOGW("A7608", "A7608 AT service resumed; parsed status will refresh on next debug/status cycle");
    return ESP_OK;
}

static void copy_csv_field(const char **cursor, char *out, size_t out_len)
{
    if ((cursor == NULL) || (*cursor == NULL) || (out == NULL) || (out_len == 0)) {
        return;
    }

    const char *start = *cursor;
    while ((*start == ' ') || (*start == '\t') || (*start == '"')) {
        start++;
    }

    const char *end = start;
    while ((*end != '\0') && (*end != ',') && (*end != '\r') && (*end != '\n')) {
        end++;
    }
    while ((end > start) && ((end[-1] == ' ') || (end[-1] == '\t') || (end[-1] == '"'))) {
        end--;
    }

    size_t len = (size_t)(end - start);
    if (len >= out_len) {
        len = out_len - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';

    const char *next = strchr(*cursor, ',');
    *cursor = next != NULL ? next + 1 : NULL;
}

static void parse_gnss_power(const char *response)
{
    const char *line = strstr(response, "+CGNSSPWR:");
    int first = 0;
    int second = -1;
    int third = -1;
    if ((line != NULL) && (sscanf(line, "%*[^:]: %d,%d,%d", &first, &second, &third) >= 1)) {
        a7608_status.gnss_powered = second >= 0 ? (second != 0) : (first != 0);
    }
}

static void parse_gps_info(const char *response)
{
    const char *line = strstr(response, "+CGPSINFO:");
    char date[12] = {0};
    char time[16] = {0};
    char ns[4] = {0};
    char ew[4] = {0};

    a7608_status.gnss_fix = false;
    a7608_status.gnss_utc[0] = '\0';
    a7608_status.gnss_latitude[0] = '\0';
    a7608_status.gnss_ns = '\0';
    a7608_status.gnss_longitude[0] = '\0';
    a7608_status.gnss_ew = '\0';

    if (line == NULL) {
        return;
    }

    const char *field = strchr(line, ':');
    if (field == NULL) {
        return;
    }
    field++;

    copy_csv_field(&field, a7608_status.gnss_latitude, sizeof(a7608_status.gnss_latitude));
    copy_csv_field(&field, ns, sizeof(ns));
    copy_csv_field(&field, a7608_status.gnss_longitude, sizeof(a7608_status.gnss_longitude));
    copy_csv_field(&field, ew, sizeof(ew));
    copy_csv_field(&field, date, sizeof(date));
    copy_csv_field(&field, time, sizeof(time));

    a7608_status.gnss_ns = ns[0];
    a7608_status.gnss_ew = ew[0];
    if ((date[0] != '\0') || (time[0] != '\0')) {
        a7608_status.gnss_utc[0] = '\0';
        strlcpy(a7608_status.gnss_utc, date, sizeof(a7608_status.gnss_utc));
        strlcat(a7608_status.gnss_utc, time, sizeof(a7608_status.gnss_utc));
    }
    a7608_status.gnss_fix = (a7608_status.gnss_latitude[0] != '\0') &&
                             (a7608_status.gnss_longitude[0] != '\0');
}

static void parse_cgnsinf(const char *response)
{
    const char *line = strstr(response, "+CGNSINF:");
    char run_status[8] = {0};
    char fix_status[8] = {0};

    a7608_status.gnss_fix = false;
    a7608_status.gnss_utc[0] = '\0';
    a7608_status.gnss_latitude[0] = '\0';
    a7608_status.gnss_ns = '\0';
    a7608_status.gnss_longitude[0] = '\0';
    a7608_status.gnss_ew = '\0';

    if (line == NULL) {
        return;
    }

    const char *field = strchr(line, ':');
    if (field == NULL) {
        return;
    }
    field++;

    copy_csv_field(&field, run_status, sizeof(run_status));
    copy_csv_field(&field, fix_status, sizeof(fix_status));
    copy_csv_field(&field, a7608_status.gnss_utc, sizeof(a7608_status.gnss_utc));
    copy_csv_field(&field, a7608_status.gnss_latitude, sizeof(a7608_status.gnss_latitude));
    copy_csv_field(&field, a7608_status.gnss_longitude, sizeof(a7608_status.gnss_longitude));

    if (run_status[0] == '1') {
        a7608_status.gnss_powered = true;
    }
    a7608_status.gnss_fix = (fix_status[0] == '1') &&
                             (a7608_status.gnss_latitude[0] != '\0') &&
                             (a7608_status.gnss_longitude[0] != '\0');
}

static esp_err_t a7608_read_gnss_info(char *response, size_t response_len, const char **command_used)
{
    static const char *commands[] = {
        "AT+CGNSSINFO?",
        "AT+CGPSINFO?",
        "AT+CGNSSINFO",
        "AT+CGNSSINFO=1",
        "AT+CGNSSINFO=0",
        "AT+CGPSINFO",
        "AT+CGPSINFO=1",
        "AT+CGPSINFO=0",
        "AT+CGNSINF",
    };

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        esp_err_t ret = a7608_send_command(commands[i], "OK", 5000, response, response_len);
        if (command_used != NULL) {
            *command_used = commands[i];
        }
        if (ret != ESP_OK) {
            continue;
        }
        if (strstr(response, "+CGNSSINFO:") != NULL) {
            parse_gnss_info(response);
            return ESP_OK;
        }
        if (strstr(response, "+CGPSINFO:") != NULL) {
            parse_gps_info(response);
            return ESP_OK;
        }
        if (strstr(response, "+CGNSINF:") != NULL) {
            parse_cgnsinf(response);
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

static void parse_gnss_info(const char *response)
{
    const char *line = strstr(response, "+CGNSSINFO:");
    char run_status[8] = {0};
    char fix_status[8] = {0};
    char ns[4] = {0};
    char ew[4] = {0};

    a7608_status.gnss_fix = false;
    a7608_status.gnss_utc[0] = '\0';
    a7608_status.gnss_latitude[0] = '\0';
    a7608_status.gnss_ns = '\0';
    a7608_status.gnss_longitude[0] = '\0';
    a7608_status.gnss_ew = '\0';

    if (line == NULL) {
        return;
    }

    const char *field = strchr(line, ':');
    if (field == NULL) {
        return;
    }
    field++;

    copy_csv_field(&field, run_status, sizeof(run_status));
    copy_csv_field(&field, fix_status, sizeof(fix_status));
    copy_csv_field(&field, a7608_status.gnss_utc, sizeof(a7608_status.gnss_utc));
    copy_csv_field(&field, a7608_status.gnss_latitude, sizeof(a7608_status.gnss_latitude));
    copy_csv_field(&field, ns, sizeof(ns));
    copy_csv_field(&field, a7608_status.gnss_longitude, sizeof(a7608_status.gnss_longitude));
    copy_csv_field(&field, ew, sizeof(ew));

    if (run_status[0] == '1') {
        a7608_status.gnss_powered = true;
    }
    a7608_status.gnss_ns = ns[0];
    a7608_status.gnss_ew = ew[0];
    a7608_status.gnss_fix = (fix_status[0] == '1') &&
                             (a7608_status.gnss_latitude[0] != '\0') &&
                             (a7608_status.gnss_longitude[0] != '\0');
}

void a7608_get_default_config(a7608_config_t *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->uart_num = A7608_DEFAULT_UART_NUM;
    config->baud_rate = A7608_DEFAULT_BAUD_RATE;
    config->rx_buffer_size = A7608_DEFAULT_RX_BUFFER_SIZE;
    config->modem_tx_pin = A7608_DEFAULT_MODEM_TX_PIN;
    config->modem_rx_pin = A7608_DEFAULT_MODEM_RX_PIN;
    config->pwrkey_pin = A7608_DEFAULT_PWRKEY_PIN;
    config->reset_pin = A7608_DEFAULT_RESET_PIN;
    config->ring_pin = A7608_DEFAULT_RING_PIN;
    config->dtr_pin = A7608_DEFAULT_DTR_PIN;
    config->sim_sel_pin = A7608_DEFAULT_SIM_SEL_PIN;
    config->sim1_det_pin = A7608_DEFAULT_SIM1_DET_PIN;
    config->sim2_det_pin = A7608_DEFAULT_SIM2_DET_PIN;
    config->pwrkey_active_level = 1;
    config->reset_active_level = 0;
    config->dtr_active_level = 0;
    config->reset_uart_driver = true;
    config->configure_control_pins = true;
}

esp_err_t a7608_init(const a7608_config_t *config)
{
    a7608_get_default_config(&a7608_cfg);
    if (config != NULL) {
        a7608_cfg = *config;
    }

    memset(&a7608_status, 0, sizeof(a7608_status));
    a7608_status.state = A7608_STATE_OFF;
    a7608_status.csq = 99;
    a7608_status.creg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cereg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cfun = A7608_STATUS_INVALID_CFUN;
    a7608_status.last_refresh_result = ESP_ERR_INVALID_STATE;
    a7608_service_state = A7608_SERVICE_RUNNING;
    if (a7608_uart_lock == NULL) {
        a7608_uart_lock = xSemaphoreCreateMutex();
    }

    if (uart_is_driver_installed(a7608_cfg.uart_num)) {
        if (!a7608_cfg.reset_uart_driver) {
            set_last_error("UART driver already installed");
            return ESP_ERR_INVALID_STATE;
        }
        (void)uart_driver_delete(a7608_cfg.uart_num);
    }

    uart_config_t uart_config = {
        .baud_rate = a7608_cfg.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_param_config(a7608_cfg.uart_num, &uart_config);
    if (ret != ESP_OK) {
        set_last_error("uart_param_config failed");
        return ret;
    }

    ret = uart_set_pin(a7608_cfg.uart_num,
                       a7608_cfg.modem_tx_pin,
                       a7608_cfg.modem_rx_pin,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        set_last_error("uart_set_pin failed");
        return ret;
    }

    ret = uart_driver_install(a7608_cfg.uart_num, a7608_cfg.rx_buffer_size, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        set_last_error("uart_driver_install failed");
        return ret;
    }
    a7608_uart_driver_owned = true;

    ret = uart_set_mode(a7608_cfg.uart_num, UART_MODE_UART);
    if (ret != ESP_OK) {
        set_last_error("uart_set_mode failed");
        return ret;
    }

    if (a7608_cfg.configure_control_pins) {
        ret = configure_output_pin(a7608_cfg.pwrkey_pin, inactive_level(a7608_cfg.pwrkey_active_level));
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_output_pin(a7608_cfg.reset_pin, inactive_level(a7608_cfg.reset_active_level));
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_output_pin(a7608_cfg.dtr_pin, inactive_level(a7608_cfg.dtr_active_level));
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_input_pin(a7608_cfg.ring_pin);
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_output_pin(a7608_cfg.sim_sel_pin, A7608_SIM1_SELECT_LEVEL);
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_input_pin(a7608_cfg.sim1_det_pin);
        if (ret != ESP_OK) {
            return ret;
        }
        ret = configure_input_pin(a7608_cfg.sim2_det_pin);
        if (ret != ESP_OK) {
            return ret;
        }
        a7608_status.active_sim_slot = A7608_SIM_SLOT_1;
        a7608_refresh_sim_detect();
        ESP_LOGI("A7608",
                 "A7608 SIM DET init: sim1=%d sim2=%d gpio1=%d gpio2=%d pullup=off inserted_level=%d",
                 a7608_status.sim1_present,
                 a7608_status.sim2_present,
                 pin_is_valid(a7608_cfg.sim1_det_pin) ? gpio_get_level(a7608_cfg.sim1_det_pin) : -1,
                 pin_is_valid(a7608_cfg.sim2_det_pin) ? gpio_get_level(a7608_cfg.sim2_det_pin) : -1,
                 A7608_SIM_DET_INSERTED_LEVEL);
        ESP_LOGI("A7608",
                 "A7608 control GPIO modes: PWRKEY gpio_mode=INPUT_OUTPUT RESET gpio_mode=INPUT_OUTPUT DTR gpio_mode=INPUT_OUTPUT SIM_SEL gpio=%d SIM1_DET gpio=%d SIM2_DET gpio=%d",
                 a7608_cfg.sim_sel_pin,
                 a7608_cfg.sim1_det_pin,
                 a7608_cfg.sim2_det_pin);
    }

    a7608_cpin_failure_raw_logged = false;
    a7608_cpin_success_raw_logged = false;
    a7608_initialized = true;
    if (a7608_cfg.configure_control_pins) {
        (void)a7608_select_sim_slot(a7608_preferred_sim_slot());
    }
    A7608_LOG("UART%d ready modem_tx=%d modem_rx=%d baud=%d",
              a7608_cfg.uart_num,
              a7608_cfg.modem_tx_pin,
              a7608_cfg.modem_rx_pin,
              a7608_cfg.baud_rate);
    return ESP_OK;
}

esp_err_t a7608_deinit(void)
{
    if (a7608_initialized && uart_is_driver_installed(a7608_cfg.uart_num)) {
        (void)uart_driver_delete(a7608_cfg.uart_num);
    }
    a7608_uart_driver_owned = false;
    a7608_initialized = false;
    a7608_status.state = A7608_STATE_OFF;
    return ESP_OK;
}

esp_err_t a7608_power_on(uint32_t pulse_ms, uint32_t boot_wait_ms)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.pwrkey_pin)) {
        return ESP_ERR_INVALID_STATE;
    }

    a7608_status.state = A7608_STATE_BOOTING;
    int idle_level = inactive_level(a7608_cfg.pwrkey_active_level);
    int before_level = gpio_get_level(a7608_cfg.pwrkey_pin);
    ESP_LOGI("A7608",
             "A7608 PWRKEY before: gpio=%d level=%d idle_level=%d active_level=%d",
             a7608_cfg.pwrkey_pin,
             before_level,
             idle_level,
             a7608_cfg.pwrkey_active_level);
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.pwrkey_pin, idle_level), "A7608", "pwrkey idle prepare failed");
    int prepared_level = gpio_get_level(a7608_cfg.pwrkey_pin);
    if (prepared_level != idle_level) {
        ESP_LOGE("A7608", "A7608 PWRKEY idle readback mismatch: expected=%d actual=%d", idle_level, prepared_level);
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(A7608_CONTROL_IDLE_SETTLE_MS));
    ESP_LOGI("A7608",
             "A7608 PWRKEY pulse begin: gpio=%d active_level=%d pulse_ms=%lu",
             a7608_cfg.pwrkey_pin,
             a7608_cfg.pwrkey_active_level,
             (unsigned long)pulse_ms);
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.pwrkey_pin, a7608_cfg.pwrkey_active_level), "A7608", "pwrkey active failed");
    int active_readback = gpio_get_level(a7608_cfg.pwrkey_pin);
    ESP_LOGI("A7608", "A7608 PWRKEY active: level=%d", active_readback);
    if (active_readback != a7608_cfg.pwrkey_active_level) {
        ESP_LOGE("A7608", "A7608 PWRKEY active readback mismatch: expected=%d actual=%d", a7608_cfg.pwrkey_active_level, active_readback);
        (void)gpio_set_level(a7608_cfg.pwrkey_pin, idle_level);
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(pulse_ms));
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.pwrkey_pin, idle_level), "A7608", "pwrkey inactive failed");
    int after_level = gpio_get_level(a7608_cfg.pwrkey_pin);
    ESP_LOGI("A7608", "A7608 PWRKEY after: level=%d", after_level);
    if (after_level != idle_level) {
        ESP_LOGE("A7608", "A7608 PWRKEY idle readback mismatch: expected=%d actual=%d", idle_level, after_level);
        return ESP_FAIL;
    }
    ESP_LOGI("A7608", "A7608 PWRKEY pulse end: gpio=%d", a7608_cfg.pwrkey_pin);
    if (boot_wait_ms > 0) {
        ESP_LOGI("A7608", "A7608 waiting after PWRKEY: wait_ms=%lu", (unsigned long)boot_wait_ms);
    }
    vTaskDelay(pdMS_TO_TICKS(boot_wait_ms));
    (void)uart_flush_input(a7608_cfg.uart_num);
    return ESP_OK;
}

esp_err_t a7608_hard_reset(uint32_t pulse_ms, uint32_t boot_wait_ms)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.reset_pin)) {
        return ESP_ERR_INVALID_STATE;
    }

    a7608_status.state = A7608_STATE_BOOTING;
    int idle_level = inactive_level(a7608_cfg.reset_active_level);
    int before_level = gpio_get_level(a7608_cfg.reset_pin);
    ESP_LOGW("A7608",
             "A7608 RESET before: gpio=%d level=%d idle_level=%d active_level=%d",
             a7608_cfg.reset_pin,
             before_level,
             idle_level,
             a7608_cfg.reset_active_level);
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.reset_pin, idle_level), "A7608", "reset idle prepare failed");
    int prepared_level = gpio_get_level(a7608_cfg.reset_pin);
    if (prepared_level != idle_level) {
        ESP_LOGE("A7608", "A7608 RESET idle readback mismatch: expected=%d actual=%d", idle_level, prepared_level);
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(A7608_CONTROL_IDLE_SETTLE_MS));
    ESP_LOGW("A7608",
             "A7608 RESET asserted: gpio=%d active_level=%d pulse_ms=%lu",
             a7608_cfg.reset_pin,
             a7608_cfg.reset_active_level,
             (unsigned long)pulse_ms);
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.reset_pin, a7608_cfg.reset_active_level), "A7608", "reset active failed");
    int asserted_level = gpio_get_level(a7608_cfg.reset_pin);
    ESP_LOGW("A7608", "A7608 RESET asserted readback: level=%d", asserted_level);
    if (asserted_level != a7608_cfg.reset_active_level) {
        ESP_LOGE("A7608", "A7608 RESET asserted readback mismatch: expected=%d actual=%d", a7608_cfg.reset_active_level, asserted_level);
        (void)gpio_set_level(a7608_cfg.reset_pin, idle_level);
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(pulse_ms));
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.reset_pin, idle_level), "A7608", "reset inactive failed");
    int released_level = gpio_get_level(a7608_cfg.reset_pin);
    ESP_LOGW("A7608", "A7608 RESET released: gpio=%d level=%d", a7608_cfg.reset_pin, released_level);
    if (released_level != idle_level) {
        ESP_LOGE("A7608", "A7608 RESET released readback mismatch: expected=%d actual=%d", idle_level, released_level);
        return ESP_FAIL;
    }
    if (boot_wait_ms > 0) {
        ESP_LOGI("A7608", "A7608 waiting after RESET: wait_ms=%lu", (unsigned long)boot_wait_ms);
    }
    vTaskDelay(pdMS_TO_TICKS(boot_wait_ms));
    (void)uart_flush_input(a7608_cfg.uart_num);
    return ESP_OK;
}

esp_err_t a7608_try_exit_data_mode(void)
{
    return a7608_startup_try_exit_data_mode(NULL) ? ESP_OK : ESP_FAIL;
}

void a7608_clear_stale_ready(void)
{
    a7608_status.at_ready = false;
    a7608_status.sim_ready = false;
    a7608_status.status_valid = false;
    a7608_status.registered_home = false;
    a7608_status.registered_roaming = false;
    a7608_status.attached = false;
    a7608_status.connected = false;
    a7608_status.creg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cereg_stat = A7608_STATUS_INVALID_STAT;
    a7608_status.cfun = A7608_STATUS_INVALID_CFUN;
    a7608_status.last_refresh_result = ESP_ERR_INVALID_STATE;
    ESP_LOGW("A7608", "A7608 stale AT/SIM ready flags cleared");
}

esp_err_t a7608_set_dtr(bool active)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.dtr_pin)) {
        return ESP_ERR_INVALID_STATE;
    }
    int level = active ? a7608_cfg.dtr_active_level : inactive_level(a7608_cfg.dtr_active_level);
    return gpio_set_level(a7608_cfg.dtr_pin, level);
}

esp_err_t a7608_set_dtr_level(int level)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.dtr_pin) || ((level != 0) && (level != 1))) {
        return ESP_ERR_INVALID_ARG;
    }
    return gpio_set_level(a7608_cfg.dtr_pin, level);
}

int a7608_get_dtr_level(void)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.dtr_pin)) {
        return -1;
    }
    return gpio_get_level(a7608_cfg.dtr_pin);
}

esp_err_t a7608_send_command(const char *cmd,
                             const char *expected,
                             uint32_t timeout_ms,
                             char *response,
                             size_t response_len)
{
    if (!a7608_initialized || cmd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (a7608_service_state != A7608_SERVICE_RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!a7608_take_uart_lock(A7608_UART_LOCK_WAIT_MS)) {
        set_last_error("UART busy");
        return ESP_ERR_TIMEOUT;
    }

    char local_response[512];
    char *out = response != NULL ? response : local_response;
    size_t out_len = response != NULL ? response_len : sizeof(local_response);
    if (out_len == 0) {
        a7608_give_uart_lock();
        return ESP_ERR_INVALID_ARG;
    }
    out[0] = '\0';

    (void)uart_flush_input(a7608_cfg.uart_num);
    int wrote = uart_write_bytes(a7608_cfg.uart_num, cmd, strlen(cmd));
    int wrote_crlf = uart_write_bytes(a7608_cfg.uart_num, "\r\n", 2);
    if ((wrote < 0) || (wrote_crlf < 0)) {
        set_last_error("uart_write_bytes failed");
        a7608_give_uart_lock();
        return ESP_FAIL;
    }

    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    size_t used = 0;
    uint8_t buf[96];

    while ((xTaskGetTickCount() - start) < timeout_ticks) {
        int len = uart_read_bytes(a7608_cfg.uart_num, buf, sizeof(buf) - 1, pdMS_TO_TICKS(50));
        if (len <= 0) {
            continue;
        }
        buf[len] = '\0';

        size_t copy_len = (size_t)len;
        if (copy_len > (out_len - used - 1)) {
            copy_len = out_len - used - 1;
        }
        if (copy_len > 0) {
            memcpy(out + used, buf, copy_len);
            used += copy_len;
            out[used] = '\0';
        }

        if ((expected != NULL) && (strstr(out, expected) != NULL)) {
            set_last_error(NULL);
            a7608_give_uart_lock();
            return ESP_OK;
        }
        if (strstr(out, "ERROR") != NULL) {
            set_last_error("modem returned ERROR");
            a7608_give_uart_lock();
            return ESP_FAIL;
        }
    }

    set_last_error("AT command timeout");
    a7608_give_uart_lock();
    return ESP_ERR_TIMEOUT;
}

esp_err_t a7608_probe(void)
{
    char response[128];
    esp_err_t ret = a7608_send_command("AT", "OK", 3000, response, sizeof(response));
    if (ret == ESP_OK) {
        a7608_status.at_ready = true;
        a7608_status.state = A7608_STATE_AT_READY;
    } else {
        a7608_status.at_ready = false;
        if (!a7608_status.sim_ready) {
            a7608_status.state = A7608_STATE_ERROR;
        }
    }
    return ret;
}

static const char *a7608_cpin_type_name(a7608_cpin_type_t type)
{
    switch (type) {
    case A7608_CPIN_READY:
        return "READY";
    case A7608_CPIN_NOT_READY:
        return "NOT_READY";
    case A7608_CPIN_SIM_BUSY:
        return "SIM_BUSY";
    case A7608_CPIN_SIM_FAILURE:
        return "SIM_FAILURE";
    case A7608_CPIN_SIM_NOT_INSERTED:
        return "SIM_NOT_INSERTED";
    case A7608_CPIN_CME_ERROR:
        return "CME_ERROR";
    case A7608_CPIN_QUERY_FAILED:
    default:
        return "QUERY_FAILED";
    }
}

static a7608_cpin_result_t a7608_query_cpin(bool *sim_ready)
{
    char response[128] = {0};
    a7608_cpin_result_t result = {
        .type = A7608_CPIN_QUERY_FAILED,
        .cme_code = -1,
        .command_ret = ESP_ERR_INVALID_RESPONSE,
    };
    if (sim_ready != NULL) {
        *sim_ready = false;
    }

    esp_err_t ret = a7608_send_command("AT+CPIN?", "OK", 1500, response, sizeof(response));
    result.command_ret = ret;
    if ((ret == ESP_OK) && (strstr(response, "+CPIN:") != NULL)) {
        a7608_cpin_failure_raw_logged = false;
        if (!a7608_cpin_success_raw_logged) {
            ESP_LOGI("A7608", "A7608 CPIN query response=\"%s\"", response);
            a7608_cpin_success_raw_logged = true;
        }
        a7608_status.at_ready = true;
        a7608_status.sim_ready = strstr(response, "READY") != NULL;
        result.type = a7608_status.sim_ready ? A7608_CPIN_READY : A7608_CPIN_NOT_READY;
        if (a7608_status.sim_ready) {
            a7608_status.state = A7608_STATE_SIM_READY;
        }
        if (sim_ready != NULL) {
            *sim_ready = a7608_status.sim_ready;
        }
    } else if (ret == ESP_OK) {
        result.command_ret = ESP_ERR_INVALID_RESPONSE;
    } else {
        const char *cme_text = strstr(response, "+CME ERROR:");
        if (cme_text != NULL) {
            const char *cme_value = cme_text + strlen("+CME ERROR:");
            char *end = NULL;
            long cme_code = strtol(cme_value, &end, 10);
            if (end != cme_value) {
                result.cme_code = (int)cme_code;
                result.type = A7608_CPIN_CME_ERROR;
                if (cme_code == 10) {
                    result.type = A7608_CPIN_SIM_NOT_INSERTED;
                } else if (cme_code == 13) {
                    result.type = A7608_CPIN_SIM_FAILURE;
                } else if (cme_code == 14) {
                    result.type = A7608_CPIN_SIM_BUSY;
                }
            }
        }
    }

    if ((result.type != A7608_CPIN_READY) &&
        (result.type != A7608_CPIN_NOT_READY) &&
        !a7608_cpin_failure_raw_logged) {
        ESP_LOGW("A7608",
                 "A7608 CPIN modem response: type=%s cme=%d ret=%s response=\"%s\"",
                 a7608_cpin_type_name(result.type),
                 result.cme_code,
                 esp_err_to_name(result.command_ret),
                 response[0] != '\0' ? response : "-");
        a7608_cpin_failure_raw_logged = true;
    }

    ESP_LOGI("A7608",
             "A7608 SIM check: type=%s cme=%d ready=%d ret=%s",
             a7608_cpin_type_name(result.type),
             result.cme_code,
             result.type == A7608_CPIN_READY,
             esp_err_to_name(result.command_ret));
    return result;
}

esp_err_t a7608_check_sim_ready(bool *sim_ready)
{
    a7608_cpin_result_t result = a7608_query_cpin(sim_ready);
    return result.command_ret;
}

esp_err_t a7608_refresh_status(void)
{
    return a7608_refresh_status_ex(true);
}

esp_err_t a7608_refresh_status_ex(bool include_operator)
{
    char response[512];
    esp_err_t first_error = ESP_OK;
    esp_err_t at_ret;
    esp_err_t cpin_ret = ESP_ERR_INVALID_STATE;
    esp_err_t csq_ret = ESP_ERR_INVALID_STATE;
    esp_err_t cfun_ret = ESP_ERR_INVALID_STATE;
    esp_err_t cereg_ret = ESP_ERR_INVALID_STATE;
    esp_err_t creg_ret = ESP_ERR_INVALID_STATE;
    esp_err_t cgatt_ret = ESP_ERR_INVALID_STATE;
    esp_err_t cgpaddr_ret = ESP_ERR_INVALID_STATE;

    a7608_refresh_sim_detect();
    a7608_status.status_valid = false;
    a7608_status.status_age_ms = UINT32_MAX;

    at_ret = a7608_probe();
    if (at_ret != ESP_OK) {
        a7608_status.last_refresh_tick = xTaskGetTickCount();
        a7608_status.status_age_ms = UINT32_MAX;
        a7608_status.last_refresh_result = ESP_FAIL;
        a7608_sync_network_status();
        ESP_LOGW("A7608",
                 "A7608 status refresh partial failure: AT=%s CPIN=%s CFUN=%s CREG=%s CEREG=%s CGATT=%s",
                 esp_err_to_name(at_ret),
                 esp_err_to_name(cpin_ret),
                 esp_err_to_name(cfun_ret),
                 esp_err_to_name(creg_ret),
                 esp_err_to_name(cereg_ret),
                 esp_err_to_name(cgatt_ret));
        ESP_LOGW("A7608",
             "A7608 status refresh: ret=%s valid=0 age_ms=invalid at=0 retained_sim=%d retained_cfun=%d",
             esp_err_to_name(ESP_FAIL),
             a7608_status.sim_ready,
             a7608_status.cfun);
        return ESP_FAIL;
    }

    cpin_ret = a7608_check_sim_ready(NULL);
    if (cpin_ret != ESP_OK) {
        first_error = ESP_FAIL;
    }

    csq_ret = a7608_send_command("AT+CSQ", "OK", 1500, response, sizeof(response));
    if ((csq_ret == ESP_OK) && (strstr(response, "+CSQ:") != NULL)) {
        parse_csq(response);
    } else {
        csq_ret = csq_ret == ESP_OK ? ESP_ERR_INVALID_RESPONSE : csq_ret;
        if (first_error == ESP_OK) {
            first_error = ESP_FAIL;
        }
    }

    cfun_ret = a7608_send_command("AT+CFUN?", "OK", 1500, response, sizeof(response));
    if ((cfun_ret == ESP_OK) && (strstr(response, "+CFUN:") != NULL)) {
        parse_cfun(response);
    } else {
        cfun_ret = cfun_ret == ESP_OK ? ESP_ERR_INVALID_RESPONSE : cfun_ret;
        if (first_error == ESP_OK) {
            first_error = ESP_FAIL;
        }
    }

    cereg_ret = a7608_send_command("AT+CEREG?", "OK", 3000, response, sizeof(response));
    if ((cereg_ret == ESP_OK) && (strstr(response, "+CEREG:") != NULL)) {
        (void)response_has_registered(response);
    }

    creg_ret = a7608_send_command("AT+CREG?", "OK", 3000, response, sizeof(response));
    if ((creg_ret == ESP_OK) && (strstr(response, "+CREG:") != NULL)) {
        (void)response_has_registered(response);
    }
    if (a7608_status_is_registered()) {
        a7608_status.state = A7608_STATE_REGISTERED;
    }

    cgatt_ret = a7608_send_command("AT+CGATT?", "OK", 3000, response, sizeof(response));
    if ((cgatt_ret == ESP_OK) && (strstr(response, "+CGATT:") != NULL)) {
        a7608_status.attached = strstr(response, "+CGATT: 1") != NULL;
        if (a7608_status.attached) {
            a7608_status.state = A7608_STATE_ATTACHED;
        }
    }

    if (include_operator && a7608_status_is_registered() &&
        (a7608_send_command("AT+COPS?", "OK", 5000, response, sizeof(response)) == ESP_OK)) {
        parse_operator(response);
    }

    if (a7608_status_is_registered() &&
        (a7608_send_command("AT+CGDCONT?", "OK", 3000, response, sizeof(response)) == ESP_OK)) {
        parse_cgdcont(response);
    }

    if (a7608_status_is_registered()) {
        cgpaddr_ret = a7608_send_command("AT+CGPADDR", "OK", 2000, response, sizeof(response));
        if (cgpaddr_ret == ESP_OK) {
            parse_ip_addr(response);
            a7608_status.connected = a7608_ip_is_valid(a7608_status.ip_addr);
            if (a7608_status.connected) {
                a7608_status.state = A7608_STATE_CONNECTED;
            }
        }
    }

    a7608_sync_network_status();

    a7608_status.status_valid = (first_error == ESP_OK) && a7608_status.at_ready;
    a7608_status.last_refresh_tick = xTaskGetTickCount();
    a7608_status.status_age_ms = a7608_status.status_valid ? 0 : UINT32_MAX;
    a7608_status.last_refresh_result = first_error;

    if (first_error != ESP_OK) {
        ESP_LOGW("A7608",
                 "A7608 status refresh partial failure: AT=%s CPIN=%s CSQ=%s CFUN=%s CREG=%s CEREG=%s CGATT=%s CGPADDR=%s",
                 esp_err_to_name(at_ret),
                 esp_err_to_name(cpin_ret),
                 esp_err_to_name(csq_ret),
                 esp_err_to_name(cfun_ret),
                 esp_err_to_name(creg_ret),
                 esp_err_to_name(cereg_ret),
                 esp_err_to_name(cgatt_ret),
                 esp_err_to_name(cgpaddr_ret));
    }

    ESP_LOGI("A7608",
             "A7608 status refresh: ret=%s valid=%d age_ms=%lu at=%d sim=%d csq=%d rssi_valid=%d rssi_dbm=%d creg=%d cereg=%d registered=%d attached=%d cfun=%d operator=%s apn=%s",
             esp_err_to_name(first_error),
             a7608_status.status_valid,
             (unsigned long)a7608_status.status_age_ms,
             a7608_status.at_ready,
             a7608_status.sim_ready,
             a7608_status.csq,
             a7608_status.rssi_valid,
             a7608_status.rssi_dbm,
             a7608_status.creg_stat,
             a7608_status.cereg_stat,
             a7608_status_is_registered(),
             a7608_status.attached,
             a7608_status.cfun,
             a7608_status.operator_name[0] != '\0' ? a7608_status.operator_name : "-",
             a7608_status.apn[0] != '\0' ? a7608_status.apn : "-");

    return first_error;
}

esp_err_t a7608_read_pdp_apn(char *apn, size_t apn_len)
{
    char response[512];

    if ((apn == NULL) || (apn_len == 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    apn[0] = '\0';

    if (a7608_status.apn[0] != '\0') {
        snprintf(apn, apn_len, "%s", a7608_status.apn);
        return ESP_OK;
    }

    if (a7608_send_command("AT+CGDCONT?", "OK", 3000, response, sizeof(response)) != ESP_OK) {
        return ESP_FAIL;
    }
    parse_cgdcont(response);
    if (a7608_status.apn[0] == '\0') {
        return ESP_ERR_NOT_FOUND;
    }
    snprintf(apn, apn_len, "%s", a7608_status.apn);
    return ESP_OK;
}

esp_err_t a7608_read_imsi(char *imsi, size_t imsi_len)
{
    char response[128];
    const char *p;

    if ((imsi == NULL) || (imsi_len == 0)) {
        return ESP_ERR_INVALID_ARG;
    }
    imsi[0] = '\0';

    if (a7608_send_command("AT+CIMI", "OK", 3000, response, sizeof(response)) != ESP_OK) {
        return ESP_FAIL;
    }

    p = response;
    while (*p != '\0') {
        if ((*p >= '0') && (*p <= '9')) {
            size_t len = 0;
            while ((p[len] >= '0') && (p[len] <= '9')) {
                len++;
            }
            if (len >= 5) {
                if (len >= imsi_len) {
                    len = imsi_len - 1;
                }
                memcpy(imsi, p, len);
                imsi[len] = '\0';
                return ESP_OK;
            }
            p += len;
            continue;
        }
        p++;
    }

    return ESP_ERR_NOT_FOUND;
}

uint32_t a7608_status_age_ms(void)
{
    if ((a7608_status.last_refresh_tick == 0) || !a7608_status.status_valid) {
        return UINT32_MAX;
    }
    uint32_t age_ms = (uint32_t)((xTaskGetTickCount() - a7608_status.last_refresh_tick) * portTICK_PERIOD_MS);
    a7608_status.status_age_ms = age_ms;
    return age_ms;
}

bool a7608_status_is_fresh(uint32_t max_age_ms)
{
    return a7608_status.status_valid && (a7608_status_age_ms() <= max_age_ms);
}

bool a7608_status_is_registered(void)
{
    return a7608_registered_from_stat(a7608_status.creg_stat) ||
           a7608_registered_from_stat(a7608_status.cereg_stat);
}

void a7608_startup_probe_init_in_progress(void)
{
    a7608_startup_probe_started_flag = true;
    a7608_startup_probe_complete_flag = false;
    a7608_startup_probe_ret = ESP_ERR_INVALID_STATE;
    if (!a7608_startup_probe_state_log_printed) {
        ESP_LOGI("A7608", "A7608 startup probe state initialized: in_progress");
        a7608_startup_probe_state_log_printed = true;
    }
}

bool a7608_startup_probe_started(void)
{
    return a7608_startup_probe_started_flag;
}

bool a7608_startup_probe_complete(void)
{
    return a7608_startup_probe_complete_flag;
}

esp_err_t a7608_startup_probe_result(void)
{
    return a7608_startup_probe_ret;
}

esp_err_t a7608_gnss_enable(void)
{
    char response[128];
    esp_err_t ret = a7608_send_command("AT+CGNSSPWR=1", "OK", 5000, response, sizeof(response));
    if (ret == ESP_OK) {
        a7608_status.gnss_powered = true;
    }
    return ret;
}

esp_err_t a7608_gnss_disable(void)
{
    char response[128];
    esp_err_t ret = a7608_send_command("AT+CGNSSPWR=0", "OK", 5000, response, sizeof(response));
    if (ret == ESP_OK) {
        a7608_status.gnss_powered = false;
        a7608_status.gnss_fix = false;
        a7608_status.gnss_utc[0] = '\0';
        a7608_status.gnss_latitude[0] = '\0';
        a7608_status.gnss_ns = '\0';
        a7608_status.gnss_longitude[0] = '\0';
        a7608_status.gnss_ew = '\0';
    }
    return ret;
}

esp_err_t a7608_refresh_gnss(void)
{
    char response[768];
    esp_err_t first_error = ESP_OK;

    if (a7608_send_command("AT+CGNSSPWR?", "OK", 2000, response, sizeof(response)) == ESP_OK) {
        parse_gnss_power(response);
    } else {
        first_error = ESP_FAIL;
    }

    if (a7608_status.gnss_powered) {
        if (a7608_read_gnss_info(response, sizeof(response), NULL) != ESP_OK) {
            first_error = ESP_FAIL;
        }
    } else {
        a7608_status.gnss_fix = false;
    }

    return first_error;
}

const a7608_status_t *a7608_get_status(void)
{
    return &a7608_status;
}

esp_err_t a7608_request_pause(void)
{
    if (a7608_service_state == A7608_SERVICE_PAUSED) {
        return ESP_OK;
    }
    if (a7608_service_state == A7608_SERVICE_RESUME_REQUESTED) {
        return ESP_ERR_INVALID_STATE;
    }
    a7608_service_state = A7608_SERVICE_PAUSE_REQUESTED;
    ESP_LOGI("A7608", "A7608 pause requested");
    return ESP_OK;
}

esp_err_t a7608_request_resume(void)
{
    if (a7608_service_state == A7608_SERVICE_RUNNING) {
        return ESP_OK;
    }
    a7608_service_state = A7608_SERVICE_RESUME_REQUESTED;
    return ESP_OK;
}

bool a7608_is_pause_requested(void)
{
    return a7608_service_state == A7608_SERVICE_PAUSE_REQUESTED;
}

bool a7608_is_paused(void)
{
    return a7608_service_state == A7608_SERVICE_PAUSED;
}

a7608_service_state_t a7608_get_service_state(void)
{
    return a7608_service_state;
}

const char *a7608_service_state_name(a7608_service_state_t state)
{
    switch (state) {
    case A7608_SERVICE_RUNNING:
        return "RUNNING";
    case A7608_SERVICE_PAUSE_REQUESTED:
        return "PAUSE_REQUESTED";
    case A7608_SERVICE_PAUSED:
        return "PAUSED";
    case A7608_SERVICE_RESUME_REQUESTED:
        return "RESUME_REQUESTED";
    case A7608_SERVICE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

const char *a7608_state_name(a7608_state_t state)
{
    switch (state) {
    case A7608_STATE_OFF:
        return "OFF";
    case A7608_STATE_BOOTING:
        return "BOOTING";
    case A7608_STATE_AT_READY:
        return "AT_READY";
    case A7608_STATE_SIM_READY:
        return "SIM_READY";
    case A7608_STATE_REGISTERED:
        return "REGISTERED";
    case A7608_STATE_ATTACHED:
        return "ATTACHED";
    case A7608_STATE_CONNECTED:
        return "CONNECTED";
    case A7608_STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

static void a7608_debug_probe_gnss_commands(void)
{
    static const char *commands[] = {
        "AT+CGNSSPWR=?",
        "AT+CGNSSPWR?",
        "AT+CGNSSINFO=?",
        "AT+CGNSSINFO?",
        "AT+CGPS=?",
        "AT+CGPS?",
        "AT+CGPSINFO=?",
        "AT+CGPSINFO?",
        "AT+CGPSSTATUS?",
        "AT+CGNSINF=?",
        "AT+CGNSINF?",
        "AT+CGNSSMODE=?",
        "AT+CGNSSMODE?",
        "AT+CGNSSPORTSWITCH=?",
        "AT+CGNSSPORTSWITCH?",
    };
    static char response[768];

    a7608_debug_write("\r\n[A7608 GNSS COMMAND PROBE]\r\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        esp_err_t ret = a7608_send_command(commands[i], "OK", 2000, response, sizeof(response));
        a7608_debug_printf("probe_%s=%s\r\n", commands[i] + 3, esp_err_to_name(ret));
        a7608_debug_write_response_block(commands[i] + 3, response);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    a7608_debug_write("[A7608 GNSS COMMAND PROBE END]\r\n");
}

static esp_err_t a7608_debug_print_parsed_status(void)
{
    esp_err_t ret = a7608_status.last_refresh_result;
    const a7608_status_t *status = a7608_get_status();

    a7608_debug_write("\r\n[A7608 PARSED STATUS]\r\n");
    a7608_debug_printf("refresh=%s state=%s at=%d sim=%d slot=%s sim1=%d sim2=%d reg_home=%d reg_roam=%d attached=%d connected=%d\r\n",
                       esp_err_to_name(ret),
                       a7608_state_name(status->state),
                       status->at_ready,
                       status->sim_ready,
                       a7608_sim_slot_name(status->active_sim_slot),
                       status->sim1_present,
                       status->sim2_present,
                       status->registered_home,
                       status->registered_roaming,
                       status->attached,
                       status->connected);
    a7608_debug_printf("csq=%d rssi_valid=%d rssi_dbm=%d creg=%d cereg=%d cfun=%d age_ms=%lu operator=%s apn=%s ip=%s last_error=%s\r\n",
                       status->csq,
                       status->rssi_valid,
                       status->rssi_dbm,
                       status->creg_stat,
                       status->cereg_stat,
                       status->cfun,
                       (unsigned long)a7608_status_age_ms(),
                       status->operator_name[0] != '\0' ? status->operator_name : "-",
                       status->apn[0] != '\0' ? status->apn : "-",
                       status->ip_addr[0] != '\0' ? status->ip_addr : "-",
                       status->last_error[0] != '\0' ? status->last_error : "-");
    a7608_debug_write("[A7608 PARSED STATUS END]\r\n");
    return ret;
}

static void a7608_debug_print_gnss_status(void)
{
    static char response[768];
    a7608_debug_write("\r\n[A7608 GNSS STATUS]\r\n");

    esp_err_t enable_ret = a7608_send_command("AT+CGNSSPWR=1", "OK", 5000, response, sizeof(response));
    a7608_debug_printf("enable=%s\r\n", esp_err_to_name(enable_ret));
    a7608_debug_write_response_block("GNSS ENABLE", response);
    if (enable_ret == ESP_OK) {
        a7608_status.gnss_powered = true;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    esp_err_t power_ret = a7608_send_command("AT+CGNSSPWR?", "OK", 3000, response, sizeof(response));
    a7608_debug_printf("power_query=%s\r\n", esp_err_to_name(power_ret));
    a7608_debug_write_response_block("GNSS POWER QUERY", response);
    if (power_ret == ESP_OK) {
        parse_gnss_power(response);
    }

    if ((power_ret == ESP_OK) && !a7608_status.gnss_powered) {
        esp_err_t fallback_ret = a7608_send_command("AT+CGNSSPWR=1,1", "OK", 5000, response, sizeof(response));
        a7608_debug_printf("enable_fallback=%s\r\n", esp_err_to_name(fallback_ret));
        a7608_debug_write_response_block("GNSS ENABLE FALLBACK", response);
        if (fallback_ret == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            power_ret = a7608_send_command("AT+CGNSSPWR?", "OK", 3000, response, sizeof(response));
            a7608_debug_printf("power_query_after_fallback=%s\r\n", esp_err_to_name(power_ret));
            a7608_debug_write_response_block("GNSS POWER QUERY AFTER FALLBACK", response);
            if (power_ret == ESP_OK) {
                parse_gnss_power(response);
            }
        }
    }

    static const char *info_commands[] = {
        "AT+CGNSSINFO",
        "AT+CGNSSINFO=1",
        "AT+CGNSSINFO=0",
        "AT+CGPSINFO",
        "AT+CGPSINFO=1",
        "AT+CGPSINFO=0",
        "AT+CGNSINF",
    };
    esp_err_t info_ret = ESP_FAIL;
    bool info_parsed = false;
    for (size_t i = 0; i < sizeof(info_commands) / sizeof(info_commands[0]); i++) {
        info_ret = a7608_send_command(info_commands[i], "OK", 5000, response, sizeof(response));
        a7608_debug_printf("info_%s=%s\r\n", info_commands[i] + 3, esp_err_to_name(info_ret));
        a7608_debug_write_response_block(info_commands[i] + 3, response);
        if (info_ret != ESP_OK) {
            continue;
        }
        if (strstr(response, "+CGNSSINFO:") != NULL) {
            parse_gnss_info(response);
            info_parsed = true;
            break;
        }
        if (strstr(response, "+CGPSINFO:") != NULL) {
            parse_gps_info(response);
            info_parsed = true;
            break;
        }
        if (strstr(response, "+CGNSINF:") != NULL) {
            parse_cgnsinf(response);
            info_parsed = true;
            break;
        }
    }
    if (!info_parsed) {
        a7608_debug_probe_gnss_commands();
    }

    const a7608_status_t *status = a7608_get_status();
    a7608_debug_printf("summary powered=%d fix=%d utc=%s lat=%s%c lon=%s%c last_error=%s\r\n",
                       status->gnss_powered,
                       status->gnss_fix,
                       status->gnss_utc[0] != '\0' ? status->gnss_utc : "-",
                       status->gnss_latitude[0] != '\0' ? status->gnss_latitude : "-",
                       status->gnss_ns != '\0' ? status->gnss_ns : ' ',
                       status->gnss_longitude[0] != '\0' ? status->gnss_longitude : "-",
                       status->gnss_ew != '\0' ? status->gnss_ew : ' ',
                       status->last_error[0] != '\0' ? status->last_error : "-");
    a7608_debug_write("GNSS remains enabled for manual AT+CGNSSINFO checks.\r\n");
    a7608_debug_write("[A7608 GNSS STATUS END]\r\n");
}

void a7608_at_debug_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI("A7608", "A7608 AT status task started");

    a7608_config_t config;
    a7608_get_default_config(&config);
    esp_err_t ret = a7608_init(&config);
    a7608_debug_write("\r\n[A7608 AT DEBUG]\r\n");
    a7608_debug_printf("UART%d baud=%d ESP_TX/MODEM_RX=%d ESP_RX/MODEM_TX=%d PWRKEY=%d(active=%d) RESET=%d(active=%d)\r\n",
                       config.uart_num,
                       config.baud_rate,
                       config.modem_tx_pin,
                       config.modem_rx_pin,
                       config.pwrkey_pin,
                       config.pwrkey_active_level,
                       config.reset_pin,
                       config.reset_active_level);

    if (ret != ESP_OK) {
        a7608_startup_probe_mark_started();
        a7608_startup_probe_mark_complete(ret);
        a7608_debug_printf("A7608 init failed: %s\r\n", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    a7608_startup_probe_mark_started();

    if (pin_is_valid(config.dtr_pin)) {
        a7608_debug_printf("Set DTR pin %d LOW before AT probe\r\n", config.dtr_pin);
        ret = a7608_set_dtr(true);
        if (ret != ESP_OK) {
            a7608_debug_printf("Set DTR failed: %s\r\n", esp_err_to_name(ret));
        } else {
            ESP_LOGI("A7608",
                     "A7608 startup DTR: gpio=%d level=%d",
                     config.dtr_pin,
                     a7608_get_dtr_level());
        }
    }

    ESP_LOGI("A7608", "A7608 startup phase: INITIAL_AT_PROBE boot_state=unknown");
    bool modem_ready_before_boot = a7608_startup_detect_running_modem();
    bool cold_boot = !modem_ready_before_boot;
    if (modem_ready_before_boot) {
        ESP_LOGI("A7608", "A7608 startup boot classification: warm");
    } else {
        ESP_LOGW("A7608", "A7608 startup boot classification: cold/unresponsive");
    }

    a7608_debug_printf("Control levels: PWRKEY=%d RESET=%d DTR=%d RING=%d\r\n",
                       gpio_get_level(config.pwrkey_pin),
                       gpio_get_level(config.reset_pin),
                       gpio_get_level(config.dtr_pin),
                       gpio_get_level(config.ring_pin));

    bool startup_ready = false;
    esp_err_t startup_ret = ESP_FAIL;
    bool at_stable = false;
    a7608_startup_sim_result_t startup_sim_result = A7608_STARTUP_SIM_TIMEOUT;
    if (modem_ready_before_boot) {
        startup_ready = a7608_startup_confirm_ready(false,
                                                    true,
                                                    "INITIAL_PROBE",
                                                    &at_stable,
                                                    &startup_sim_result,
                                                    &startup_ret,
                                                    A7608_STARTUP_AT_TIMEOUT_MS);
    }

    if (!startup_ready && !at_stable) {
        bool uart_silent = true;
        if (a7608_startup_try_exit_data_mode(&uart_silent)) {
            startup_ready = a7608_startup_confirm_ready(false,
                                                        true,
                                                        "PPP_ESCAPE",
                                                        &at_stable,
                                                        &startup_sim_result,
                                                        &startup_ret,
                                                        A7608_STARTUP_AT_TIMEOUT_MS);
        } else if (uart_silent) {
            ESP_LOGW("A7608", "A7608 startup UART silent after PPP escape; skip RESET and use PWRKEY");
        }

        if (!startup_ready && !at_stable && !uart_silent) {
            ESP_LOGW("A7608", "A7608 startup recovery stage=RESET");
            ret = a7608_hard_reset(A7608_STARTUP_RESET_PULSE_MS,
                                   A7608_STARTUP_RESET_QUIET_MS);
            if (ret == ESP_OK) {
                startup_ready = a7608_startup_confirm_ready(true,
                                                            false,
                                                            "RESET",
                                                            &at_stable,
                                                            &startup_sim_result,
                                                            &startup_ret,
                                                            A7608_STARTUP_RESET_AT_TIMEOUT_MS);
            } else {
                ESP_LOGE("A7608", "A7608 startup RESET recovery failed: %s", esp_err_to_name(ret));
            }
        }
    }

    if (!startup_ready && !at_stable) {
        cold_boot = true;
        at_stable = false;
        ESP_LOGW("A7608", "A7608 startup AT still down; recovery stage=PWRKEY");
        ESP_LOGW("A7608", "A7608 startup recovery stage=PWRKEY");
        ret = a7608_power_on(A7608_STARTUP_PWRKEY_PULSE_MS,
                             A7608_STARTUP_BOOT_QUIET_MS);
        if (ret == ESP_OK) {
            startup_ready = a7608_startup_confirm_ready(true,
                                                        false,
                                                        "PWRKEY",
                                                        &at_stable,
                                                        &startup_sim_result,
                                                        &startup_ret,
                                                        A7608_STARTUP_AT_TIMEOUT_MS);
        } else {
            ESP_LOGE("A7608", "A7608 startup PWRKEY recovery failed: %s", esp_err_to_name(ret));
        }
    }

    if (!startup_ready &&
        at_stable &&
        (startup_sim_result == A7608_STARTUP_SIM_PERSISTENT_FAILURE)) {
        (void)a7608_startup_radio_restart();
        startup_sim_result = a7608_startup_wait_sim_ready(cold_boot, true);
        if (startup_sim_result == A7608_STARTUP_SIM_READY) {
            startup_ready = a7608_startup_finish_ready(cold_boot,
                                                       "RADIO_RESTART",
                                                       &startup_ret);
        } else if ((startup_sim_result == A7608_STARTUP_SIM_AT_LOST) ||
                   (startup_sim_result == A7608_STARTUP_SIM_PERSISTENT_FAILURE)) {
            if (startup_sim_result == A7608_STARTUP_SIM_PERSISTENT_FAILURE) {
                ESP_LOGE("A7608", "A7608 persistent CME 13 after radio restart");
            } else {
                ESP_LOGE("A7608", "A7608 AT interface lost after radio restart");
            }
            ESP_LOGE("A7608", "A7608 SIM recovery action=HARD_RESET");
            at_stable = false;
            ret = a7608_hard_reset(A7608_STARTUP_RESET_PULSE_MS,
                                   A7608_STARTUP_RESET_QUIET_MS);
            if (ret == ESP_OK) {
                startup_ready = a7608_startup_confirm_ready(true,
                                                            false,
                                                            "SIM_HARD_RESET",
                                                            &at_stable,
                                                            &startup_sim_result,
                                                            &startup_ret,
                                                            A7608_STARTUP_AT_TIMEOUT_MS);
            } else {
                ESP_LOGE("A7608", "A7608 SIM recovery HARD_RESET failed: %s", esp_err_to_name(ret));
            }
        }
    }

    if (!startup_ready && !at_stable) {
        ESP_LOGW("A7608", "A7608 startup PWRKEY did not restore stable AT");
        ESP_LOGW("A7608", "A7608 startup recovery stage=RESET_RETRY");
        ret = a7608_hard_reset(A7608_STARTUP_RESET_PULSE_MS,
                               A7608_STARTUP_RESET_QUIET_MS);
        if (ret == ESP_OK) {
            startup_ready = a7608_startup_confirm_ready(true,
                                                        false,
                                                        "RESET_RETRY",
                                                        &at_stable,
                                                        &startup_sim_result,
                                                        &startup_ret,
                                                        A7608_STARTUP_AT_TIMEOUT_MS);
        } else {
            ESP_LOGE("A7608", "A7608 startup RESET retry failed: %s", esp_err_to_name(ret));
        }
    }

    if (startup_ready) {
        a7608_startup_log_phase("STARTUP_COMPLETE", cold_boot);
        ESP_LOGI("A7608",
                 "A7608 startup complete: slot=%s sim1_present=%d sim2_present=%d",
                 a7608_sim_slot_name(a7608_get_active_sim_slot()),
                 a7608_get_status()->sim1_present,
                 a7608_get_status()->sim2_present);
        a7608_startup_probe_mark_complete(startup_ret);
        a7608_debug_write("\r\nTransparent AT bridge ready. Type AT commands with CR/LF.\r\n");
#if HUB_LTE_PPPOS_ENABLE && HUB_LTE_PPPOS_TEST_MODE && HUB_LTE_PPPOS_REAL_RUNTIME && HUB_LTE_PPPOS_MANUAL_TEST
        a7608_debug_write("Skip GNSS status/probe in PPPoS manual test mode.\r\n");
#else
        a7608_debug_print_gnss_status();
#endif
    } else if (at_stable) {
        ESP_LOGE("A7608", "A7608 startup incomplete: AT stable but SIM not ready or status unknown");
        a7608_startup_probe_mark_complete(ESP_FAIL);
        a7608_debug_write("A7608 startup incomplete: AT stable, SIM not ready or status unknown.\r\n");
        a7608_debug_write("\r\nManual AT bridge ready, modem background read is paused until USB input is sent.\r\n");
    } else {
        ESP_LOGE("A7608", "A7608 startup timeout: recovery exhausted cold_boot=%d", cold_boot);
        a7608_startup_probe_mark_complete(ESP_FAIL);
        a7608_debug_write("A7608 startup failed after RESET/PWRKEY recovery.\r\n");
        a7608_debug_write("\r\nManual AT bridge ready, modem background read is paused until USB input is sent.\r\n");
    }

    uint8_t usb_buf[128];
    uint8_t usb_tx_buf[256];
    uint8_t modem_buf[128];
    bool last_usb_was_cr = false;
    TickType_t modem_read_until = 0;
    while (1) {
        if (a7608_service_state == A7608_SERVICE_PAUSE_REQUESTED) {
            (void)a7608_pause_service();
        }
        if (a7608_service_state == A7608_SERVICE_RESUME_REQUESTED) {
            (void)a7608_resume_service();
        }
        if (a7608_service_state == A7608_SERVICE_PAUSED) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        int usb_len = hub_usb_serial_read(usb_buf, sizeof(usb_buf), 10);
        if (usb_len > 0) {
            size_t tx_len = 0;
            for (int i = 0; i < usb_len; i++) {
                if ((usb_buf[i] == '\n') && !last_usb_was_cr) {
                    usb_tx_buf[tx_len++] = '\r';
                    usb_tx_buf[tx_len++] = '\n';
                } else {
                    usb_tx_buf[tx_len++] = usb_buf[i];
                }
                last_usb_was_cr = usb_buf[i] == '\r';
            }
            if (a7608_take_uart_lock(100)) {
                uart_write_bytes(a7608_cfg.uart_num, (const char *)usb_tx_buf, tx_len);
                a7608_give_uart_lock();
            }
            modem_read_until = xTaskGetTickCount() + pdMS_TO_TICKS(2500);
        }

        if (xTaskGetTickCount() < modem_read_until) {
            if (a7608_take_uart_lock(0)) {
                int modem_len = uart_read_bytes(a7608_cfg.uart_num, modem_buf, sizeof(modem_buf), pdMS_TO_TICKS(10));
                a7608_give_uart_lock();
                if (modem_len > 0) {
                    a7608_debug_write_modem_bytes(modem_buf, modem_len);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
