#ifndef UI_REMOTE_ACCESS_H
#define UI_REMOTE_ACCESS_H

#include "lvgl.h"

extern lv_obj_t *ui_WireGuardScreen;
extern void ui_WireGuardScreen_screen_init(void);
extern void ui_WireGuardScreen_screen_destroy(void);
#if 1
extern lv_obj_t *ui_DdnsScreen;
extern void ui_DdnsScreen_screen_init(void);
extern void ui_DdnsScreen_screen_destroy(void);
#endif
#endif
