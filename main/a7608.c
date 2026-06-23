#include "a7608.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
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

extern int hub_usb_serial_read(uint8_t *buf, uint32_t length, uint32_t timeout_ms);
extern int hub_usb_serial_write(const uint8_t *buf, size_t length, uint32_t timeout_ms);

static bool pin_is_valid(gpio_num_t pin);
static int inactive_level(int active_level);
static bool a7608_ip_is_valid(const char *ip_addr);
static void a7608_sync_network_status(void);
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

static bool a7608_debug_read_modem(uint32_t timeout_ms, char *response, size_t response_len, bool echo)
{
    uint8_t buf[128];
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    bool received = false;

    if ((response != NULL) && (response_len > 0)) {
        response[0] = '\0';
    }

    while ((xTaskGetTickCount() - start) < timeout_ticks) {
        int len = uart_read_bytes(a7608_cfg.uart_num, buf, sizeof(buf), pdMS_TO_TICKS(50));
        if (len > 0) {
            received = true;
            if (echo) {
                a7608_debug_write_modem_bytes(buf, len);
            }
            if ((response != NULL) && (response_len > 1)) {
                size_t used = strlen(response);
                for (int i = 0; (i < len) && (used < response_len - 1); i++) {
                    if ((buf[i] == '\r') || (buf[i] == '\n') || (buf[i] == '\t') || ((buf[i] >= 0x20) && (buf[i] <= 0x7e))) {
                        response[used++] = (char)buf[i];
                    }
                }
                response[used] = '\0';
            }
        }
    }

    return received;
}

