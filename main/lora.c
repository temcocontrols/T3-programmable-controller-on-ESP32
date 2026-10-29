#include "lora.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "controls.h"

/* Respect the application log policy: UART0 is also used for RS485. */
#define LORA_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        12
#define UART_RX_PIN        15
#define UART_BUF_SIZE      1024

#define LORA_STACK_SIZE    4096
#define LORA_TASK_PRIO     5
#define MAX_SENSORS        LORA_MAX_SENSORS

/* IN point mapping for AT+SENSOR fields (ordered) */
#define LORA_IN_BASE       0
#define LORA_IN_COUNT      9

static const char *TAG = "lora";

static lora_sensor_data_t sensors[MAX_SENSORS];

/* Only rx_task reads UART. tx_mutex serializes outgoing command lines.
 * The spinlock protects short state/cache copies; never wait while holding it. */
static SemaphoreHandle_t tx_mutex;
static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static lora_status_t status;
static TaskHandle_t rx_task_handle;
static bool lora_points_initialized;

static void parse_at_sensor_frame(const char *line);

static void count_event(uint32_t *counter)
{
    portENTER_CRITICAL(&state_lock);
    ++*counter;
    portEXIT_CRITICAL(&state_lock);
}

static void set_state(lora_state_t state, esp_err_t error)
{
    portENTER_CRITICAL(&state_lock);
    status.state = state;
    status.last_error = error;
    portEXIT_CRITICAL(&state_lock);
}

void lora_get_status(lora_status_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&state_lock);
    *out = status;
    portEXIT_CRITICAL(&state_lock);
}

static bool is_ready(void)
{
    lora_status_t current;
    lora_get_status(&current);
    return current.state == LORA_READY;
}

static void lora_init_input_points(void)
{
    static const char *desc[LORA_IN_COUNT] = {
        "LORA UID",
        "LORA TEMP C",
        "LORA RH %",
        "LORA VCAP V",
        "LORA CO2 ppm",
        "LORA TVOC ppb",
        "LORA LUX",
        "LORA RSSI",
        "LORA SNR"
    };

    static const char *label[LORA_IN_COUNT] = {
        "UID",
        "TEMP",
        "HUMIDITY",
        "VOLTAGE",
        "CO2",
        "TVOC",
        "LUX",
        "RSSI",
        "SNR"
    };

    static const uint8_t range[LORA_IN_COUNT] = {
        N0_2_32counts,      /* UID numeric */
        R10K_40_120DegC,    /* temperature */
        Humidty,            /* relative humidity */
        V0_5,               /* capacitor voltage */
        CO2_PPM,            /* CO2 */
        TVOC_PPB,           /* TVOC */
        LUX,                /* illuminance */
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

    LORA_LOG("Initializing LoRa input points IN%u-IN%u", LORA_IN_BASE + 1, LORA_IN_BASE + LORA_IN_COUNT);

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
    /* Keep the legacy raw uint32 bit pattern, but expose the exact hexadecimal
     * UID as text because the signed/scaled numeric point cannot represent it. */
    snprintf((char *)ptr.pin->description, sizeof(ptr.pin->description), "LORA UID %s", s->uid);
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
    ptr.pin->value = (int32_t)s->lux * 1000;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 7));
    ptr.pin->value = s->rssi * 1000;

    ptr = put_io_buf(IN, (uint8_t)(LORA_IN_BASE + 8));
    ptr.pin->value = s->snr * 1000;

    LORA_LOG("IN%u..IN%u updated uid=%08lX t=%ld rh=%lu vcap=%lu co2=%lu tvoc=%lu lux=%lu rssi=%ld snr=%ld",
             (unsigned)(LORA_IN_BASE + 1),
             (unsigned)(LORA_IN_BASE + LORA_IN_COUNT),
             (unsigned long)uid_num,
             (long)s->t_x100,
             (unsigned long)s->rh_x100,
             (unsigned long)s->vcap_mV,
             (unsigned long)s->co2_ppm,
             (unsigned long)s->tvoc_ppb,
             (unsigned long)s->lux,
             (long)s->rssi,
             (long)s->snr);
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
        return ESP_ERR_INVALID_STATE; /* Do not steal another driver. */
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
        uart_driver_delete(UART_PORT_NUM);
        return ret;
    }

    LORA_LOG("UART%d ready on TX=%d RX=%d baud=%d", UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    return ret;
}

bool lora_send_find_command(int addr, int duration)
{
    if (!is_ready() || !tx_mutex) return false;
    if (addr < 0 || addr > 255) addr = 255;
    if (duration <= 0) duration = 5;
    if (duration > 60) duration = 60;
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "AT+FIND=%d,%d\r\n", addr, duration);
    xSemaphoreTake(tx_mutex, portMAX_DELAY);
    bool ok = is_ready() && len > 0 && len < (int)sizeof(buf) &&
              uart_write_bytes(UART_PORT_NUM, buf, len) == len;
    xSemaphoreGive(tx_mutex);
    if (!ok) {
        portENTER_CRITICAL(&state_lock);
        ++status.tx_failures;
        status.last_error = ESP_FAIL;
        portEXIT_CRITICAL(&state_lock);
    }
    return ok;
}

