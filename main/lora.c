#include "lora.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "controls.h"

/* Debug logging - use printf directly to ensure output */
#define LORA_LOG(fmt, ...) do { printf("[LORA] " fmt "\n", ##__VA_ARGS__); fflush(stdout); } while(0)

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        12
#define UART_RX_PIN        15
#define UART_BUF_SIZE      1024

#define LORA_STACK_SIZE    4096
#define LORA_TASK_PRIO     5
#define MAX_SENSORS        16

/* IN point mapping for AT+SENSOR fields (ordered) */
#define LORA_IN_BASE       0
#define LORA_IN_COUNT      8

static const char *TAG = "lora";

static lora_sensor_data_t sensors[MAX_SENSORS];

static SemaphoreHandle_t ack_sem;
static volatile uint16_t ack_seq;
static uint16_t next_seq = 1;
static TaskHandle_t rx_task_handle;
static bool lora_points_initialized;

static void parse_at_sensor_frame(const char *line);

static void lora_init_input_points(void)
{
    static const char *desc[LORA_IN_COUNT] = {
        "LORA UID",
        "LORA T x100",
        "LORA RH x100",
        "LORA VCAP mV",
        "LORA CO2 ppm",
        "LORA TVOC ppb",
        "LORA RSSI",
        "LORA SNR"
    };

    static const char *label[LORA_IN_COUNT] = {
        "L_UID",
        "L_TX100",
        "L_RHX100",
        "L_VCAP",
        "L_CO2",
        "L_TVOC",
        "L_RSSI",
        "L_SNR"
    };

    static const uint8_t range[LORA_IN_COUNT] = {
        N0_2_32counts,      /* UID numeric */
        R10K_40_120DegC,    /* temperature */
        Humidty,            /* relative humidity */
        V0_5,               /* capacitor voltage */
        CO2_PPM,            /* CO2 */
        TVOC_PPB,           /* TVOC */
        DB,                 /* RSSI */
        DB                  /* SNR */
    };

    if (lora_points_initialized) {
        LORA_LOG("Input points already initialized");
        return;
    }

    if ((LORA_IN_BASE + LORA_IN_COUNT) > MAX_INS) {
        LORA_LOG("ERROR: LoRa IN mapping OUT OF RANGE: base=%d count=%d MAX=%d", LORA_IN_BASE, LORA_IN_COUNT, MAX_INS);
        return;
    }

    LORA_LOG("Initializing LoRa input points IN%u-IN%u", LORA_IN_BASE, LORA_IN_BASE + LORA_IN_COUNT - 1);

    for (int i = 0; i < LORA_IN_COUNT; i++) {
        Str_points_ptr ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + i));
        ptr.pin->digital_analog = 1;
        ptr.pin->range = range[i];
        memcpy(ptr.pin->description, desc[i], strlen(desc[i]) + 1);
        memcpy(ptr.pin->label, label[i], strlen(label[i]) + 1);
        ptr.pin->value = 0;
    }

    lora_points_initialized = true;
}

static void lora_publish_points(const lora_sensor_data_t *s)
{
    Str_points_ptr ptr;
    uint32_t uid_num;

    if (!s || !lora_points_initialized) {
        return;
    }

    uid_num = (uint32_t)strtoul(s->uid, NULL, 16);

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 0));
    ptr.pin->value = (int32_t)uid_num;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 1));
    ptr.pin->value = s->t_x100 * 10;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 2));
    ptr.pin->value = (int32_t)s->rh_x100 * 10;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 3));
    ptr.pin->value = (int32_t)s->vcap_mV;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 4));
    ptr.pin->value = (int32_t)s->co2_ppm * 1000;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 5));
    ptr.pin->value = (int32_t)s->tvoc_ppb * 1000;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 6));
    ptr.pin->value = s->rssi * 1000;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 7));
    ptr.pin->value = s->snr * 1000;

    LORA_LOG("IN%u..IN%u updated uid=%08lX t=%ld rh=%lu vcap=%lu co2=%lu tvoc=%lu rssi=%ld snr=%ld",
             (unsigned)(LORA_IN_BASE + 1),
             (unsigned)(LORA_IN_BASE + LORA_IN_COUNT),
             (unsigned long)uid_num,
             (long)s->t_x100,
             (unsigned long)s->rh_x100,
             (unsigned long)s->vcap_mV,
             (unsigned long)s->co2_ppm,
             (unsigned long)s->tvoc_ppb,
             (long)s->rssi,
             (long)s->snr);
}

