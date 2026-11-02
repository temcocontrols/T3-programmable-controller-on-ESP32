#include "ir_blaster.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

#include "IRac.h"
#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"
#include "IRremoteESP8266.h"

static const char *TAG = "ir_blaster";
#define IR_BLASTER_MAGIC 0x32425249u
#define LEARN_TIMEOUT_MS 20000

typedef struct {
    uint8_t  used;
    uint8_t  owner_slot;
    char     name[IR_BLASTER_NAME_MAX];
    int32_t  protocol;
    uint64_t value;
    uint16_t bits;
    uint16_t freq_hz;
    uint16_t raw_len;
    uint16_t raw[IR_BLASTER_MAX_RAW];
    uint8_t  state[IR_BLASTER_STATE_MAX];
    uint8_t  state_len;
    uint8_t  has_ac_state;
} ir_key_t;

typedef struct {
    uint8_t  kind;
    char     name[IR_BLASTER_NAME_MAX];
    int32_t  protocol;
    uint64_t lib_value;
    uint16_t lib_bits;
    uint16_t lib_raw_len;
    uint16_t lib_raw[IR_BLASTER_MAX_RAW];
    uint8_t  has_ac;
    uint8_t  ac_state[IR_BLASTER_STATE_MAX];
    uint8_t  ac_state_len;
    int8_t   custom_ids[IR_BLASTER_CUSTOM_PER_SLOT];
} ir_slot_t;

typedef struct {
    uint32_t magic;
    ir_slot_t slots[IR_BLASTER_SLOT_COUNT];
    ir_key_t  keys[IR_BLASTER_CUSTOM_TOTAL];
} ir_store_t;

static IRsend *s_tx;
static IRrecv *s_rx;
static IRac *s_ac;
static decode_results s_results;
static ir_store_t *s_store;
static SemaphoreHandle_t s_lock;
static bool s_inited;
static volatile int s_learn_slot = -1;
static volatile uint8_t s_learn_add_custom;
static volatile uint8_t s_learn_status = IR_LEARN_IDLE;
static TickType_t s_learn_deadline;
static char s_learn_msg[64];

static const char *k_lib_names[IR_LIB_BTN_MAX] = {
    "Power", "Mode", "Fan", "Temp+", "Temp-",
    "SwingV", "SwingH", "Turbo", "Sleep", "Quiet", "Send"
};

static void ir_print(const char *fmt, ...)
{
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return;
    }
    if (n >= (int)sizeof(buf)) {
        n = (int)sizeof(buf) - 1;
    }
    uart_write_bytes(UART_NUM_0, buf, (size_t)n);
    ESP_LOGI(TAG, "%s", buf);
}

static void *ir_psram(size_t n)
{
    void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (p == NULL) {
        p = heap_caps_malloc(n, MALLOC_CAP_8BIT);
    }
    if (p != NULL) {
        memset(p, 0, n);
    }
    return p;
}

static uint8_t custom_used_count(void)
{
    uint8_t n = 0;
    for (int i = 0; i < IR_BLASTER_CUSTOM_TOTAL; i++) {
        if (s_store->keys[i].used) {
            n++;
        }
    }
    return n;
}

static int slot_custom_count(const ir_slot_t *sl)
{
    int n = 0;
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        if (sl->custom_ids[i] >= 0) {
            n++;
        }
    }
    return n;
}

