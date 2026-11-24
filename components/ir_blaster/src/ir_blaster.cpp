#include "ir_blaster.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
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

/* IRrecv capture state (ISR writes rawlen). Used only to log "is the pin seeing IR". */
namespace _IRrecv {
extern atomic_irparams_t params;
}

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
static volatile uint32_t s_gpio_edges;
static volatile uint32_t s_decode_ok;
static volatile uint16_t s_max_rawlen;
static int s_last_gpio = -1;
static TickType_t s_rx_log_tick;

static const char *k_lib_names[IR_LIB_BTN_MAX] = {
    "Power", "Mode", "Fan", "Temp+", "Temp-",
    "SwingV", "SwingH", "Turbo", "Sleep", "Quiet", "Send"
};
static char s_lib_label[IR_BLASTER_SLOT_COUNT][IR_LIB_BTN_MAX][IR_BLASTER_NAME_MAX];

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

#define IR_NVS_VER 3

static bool wr_bytes(uint8_t **p, uint8_t *end, const void *src, size_t n)
{
    if ((size_t)(end - *p) < n) {
        return false;
    }
    memcpy(*p, src, n);
    *p += n;
    return true;
}

static bool rd_bytes(const uint8_t **p, const uint8_t *end, void *dst, size_t n)
{
    if ((size_t)(end - *p) < n) {
        return false;
    }
    memcpy(dst, *p, n);
    *p += n;
    return true;
}

static void store_reset(void)
{
    memset(s_store, 0, sizeof(*s_store));
    memset(s_lib_label, 0, sizeof(s_lib_label));
    s_store->magic = IR_BLASTER_MAGIC;
    for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
        for (int k = 0; k < IR_BLASTER_CUSTOM_PER_SLOT; k++) {
            s_store->slots[s].custom_ids[k] = -1;
        }
    }
}

static void store_sanitize(void)
{
    s_store->magic = IR_BLASTER_MAGIC;
    for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
        ir_slot_t *sl = &s_store->slots[s];
        sl->name[IR_BLASTER_NAME_MAX - 1] = 0;
        for (int b = 0; b < IR_LIB_BTN_MAX; b++) {
            s_lib_label[s][b][IR_BLASTER_NAME_MAX - 1] = 0;
        }
        if (sl->lib_raw_len > IR_BLASTER_MAX_RAW) {
            sl->lib_raw_len = IR_BLASTER_MAX_RAW;
        }
        if (sl->ac_state_len > IR_BLASTER_STATE_MAX) {
            sl->ac_state_len = IR_BLASTER_STATE_MAX;
        }
        for (int k = 0; k < IR_BLASTER_CUSTOM_PER_SLOT; k++) {
            int id = sl->custom_ids[k];
            if ((id < 0) || (id >= IR_BLASTER_CUSTOM_TOTAL) || !s_store->keys[id].used) {
                sl->custom_ids[k] = -1;
            }
        }
    }
    for (int i = 0; i < IR_BLASTER_CUSTOM_TOTAL; i++) {
        ir_key_t *key = &s_store->keys[i];
        key->name[IR_BLASTER_NAME_MAX - 1] = 0;
        if (key->raw_len > IR_BLASTER_MAX_RAW) {
            key->raw_len = IR_BLASTER_MAX_RAW;
        }
        if (key->state_len > IR_BLASTER_STATE_MAX) {
            key->state_len = IR_BLASTER_STATE_MAX;
        }
    }
}