static bool normalize_uid(const char *uid, char out[9])
{
    if (!uid || strlen(uid) != 8) return false;
    for (int i = 0; i < 8; ++i) {
        if (!isxdigit((unsigned char)uid[i])) return false;
        out[i] = (char)toupper((unsigned char)uid[i]);
    }
    out[8] = '\0';
    return true;
}

/* Full-token decimal validation, no empty fields, suffixes or signed overflow. */
static bool parse_number(const char *text, int32_t min, int32_t max, int32_t *out)
{
    if (!text || !*text) return false;
    const char *p = text;
    if (*p == '-' || *p == '+') ++p;
    if (!*p) return false;
    for (; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    errno = 0;
    char *end;
    long long value = strtoll(text, &end, 10);
    if (errno == ERANGE || *end || value < min || value > max) return false;
    *out = (int32_t)value;
    return true;
}

static void lora_handle_rx_line(char *line)
{
    while (*line == ' ') ++line;
    size_t len = strlen(line);
    while (len && line[len - 1] == ' ') line[--len] = '\0';
    if (!len) return;

    const char *sensor = strstr(line, "AT+SENSOR=");
    if (sensor) {
        parse_at_sensor_frame(sensor + strlen("AT+SENSOR="));
        return;
    }
    sensor = strstr(line, "SENSOR=");
    if (sensor) {
        parse_at_sensor_frame(sensor + strlen("SENSOR="));
        return;
    }

    if (strncmp(line, "AT+DROP=", 8) == 0) {
        count_event(&status.module_drop_frames);
    }
    if (strcmp(line, "Node init OK") == 0) {
        portENTER_CRITICAL(&state_lock);
        status.node_firmware_detected = true;
        portEXIT_CRITICAL(&state_lock);
        ESP_LOGE(TAG, "RA08 booted as a node; build module firmware with CONFIG_GATEWAY");
    }
    ESP_LOGD(TAG, "RX: %.80s", line);
}

/* Parse the payload after SENSOR=. Legacy frames have no application CRC.
 * Keep the wire protocol unchanged until the node firmware is agreed. */
static void parse_at_sensor_frame(const char *payload)
{
    char buf[160];
    if (strlen(payload) >= sizeof(buf)) goto invalid;
    strcpy(buf, payload);
    char *fields[9];
    fields[0] = buf;
    for (int i = 1; i < 9; ++i) {
        char *comma = strchr(fields[i - 1], ',');
        if (!comma) goto invalid;
        *comma = '\0';
        fields[i] = comma + 1;
    }
    if (strchr(fields[8], ',')) goto invalid;

    lora_sensor_data_t sample = {0};
    if (!normalize_uid(fields[0], sample.uid)) goto invalid;
    /* Match the custom module's binary wire types: int16 temperature,
     * uint16 measurements and int8 RSSI/SNR. RH is limited to 0..100%.
     * These limits also keep all int32 point scaling safe. */
    static const int32_t minimum[8] = {
        INT16_MIN, 0, 0, 0, 0, 0, INT8_MIN, INT8_MIN
    };
    static const int32_t maximum[8] = {
        INT16_MAX, 10000, UINT16_MAX, UINT16_MAX,
        UINT16_MAX, UINT16_MAX, INT8_MAX, INT8_MAX
    };
    int32_t values[8];
    for (int i = 0; i < 8; ++i) {
        if (!parse_number(fields[i + 1], minimum[i], maximum[i], &values[i])) goto invalid;
    }
    sample.t_x100 = values[0];
    sample.rh_x100 = (uint32_t)values[1];
    sample.vcap_mV = (uint32_t)values[2];
    sample.co2_ppm = (uint32_t)values[3];
    sample.tvoc_ppb = (uint32_t)values[4];
    sample.lux = (uint32_t)values[5];
    sample.rssi = values[6];
    sample.snr = values[7];
    sample.temperature_c = (float)sample.t_x100 / 100.0f;
    sample.humidity_rh = (float)sample.rh_x100 / 100.0f;
    sample.vcap_v = (float)sample.vcap_mV / 1000.0f;
    sample.valid = true;
    sample.online = true;
    sample.last_seen_ms = (uint64_t)esp_timer_get_time() / 1000;

    int slot = -1, empty = -1;
    portENTER_CRITICAL(&state_lock);
    for (int i = 0; i < MAX_SENSORS; ++i) {
        if (sensors[i].valid && strcmp(sensors[i].uid, sample.uid) == 0) {
            slot = i;
            break;
        }
        if (!sensors[i].valid && empty < 0) empty = i;
    }
    if (slot < 0) slot = empty;
    ++status.sensor_frames;
    status.last_sensor_ms = sample.last_seen_ms;
    if (slot >= 0) sensors[slot] = sample;
    else ++status.cache_full;
    portEXIT_CRITICAL(&state_lock);
    if (slot < 0) {
        ESP_LOGW(TAG, "Sensor cache full; UID %s ignored", sample.uid);
        return;
    }
    lora_publish_points(&sample);
    return;
invalid:
    count_event(&status.invalid_frames);
    ESP_LOGW(TAG, "Invalid SENSOR frame rejected");
}

static void update_age(lora_sensor_data_t *out)
{
    uint64_t age = (uint64_t)esp_timer_get_time() / 1000 - out->last_seen_ms;
    out->age_ms = age > UINT32_MAX ? UINT32_MAX : (uint32_t)age;
    out->online = age < LORA_SENSOR_TIMEOUT_MS;
}

bool lora_get_sensor_by_index(size_t index, lora_sensor_data_t *out)
{
    if (!out || index >= MAX_SENSORS) return false;
    portENTER_CRITICAL(&state_lock);
    bool found = sensors[index].valid;
    if (found) *out = sensors[index];
    portEXIT_CRITICAL(&state_lock);
    if (found) update_age(out);
    return found;
}

bool lora_get_sensor_data(const char *uid, lora_sensor_data_t *out)
{
    char normalized[9];
    if (!out || !normalize_uid(uid, normalized)) return false;
    bool found = false;
    portENTER_CRITICAL(&state_lock);
    for (int i = 0; i < MAX_SENSORS; ++i) {
        if (sensors[i].valid && strcmp(sensors[i].uid, normalized) == 0) {
            *out = sensors[i];
            found = true;
            break;
        }
    }
    portEXIT_CRITICAL(&state_lock);
    if (found) update_age(out);
    return found;
}

bool lora_send_payload_reliable(const char *payload, size_t len, int max_retries, uint32_t ack_timeout_ms)
{
    (void)payload;
    (void)len;
    (void)max_retries;
    (void)ack_timeout_ms;
    /* ra08-solar-polling does not implement CTX/MSG/ACK text transactions.
     * Keep the public symbol for callers, but never transmit unsupported data. */
    portENTER_CRITICAL(&state_lock);
    status.last_error = ESP_ERR_NOT_SUPPORTED;
    ++status.tx_failures;
    portEXIT_CRITICAL(&state_lock);
    ESP_LOGW(TAG, "Reliable payload TX is unsupported by the custom RA08 firmware");
    return false;
}

typedef struct {
    char line[UART_BUF_SIZE];
    size_t used;
    bool dropping;
} rx_stream_t;

/* Byte-wise assembly makes fragmentation independent of UART read boundaries.
 * On NUL/overflow discard the entire line through the next delimiter. */
static void rx_consume(rx_stream_t *stream, const uint8_t *bytes, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char c = (char)bytes[i];
        if (c == '\r' || c == '\n') {
            if (!stream->dropping && stream->used) {
                stream->line[stream->used] = '\0';
                lora_handle_rx_line(stream->line);
            }
            stream->used = 0;
            stream->dropping = false;
        } else if (!stream->dropping) {
            if (c == '\0' || stream->used == sizeof(stream->line) - 1) {
                stream->used = 0;
                stream->dropping = true;
                count_event(&status.dropped_lines);
            } else {
                stream->line[stream->used++] = c;
            }
        }
    }
}

static void rx_task(void *arg)
{
    uint8_t bytes[256];
    rx_stream_t stream = {0};
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, bytes, sizeof(bytes), pdMS_TO_TICKS(200));
        if (len > 0) rx_consume(&stream, bytes, (size_t)len);
        else if (len < 0) vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t lora_start(void)
{
    portENTER_CRITICAL(&state_lock);
    lora_state_t previous = status.state;
    if (previous != LORA_READY && previous != LORA_STARTING) status.state = LORA_STARTING;
    portEXIT_CRITICAL(&state_lock);
    if (previous == LORA_READY) return ESP_OK;
    if (previous == LORA_STARTING) return ESP_ERR_INVALID_STATE;

    esp_err_t ret;
    if (!rx_task_handle) {
        if (!tx_mutex) tx_mutex = xSemaphoreCreateMutex();
        if (!tx_mutex) {
            set_state(LORA_ERROR, ESP_ERR_NO_MEM);
            return ESP_ERR_NO_MEM;
        }
        ret = uart_init_at();
        if (ret != ESP_OK) {
            set_state(LORA_ERROR, ret);
            return ret;
        }
        lora_init_input_points();
        BaseType_t ok = xTaskCreate(rx_task, "lora_rx", LORA_STACK_SIZE, NULL,
                                   LORA_TASK_PRIO, &rx_task_handle);
        if (ok != pdPASS) {
            rx_task_handle = NULL;
            uart_driver_delete(UART_PORT_NUM);
            set_state(LORA_ERROR, ESP_ERR_NO_MEM);
            return ESP_ERR_NO_MEM;
        }
    }
    /* The custom gateway initializes its own RF channel and continuous RX.
     * It has no AT/OK handshake or CADDRSET/CTXADDRSET commands. */
    set_state(LORA_READY, ESP_OK);
    LORA_LOG("LoRa UART%d RX ready; waiting for custom RA08 SENSOR reports", UART_PORT_NUM);
    return ESP_OK;
}