static esp_err_t store_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("ir_blast", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    s_store->magic = IR_BLASTER_MAGIC;
    err = nvs_set_blob(h, "store", s_store, sizeof(*s_store));
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

static void store_load(void)
{
    nvs_handle_t h;
    if (nvs_open("ir_blast", NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    size_t len = sizeof(*s_store);
    esp_err_t err = nvs_get_blob(h, "store", s_store, &len);
    nvs_close(h);
    if ((err != ESP_OK) || (s_store->magic != IR_BLASTER_MAGIC)) {
        memset(s_store, 0, sizeof(*s_store));
        for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
            for (int k = 0; k < IR_BLASTER_CUSTOM_PER_SLOT; k++) {
                s_store->slots[s].custom_ids[k] = -1;
            }
        }
    }
}

static int alloc_custom_key(uint8_t slot)
{
    if (custom_used_count() >= IR_BLASTER_CUSTOM_TOTAL) {
        return -1;
    }
    if (slot_custom_count(&s_store->slots[slot]) >= IR_BLASTER_CUSTOM_PER_SLOT) {
        return -1;
    }
    int kid = -1;
    for (int i = 0; i < IR_BLASTER_CUSTOM_TOTAL; i++) {
        if (!s_store->keys[i].used) {
            kid = i;
            break;
        }
    }
    if (kid < 0) {
        return -1;
    }
    ir_slot_t *sl = &s_store->slots[slot];
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        if (sl->custom_ids[i] < 0) {
            sl->custom_ids[i] = (int8_t)kid;
            s_store->keys[kid].used = 1;
            s_store->keys[kid].owner_slot = slot;
            return kid;
        }
    }
    return -1;
}

static bool protocol_is_ac(decode_type_t proto)
{
    return hasACState(proto);
}

static void apply_capture_to_key(ir_key_t *key, const decode_results *r)
{
    memset(key, 0, sizeof(*key));
    key->used = 1;
    key->protocol = (int32_t)r->decode_type;
    key->value = r->value;
    key->bits = r->bits;
    key->freq_hz = 38000;
    strncpy(key->name, typeToString(r->decode_type).c_str(), sizeof(key->name) - 1);
    if (hasACState(r->decode_type)) {
        uint16_t n = (uint16_t)(r->bits / 8);
        if (n > IR_BLASTER_STATE_MAX) {
            n = IR_BLASTER_STATE_MAX;
        }
        key->has_ac_state = 1;
        key->state_len = (uint8_t)n;
        memcpy(key->state, r->state, n);
    }
    uint16_t *raw = resultToRawArray(r);
    uint16_t len = getCorrectedRawLength(r);
    if ((raw != NULL) && (len >= 2)) {
        if (len > IR_BLASTER_MAX_RAW) {
            len = IR_BLASTER_MAX_RAW;
        }
        key->raw_len = len;
        memcpy(key->raw, raw, len * sizeof(uint16_t));
    }
    delete[] raw;
}

static void replay_key(const ir_key_t *key)
{
    s_rx->disableIRIn();
    decode_type_t proto = (decode_type_t)key->protocol;
    bool ok = false;
    if (key->has_ac_state && (proto != decode_type_t::UNKNOWN)) {
        ok = s_tx->send(proto, key->state, key->state_len);
    } else if (proto != decode_type_t::UNKNOWN) {
        ok = s_tx->send(proto, key->value, key->bits);
    }
    if (!ok && (key->raw_len >= 2)) {
        s_tx->sendRaw(key->raw, key->raw_len, key->freq_hz ? key->freq_hz : 38000);
    }
    vTaskDelay(pdMS_TO_TICKS(80));
    s_rx->enableIRIn();
    s_rx->resume();
}

static void handle_learn_capture(const decode_results *r)
{
    int slot = s_learn_slot;
    if ((slot < 0) || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return;
    }
    ir_slot_t *sl = &s_store->slots[slot];
    decode_type_t proto = r->decode_type;
    bool known = (proto != decode_type_t::UNKNOWN);

    if (!s_learn_add_custom && (sl->kind == IR_SLOT_EMPTY)) {
        if (known) {
            sl->kind = IR_SLOT_LIBRARY;
            sl->protocol = (int32_t)proto;
            sl->lib_value = r->value;
            sl->lib_bits = r->bits;
            sl->has_ac = hasACState(proto) ? 1 : 0;
            if (sl->has_ac) {
                uint16_t n = (uint16_t)(r->bits / 8);
                if (n > IR_BLASTER_STATE_MAX) {
                    n = IR_BLASTER_STATE_MAX;
                }
                sl->ac_state_len = (uint8_t)n;
                memcpy(sl->ac_state, r->state, n);
            }
            uint16_t *raw = resultToRawArray(r);
            uint16_t len = getCorrectedRawLength(r);
            if ((raw != NULL) && (len >= 2)) {
                if (len > IR_BLASTER_MAX_RAW) {
                    len = IR_BLASTER_MAX_RAW;
                }
                sl->lib_raw_len = len;
                memcpy(sl->lib_raw, raw, len * sizeof(uint16_t));
            }
            delete[] raw;
            strncpy(sl->name, typeToString(proto).c_str(), sizeof(sl->name) - 1);
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Library: %s", sl->name);
        } else {
            int kid = alloc_custom_key((uint8_t)slot);
            if (kid < 0) {
                s_learn_status = IR_LEARN_FAIL;
                snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key limit");
                s_learn_slot = -1;
                return;
            }
            apply_capture_to_key(&s_store->keys[kid], r);
            snprintf(s_store->keys[kid].name, sizeof(s_store->keys[kid].name), "Key %d",
                     slot_custom_count(sl));
            sl->kind = IR_SLOT_CUSTOM;
            snprintf(sl->name, sizeof(sl->name), "Custom %u", (unsigned)(slot + 1));
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key saved");
        }
    } else {
        int kid = alloc_custom_key((uint8_t)slot);
        if (kid < 0) {
            s_learn_status = IR_LEARN_FAIL;
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key limit");
            s_learn_slot = -1;
            return;
        }
        apply_capture_to_key(&s_store->keys[kid], r);
        snprintf(s_store->keys[kid].name, sizeof(s_store->keys[kid].name), "Key %d",
                 slot_custom_count(sl));
        if (sl->kind == IR_SLOT_EMPTY) {
            sl->kind = IR_SLOT_CUSTOM;
            snprintf(sl->name, sizeof(sl->name), "Custom %u", (unsigned)(slot + 1));
        }
        snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key saved");
    }
    (void)store_save();
    s_learn_status = IR_LEARN_OK;
    s_learn_slot = -1;
}

static void ir_blaster_task(void *arg)
{
    (void)arg;
    s_rx->enableIRIn();
    ir_print("ir_blaster RX on\r\n");

    for (;;) {
        if ((s_learn_status == IR_LEARN_WAIT) &&
            (xTaskGetTickCount() > s_learn_deadline)) {
            s_learn_slot = -1;
            s_learn_status = IR_LEARN_FAIL;
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Learn timeout");
        }
        if (s_rx->decode(&s_results)) {
            if ((s_learn_status == IR_LEARN_WAIT) && (xSemaphoreTake(s_lock, pdMS_TO_TICKS(50)) == pdTRUE)) {
                handle_learn_capture(&s_results);
                xSemaphoreGive(s_lock);
                ir_print("ir_blaster %s\r\n", s_learn_msg);
            }
            s_rx->resume();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t ir_blaster_init(uint8_t mini_type)
{
    /* GPIO17 TX / GPIO18 RX are TSTAT11 only. HUB uses those pins for A7608 UART. */
    if (mini_type != IR_BLASTER_MINI_TSTAT11) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    ir_print("ir_blaster init enter TX GPIO%u RX GPIO%u\r\n",
             (unsigned)CONFIG_IR_BLASTER_TX_GPIO,
             (unsigned)CONFIG_IR_BLASTER_RX_GPIO);
    if (s_inited) {
        return ESP_OK;
    }

    s_store = (ir_store_t *)ir_psram(sizeof(ir_store_t));
    if (s_store == NULL) {
        ir_print("ir_blaster PSRAM alloc failed (%u)\r\n", (unsigned)sizeof(ir_store_t));
        return ESP_ERR_NO_MEM;
    }
    for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
        for (int k = 0; k < IR_BLASTER_CUSTOM_PER_SLOT; k++) {
            s_store->slots[s].custom_ids[k] = -1;
        }
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }
    store_load();

    s_tx = new IRsend((uint16_t)CONFIG_IR_BLASTER_TX_GPIO, false, true);
    s_rx = new IRrecv((uint16_t)CONFIG_IR_BLASTER_RX_GPIO,
                      (uint16_t)CONFIG_IR_BLASTER_CAPTURE_SIZE,
                      (uint8_t)CONFIG_IR_BLASTER_TIMEOUT_MS,
                      true);
    s_ac = new IRac((uint16_t)CONFIG_IR_BLASTER_TX_GPIO);
    if ((s_tx == NULL) || (s_rx == NULL) || (s_ac == NULL)) {
        return ESP_ERR_NO_MEM;
    }
    s_tx->begin();

    if (xTaskCreate(ir_blaster_task, "ir_blaster", 8192, NULL, 10, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    s_inited = true;
    ir_print("ir_blaster ready store=%u bytes PSRAM\r\n", (unsigned)sizeof(ir_store_t));
    return ESP_OK;
}

bool ir_blaster_is_supported(void)
{
    return s_inited;
}

uint8_t ir_blaster_custom_used(void)
{
    return s_inited ? custom_used_count() : 0;
}

uint8_t ir_blaster_custom_free(void)
{
    return (uint8_t)(IR_BLASTER_CUSTOM_TOTAL - ir_blaster_custom_used());
}

esp_err_t ir_blaster_get_slot_info(uint8_t slot, ir_blaster_slot_info_t *out)
{
    if (!s_inited || (out == NULL) || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    const ir_slot_t *sl = &s_store->slots[slot];
    memset(out, 0, sizeof(*out));
    out->kind = sl->kind;
    out->used = (sl->kind != IR_SLOT_EMPTY);
    out->custom_count = (uint8_t)slot_custom_count(sl);
    memcpy(out->name, sl->name, sizeof(out->name));
    if (sl->kind == IR_SLOT_LIBRARY) {
        strncpy(out->proto_name, typeToString((decode_type_t)sl->protocol).c_str(),
                sizeof(out->proto_name) - 1);
    } else if (sl->kind == IR_SLOT_CUSTOM) {
        strncpy(out->proto_name, "Custom", sizeof(out->proto_name) - 1);
    } else {
        strncpy(out->proto_name, "Empty", sizeof(out->proto_name) - 1);
    }
    return ESP_OK;
}

esp_err_t ir_blaster_clear_slot(uint8_t slot)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    ir_slot_t *sl = &s_store->slots[slot];
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        int kid = sl->custom_ids[i];
        if (kid >= 0) {
            memset(&s_store->keys[kid], 0, sizeof(s_store->keys[kid]));
        }
        sl->custom_ids[i] = -1;
    }
    memset(sl, 0, sizeof(*sl));
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        sl->custom_ids[i] = -1;
    }
    return store_save();
}

esp_err_t ir_blaster_rename_slot(uint8_t slot, const char *name)
{
    if (!s_inited || (name == NULL) || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    strncpy(s_store->slots[slot].name, name, IR_BLASTER_NAME_MAX - 1);
    s_store->slots[slot].name[IR_BLASTER_NAME_MAX - 1] = 0;
    return store_save();
}

esp_err_t ir_blaster_begin_learn(uint8_t slot, uint8_t add_custom)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_learn_status == IR_LEARN_WAIT) {
        return ESP_ERR_INVALID_STATE;
    }
    if (add_custom) {
        if (custom_used_count() >= IR_BLASTER_CUSTOM_TOTAL) {
            snprintf(s_learn_msg, sizeof(s_learn_msg), "No custom keys left");
            s_learn_status = IR_LEARN_FAIL;
            return ESP_ERR_NO_MEM;
        }
        if (slot_custom_count(&s_store->slots[slot]) >= IR_BLASTER_CUSTOM_PER_SLOT) {
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Slot custom limit");
            s_learn_status = IR_LEARN_FAIL;
            return ESP_ERR_NO_MEM;
        }
    }
    s_learn_add_custom = add_custom ? 1 : 0;
    s_learn_slot = (int)slot;
    s_learn_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(LEARN_TIMEOUT_MS);
    s_learn_status = IR_LEARN_WAIT;
    snprintf(s_learn_msg, sizeof(s_learn_msg), "Point remote, press one key");
    return ESP_OK;
}

void ir_blaster_cancel_learn(void)
{
    s_learn_slot = -1;
    s_learn_status = IR_LEARN_IDLE;
    s_learn_msg[0] = 0;
}

uint8_t ir_blaster_learn_status(void)
{
    return s_learn_status;
}

const char *ir_blaster_learn_message(void)
{
    return s_learn_msg;
}

int ir_blaster_lib_button_count(uint8_t slot)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return 0;
    }
    if (s_store->slots[slot].kind != IR_SLOT_LIBRARY) {
        return 0;
    }
    if (protocol_is_ac((decode_type_t)s_store->slots[slot].protocol)) {
        return 10;
    }
    return 1;
}