static size_t store_pack(uint8_t *out, size_t out_max)
{
    uint8_t *p = out;
    uint8_t *end = out + out_max;
    uint16_t ver = IR_NVS_VER;
    if (!wr_bytes(&p, end, &s_store->magic, sizeof(s_store->magic)) ||
        !wr_bytes(&p, end, &ver, sizeof(ver))) {
        return 0;
    }
    for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
        const ir_slot_t *sl = &s_store->slots[s];
        uint16_t raw_n = sl->lib_raw_len;
        uint8_t st_n = sl->ac_state_len;
        if (!wr_bytes(&p, end, &sl->kind, 1) ||
            !wr_bytes(&p, end, sl->name, IR_BLASTER_NAME_MAX) ||
            !wr_bytes(&p, end, &sl->protocol, sizeof(sl->protocol)) ||
            !wr_bytes(&p, end, &sl->lib_value, sizeof(sl->lib_value)) ||
            !wr_bytes(&p, end, &sl->lib_bits, sizeof(sl->lib_bits)) ||
            !wr_bytes(&p, end, &raw_n, sizeof(raw_n)) ||
            !wr_bytes(&p, end, &sl->has_ac, 1) ||
            !wr_bytes(&p, end, &st_n, 1) ||
            !wr_bytes(&p, end, sl->custom_ids, sizeof(sl->custom_ids)) ||
            !wr_bytes(&p, end, sl->lib_raw, raw_n * sizeof(uint16_t)) ||
            !wr_bytes(&p, end, sl->ac_state, st_n)) {
            return 0;
        }
    }
    for (int i = 0; i < IR_BLASTER_CUSTOM_TOTAL; i++) {
        const ir_key_t *key = &s_store->keys[i];
        uint8_t used = key->used;
        if (!wr_bytes(&p, end, &used, 1)) {
            return 0;
        }
        if (!used) {
            continue;
        }
        uint16_t raw_n = key->raw_len;
        uint8_t st_n = key->state_len;
        if (!wr_bytes(&p, end, &key->owner_slot, 1) ||
            !wr_bytes(&p, end, key->name, IR_BLASTER_NAME_MAX) ||
            !wr_bytes(&p, end, &key->protocol, sizeof(key->protocol)) ||
            !wr_bytes(&p, end, &key->value, sizeof(key->value)) ||
            !wr_bytes(&p, end, &key->bits, sizeof(key->bits)) ||
            !wr_bytes(&p, end, &key->freq_hz, sizeof(key->freq_hz)) ||
            !wr_bytes(&p, end, &raw_n, sizeof(raw_n)) ||
            !wr_bytes(&p, end, &st_n, 1) ||
            !wr_bytes(&p, end, &key->has_ac_state, 1) ||
            !wr_bytes(&p, end, key->raw, raw_n * sizeof(uint16_t)) ||
            !wr_bytes(&p, end, key->state, st_n)) {
            return 0;
        }
    }
    if (!wr_bytes(&p, end, s_lib_label, sizeof(s_lib_label))) {
        return 0;
    }
    return (size_t)(p - out);
}

