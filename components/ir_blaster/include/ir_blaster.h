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
#define IR_BLASTER_MINI_TSTAT11     31  /* same as MINI_TSTAT11 in define.h */

#define IR_SLOT_EMPTY     0
#define IR_SLOT_LIBRARY   1
#define IR_SLOT_CUSTOM    2

#define IR_LEARN_IDLE     0
#define IR_LEARN_WAIT     1
#define IR_LEARN_OK       2
#define IR_LEARN_FAIL     3

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

/* Call only for TSTAT11. Pass Modbus.mini_type; other products return NOT_SUPPORTED. */
esp_err_t ir_blaster_init(uint8_t mini_type);
/* True only after a successful init on MINI_TSTAT11. Use this to hide LCD UI. */
bool ir_blaster_is_supported(void);

uint8_t ir_blaster_custom_used(void);
uint8_t ir_blaster_custom_free(void);

esp_err_t ir_blaster_get_slot_info(uint8_t slot, ir_blaster_slot_info_t *out);
esp_err_t ir_blaster_clear_slot(uint8_t slot);
esp_err_t ir_blaster_rename_slot(uint8_t slot, const char *name);

esp_err_t ir_blaster_begin_learn(uint8_t slot, uint8_t add_custom);
void      ir_blaster_cancel_learn(void);
uint8_t   ir_blaster_learn_status(void);
const char *ir_blaster_learn_message(void);

int       ir_blaster_lib_button_count(uint8_t slot);
const char *ir_blaster_lib_button_name(uint8_t slot, int index);
esp_err_t ir_blaster_send_lib(uint8_t slot, int index);

int       ir_blaster_custom_count(uint8_t slot);
esp_err_t ir_blaster_custom_name(uint8_t slot, int index, char *out, size_t out_len);
esp_err_t ir_blaster_rename_custom(uint8_t slot, int index, const char *name);
esp_err_t ir_blaster_send_custom(uint8_t slot, int index);

#ifdef __cplusplus
}
#endif

#endif