static void __attribute__((unused)) lora_publish_boot_signature(void)
{
    lora_sensor_data_t s;

    memset(&s, 0, sizeof(s));
    memcpy(s.uid, "A1B2C3D4", 8);
    s.uid[8] = '\0';
    s.t_x100 = 2234;
    s.rh_x100 = 5566;
    s.vcap_mV = 3300;
    s.co2_ppm = 888;
    s.tvoc_ppb = 123;
    s.rssi = -70;
    s.snr = 9;
    s.valid = true;

    lora_publish_points(&s);
    LORA_LOG("Boot signature written to IN%u..IN%u for Modbus mapping check",
             (unsigned)(LORA_IN_BASE + 1),
             (unsigned)(LORA_IN_BASE + LORA_IN_COUNT));
}

static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static esp_err_t uart_init_at(void)
{
    uart_config_t cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    if (uart_is_driver_installed(UART_PORT_NUM)) {
        (void)uart_driver_delete(UART_PORT_NUM);
    }

    esp_err_t ret = uart_param_config(UART_PORT_NUM, &cfg);
    if (ret != ESP_OK) {
        LORA_LOG("uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        LORA_LOG("uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        LORA_LOG("uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_mode(UART_PORT_NUM, UART_MODE_UART);
    if (ret != ESP_OK) {
        LORA_LOG("uart_set_mode failed: %s", esp_err_to_name(ret));
        return ret;
    }

    LORA_LOG("UART%d ready on TX=%d RX=%d baud=%d", UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    return ret;
}

static bool at_send_and_wait(const char *cmd, const char *wait_for, uint32_t timeout_ms)
{
    if (!cmd || !wait_for) {
        return false;
    }

    uart_flush(UART_PORT_NUM);
    int wrote = uart_write_bytes(UART_PORT_NUM, cmd, strlen(cmd));
    int wrote_crlf = uart_write_bytes(UART_PORT_NUM, "\r\n", 2);
    if ((wrote < 0) || (wrote_crlf < 0)) {
        LORA_LOG("uart_write_bytes failed for '%s': wrote=%d crlf=%d", cmd, wrote, wrote_crlf);
    }

    uint8_t buf[UART_BUF_SIZE];
    uint32_t elapsed = 0;
    const TickType_t tick = pdMS_TO_TICKS(100);

    while (elapsed < timeout_ms) {
        int r = uart_read_bytes(UART_PORT_NUM, buf, sizeof(buf) - 1, tick);
        if (r > 0) {
            buf[r] = 0;
            if (strstr((char *)buf, wait_for) != NULL) {
                return true;
            }
        }
        elapsed += 100;
    }

    return false;
}

static bool at_send_cmd_retry(const char *cmd, const char *wait_for, uint32_t timeout_ms, int retries)
{
    for (int i = 0; i < retries; i++) {
        if (at_send_and_wait(cmd, wait_for, timeout_ms)) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return false;
}

static bool configure_ra08(void)
{
    if (!at_send_cmd_retry("AT", "OK", 1000, 3)) {
        LORA_LOG("WARNING: RA08 not responding to AT");
        return false;
    }

    if (!at_send_cmd_retry("AT+CADDRSET=0", "OK", 500, 3)) {
        return false;
    }

    if (!at_send_cmd_retry("AT+CTXADDRSET=1", "OK", 500, 3)) {
        return false;
    }

    return true;
}

void lora_send_find_command(int addr, int duration)
{
    if (addr < 0 || addr > 255) {
        addr = 255;
    }
    if (duration <= 0) {
        duration = 5;
    }
    if (duration > 60) {
        duration = 60;
    }

    char buf[32];
    int len = snprintf(buf, sizeof(buf), "AT+FIND=%d,%d\r\n", addr, duration);
    if (len > 0) {
        uart_write_bytes(UART_PORT_NUM, buf, len);
    }
}

static bool uid_is_valid_hex8(const char *uid)
{
    if (!uid || strlen(uid) != 8) {
        return false;
    }

    for (int i = 0; i < 8; i++) {
        if (!isxdigit((unsigned char)uid[i])) {
            return false;
        }
    }

    return true;
}

static void lora_handle_rx_line(char *line)
{
    if (!line) {
        return;
    }

    while (*line == '\r' || *line == '\n' || *line == ' ') {
        line++;
    }

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ')) {
        line[--len] = '\0';
    }

    if (len == 0) {
        return;
    }

    /* Some RA08 firmwares wrap payloads; accept AT+SENSOR anywhere in line. */
    char *sensor = strstr(line, "AT+SENSOR=");
    if (sensor) {
        parse_at_sensor_frame(sensor);
        return;
    }

    /* Fallback for payloads that start directly from SENSOR=. */
    sensor = strstr(line, "SENSOR=");
    if (sensor) {
        char tmp[160];
        int n = snprintf(tmp, sizeof(tmp), "AT+%s", sensor);
        if (n > 0 && n < (int)sizeof(tmp)) {
            parse_at_sensor_frame(tmp);
        }
        return;
    }

    LORA_LOG("RX line: %.80s", line);

    /* All other frames use trailing :crc field */
    char *last_colon = strrchr(line, ':');
    if (!last_colon) {
        return;
    }

    uint16_t rx_crc = (uint16_t)atoi(last_colon + 1);
    *last_colon = '\0';
    uint16_t calc_crc = crc16_ccitt((const uint8_t *)line, strlen(line));
    if (calc_crc != rx_crc) {
        return;
    }

    char *ack = strstr(line, "ACK:");
    if (ack) {
        ack_seq = (uint16_t)atoi(ack + 4);
        if (ack_sem) {
            xSemaphoreGive(ack_sem);
        }
    } else {
        ESP_LOGI(TAG, "RA08 frame: %s", line);
    }
}

/*
 * Parse:
 * AT+SENSOR=UID,T_x100,RH_x100,Vcap_mV,CO2_ppm,TVOC_ppb,RSSI,SNR\r\n
 * The line passed in has already had trailing '\n' stripped.
 */
static void parse_at_sensor_frame(const char *line)
{
    LORA_LOG("parse_at_sensor_frame called with: %.50s", line);
    /* skip "AT+SENSOR=" prefix (10 chars) */
    const char *p = line + 10;

    char buf[128];
    strncpy(buf, p, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* strip trailing \r if present */
    size_t blen = strlen(buf);
    if (blen > 0 && buf[blen - 1] == '\r') {
        buf[blen - 1] = '\0';
    }

    char *tok = strtok(buf, ",");
    if (!tok) return;
    char uid[9];
    strncpy(uid, tok, sizeof(uid) - 1);
    uid[sizeof(uid) - 1] = '\0';
    if (!uid_is_valid_hex8(uid)) {
        LORA_LOG("ERROR: INVALID UID (not 8 hex chars): %s", uid);
        return;
    }
    LORA_LOG("UID valid: %s, starting parse of 8 fields", uid);

    tok = strtok(NULL, ","); if (!tok) return;
    int32_t t_x100 = (int32_t)strtol(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    uint32_t rh_x100 = (uint32_t)strtoul(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    uint32_t vcap_mV = (uint32_t)strtoul(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    uint32_t co2_ppm = (uint32_t)strtoul(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    uint32_t tvoc_ppb = (uint32_t)strtoul(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    int32_t rssi = (int32_t)strtol(tok, NULL, 10);

    tok = strtok(NULL, ","); if (!tok) return;
    int32_t snr = (int32_t)strtol(tok, NULL, 10);

    /* find existing slot for this UID or claim an empty one */
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (!sensors[i].valid || strncmp(sensors[i].uid, uid, sizeof(sensors[i].uid)) == 0) {
            strncpy(sensors[i].uid, uid, sizeof(sensors[i].uid) - 1);
            sensors[i].uid[sizeof(sensors[i].uid) - 1] = '\0';
            sensors[i].t_x100      = t_x100;
            sensors[i].rh_x100     = rh_x100;
            sensors[i].vcap_mV     = vcap_mV;
            sensors[i].co2_ppm     = co2_ppm;
            sensors[i].tvoc_ppb    = tvoc_ppb;
            sensors[i].rssi        = rssi;
            sensors[i].snr         = snr;
            sensors[i].temperature_c = (float)t_x100 / 100.0f;
            sensors[i].humidity_rh   = (float)rh_x100 / 100.0f;
            sensors[i].vcap_v        = (float)vcap_mV / 1000.0f;
            sensors[i].valid       = true;
            lora_publish_points(&sensors[i]);
            ESP_LOGI(TAG,
                     "SENSOR uid=%s T=%.2fC RH=%.2f%% Vcap=%.3fV CO2=%u TVOC=%u RSSI=%ld SNR=%ld",
                     uid,
                     sensors[i].temperature_c,
                     sensors[i].humidity_rh,
                     sensors[i].vcap_v,
                     (unsigned)sensors[i].co2_ppm,
                     (unsigned)sensors[i].tvoc_ppb,
                     (long)sensors[i].rssi,
                     (long)sensors[i].snr);
            break;
        }
    }
}

bool lora_get_sensor_data(const char *uid, lora_sensor_data_t *out)
{
    if (!uid || !out) {
        return false;
    }

    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensors[i].valid && strncmp(sensors[i].uid, uid, sizeof(sensors[i].uid)) == 0) {
            *out = sensors[i];
            return true;
        }
    }

    return false;
}

bool lora_send_payload_reliable(const char *payload, size_t len, int max_retries, uint32_t ack_timeout_ms)
{
    if (!payload || len == 0 || !ack_sem) {
        return false;
    }

    uint16_t seq = next_seq++;
    if (next_seq == 0) {
        next_seq = 1;
    }

    char body[512];
    int body_len = snprintf(body, sizeof(body), "MSG:%u:", seq);
    if (body_len < 0) {
        return false;
    }
    if ((size_t)body_len + len + 1 > sizeof(body)) {
        return false;
    }

    memcpy(body + body_len, payload, len);
    body_len += (int)len;
    body[body_len] = '\0';

    uint16_t crc = crc16_ccitt((const uint8_t *)body, (size_t)body_len);

    char frame[600];
    int frame_len = snprintf(frame, sizeof(frame), "%s:%u\n", body, (unsigned)crc);
    if (frame_len <= 0 || frame_len >= (int)sizeof(frame)) {
        return false;
    }

    for (int attempt = 0; attempt < max_retries; attempt++) {
        if (!at_send_and_wait("AT+CTX=868000000,5,0,1,14,0", ">", 2000)) {
            continue;
        }

        uart_write_bytes(UART_PORT_NUM, frame, frame_len);
        vTaskDelay(pdMS_TO_TICKS(50));

        if (!at_send_and_wait("+++", "OK", 1000)) {
            continue;
        }

        if (xSemaphoreTake(ack_sem, pdMS_TO_TICKS(ack_timeout_ms)) == pdTRUE && ack_seq == seq) {
            return true;
        }
    }

    return false;
}

static void rx_task(void *arg)
{
    uint8_t rbuf[256];
    uint8_t accum[UART_BUF_SIZE];
    size_t idx = 0;

    while (1) {
        int r = uart_read_bytes(UART_PORT_NUM, rbuf, sizeof(rbuf), pdMS_TO_TICKS(200));
        if (r > 0) {
            if (idx + (size_t)r >= sizeof(accum)) {
                idx = 0;
            }

            memcpy(accum + idx, rbuf, (size_t)r);
            idx += (size_t)r;

            while (idx > 0) {
                size_t j;
                bool found = false;

                for (j = 0; j < idx; j++) {
                    if (accum[j] == '\n' || accum[j] == '\r') {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    break;
                }

                accum[j] = '\0';
                lora_handle_rx_line((char *)accum);

                size_t skip = 1;
                while ((j + skip) < idx && (accum[j + skip] == '\n' || accum[j + skip] == '\r')) {
                    skip++;
                }

                size_t rem = idx - (j + skip);
                if (rem > 0) {
                    memmove(accum, accum + j + skip, rem);
                }
                idx = rem;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t lora_start(void)
{
    if (rx_task_handle) {
        return ESP_OK;
    }

    memset(sensors, 0, sizeof(sensors));

    esp_err_t ret = uart_init_at();
    if (ret != ESP_OK) {
        LORA_LOG("ERROR: uart_init_at failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!ack_sem) {
        ack_sem = xSemaphoreCreateBinary();
        if (!ack_sem) {
            return ESP_ERR_NO_MEM;
        }
    }

    lora_init_input_points();
    //lora_publish_boot_signature();

    vTaskDelay(pdMS_TO_TICKS(200));
    (void)configure_ra08();

    BaseType_t ok = xTaskCreate(rx_task, "lora_rx", LORA_STACK_SIZE, NULL, LORA_TASK_PRIO, &rx_task_handle);
    if (ok != pdPASS) {
        rx_task_handle = NULL;
        return ESP_FAIL;
    }

    LORA_LOG("LoRa RX started on UART%d (TX=%d, RX=%d, %d)",
             UART_PORT_NUM,
             UART_TX_PIN,
             UART_RX_PIN,
             UART_BAUD_RATE);
    return ESP_OK;
}