const char *ir_blaster_lib_button_name(uint8_t slot, int index)
{
    if (ir_blaster_lib_button_count(slot) == 1) {
        return k_lib_names[IR_LIB_BTN_SEND];
    }
    if ((index < 0) || (index >= 10)) {
        return "";
    }
    return k_lib_names[index];
}

esp_err_t ir_blaster_send_lib(uint8_t slot, int index)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    ir_slot_t *sl = &s_store->slots[slot];
    if (sl->kind != IR_SLOT_LIBRARY) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    s_rx->disableIRIn();
    decode_type_t proto = (decode_type_t)sl->protocol;
    if (protocol_is_ac(proto) && (index != IR_LIB_BTN_SEND)) {
        s_ac->next.protocol = proto;
        s_ac->next.power = true;
        s_ac->next.celsius = true;
        if (s_ac->next.degrees < 16) {
            s_ac->next.degrees = 24;
        }
        switch (index) {
        case IR_LIB_BTN_POWER:
            s_ac->next.power = !s_ac->next.power;
            break;
        case IR_LIB_BTN_MODE: {
            int m = (int)s_ac->next.mode + 1;
            if (m > (int)stdAc::opmode_t::kFan) {
                m = (int)stdAc::opmode_t::kAuto;
            }
            s_ac->next.mode = (stdAc::opmode_t)m;
            break;
        }
        case IR_LIB_BTN_FAN: {
            int f = (int)s_ac->next.fanspeed + 1;
            if (f > (int)stdAc::fanspeed_t::kMax) {
                f = (int)stdAc::fanspeed_t::kAuto;
            }
            s_ac->next.fanspeed = (stdAc::fanspeed_t)f;
            break;
        }
        case IR_LIB_BTN_TEMP_UP:
            if (s_ac->next.degrees < 30) {
                s_ac->next.degrees += 1;
            }
            break;
        case IR_LIB_BTN_TEMP_DOWN:
            if (s_ac->next.degrees > 16) {
                s_ac->next.degrees -= 1;
            }
            break;
        case IR_LIB_BTN_SWINGV:
            s_ac->next.swingv = (s_ac->next.swingv == stdAc::swingv_t::kOff)
                                    ? stdAc::swingv_t::kAuto
                                    : stdAc::swingv_t::kOff;
            break;
        case IR_LIB_BTN_SWINGH:
            s_ac->next.swingh = (s_ac->next.swingh == stdAc::swingh_t::kOff)
                                    ? stdAc::swingh_t::kAuto
                                    : stdAc::swingh_t::kOff;
            break;
        case IR_LIB_BTN_TURBO:
            s_ac->next.turbo = !s_ac->next.turbo;
            break;
        case IR_LIB_BTN_SLEEP:
            s_ac->next.sleep = (s_ac->next.sleep > 0) ? -1 : 0;
            break;
        case IR_LIB_BTN_QUIET:
            s_ac->next.quiet = !s_ac->next.quiet;
            break;
        default:
            break;
        }
        (void)s_ac->sendAc();
    } else {
        bool ok = false;
        if (sl->has_ac && (sl->ac_state_len > 0)) {
            ok = s_tx->send(proto, sl->ac_state, sl->ac_state_len);
        } else if (proto != decode_type_t::UNKNOWN) {
            ok = s_tx->send(proto, sl->lib_value, sl->lib_bits);
        }
        if (!ok && (sl->lib_raw_len >= 2)) {
            s_tx->sendRaw(sl->lib_raw, sl->lib_raw_len, 38000);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(80));
    s_rx->enableIRIn();
    s_rx->resume();
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

int ir_blaster_custom_count(uint8_t slot)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return 0;
    }
    return slot_custom_count(&s_store->slots[slot]);
}

