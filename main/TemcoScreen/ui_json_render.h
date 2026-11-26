#pragma once

#include <stdbool.h>

/* Registers the JSON renderer with the deploy API (status + hot reload). */
void ui_json_render_register(void);

/* Called from the LVGL task loop: builds or hot-reloads the UI from the
 * active slot. Safe to call every iteration. */
void ui_json_render_service(void);

/* True while the JSON UI is driving the display (compiled navigation is
 * suppressed in that case). */
bool ui_json_render_active(void);

/* Maps the active JSON screen back to the compiled screen object it represents
 * so per-screen refresh/dispatch code that compares pointers keeps working.
 * Returns `active` unchanged when the JSON UI is not driving the display. */
struct _lv_obj_t;
struct _lv_obj_t *ui_json_render_compiled_screen(struct _lv_obj_t *active);

/* Queue a navigation to a design screen (served from the active slot).
 * Thread-safe: the request is applied by the LVGL task. */
bool ui_json_render_goto(const char *screen_name);

/* Look up a widget by its design name on the currently active JSON screen.
 * Returns NULL when the JSON UI is inactive or the name is not on that screen,
 * so callers can fall back to the compiled widget.  Needed by the handlers that
 * address their widgets through file-static pointers (WireGuard / DDNS), which
 * still point at the compiled objects after the JSON UI takes over. */
struct _lv_obj_t *ui_json_render_find(const char *name);