static bool a7608_debug_check_respond(void)
{
    char response[256];

    for (int i = 0; i < 10; i++) {
        int wrote = uart_write_bytes(a7608_cfg.uart_num, "AT\r\n", 4);
        if (wrote < 0) {
            a7608_debug_printf("AT probe write failed on UART%d\r\n", a7608_cfg.uart_num);
            continue;
        }
        if (a7608_debug_read_modem(1000, response, sizeof(response), false) && strstr(response, "OK") != NULL) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    a7608_debug_write("No OK after 10 AT probe attempts.\r\n");
    return false;
}

static esp_err_t a7608_debug_lilygo_boot_sequence(void)
{
    esp_err_t ret;

    if (pin_is_valid(a7608_cfg.reset_pin)) {
        a7608_debug_printf("Reset modem via pin %d\r\n", a7608_cfg.reset_pin);
        ret = gpio_set_level(a7608_cfg.reset_pin, inactive_level(a7608_cfg.reset_active_level));
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        ret = gpio_set_level(a7608_cfg.reset_pin, a7608_cfg.reset_active_level);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(2600));
        ret = gpio_set_level(a7608_cfg.reset_pin, inactive_level(a7608_cfg.reset_active_level));
        if (ret != ESP_OK) {
            return ret;
        }
    }

    if (pin_is_valid(a7608_cfg.dtr_pin)) {
        a7608_debug_printf("Set DTR pin %d LOW\r\n", a7608_cfg.dtr_pin);
        ret = a7608_set_dtr(true);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    if (pin_is_valid(a7608_cfg.pwrkey_pin)) {
        a7608_debug_printf("Power on modem via pin %d\r\n", a7608_cfg.pwrkey_pin);
        ret = gpio_set_level(a7608_cfg.pwrkey_pin, 0);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        ret = gpio_set_level(a7608_cfg.pwrkey_pin, 1);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        ret = gpio_set_level(a7608_cfg.pwrkey_pin, 0);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

static bool a7608_debug_sync_baudrate(void)
{
    static const int baudrates[] = {A7608_DEFAULT_BAUD_RATE};
    char response[256];

    for (size_t i = 0; i < sizeof(baudrates) / sizeof(baudrates[0]); i++) {
        int baudrate = baudrates[i];
        if (uart_set_baudrate(a7608_cfg.uart_num, baudrate) != ESP_OK) {
            continue;
        }
        a7608_cfg.baud_rate = baudrate;
        vTaskDelay(pdMS_TO_TICKS(10));
        a7608_debug_printf("Trying baud rate %d\r\n", baudrate);
        for (int attempt = 0; attempt < 10; attempt++) {
            uart_write_bytes(a7608_cfg.uart_num, "AT\r\n", 4);
            if (a7608_debug_read_modem(1000, response, sizeof(response), false) && strstr(response, "OK") != NULL) {
                a7608_debug_printf("Modem responded at rate:%d\r\n", baudrate);
                return true;
            }
        }
    }

    uart_set_baudrate(a7608_cfg.uart_num, A7608_DEFAULT_BAUD_RATE);
    a7608_cfg.baud_rate = A7608_DEFAULT_BAUD_RATE;

    return false;
}

static bool a7608_debug_send_snapshot_command(const char *cmd, uint32_t timeout_ms)
{
    char response[768];
    size_t used = 0;
    bool received = false;
    bool command_ok = false;
    uint8_t buf[128];

    a7608_debug_printf("\r\n>>> %s\r\n", cmd);
    (void)uart_flush_input(a7608_cfg.uart_num);
    int wrote = uart_write_bytes(a7608_cfg.uart_num, cmd, strlen(cmd));
    int wrote_crlf = uart_write_bytes(a7608_cfg.uart_num, "\r\n", 2);
    if ((wrote < 0) || (wrote_crlf < 0)) {
        a7608_debug_printf("UART write failed for %s\r\n", cmd);
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
    vTaskDelay(pdMS_TO_TICKS(200));

    return command_ok;
}

static bool a7608_debug_run_status_snapshot(void)
{
    static const struct {
        const char *cmd;
        uint32_t timeout_ms;
    } commands[] = {
        {"AT", 1000},
        {"ATI", 2000},
        {"AT+CPIN?", 3000},
        {"AT+CSQ", 3000},
        {"AT+CEREG?", 3000},
        {"AT+CREG?", 3000},
        {"AT+CGATT?", 3000},
        {"AT+COPS?", 5000},
        {"AT+CGDCONT?", 3000},
        {"AT+CGPADDR", 3000},
    };

    a7608_debug_write("\r\n[A7608 STATUS SNAPSHOT]\r\n");
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        (void)a7608_debug_send_snapshot_command(commands[i].cmd, commands[i].timeout_ms);
        if (strcmp(commands[i].cmd, "ATI") == 0) {
            a7608_debug_write("\r\nWaiting 5 seconds for SIM/network stack to settle...\r\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            a7608_debug_write("Rechecking modem response after ATI settle wait...\r\n");
            if (!a7608_debug_send_snapshot_command("AT", 1000)) {
                a7608_debug_write("Modem lost after ATI settle wait; skip remaining status snapshot.\r\n");
                a7608_debug_write("\r\n[A7608 STATUS SNAPSHOT END]\r\n");
                return false;
            }
        }
    }
    a7608_debug_write("\r\n[A7608 STATUS SNAPSHOT END]\r\n");
    return true;
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
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
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

static bool response_has_registered(const char *response)
{
    const char *line = strstr(response, "+CEREG:");
    if (line == NULL) {
        line = strstr(response, "+CREG:");
    }
    if (line == NULL) {
        return false;
    }

    int n = 0;
    int stat = 0;
    if (sscanf(line, "%*[^:]: %d,%d", &n, &stat) == 2) {
        a7608_status.registered_home = (stat == 1);
        a7608_status.registered_roaming = (stat == 5);
        return (stat == 1) || (stat == 5);
    }
    return false;
}

static void parse_csq(const char *response)
{
    const char *line = strstr(response, "+CSQ:");
    int csq = 99;
    int ber = 99;
    if ((line != NULL) && (sscanf(line, "%*[^:]: %d,%d", &csq, &ber) == 2)) {
        a7608_status.csq = csq;
        a7608_status.rssi_dbm = (csq >= 0 && csq <= 31) ? (-113 + (2 * csq)) : 0;
    }
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

static bool a7608_ip_is_valid(const char *ip_addr)
{
    return (ip_addr != NULL) && (ip_addr[0] != '\0') && (strcmp(ip_addr, "0.0.0.0") != 0);
}

static void a7608_sync_network_status(void)
{
    bool connected = a7608_status.connected && a7608_ip_is_valid(a7608_status.ip_addr);
    const char *ip_addr = connected ? a7608_status.ip_addr : NULL;

    (void)hub_lte_pppos_set_connected(connected, ip_addr);
    hub_network_manager_set_lte_status(connected, ip_addr);
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
    }

    a7608_initialized = true;
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
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.pwrkey_pin, a7608_cfg.pwrkey_active_level), "A7608", "pwrkey active failed");
    vTaskDelay(pdMS_TO_TICKS(pulse_ms));
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.pwrkey_pin, inactive_level(a7608_cfg.pwrkey_active_level)), "A7608", "pwrkey inactive failed");
    vTaskDelay(pdMS_TO_TICKS(boot_wait_ms));
    return ESP_OK;
}

esp_err_t a7608_hard_reset(uint32_t pulse_ms, uint32_t boot_wait_ms)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.reset_pin)) {
        return ESP_ERR_INVALID_STATE;
    }

    a7608_status.state = A7608_STATE_BOOTING;
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.reset_pin, a7608_cfg.reset_active_level), "A7608", "reset active failed");
    vTaskDelay(pdMS_TO_TICKS(pulse_ms));
    ESP_RETURN_ON_ERROR(gpio_set_level(a7608_cfg.reset_pin, inactive_level(a7608_cfg.reset_active_level)), "A7608", "reset inactive failed");
    vTaskDelay(pdMS_TO_TICKS(boot_wait_ms));
    return ESP_OK;
}