static bool store_unpack(const uint8_t *in, size_t in_len)
{
    const uint8_t *p = in;
    const uint8_t *end = in + in_len;
    uint32_t magic = 0;
    uint16_t ver = 0;
    if (!rd_bytes(&p, end, &magic, sizeof(magic)) || (magic != IR_BLASTER_MAGIC) ||
        !rd_bytes(&p, end, &ver, sizeof(ver)) || ((ver != 2) && (ver != IR_NVS_VER))) {
        return false;
    }
    store_reset();
    for (int s = 0; s < IR_BLASTER_SLOT_COUNT; s++) {
        ir_slot_t *sl = &s_store->slots[s];
        uint16_t raw_n = 0;
        uint8_t st_n = 0;
        if (!rd_bytes(&p, end, &sl->kind, 1) ||
            !rd_bytes(&p, end, sl->name, IR_BLASTER_NAME_MAX) ||
            !rd_bytes(&p, end, &sl->protocol, sizeof(sl->protocol)) ||
            !rd_bytes(&p, end, &sl->lib_value, sizeof(sl->lib_value)) ||
            !rd_bytes(&p, end, &sl->lib_bits, sizeof(sl->lib_bits)) ||
            !rd_bytes(&p, end, &raw_n, sizeof(raw_n)) ||
            !rd_bytes(&p, end, &sl->has_ac, 1) ||
            !rd_bytes(&p, end, &st_n, 1) ||
            !rd_bytes(&p, end, sl->custom_ids, sizeof(sl->custom_ids))) {
            return false;
        }
        if (raw_n > IR_BLASTER_MAX_RAW) {
            return false;
        }
        if (st_n > IR_BLASTER_STATE_MAX) {
            return false;
        }
        sl->lib_raw_len = raw_n;
        sl->ac_state_len = st_n;
        if (!rd_bytes(&p, end, sl->lib_raw, raw_n * sizeof(uint16_t)) ||
            !rd_bytes(&p, end, sl->ac_state, st_n)) {
            return false;
        }
        sl->name[IR_BLASTER_NAME_MAX - 1] = 0;
    }
    for (int i = 0; i < IR_BLASTER_CUSTOM_TOTAL; i++) {
        ir_key_t *key = &s_store->keys[i];
        uint8_t used = 0;
        if (!rd_bytes(&p, end, &used, 1)) {
            return false;
        }
        key->used = used;
        if (!used) {
            continue;
        }
        uint16_t raw_n = 0;
        uint8_t st_n = 0;
        if (!rd_bytes(&p, end, &key->owner_slot, 1) ||
            !rd_bytes(&p, end, key->name, IR_BLASTER_NAME_MAX) ||
            !rd_bytes(&p, end, &key->protocol, sizeof(key->protocol)) ||
            !rd_bytes(&p, end, &key->value, sizeof(key->value)) ||
            !rd_bytes(&p, end, &key->bits, sizeof(key->bits)) ||
            !rd_bytes(&p, end, &key->freq_hz, sizeof(key->freq_hz)) ||
            !rd_bytes(&p, end, &raw_n, sizeof(raw_n)) ||
            !rd_bytes(&p, end, &st_n, 1) ||
            !rd_bytes(&p, end, &key->has_ac_state, 1)) {
            return false;
        }
        if (raw_n > IR_BLASTER_MAX_RAW) {
            return false;
        }
        if (st_n > IR_BLASTER_STATE_MAX) {
            return false;
        }
        key->raw_len = raw_n;
        key->state_len = st_n;
        if (!rd_bytes(&p, end, key->raw, raw_n * sizeof(uint16_t)) ||
            !rd_bytes(&p, end, key->state, st_n)) {
            return false;
        }
        key->name[IR_BLASTER_NAME_MAX - 1] = 0;
    }
    if (ver >= 3) {
        if (!rd_bytes(&p, end, s_lib_label, sizeof(s_lib_label))) {
            return false;
        }
    }
    store_sanitize();
    return true;
}