static int custom_id_at(uint8_t slot, int index)
{
    const ir_slot_t *sl = &s_store->slots[slot];
    int seen = 0;
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        if (sl->custom_ids[i] >= 0) {
            if (seen == index) {
                return sl->custom_ids[i];
            }
            seen++;
        }
    }
    return -1;
}

esp_err_t ir_blaster_custom_name(uint8_t slot, int index, char *out, size_t out_len)
{
    if ((out == NULL) || (out_len == 0) || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    int kid = custom_id_at(slot, index);
    if (kid < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    strncpy(out, s_store->keys[kid].name, out_len - 1);
    out[out_len - 1] = 0;
    return ESP_OK;
}

esp_err_t ir_blaster_rename_custom(uint8_t slot, int index, const char *name)
{
    if ((name == NULL) || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    int kid = custom_id_at(slot, index);
    if (kid < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    strncpy(s_store->keys[kid].name, name, IR_BLASTER_NAME_MAX - 1);
    s_store->keys[kid].name[IR_BLASTER_NAME_MAX - 1] = 0;
    return store_save();
}

esp_err_t ir_blaster_send_custom(uint8_t slot, int index)
{
    if (!s_inited || (slot >= IR_BLASTER_SLOT_COUNT)) {
        return ESP_ERR_INVALID_ARG;
    }
    int kid = custom_id_at(slot, index);
    if (kid < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    replay_key(&s_store->keys[kid]);
    xSemaphoreGive(s_lock);
    return ESP_OK;
}
