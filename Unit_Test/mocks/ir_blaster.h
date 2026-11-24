#ifndef IR_BLASTER_H
#define IR_BLASTER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IR_BLASTER_SLOT_COUNT       10
#define IR_BLASTER_CUSTOM_PER_SLOT  10
#define IR_BLASTER_CUSTOM_TOTAL     20
#define IR_BLASTER_MAX_RAW          400
#define IR_BLASTER_STATE_MAX        64
#define IR_BLASTER_NAME_MAX         24
#define IR_BLASTER_MINI_TSTAT11     31

#define IR_SLOT_EMPTY     0
#define IR_SLOT_LIBRARY   1
#define IR_SLOT_CUSTOM    2

#define IR_LEARN_IDLE     0
#define IR_LEARN_WAIT     1
#define IR_LEARN_OK       2
#define IR_LEARN_FAIL     3
#define IR_LEARN_ASK_LIB  4

#define IR_LIB_BTN_POWER      0
#define IR_LIB_BTN_MODE       1
#define IR_LIB_BTN_FAN        2
#define IR_LIB_BTN_TEMP_UP    3
#define IR_LIB_BTN_TEMP_DOWN  4
#define IR_LIB_BTN_SWINGV     5
#define IR_LIB_BTN_SWINGH     6
#define IR_LIB_BTN_TURBO      7
#define IR_LIB_BTN_SLEEP      8
#define IR_LIB_BTN_QUIET      9
#define IR_LIB_BTN_SEND       10
#define IR_LIB_BTN_MAX        11

typedef struct {
    uint8_t  kind;
    char     name[IR_BLASTER_NAME_MAX];
    char     proto_name[IR_BLASTER_NAME_MAX];
    uint8_t  custom_count;
    uint8_t  used;
} ir_blaster_slot_info_t;

static inline esp_err_t ir_blaster_init(uint8_t mini_type)
{
    (void)mini_type;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline bool ir_blaster_is_supported(void) { return false; }
static inline uint8_t ir_blaster_custom_used(void) { return 0; }
static inline uint8_t ir_blaster_custom_free(void) { return 0; }
static inline esp_err_t ir_blaster_get_slot_info(uint8_t slot, ir_blaster_slot_info_t *out)
{
    (void)slot;
    (void)out;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_clear_slot(uint8_t slot)
{
    (void)slot;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_rename_slot(uint8_t slot, const char *name)
{
    (void)slot;
    (void)name;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_begin_learn(uint8_t slot, uint8_t add_custom)
{
    (void)slot;
    (void)add_custom;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_confirm_library(uint8_t accept)
{
    (void)accept;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline void ir_blaster_cancel_learn(void) {}
static inline uint8_t ir_blaster_learn_status(void) { return IR_LEARN_IDLE; }
static inline int ir_blaster_learn_slot(void) { return -1; }
static inline const char *ir_blaster_learn_message(void) { return ""; }
static inline int ir_blaster_lib_button_count(uint8_t slot)
{
    (void)slot;
    return 0;
}
static inline const char *ir_blaster_lib_button_name(uint8_t slot, int index)
{
    (void)slot;
    (void)index;
    return "";
}
static inline esp_err_t ir_blaster_rename_lib(uint8_t slot, int index, const char *name)
{
    (void)slot;
    (void)index;
    (void)name;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_send_lib(uint8_t slot, int index)
{
    (void)slot;
    (void)index;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline int ir_blaster_custom_count(uint8_t slot)
{
    (void)slot;
    return 0;
}
static inline esp_err_t ir_blaster_custom_name(uint8_t slot, int index, char *out, size_t out_len)
{
    (void)slot;
    (void)index;
    (void)out;
    (void)out_len;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_rename_custom(uint8_t slot, int index, const char *name)
{
    (void)slot;
    (void)index;
    (void)name;
    return ESP_ERR_NOT_SUPPORTED;
}
static inline esp_err_t ir_blaster_send_custom(uint8_t slot, int index)
{
    (void)slot;
    (void)index;
    return ESP_ERR_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif
