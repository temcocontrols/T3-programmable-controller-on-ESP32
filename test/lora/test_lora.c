/* This includes an unchanged staged copy of production lora.c. Hardware and
 * RTOS calls are replaced with deterministic fakes, not the protocol logic. */
#include <assert.h>
#include "lora.c"

struct test_mutex { bool held; };
static int64_t now_us;
static bool driver_installed, task_failure, allocation_failure;
static unsigned task_creates, uart_reads, uart_writes;
static rx_stream_t stream;
static Str_in_point points[MAX_INS];
static char last_command[96];
static bool short_write;

static void feed(const char *text)
{
    /* Worst-case fragmentation: every byte is delivered separately. */
    for (size_t i = 0; text[i]; ++i) rx_consume(&stream, (const uint8_t *)&text[i], 1);
}

int64_t esp_timer_get_time(void) { return now_us; }
TickType_t xTaskGetTickCount(void) { return (TickType_t)(now_us / 1000); }
void vTaskDelay(TickType_t ticks) { now_us += (int64_t)ticks * 1000; }
SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return allocation_failure ? NULL : calloc(1, sizeof(struct test_mutex));
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t mutex, TickType_t timeout)
{
    (void)timeout;
    assert(state_lock == 0 && mutex && !mutex->held);
    mutex->held = true;
    return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex)
{
    assert(state_lock == 0 && mutex && mutex->held);
    mutex->held = false;
    return pdTRUE;
}
BaseType_t xTaskCreate(void (*task)(void *), const char *name, unsigned stack,
                      void *arg, unsigned priority, TaskHandle_t *handle)
{
    (void)task; (void)name; (void)stack; (void)arg; (void)priority;
    ++task_creates;
    if (task_failure) return pdFALSE;
    *handle = (void *)1;
    return pdPASS;
}
bool uart_is_driver_installed(int port) { assert(port == 1); return driver_installed; }
esp_err_t uart_driver_delete(int port) { assert(port == 1); driver_installed = false; return ESP_OK; }
esp_err_t uart_param_config(int port, const uart_config_t *c)
{
    assert(port == 1 && c->baud_rate == 115200 && c->data_bits == 8);
    return ESP_OK;
}
esp_err_t uart_set_pin(int port, int tx, int rx, int rts, int cts)
{
    assert(port == 1 && tx == 12 && rx == 15 && rts == -1 && cts == -1);
    return ESP_OK;
}
esp_err_t uart_driver_install(int port, int rx, int tx, int count, void *queue, int flags)
{
    (void)rx; (void)tx; (void)count; (void)queue; (void)flags;
    assert(port == 1 && !driver_installed);
    driver_installed = true;
    return ESP_OK;
}
esp_err_t uart_set_mode(int port, int mode) { assert(port == 1 && mode == 0); return ESP_OK; }
int uart_read_bytes(int port, void *data, size_t len, TickType_t timeout)
{
    (void)port; (void)data; (void)len; (void)timeout;
    ++uart_reads;
    return 0;
}
Str_points_ptr put_io_buf(int type, uint8_t index)
{
    assert(type == IN && index < MAX_INS);
    return (Str_points_ptr){ &points[index] };
}

int uart_write_bytes(int port, const void *data, size_t len)
{
    assert(port == 1 && driver_installed && tx_mutex->held && rx_task_handle);
    assert(len < sizeof(last_command));
    memcpy(last_command, data, len);
    last_command[len] = 0;
    ++uart_writes;
    /* This models the custom module: no OK or ASCII ACK response. */
    return short_write ? (int)len - 1 : (int)len;
}

static void reset(void)
{
    free(tx_mutex);
    tx_mutex = NULL;
    memset(&status, 0, sizeof(status));
    memset(sensors, 0, sizeof(sensors));
    memset(points, 0, sizeof(points));
    memset(&stream, 0, sizeof(stream));
    rx_task_handle = NULL;
    lora_points_initialized = false;
    now_us = 0; task_creates = 0; uart_reads = 0; uart_writes = 0;
    driver_installed = false; task_failure = false; allocation_failure = false;
    short_write = false;
}

static void test_start_and_recovery(void)
{
    reset();
    assert(!lora_send_find_command(1, 5));
    assert(lora_start() == ESP_OK && status.state == LORA_READY);
    assert(uart_writes == 0 && uart_reads == 0 && now_us == 0);
    feed("AT+SENSOR=A1B2C3D4,2234,5566,3300,888,123,456,-70,9\r\n");
    assert(sensors[0].valid && lora_start() == ESP_OK && task_creates == 1);
    assert(sensors[0].valid && uart_writes == 0);
    reset(); task_failure = true;
    assert(lora_start() == ESP_ERR_NO_MEM && !driver_installed && !rx_task_handle);
    task_failure = false;
    assert(lora_start() == ESP_OK);
    reset(); allocation_failure = true;
    assert(lora_start() == ESP_ERR_NO_MEM && !driver_installed);
    allocation_failure = false;
    assert(lora_start() == ESP_OK);
    reset(); driver_installed = true;
    assert(lora_start() == ESP_ERR_INVALID_STATE && driver_installed && !rx_task_handle);
    reset(); assert(lora_start() == ESP_OK);
    feed("Node init OK\r\nAT+DROP=10,A0,21,01\r\n");
    assert(status.node_firmware_detected && status.module_drop_frames == 1);
}

