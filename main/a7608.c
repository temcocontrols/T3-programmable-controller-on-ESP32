#include "a7608.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

#define A7608_LOG(fmt, ...) do { printf("[A7608] " fmt "\n", ##__VA_ARGS__); fflush(stdout); } while (0)

static a7608_config_t a7608_cfg;
static a7608_status_t a7608_status;
static bool a7608_initialized;

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
    config->reset_active_level = 1;
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
                       a7608_cfg.modem_rx_pin,
                       a7608_cfg.modem_tx_pin,
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
        a7608_status.connected = a7608_status.ip_addr[0] != '\0';
        if (a7608_status.connected) {
            a7608_status.state = A7608_STATE_CONNECTED;
        }
    } else if (first_error == ESP_OK) {
        first_error = ESP_FAIL;
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
