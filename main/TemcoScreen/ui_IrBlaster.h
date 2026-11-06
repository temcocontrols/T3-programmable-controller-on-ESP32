#ifndef UI_IRBLASTER_H
#define UI_IRBLASTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

extern lv_obj_t *ui_IrBlaster;
extern lv_obj_t *ui_IrSlotDetail;

/* True only on TSTAT11 after IR hardware init. Use to hide the Main Menu entry. */
bool ui_ir_blaster_visible(void);

void ui_IrBlaster_screen_init(void);
void ui_IrBlaster_screen_destroy(void);
void ui_IrBlaster_refresh(void);

void ui_IrSlotDetail_screen_init(void);
void ui_IrSlotDetail_screen_destroy(void);
void ui_IrSlotDetail_refresh(void);
void ui_IrSlotDetail_set_slot(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif
