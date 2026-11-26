#ifndef UI_JSON_BIND_H
#define UI_JSON_BIND_H

#include <stdbool.h>
#include <stddef.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Capture the compiled widget pointers so the JSON UI can hand them back. */
void ui_json_bind_snapshot(void);

/* Hand every global back to its compiled object (call before deleting JSON screens). */
void ui_json_bind_restore(void);

/* Point ui_<name>/uic_<name> at obj; returns true when at least one slot matched. */
bool ui_json_bind_apply(const char *name, lv_obj_t *obj);

size_t ui_json_bind_count(void);

/* The compiled object this design name maps to (captured at boot), so the
 * renderer can inherit flags the JSON export does not carry. */
lv_obj_t *ui_json_bind_original(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* UI_JSON_BIND_H */