esp_err_t a7608_set_dtr(bool active)
{
    if (!a7608_initialized || !pin_is_valid(a7608_cfg.dtr_pin)) {
        return ESP_ERR_INVALID_STATE;
    }
    int level = active ? a7608_cfg.dtr_active_level : inactive_level(a7608_cfg.dtr_active_level);
    return gpio_set_level(a7608_cfg.dtr_pin, level);
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

    char local_response[512];
    char *out = response != NULL ? response : local_response;
    size_t out_len = response != NULL ? response_len : sizeof(local_response);
    if (out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    out[0] = '\0';

    (void)uart_flush_input(a7608_cfg.uart_num);
    int wrote = uart_write_bytes(a7608_cfg.uart_num, cmd, strlen(cmd));
    int wrote_crlf = uart_write_bytes(a7608_cfg.uart_num, "\r\n", 2);
    if ((wrote < 0) || (wrote_crlf < 0)) {
        set_last_error("uart_write_bytes failed");
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
            return ESP_OK;
        }
        if (strstr(out, "ERROR") != NULL) {
            set_last_error("modem returned ERROR");
            return ESP_FAIL;
        }
    }

    set_last_error("AT command timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t a7608_probe(void)
{
    char response[128];
    esp_err_t ret = a7608_send_command("AT", "OK", 1000, response, sizeof(response));
    if (ret == ESP_OK) {
        a7608_status.at_ready = true;
        a7608_status.state = A7608_STATE_AT_READY;
    } else {
        a7608_status.at_ready = false;
        a7608_status.state = A7608_STATE_ERROR;
    }
    return ret;
}

esp_err_t a7608_refresh_status(void)
{
    char response[512];
    esp_err_t first_error = ESP_OK;

    if (a7608_probe() != ESP_OK) {
        a7608_status.connected = false;
        a7608_status.ip_addr[0] = '\0';
        a7608_sync_network_status();
        return ESP_FAIL;
    }

    if (a7608_send_command("AT+CPIN?", "OK", 1500, response, sizeof(response)) == ESP_OK) {
        a7608_status.sim_ready = strstr(response, "READY") != NULL;
        if (a7608_status.sim_ready) {
            a7608_status.state = A7608_STATE_SIM_READY;
        }
    } else {
        first_error = ESP_FAIL;
    }

    if (a7608_send_command("AT+CSQ", "OK", 1500, response, sizeof(response)) == ESP_OK) {
        parse_csq(response);
    } else if (first_error == ESP_OK) {
        first_error = ESP_FAIL;
    }

    bool registered = false;
    if (a7608_send_command("AT+CEREG?", "OK", 1500, response, sizeof(response)) == ESP_OK) {
        registered = response_has_registered(response);
    }
    if (!registered && (a7608_send_command("AT+CREG?", "OK", 1500, response, sizeof(response)) == ESP_OK)) {
        registered = response_has_registered(response);
    }
    if (registered) {
        a7608_status.state = A7608_STATE_REGISTERED;
    }

    if (a7608_send_command("AT+CGATT?", "OK", 1500, response, sizeof(response)) == ESP_OK) {
        a7608_status.attached = strstr(response, "+CGATT: 1") != NULL;
        if (a7608_status.attached) {
            a7608_status.state = A7608_STATE_ATTACHED;
        }
    } else if (first_error == ESP_OK) {
        first_error = ESP_FAIL;
    }

    if (a7608_send_command("AT+COPS?", "OK", 1500, response, sizeof(response)) == ESP_OK) {
        parse_operator(response);
    }

    if (a7608_send_command("AT+CGPADDR", "OK", 2000, response, sizeof(response)) == ESP_OK) {
        parse_ip_addr(response);
        a7608_status.connected = a7608_ip_is_valid(a7608_status.ip_addr);
        if (a7608_status.connected) {
            a7608_status.state = A7608_STATE_CONNECTED;
        }
    } else if (first_error == ESP_OK) {
        a7608_status.connected = false;
        a7608_status.ip_addr[0] = '\0';
        first_error = ESP_FAIL;
    }

    a7608_sync_network_status();

    return first_error;
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

static void a7608_debug_print_parsed_status(void)
{
    esp_err_t ret = a7608_refresh_status();
    const a7608_status_t *status = a7608_get_status();

    a7608_debug_write("\r\n[A7608 PARSED STATUS]\r\n");
    a7608_debug_printf("refresh=%s state=%s at=%d sim=%d reg_home=%d reg_roam=%d attached=%d connected=%d\r\n",
                       esp_err_to_name(ret),
                       a7608_state_name(status->state),
                       status->at_ready,
                       status->sim_ready,
                       status->registered_home,
                       status->registered_roaming,
                       status->attached,
                       status->connected);
    a7608_debug_printf("csq=%d rssi_dbm=%d operator=%s ip=%s last_error=%s\r\n",
                       status->csq,
                       status->rssi_dbm,
                       status->operator_name[0] != '\0' ? status->operator_name : "-",
                       status->ip_addr[0] != '\0' ? status->ip_addr : "-",
                       status->last_error[0] != '\0' ? status->last_error : "-");
    a7608_debug_write("[A7608 PARSED STATUS END]\r\n");
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
        a7608_debug_printf("A7608 init failed: %s\r\n", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    bool modem_ready_before_boot = false;
    if (pin_is_valid(config.dtr_pin)) {
        a7608_debug_printf("Set DTR pin %d LOW before AT probe\r\n", config.dtr_pin);
        ret = a7608_set_dtr(true);
        if (ret != ESP_OK) {
            a7608_debug_printf("Set DTR failed: %s\r\n", esp_err_to_name(ret));
        }
    }

    a7608_debug_write("Checking modem response before power key pulse...\r\n");
    modem_ready_before_boot = a7608_debug_check_respond();
    if (modem_ready_before_boot) {
        a7608_debug_write("Modem already responds; skip reset/PWRKEY boot sequence.\r\n");
    } else {
        ret = a7608_debug_lilygo_boot_sequence();
        if (ret != ESP_OK) {
            a7608_debug_printf("LilyGO boot sequence failed: %s\r\n", esp_err_to_name(ret));
        }
    }

    a7608_debug_printf("Control levels: PWRKEY=%d RESET=%d DTR=%d RING=%d\r\n",
                       gpio_get_level(config.pwrkey_pin),
                       gpio_get_level(config.reset_pin),
                       gpio_get_level(config.dtr_pin),
                       gpio_get_level(config.ring_pin));

    a7608_debug_write("Checking modem response...\r\n");
    bool at_ready = a7608_debug_check_respond();
    if (!at_ready) {
        a7608_debug_write("Wait modem started...\r\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
        at_ready = a7608_debug_sync_baudrate();
    }

    if (at_ready) {
        a7608_debug_write("\r\nTransparent AT bridge ready. Type AT commands with CR/LF.\r\n");
        bool snapshot_ok = a7608_debug_run_status_snapshot();
        if (snapshot_ok) {
            a7608_debug_print_parsed_status();
            a7608_debug_print_gnss_status();
        } else {
            a7608_debug_write("\r\nSkip parsed status because modem did not stay responsive after ATI.\r\n");
        }
    } else {
        a7608_debug_write("A7608 did not return OK after LilyGO ATDebug boot sequence.\r\n");
        a7608_debug_write("\r\nManual AT bridge ready, modem background read is paused until USB input is sent.\r\n");
    }

    uint8_t usb_buf[128];
    uint8_t usb_tx_buf[256];
    uint8_t modem_buf[128];
    bool last_usb_was_cr = false;
    TickType_t modem_read_until = 0;
    while (1) {
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
            uart_write_bytes(a7608_cfg.uart_num, (const char *)usb_tx_buf, tx_len);
            modem_read_until = xTaskGetTickCount() + pdMS_TO_TICKS(2500);
        }

        if (at_ready || (xTaskGetTickCount() < modem_read_until)) {
            int modem_len = uart_read_bytes(a7608_cfg.uart_num, modem_buf, sizeof(modem_buf), pdMS_TO_TICKS(10));
            if (modem_len > 0) {
                a7608_debug_write_modem_bytes(modem_buf, modem_len);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