static void *ir_internal(size_t n)
{
    void *p = heap_caps_malloc(n, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (p == NULL) {
        p = malloc(n);
    }
    return p;
}

static esp_err_t store_save(void)
{
    size_t max = sizeof(*s_store) + sizeof(s_lib_label) + 64;
    uint8_t *buf = (uint8_t *)ir_internal(max);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    s_store->magic = IR_BLASTER_MAGIC;
    size_t n = store_pack(buf, max);
    if (n == 0) {
        free(buf);
        return ESP_ERR_INVALID_SIZE;
    }
    nvs_handle_t h;
    esp_err_t err = nvs_open("ir_blast", NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_set_blob(h, "store2", buf, n);
        if (err == ESP_OK) {
            err = nvs_commit(h);
        }
        nvs_close(h);
    }
    free(buf);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "IR NVS save failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void store_load(void)
{
    nvs_handle_t h;
    if (nvs_open("ir_blast", NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    size_t len = 0;
    esp_err_t err = nvs_get_blob(h, "store2", NULL, &len);
    if (((err == ESP_OK) || (err == ESP_ERR_NVS_INVALID_LENGTH)) && (len > 0)) {
        uint8_t *buf = (uint8_t *)ir_internal(len);
        if (buf != NULL) {
            err = nvs_get_blob(h, "store2", buf, &len);
            if ((err != ESP_OK) || !store_unpack(buf, len)) {
                store_reset();
            }
            free(buf);
            nvs_close(h);
            return;
        }
    }
    /* Legacy full-struct blob (may miss names/keys if saved from PSRAM). */
    len = sizeof(*s_store);
    uint8_t *old = (uint8_t *)ir_internal(len);
    if (old != NULL) {
        err = nvs_get_blob(h, "store", old, &len);
        if ((err == ESP_OK) && (len >= sizeof(uint32_t))) {
            memcpy(s_store, old, (len < sizeof(*s_store)) ? len : sizeof(*s_store));
            if (s_store->magic != IR_BLASTER_MAGIC) {
                store_reset();
            } else {
                store_sanitize();
            }
        }
        free(old);
    }
    nvs_close(h);
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
    uint8_t owner = key->owner_slot;
    memset(key, 0, sizeof(*key));
    key->used = 1;
    key->owner_slot = owner;
    key->protocol = (int32_t)r->decode_type;
    key->value = r->value;
    key->bits = r->bits;
    key->freq_hz = 38000;
    strncpy(key->name, typeToString(r->decode_type).c_str(), sizeof(key->name) - 1);
    key->name[sizeof(key->name) - 1] = 0;
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
            sl->name[sizeof(sl->name) - 1] = 0;
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Accept Library: %s?", sl->name);
            s_learn_status = IR_LEARN_ASK_LIB;
            return;
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

static void slot_reset_empty(uint8_t slot)
{
    ir_slot_t *sl = &s_store->slots[slot];
    memset(sl, 0, sizeof(*sl));
    for (int i = 0; i < IR_BLASTER_CUSTOM_PER_SLOT; i++) {
        sl->custom_ids[i] = -1;
    }
    memset(s_lib_label[slot], 0, sizeof(s_lib_label[slot]));
}

static esp_err_t convert_library_to_custom(uint8_t slot)
{
    ir_slot_t *sl = &s_store->slots[slot];
    int kid = alloc_custom_key(slot);
    if (kid < 0) {
        return ESP_ERR_NO_MEM;
    }
    ir_key_t *key = &s_store->keys[kid];
    uint8_t owner = key->owner_slot;
    memset(key, 0, sizeof(*key));
    key->used = 1;
    key->owner_slot = owner;
    key->protocol = sl->protocol;
    key->value = sl->lib_value;
    key->bits = sl->lib_bits;
    key->freq_hz = 38000;
    key->raw_len = sl->lib_raw_len;
    if (key->raw_len > IR_BLASTER_MAX_RAW) {
        key->raw_len = IR_BLASTER_MAX_RAW;
    }
    memcpy(key->raw, sl->lib_raw, key->raw_len * sizeof(uint16_t));
    key->has_ac_state = sl->has_ac;
    key->state_len = sl->ac_state_len;
    if (key->state_len > IR_BLASTER_STATE_MAX) {
        key->state_len = IR_BLASTER_STATE_MAX;
    }
    memcpy(key->state, sl->ac_state, key->state_len);
    snprintf(key->name, sizeof(key->name), "Key %d", slot_custom_count(sl));

    int8_t ids[IR_BLASTER_CUSTOM_PER_SLOT];
    memcpy(ids, sl->custom_ids, sizeof(ids));
    memset(sl, 0, sizeof(*sl));
    memcpy(sl->custom_ids, ids, sizeof(ids));
    sl->kind = IR_SLOT_CUSTOM;
    snprintf(sl->name, sizeof(sl->name), "Custom %u", (unsigned)(slot + 1));
    memset(s_lib_label[slot], 0, sizeof(s_lib_label[slot]));
    return ESP_OK;
}

static void log_rx_snapshot(const char *why)
{
    int pin = CONFIG_IR_BLASTER_RX_GPIO;
    int lvl = gpio_get_level((gpio_num_t)pin);
    uint16_t rawlen = _IRrecv::params.rawlen;
    uint8_t st = _IRrecv::params.rcvstate;
    if (rawlen > s_max_rawlen) {
        s_max_rawlen = rawlen;
    }
    ESP_LOGI(TAG,
             "IR RX %s GPIO%u=%d (idle=1) edges=%u decode_ok=%u cap_rawlen=%u max_rawlen=%u rcvstate=%u learn=%u",
             why,
             (unsigned)pin,
             lvl,
             (unsigned)s_gpio_edges,
             (unsigned)s_decode_ok,
             (unsigned)rawlen,
             (unsigned)s_max_rawlen,
             (unsigned)st,
             (unsigned)s_learn_status);
}

static void sample_rx_gpio(void)
{
    int lvl = gpio_get_level((gpio_num_t)CONFIG_IR_BLASTER_RX_GPIO);
    if (s_last_gpio < 0) {
        s_last_gpio = lvl;
        return;
    }
    if (lvl != s_last_gpio) {
        s_gpio_edges++;
        s_last_gpio = lvl;
        ESP_LOGI(TAG, "IR RX GPIO%u edge -> %d  edges=%u cap_rawlen=%u",
                 (unsigned)CONFIG_IR_BLASTER_RX_GPIO,
                 lvl,
                 (unsigned)s_gpio_edges,
                 (unsigned)_IRrecv::params.rawlen);
    }
}

static void ir_blaster_task(void *arg)
{
    (void)arg;
    s_rx->enableIRIn();
    s_last_gpio = gpio_get_level((gpio_num_t)CONFIG_IR_BLASTER_RX_GPIO);
    ESP_LOGI(TAG, "IR RX on GPIO%u level=%d (TSOP idle should be 1)",
             (unsigned)CONFIG_IR_BLASTER_RX_GPIO, s_last_gpio);
    ir_print("ir_blaster RX on\r\n");

    for (;;) {
        sample_rx_gpio();

        if ((s_learn_status == IR_LEARN_WAIT) &&
            (xTaskGetTickCount() > s_learn_deadline)) {
            log_rx_snapshot("TIMEOUT");
            s_learn_slot = -1;
            s_learn_status = IR_LEARN_FAIL;
            snprintf(s_learn_msg, sizeof(s_learn_msg), "Timeout");
            ESP_LOGW(TAG, "IR learn fail: %s", s_learn_msg);
        }

        if ((s_learn_status == IR_LEARN_WAIT) &&
            ((xTaskGetTickCount() - s_rx_log_tick) >= pdMS_TO_TICKS(1000))) {
            s_rx_log_tick = xTaskGetTickCount();
            log_rx_snapshot("wait");
        }

        if (s_rx->decode(&s_results)) {
            s_decode_ok++;
            if (s_results.rawlen > s_max_rawlen) {
                s_max_rawlen = s_results.rawlen;
            }
            ESP_LOGI(TAG,
                     "IR RX FRAME proto=%s bits=%u rawlen=%u overflow=%u value=0x%llx learn=%u",
                     typeToString(s_results.decode_type).c_str(),
                     (unsigned)s_results.bits,
                     (unsigned)s_results.rawlen,
                     (unsigned)s_results.overflow,
                     (unsigned long long)s_results.value,
                     (unsigned)s_learn_status);
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
    /* Default: hide ESP_LOGI on this tag. Raise to ESP_LOG_INFO to see RX/learn debug. */
    esp_log_level_set(TAG, ESP_LOG_WARN);
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
#if DECODE_HASH
    s_rx->setUnknownThreshold(6);
#endif

    {
        gpio_num_t rx = (gpio_num_t)CONFIG_IR_BLASTER_RX_GPIO;
        gpio_num_t tx = (gpio_num_t)CONFIG_IR_BLASTER_TX_GPIO;
        gpio_set_direction(rx, GPIO_MODE_INPUT);
        gpio_set_direction(tx, GPIO_MODE_OUTPUT);
        int idle = gpio_get_level(rx);
        ESP_LOGI(TAG, "HW CHECK RX GPIO%u idle=%d (expect 1 = ~3.3V on TSOP D26 OUT)",
                 (unsigned)rx, idle);
        ESP_LOGI(TAG, "HW CHECK TX GPIO%u: 3 blinks — look at D23/D24/D25 with a phone camera",
                 (unsigned)tx);
        for (int i = 0; i < 3; i++) {
            gpio_set_level(tx, 1);
            vTaskDelay(pdMS_TO_TICKS(150));
            gpio_set_level(tx, 0);
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        ESP_LOGI(TAG, "HW CHECK done RX now=%d", gpio_get_level(rx));
    }

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
    memset(s_lib_label[slot], 0, sizeof(s_lib_label[slot]));
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
    if ((s_learn_status == IR_LEARN_WAIT) || (s_learn_status == IR_LEARN_ASK_LIB)) {
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
    s_gpio_edges = 0;
    s_decode_ok = 0;
    s_max_rawlen = 0;
    s_last_gpio = gpio_get_level((gpio_num_t)CONFIG_IR_BLASTER_RX_GPIO);
    s_rx_log_tick = xTaskGetTickCount();
    s_rx->resume();
    snprintf(s_learn_msg, sizeof(s_learn_msg), "Point remote, press one key");
    ESP_LOGI(TAG, "IR learn start slot=%u add_custom=%u GPIO%u=%d (idle should be 1)",
             (unsigned)slot, (unsigned)s_learn_add_custom,
             (unsigned)CONFIG_IR_BLASTER_RX_GPIO, s_last_gpio);
    return ESP_OK;
}

esp_err_t ir_blaster_confirm_library(uint8_t accept)
{
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (s_learn_status == IR_LEARN_ASK_LIB) {
        int slot = s_learn_slot;
        if ((slot >= 0) && (slot < IR_BLASTER_SLOT_COUNT) &&
            (s_store->slots[slot].kind == IR_SLOT_LIBRARY)) {
            if (accept) {
                err = store_save();
                snprintf(s_learn_msg, sizeof(s_learn_msg), "Library: %s",
                         s_store->slots[slot].name);
                s_learn_status = IR_LEARN_OK;
                s_learn_slot = -1;
            } else {
                err = convert_library_to_custom((uint8_t)slot);
                if (err == ESP_OK) {
                    err = store_save();
                    snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key saved");
                    s_learn_status = IR_LEARN_OK;
                    s_learn_slot = -1;
                } else {
                    slot_reset_empty((uint8_t)slot);
                    snprintf(s_learn_msg, sizeof(s_learn_msg), "Custom key limit");
                    s_learn_status = IR_LEARN_FAIL;
                    s_learn_slot = -1;
                }
            }
        }
    }
    xSemaphoreGive(s_lock);
    return err;
}

void ir_blaster_cancel_learn(void)
{
    if ((s_learn_status == IR_LEARN_ASK_LIB) && (s_learn_slot >= 0) &&
        (s_learn_slot < IR_BLASTER_SLOT_COUNT) && (s_store != NULL)) {
        if (s_store->slots[s_learn_slot].kind == IR_SLOT_LIBRARY) {
            slot_reset_empty((uint8_t)s_learn_slot);
        }
    }
    s_learn_slot = -1;
    s_learn_status = IR_LEARN_IDLE;
    s_learn_msg[0] = 0;
}

uint8_t ir_blaster_learn_status(void)
{
    return s_learn_status;
}

int ir_blaster_learn_slot(void)
{
    return s_learn_slot;
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
    int n = ir_blaster_lib_button_count(slot);
    if ((n <= 0) || (index < 0) || (index >= n)) {
        return "";
    }
    if (s_lib_label[slot][index][0]) {
        return s_lib_label[slot][index];
    }
    if (n == 1) {
        return k_lib_names[IR_LIB_BTN_SEND];
    }
    return k_lib_names[index];
}

esp_err_t ir_blaster_rename_lib(uint8_t slot, int index, const char *name)
{
    int n = ir_blaster_lib_button_count(slot);
    if ((name == NULL) || (n <= 0) || (index < 0) || (index >= n)) {
        return ESP_ERR_INVALID_ARG;
    }
    strncpy(s_lib_label[slot][index], name, IR_BLASTER_NAME_MAX - 1);
    s_lib_label[slot][index][IR_BLASTER_NAME_MAX - 1] = 0;
    return store_save();
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