static void test_parse_and_age(void)
{
    reset(); assert(lora_start() == ESP_OK);
    feed("+RX:AT+SENSOR=a1b2c3d4,-1234,5566,3300,888,123,456,-70,-9\r\n");
    lora_sensor_data_t out;
    assert(lora_get_sensor_data("a1b2c3d4", &out));
    assert(strcmp(out.uid, "A1B2C3D4") == 0 && out.t_x100 == -1234 && out.online);
    assert((uint32_t)points[0].value == 0xA1B2C3D4U);
    assert(strcmp((char *)points[0].description, "LORA UID A1B2C3D4") == 0);
    const int32_t expected[] = {-12340,55660,3300,888000,123000,456000,-70000,-9000};
    for (int i = 1; i < 9; ++i) assert(points[i].value == expected[i - 1]);
    now_us += (int64_t)LORA_SENSOR_TIMEOUT_MS * 1000;
    assert(lora_get_sensor_by_index(0, &out) && out.valid && !out.online);
    now_us += (int64_t)UINT32_MAX * 1000;
    assert(lora_get_sensor_by_index(0, &out) && out.age_ms == UINT32_MAX);
    feed("SENSOR=A1B2C3D4,1,2,3,4,5,6,7,8\n");
    assert(lora_get_sensor_by_index(0, &out) && out.online && out.t_x100 == 1);
    assert(!lora_get_sensor_by_index(1, &out) && !lora_get_sensor_by_index(16, &out));
    assert(!lora_get_sensor_data("A1B2C3D40", &out));
    const char *bad[] = {
        "A1B2C3D40,1,2,3,4,5,6,7,8", "G1B2C3D4,1,2,3,4,5,6,7,8",
        "A1B2C3D4,,2,3,4,5,6,7,8", "A1B2C3D4,1,2,3,4,5,6,7",
        "A1B2C3D4,1,2,3,4,5,6,7,8,9", "A1B2C3D4,1,2,3,4,5,6,7,8junk",
        "A1B2C3D4,214748365,2,3,4,5,6,7,8", "A1B2C3D4,-214748365,2,3,4,5,6,7,8",
        "A1B2C3D4,1,-2,3,4,5,6,7,8", "A1B2C3D4,1,10001,3,4,5,6,7,8",
        "A1B2C3D4,1,2,3,2147484,5,6,7,8", "A1B2C3D4,1,2,3,4,5,6,-2147484,8",
        "A1B2C3D4,99999999999999999999999,2,3,4,5,6,7,8",
        "A1B2C3D4,32768,2,3,4,5,6,7,8", "A1B2C3D4,1,2,65536,4,5,6,7,8",
        "A1B2C3D4,1,2,3,4,5,6,-129,8", "A1B2C3D4,hello,2,3,4,5,6,7,8", "A1B2C3D4,1,2,3,4,5,6,7,"
    };
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        unsigned before = status.invalid_frames;
        char wire[200]; snprintf(wire, sizeof(wire), "AT+SENSOR=%s\n", bad[i]); feed(wire);
        assert(status.invalid_frames == before + 1 && sensors[0].t_x100 == 1);
    }
    feed("AT+SENSOR=12345678,32767,10000,65535,65535,65535,65535,-128,127\n");
    assert(sensors[1].valid && points[1].value == 327670);
}

static void test_stream_and_cache(void)
{
    reset(); assert(lora_start() == ESP_OK);
    char garbage[1100]; memset(garbage, 'X', sizeof(garbage));
    rx_consume(&stream, (const uint8_t *)garbage, sizeof(garbage));
    feed("SENSOR=12345678,1,2,3,4,5,6,7,8\n");
    assert(status.dropped_lines == 1 && status.sensor_frames == 0);
    const uint8_t nul_line[] = {'x',0,'y','\n'};
    rx_consume(&stream, nul_line, sizeof(nul_line));
    assert(status.dropped_lines == 2);
    for (unsigned i = 0; i < 17; ++i) {
        char wire[100]; snprintf(wire, sizeof(wire), "AT+SENSOR=%08X,1,2,3,4,5,6,7,8\r\n", i);
        /* Exercise multiple complete frames in the same read as well. */
        rx_consume(&stream, (const uint8_t *)wire, strlen(wire));
    }
    assert(status.sensor_frames == 17 && status.cache_full == 1 && points[0].value == 15);
    feed("SENSOR=00000000,9,2,3,4,5,6,7,8\n");
    assert(sensors[0].t_x100 == 9 && status.cache_full == 1);
}

static void test_transactions(void)
{
    reset(); assert(lora_start() == ESP_OK);
    assert(lora_send_find_command(-1, 100) && strcmp(last_command, "AT+FIND=255,60\r\n") == 0);
    assert(lora_send_find_command(1, 0) && strcmp(last_command, "AT+FIND=1,5\r\n") == 0);
    assert(uart_reads == 0 && uart_writes == 2 && now_us == 0);
    assert(!lora_send_payload_reliable("hello", 5, 3, 100));
    assert(status.last_error == ESP_ERR_NOT_SUPPORTED && uart_writes == 2);
    assert(status.state == LORA_READY);
    short_write = true;
    assert(!lora_send_find_command(1, 5));
    assert(status.tx_failures == 2 && !tx_mutex->held);
    /* No ACK protocol is fabricated: legacy text cannot alter sensor values. */
    feed("ACK:1:1234\nOK\nMSG:1:data:1234\n");
    assert(status.sensor_frames == 0);
}

int main(void)
{
    test_start_and_recovery(); puts("PASS custom-firmware startup (no AT handshake), retry and resource failures");
    test_parse_and_age(); puts("PASS strict parsing, point scaling, UID normalization and offline age");
    test_stream_and_cache(); puts("PASS fragmented/overflow/NUL stream recovery and 16-node cache");
    test_transactions(); puts("PASS FIND without OK, short writes and unsupported reliable TX without UART output");
    reset();
    return 0;
}
