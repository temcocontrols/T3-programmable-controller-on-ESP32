#pragma once

#include <stdbool.h>
#include "esp_http_server.h"

/* Status reported by an optional device-side (JSON) renderer. */
typedef struct {
    bool ui_json;       /* JSON UI is driving the display */
    bool build_ok;
    bool forced_compiled; /* runtime switch: JSON renderer disabled by request */
    char screen[40];    /* current screen name */
    char last_error[96];
} screen_ui_status_t;

typedef void (*screen_ui_status_fn)(screen_ui_status_t *out);
/* Called after a successful commit; returns true when a UI reload was queued. */
typedef bool (*screen_ui_apply_fn)(const char *slot, int revision);

/* Explicit JSON / compiled switch (GET+POST /api/eez-device/ui-mode).
 * set_fn(true) means "use the compiled SquareLine UI"; returns the resulting
 * state as 1 (compiled) or 0 (JSON). Either hook may be NULL. */
typedef int (*screen_ui_set_mode_fn)(bool compiled_ui);

/* Queue navigation to a design screen; returns false when unavailable. */
typedef bool (*screen_ui_goto_fn)(const char *screen_name);

/* Enable/disable the renderer's geometry dump (POST /api/eez-device/ui-debug).
 * enable < 0 only reports the current state.  Returns 1 when enabled. */
typedef int (*screen_ui_debug_fn)(int enable);

/* Registers the renderer hooks (all may be NULL to report "compiled"). */
void screen_deploy_set_ui_hooks(screen_ui_status_fn status_fn, screen_ui_apply_fn apply_fn);
void screen_deploy_set_ui_mode_hook(screen_ui_set_mode_fn set_mode_fn);
void screen_deploy_set_ui_goto_hook(screen_ui_goto_fn goto_fn);
void screen_deploy_set_ui_debug_hook(screen_ui_debug_fn debug_fn);

/* Registers the deploy-transaction, capacity and status endpoints.
 * Called once from dynamic_display_task() after the legacy routes. */
esp_err_t screen_deploy_register_handlers(httpd_handle_t server);
