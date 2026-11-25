/*
 * Runtime renderer for the deployed screen JSON (Level 1: visual MVP).
 *
 * Screens are read from the active slot of the "screen_data" SPIFFS partition
 * and turned into LVGL objects. Supported today:
 *   - screen background
 *   - panel / container (bg, border, radius, padding, gradients, shadows)
 *   - label (text, font, alignment, colour)
 *   - image / imagebutton (PNG from images/<name>.json, base64)
 *   - buttons, switches, textareas, dropdowns, checkboxes, rollers, arcs and
 *     calendars (visual only, no Modbus binding yet)
 *   - events: screen_change navigation (SCREEN_LOADED / READY delay, CLICKED,
 *     ALL, VALUE_CHANGED, FOCUSED, DEFOCUSED) and flag_modify show/hide of a
 *     widget addressed by design name
 *
 * Everything that is not understood is ignored; if the start screen cannot be
 * built the compiled ui_init() UI stays on the display.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "cJSON.h"
#include "mbedtls/base64.h"

#include "lvgl.h"
#include "misc/cache/instance/lv_image_cache.h"
#include "misc/cache/instance/lv_image_header_cache.h"
#include "ui.h"
#include "screen_transaction.h"
#include "screen_deploy_api.h"
#include "ui_json_render.h"
#include "ui_json_bind.h"
#include "ui_json_events.h"

#define TAG "JsonRender"
#define MAX_SCREENS 16
#define MAX_IMAGES 24

typedef struct {
    char name[40];
    lv_obj_t *scr;
} scr_cache_t;

typedef struct {
    char name[48];
    lv_image_dsc_t dsc;
    void *pixels;
} img_cache_t;

typedef struct {
    char name[40];
    int anim;
    int speed;
    int delay;        /* ms before navigating; used when loading a screen triggers it */
} nav_req_t;

/* flag_modify action: show/hide another widget of the same screen */
typedef struct {
    char target[32];
    char mode[8];     /* add | remove | toggle */
    lv_obj_t *scope;  /* only used when the action is deferred from build time */
} flag_req_t;

/* design name -> object, for the widgets that events address by name */
typedef struct {
    char name[32];
    lv_obj_t *obj;
} named_obj_t;

#define MAX_NAMED_OBJS 96
/* 1 = compile the geometry dumper in.  It stays disabled at runtime because a
 * dump blocks the LVGL task for up to a second on the 115200 console; enable it
 * with POST /api/eez-device/ui-debug {"layout":true} while chasing a layout bug. */
#define UI_JSON_DEBUG_LAYOUT 1

#if UI_JSON_DEBUG_LAYOUT
typedef struct {
    char name[24];
    lv_obj_t *obj;
} dbg_rec_t;
#define DBG_MAX 200
static dbg_rec_t s_dbg[DBG_MAX];
static int s_dbg_n;
static char s_dbg_screen[32];
static bool s_dbg_on;

static int dbg_set(int on)
{
    if (on >= 0 && (bool)on != s_dbg_on) {
        s_dbg_on = on != 0;
        ESP_LOGW(TAG, "layout dump %s", s_dbg_on ? "enabled" : "disabled");
    }
    return s_dbg_on ? 1 : 0;
}

static void dbg_add(const char *name, lv_obj_t *o)
{
    if (!s_dbg_on || !name || !o || s_dbg_n >= DBG_MAX) return;
    snprintf(s_dbg[s_dbg_n].name, sizeof(s_dbg[s_dbg_n].name), "%s", name);
    s_dbg[s_dbg_n].obj = o;
    s_dbg_n++;
}

static void dbg_dump_cb(lv_timer_t *t)
{
    lv_obj_t *scr = (lv_obj_t *)lv_timer_get_user_data(t);
    if (scr && lv_obj_is_valid(scr)) lv_obj_update_layout(scr);
    ESP_LOGI(TAG, "---- %s: layout dump (%d objects) ----", s_dbg_screen, s_dbg_n);
    for (int i = 0; i < s_dbg_n; i++) {
        lv_obj_t *o = s_dbg[i].obj;
        if (!o || !lv_obj_is_valid(o)) continue;
        lv_area_t a;
        lv_obj_get_coords(o, &a);
        const lv_style_selector_t sel = LV_PART_MAIN | LV_STATE_DEFAULT;
        int bw = (int)lv_obj_get_style_border_width(o, sel);
        int bo = (int)lv_obj_get_style_border_opa(o, sel);
        int bg = (int)lv_obj_get_style_bg_opa(o, sel);
        int rad = (int)lv_obj_get_style_radius(o, sel);
        const void *isrc = NULL;
        if (lv_obj_check_type(o, &lv_image_class)) isrc = lv_image_get_src(o);
        ESP_LOGI(TAG, "  %-22s %3d,%-3d %3dx%-3d rel=%d,%d sty=%dx%d al=%d hid=%d foc=%d bg=%3d bw=%d bo=%3d rad=%d flex=%d pad=%d,%d,%d,%d img=%d",
                 s_dbg[i].name, (int)a.x1, (int)a.y1,
                 (int)(a.x2 - a.x1 + 1), (int)(a.y2 - a.y1 + 1),
                 (int)lv_obj_get_x(o), (int)lv_obj_get_y(o),
                 (int)lv_obj_get_style_width(o, sel), (int)lv_obj_get_style_height(o, sel),
                 (int)lv_obj_get_style_align(o, sel),
                 lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN) ? 1 : 0,
                 lv_obj_has_state(o, LV_STATE_FOCUSED) ? 1 : 0,
                 bg, bw, bo, rad,
                 (int)lv_obj_get_style_flex_flow(o, sel),
                 (int)lv_obj_get_style_pad_left(o, sel),
                 (int)lv_obj_get_style_pad_top(o, sel),
                 (int)lv_obj_get_style_pad_right(o, sel),
                 (int)lv_obj_get_style_pad_bottom(o, sel),
                 isrc ? 1 : 0);
    }
}

/* watch read-only value fields for focus changes (they must never show a caret) */
#define WATCH_MAX 10
static struct {
    const char *name;
    lv_obj_t *obj;
    bool foc;
} s_watch[WATCH_MAX];
static int s_watch_n;

static void watch_focus(const char *name, lv_obj_t *o)
{
    if (s_watch_n >= WATCH_MAX) return;
    s_watch[s_watch_n].name = name;
    s_watch[s_watch_n].obj = o;
    s_watch[s_watch_n].foc = false;
    s_watch_n++;
}

static void watch_poll(void)
{
    for (int i = 0; i < s_watch_n; i++) {
        lv_obj_t *o = s_watch[i].obj;
        if (!o || !lv_obj_is_valid(o)) continue;
        bool f = lv_obj_has_state(o, LV_STATE_FOCUSED);
        if (f != s_watch[i].foc) {
            s_watch[i].foc = f;
            ESP_LOGW(TAG, "focus %s = %d", s_watch[i].name ? s_watch[i].name : "?", f);
        }
    }
}
#else
static void watch_focus(const char *name, lv_obj_t *o) { (void)name; (void)o; }
static void watch_poll(void) { }
#endif

static scr_cache_t s_screens[MAX_SCREENS];
static int s_screen_count;
static img_cache_t s_images[MAX_IMAGES];
static int s_image_count;
static named_obj_t s_named[MAX_NAMED_OBJS];
static int s_named_count;
static bool s_named_overflow;

/* soft keyboards, so clicking a textarea can attach the one on its screen */
#define MAX_KEYBOARDS 8
static lv_obj_t *s_keyboards[MAX_KEYBOARDS];
static int s_keyboard_count;

static void register_keyboard(lv_obj_t *kb)
{
    if (!kb || s_keyboard_count >= MAX_KEYBOARDS) return;
    s_keyboards[s_keyboard_count++] = kb;
}

static lv_obj_t *find_keyboard(lv_obj_t *scope)
{
    lv_obj_t *scr = lv_obj_get_screen(scope);
    for (int i = 0; i < s_keyboard_count; i++) {
        lv_obj_t *kb = s_keyboards[i];
        if (kb && lv_obj_get_screen(kb) == scr) return kb;
    }
    return NULL;
}

static lv_obj_t *find_named(lv_obj_t *scope, const char *name);

/* The exported JSON has no keyboard->textarea link (the compiled screens get it
 * from _ui_keyboard_set_target), so attach the screen's keyboard on click. */
static void textarea_clicked_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_current_target(e);
    lv_obj_t *kb = find_keyboard(ta);
    if (!kb) return;
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

/* a ready/cancel key closes the keyboard, as in the compiled screens */
static void keyboard_done_cb(lv_event_t *e)
{
    lv_obj_t *kb = lv_event_get_current_target(e);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

/* Forwards LVGL events to the compiled ui_event_<Name>() handler of the widget's
 * design name.  The exported JSON only carries declarative actions (show/hide,
 * navigate); everything semantic (save, apply, scan, set mode…) lives in the
 * firmware handlers, so the renderer bridges by name. */
static void bridge_event_cb(lv_event_t *e)
{
    ui_json_event_fn fn = (ui_json_event_fn)lv_event_get_user_data(e);
    if (fn) fn(e);
}

static void bridge_attach(lv_obj_t *o, const char *name)
{
    ui_json_event_fn fn = ui_json_events_lookup(name);
    if (!fn) return;
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(o, bridge_event_cb, LV_EVENT_ALL, (void *)fn);
}

/* keyboards declared with an explicit target (future JSON schema):
 * "textarea": "PasswordText"  or  "target": "PasswordText" */
typedef struct {
    lv_obj_t *kb;
    lv_obj_t *scope;
    char target[32];
} kb_target_t;

#define MAX_KB_TARGETS 8
static kb_target_t s_kb_targets[MAX_KB_TARGETS];
static int s_kb_target_count;

static void kb_target_timer_cb(lv_timer_t *t)
{
    kb_target_t *r = lv_timer_get_user_data(t);
    if (!r) return;
    lv_obj_t *ta = find_named(r->scope, r->target);
    if (ta && r->kb) lv_keyboard_set_textarea(r->kb, ta);
    else ESP_LOGW(TAG, "keyboard target %s not found", r->target);
}

static bool s_active;
static bool s_build_ok = true;
static bool s_boot_done;
static bool s_force_compiled;   /* explicit switch: use the compiled SquareLine UI */
static volatile bool s_reload;
static char s_goto[40];
static volatile bool s_goto_req;
static char s_current[40];
static char s_error[96];
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

/* ---------- small helpers ---------- */

static int jint(const cJSON *o, const char *k, int def)
{
    const cJSON *v = o ? cJSON_GetObjectItem(o, k) : NULL;
    return cJSON_IsNumber(v) ? v->valueint : def;
}

static const char *jstr(const cJSON *o, const char *k)
{
    const cJSON *v = o ? cJSON_GetObjectItem(o, k) : NULL;
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

static bool jbool(const cJSON *o, const char *k)
{
    const cJSON *v = o ? cJSON_GetObjectItem(o, k) : NULL;
    if (cJSON_IsBool(v)) return cJSON_IsTrue(v);
    if (cJSON_IsNumber(v)) return v->valueint != 0;
    return false;
}

static lv_color_t parse_color(const char *s)
{
    if (!s || !s[0]) return lv_color_black();
    if (s[0] == '#') s++;
    return lv_color_hex((uint32_t)strtoul(s, NULL, 16));
}

static lv_align_t parse_align(const char *s)
{
    if (!s) return LV_ALIGN_CENTER;
    if (!strcmp(s, "top_left")) return LV_ALIGN_TOP_LEFT;
    if (!strcmp(s, "top_mid")) return LV_ALIGN_TOP_MID;
    if (!strcmp(s, "top_right")) return LV_ALIGN_TOP_RIGHT;
    if (!strcmp(s, "left_mid")) return LV_ALIGN_LEFT_MID;
    if (!strcmp(s, "right_mid")) return LV_ALIGN_RIGHT_MID;
    if (!strcmp(s, "bottom_left")) return LV_ALIGN_BOTTOM_LEFT;
    if (!strcmp(s, "bottom_mid")) return LV_ALIGN_BOTTOM_MID;
    if (!strcmp(s, "bottom_right")) return LV_ALIGN_BOTTOM_RIGHT;
    return LV_ALIGN_CENTER;
}

static const lv_font_t *parse_font(const char *s)
{
    if (!s) return NULL;
    if (!strcmp(s, "Arial80") || !strcmp(s, "ui_font_Arial80")) return &ui_font_Arial80;
    if (!strcmp(s, "lv_font_montserrat_10")) return &lv_font_montserrat_10;
    if (!strcmp(s, "lv_font_montserrat_12")) return &lv_font_montserrat_12;
    if (!strcmp(s, "lv_font_montserrat_14")) return &lv_font_montserrat_14;
    if (!strcmp(s, "lv_font_montserrat_16")) return &lv_font_montserrat_16;
    if (!strcmp(s, "lv_font_montserrat_18")) return &lv_font_montserrat_18;
    if (!strcmp(s, "lv_font_montserrat_30")) return &lv_font_montserrat_30;
    if (!strcmp(s, "lv_font_montserrat_36")) return &lv_font_montserrat_36;
    if (!strcmp(s, "lv_font_montserrat_40")) return &lv_font_montserrat_40;
    return NULL;
}

/* lv_obj_align_to() variant used by "align_to": out_top_left, out_bottom_mid, ... */
static lv_align_t parse_align_out(const char *s)
{
    if (!s) return LV_ALIGN_OUT_RIGHT_MID;
    if (!strcmp(s, "out_top_left")) return LV_ALIGN_OUT_TOP_LEFT;
    if (!strcmp(s, "out_top_mid")) return LV_ALIGN_OUT_TOP_MID;
    if (!strcmp(s, "out_top_right")) return LV_ALIGN_OUT_TOP_RIGHT;
    if (!strcmp(s, "out_bottom_left")) return LV_ALIGN_OUT_BOTTOM_LEFT;
    if (!strcmp(s, "out_bottom_mid")) return LV_ALIGN_OUT_BOTTOM_MID;
    if (!strcmp(s, "out_bottom_right")) return LV_ALIGN_OUT_BOTTOM_RIGHT;
    if (!strcmp(s, "out_left_top")) return LV_ALIGN_OUT_LEFT_TOP;
    if (!strcmp(s, "out_left_mid")) return LV_ALIGN_OUT_LEFT_MID;
    if (!strcmp(s, "out_left_bottom")) return LV_ALIGN_OUT_LEFT_BOTTOM;
    if (!strcmp(s, "out_right_top")) return LV_ALIGN_OUT_RIGHT_TOP;
    if (!strcmp(s, "out_right_bottom")) return LV_ALIGN_OUT_RIGHT_BOTTOM;
    return LV_ALIGN_OUT_RIGHT_MID;
}

static int parse_anim(const char *s)
{
    if (!s) return LV_SCREEN_LOAD_ANIM_NONE;
    if (!strcmp(s, "FADE_IN") || !strcmp(s, "FADE_ON")) return LV_SCREEN_LOAD_ANIM_FADE_IN;
    if (!strcmp(s, "FADE_OUT")) return LV_SCREEN_LOAD_ANIM_FADE_OUT;
    if (!strcmp(s, "MOVE_LEFT")) return LV_SCREEN_LOAD_ANIM_MOVE_LEFT;
    if (!strcmp(s, "MOVE_RIGHT")) return LV_SCREEN_LOAD_ANIM_MOVE_RIGHT;
    if (!strcmp(s, "MOVE_TOP")) return LV_SCREEN_LOAD_ANIM_MOVE_TOP;
    if (!strcmp(s, "MOVE_BOTTOM")) return LV_SCREEN_LOAD_ANIM_MOVE_BOTTOM;
    if (!strcmp(s, "OVER_LEFT")) return LV_SCREEN_LOAD_ANIM_OVER_LEFT;
    if (!strcmp(s, "OVER_RIGHT")) return LV_SCREEN_LOAD_ANIM_OVER_RIGHT;
    if (!strcmp(s, "OVER_TOP")) return LV_SCREEN_LOAD_ANIM_OVER_TOP;
    if (!strcmp(s, "OVER_BOTTOM")) return LV_SCREEN_LOAD_ANIM_OVER_BOTTOM;
    if (!strcmp(s, "OUT_LEFT")) return LV_SCREEN_LOAD_ANIM_OUT_LEFT;
    if (!strcmp(s, "OUT_RIGHT")) return LV_SCREEN_LOAD_ANIM_OUT_RIGHT;
    if (!strcmp(s, "OUT_TOP")) return LV_SCREEN_LOAD_ANIM_OUT_TOP;
    if (!strcmp(s, "OUT_BOTTOM")) return LV_SCREEN_LOAD_ANIM_OUT_BOTTOM;
    return LV_SCREEN_LOAD_ANIM_NONE;
}

static lv_flex_align_t parse_flex_align(const char *s)
{
    if (!s) return LV_FLEX_ALIGN_START;
    if (!strcmp(s, "space_between")) return LV_FLEX_ALIGN_SPACE_BETWEEN;
    if (!strcmp(s, "space_around")) return LV_FLEX_ALIGN_SPACE_AROUND;
    if (!strcmp(s, "space_evenly")) return LV_FLEX_ALIGN_SPACE_EVENLY;
    if (!strcmp(s, "center")) return LV_FLEX_ALIGN_CENTER;
    if (!strcmp(s, "end")) return LV_FLEX_ALIGN_END;
    return LV_FLEX_ALIGN_START;
}

/* EEZ exports the side as a string ("none", "bottom", "full", ...) */
static lv_border_side_t parse_border_side(const cJSON *v)
{
    if (cJSON_IsString(v) && v->valuestring) {
        const char *s = v->valuestring;
        if (!strcmp(s, "none")) return LV_BORDER_SIDE_NONE;
        if (!strcmp(s, "bottom")) return LV_BORDER_SIDE_BOTTOM;
        if (!strcmp(s, "top")) return LV_BORDER_SIDE_TOP;
        if (!strcmp(s, "left")) return LV_BORDER_SIDE_LEFT;
        if (!strcmp(s, "right")) return LV_BORDER_SIDE_RIGHT;
        if (!strcmp(s, "full")) return LV_BORDER_SIDE_FULL;
        return LV_BORDER_SIDE_FULL;
    }
    if (cJSON_IsNumber(v)) {
        return (lv_border_side_t)v->valueint;
    }
    return LV_BORDER_SIDE_FULL;
}

/* ---------- status ---------- */

static void set_active(bool v)
{
    taskENTER_CRITICAL(&s_lock);
    s_active = v;
    taskEXIT_CRITICAL(&s_lock);
}

static void set_build_ok(bool v)
{
    taskENTER_CRITICAL(&s_lock);
    s_build_ok = v;
    taskEXIT_CRITICAL(&s_lock);
}

static void set_error(const char *e)
{
    taskENTER_CRITICAL(&s_lock);
    snprintf(s_error, sizeof(s_error), "%s", e ? e : "");
    taskEXIT_CRITICAL(&s_lock);
}

static void set_current(const char *n)
{
    taskENTER_CRITICAL(&s_lock);
    snprintf(s_current, sizeof(s_current), "%s", n ? n : "");
    taskEXIT_CRITICAL(&s_lock);
}

/* ---------- images ---------- */

static lv_image_dsc_t *get_image(const char *name)
{
    if (!name || !name[0]) return NULL;
    for (int i = 0; i < s_image_count; i++) {
        if (!strcmp(s_images[i].name, name)) return &s_images[i].dsc;
    }
    if (s_image_count >= MAX_IMAGES) return NULL;

    char *buf = NULL;
    size_t len = 0;
    if (screen_txn_read_active("images", name, &buf, &len) != ESP_OK || !buf) {
        ESP_LOGW(TAG, "image %s not readable", name);
        return NULL;
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    free(buf);
    if (!root) return NULL;

    const char *b64 = jstr(root, "png_base64");
    if (!b64) b64 = jstr(root, "data_base64");
    if (!b64) {
        cJSON_Delete(root);
        return NULL;
    }

    size_t b64len = strlen(b64);
    size_t png_len = 0;
    int rc = mbedtls_base64_decode(NULL, 0, &png_len, (const unsigned char *)b64, b64len);
    if (rc != 0 && rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        cJSON_Delete(root);
        return NULL;
    }

    unsigned char *png = malloc(png_len + 1);
    size_t written = 0;
    if (!png ||
        mbedtls_base64_decode(png, png_len, &written, (const unsigned char *)b64, b64len) != 0) {
        free(png);
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_Delete(root);

    /* Keep the PNG bytes in the descriptor; LVGL's LODEPNG decoder is selected
     * by the PNG signature and the result is held in the image cache. */
    img_cache_t *e = &s_images[s_image_count];
    memset(e, 0, sizeof(*e));
    snprintf(e->name, sizeof(e->name), "%s", name);
    e->pixels = png;
    e->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    e->dsc.header.cf = LV_COLOR_FORMAT_RAW;
    e->dsc.data_size = (uint32_t)written;
    e->dsc.data = png;
    s_image_count++;

    return &e->dsc;
}

/* ---------- navigation ---------- */

static lv_obj_t *get_screen(const char *name);
static lv_obj_t *build_screen(const char *name);
static void settle_content(lv_obj_t *scr);
static lv_obj_t *find_named(lv_obj_t *scope, const char *name);

static void navigate(const char *name, int anim, int speed)
{
    lv_obj_t *scr = get_screen(name);
    if (!scr) {
        ESP_LOGW(TAG, "navigate: screen %s unavailable", name);
        set_error("target screen build failed");
        return;
    }
    lv_screen_load_anim(scr, (lv_screen_load_anim_t)anim, (uint32_t)speed, 0, false);
    settle_content(scr);
    set_current(name);
}

static void nav_timer_cb(lv_timer_t *t)
{
    nav_req_t *r = lv_timer_get_user_data(t);
    if (r) {
        navigate(r->name, r->anim, r->speed);
        free(r);
    }
}

static void nav_clicked_cb(lv_event_t *e)
{
    nav_req_t *r = lv_event_get_user_data(e);
    if (r) navigate(r->name, r->anim, r->speed);
}

/* Screen-loading action ("SCREEN_LOADED"): EEZ's delay is the hold time, e.g.
 * the splash screen stays up for 3000 ms before switching to home. */
static void nav_loaded_cb(lv_event_t *e)
{
    nav_req_t *r = lv_event_get_user_data(e);
    if (!r) return;
    if (r->delay > 0) {
        nav_req_t *c = malloc(sizeof(*c));
        if (!c) return;
        *c = *r;
        lv_timer_t *t = lv_timer_create(nav_timer_cb, (uint32_t)r->delay, c);
        if (t) lv_timer_set_repeat_count(t, 1);
        else free(c);
        return;
    }
    navigate(r->name, r->anim, r->speed);
}

static void nav_free_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

/* The JSON export carries no flags at all, so take them from the compiled
 * widget of the same design name (snapshot from ui_init()).  This is what keeps
 * e.g. value textareas from getting focus (which paints a cursor box) and keeps
 * scroll behaviour identical to the SquareLine build. */
static void inherit_flags(lv_obj_t *o, const char *name)
{
    lv_obj_t *src = ui_json_bind_original(name);
    if (!src) return;

    static const lv_obj_flag_t flags[] = {
        LV_OBJ_FLAG_CLICKABLE,
        LV_OBJ_FLAG_CLICK_FOCUSABLE,
        LV_OBJ_FLAG_SCROLLABLE,
        LV_OBJ_FLAG_SCROLL_ELASTIC,
        LV_OBJ_FLAG_SCROLL_MOMENTUM,
        LV_OBJ_FLAG_SCROLL_CHAIN,
        LV_OBJ_FLAG_SCROLL_ON_FOCUS,
        LV_OBJ_FLAG_CHECKABLE,
        LV_OBJ_FLAG_PRESS_LOCK,
    };
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        if (lv_obj_has_flag(src, flags[i])) lv_obj_add_flag(o, flags[i]);
        else lv_obj_remove_flag(o, flags[i]);
    }
}

/* ---------- style ---------- */

static lv_style_selector_t parse_part(const char *p)
{
    if (!strcmp(p, "MAIN")) return LV_PART_MAIN;
    if (!strcmp(p, "SCROLLBAR")) return LV_PART_SCROLLBAR;
    if (!strcmp(p, "INDICATOR")) return LV_PART_INDICATOR;
    if (!strcmp(p, "KNOB")) return LV_PART_KNOB;
    if (!strcmp(p, "ITEMS")) return LV_PART_ITEMS;
    if (!strcmp(p, "SELECTED")) return LV_PART_SELECTED;
    if (!strcmp(p, "CURSOR")) return LV_PART_CURSOR;
    return LV_PART_MAIN;
}

static lv_state_t parse_state(const char *s)
{
    if (!strcmp(s, "PRESSED")) return LV_STATE_PRESSED;
    if (!strcmp(s, "CHECKED")) return LV_STATE_CHECKED;
    if (!strcmp(s, "FOCUSED")) return LV_STATE_FOCUSED;
    if (!strcmp(s, "EDITED")) return LV_STATE_EDITED;
    if (!strcmp(s, "HOVERED")) return LV_STATE_HOVERED;
    if (!strcmp(s, "DISABLED")) return LV_STATE_DISABLED;
    if (!strcmp(s, "SCROLLED")) return LV_STATE_SCROLLED;
    if (!strcmp(s, "USER_1")) return LV_STATE_USER_1;
    if (!strcmp(s, "USER_2")) return LV_STATE_USER_2;
    return LV_STATE_DEFAULT;
}

static void apply_style_props(lv_obj_t *o, const cJSON *def, lv_style_selector_t sel)
{
    const char *c;
    int n;

    if ((c = jstr(def, "bg_color"))) lv_obj_set_style_bg_color(o, parse_color(c), sel);
    if ((n = jint(def, "bg_opa", -1)) >= 0) lv_obj_set_style_bg_opa(o, (lv_opa_t)n, sel);
    if ((n = jint(def, "opa", -1)) >= 0) lv_obj_set_style_opa(o, (lv_opa_t)n, sel);
    if ((n = jint(def, "radius", -1)) >= 0) lv_obj_set_style_radius(o, n, sel);
    if ((n = jint(def, "border_width", -1)) >= 0) lv_obj_set_style_border_width(o, n, sel);
    if ((c = jstr(def, "border_color"))) lv_obj_set_style_border_color(o, parse_color(c), sel);
    if ((n = jint(def, "border_opa", -1)) >= 0) lv_obj_set_style_border_opa(o, (lv_opa_t)n, sel);
    const cJSON *bs = cJSON_GetObjectItem(def, "border_side");
    if (bs) lv_obj_set_style_border_side(o, parse_border_side(bs), sel);
    if ((n = jint(def, "pad_all", -1)) >= 0) lv_obj_set_style_pad_all(o, n, sel);
    if ((n = jint(def, "pad_top", -1)) >= 0) lv_obj_set_style_pad_top(o, n, sel);
    if ((n = jint(def, "pad_bottom", -1)) >= 0) lv_obj_set_style_pad_bottom(o, n, sel);
    if ((n = jint(def, "pad_left", -1)) >= 0) lv_obj_set_style_pad_left(o, n, sel);
    if ((n = jint(def, "pad_right", -1)) >= 0) lv_obj_set_style_pad_right(o, n, sel);
    if ((n = jint(def, "pad_row", -1)) >= 0) lv_obj_set_style_pad_row(o, n, sel);
    if ((n = jint(def, "pad_column", -1)) >= 0) lv_obj_set_style_pad_column(o, n, sel);
    if ((c = jstr(def, "text_color"))) lv_obj_set_style_text_color(o, parse_color(c), sel);
    if ((n = jint(def, "text_opa", -1)) >= 0) lv_obj_set_style_text_opa(o, (lv_opa_t)n, sel);
    if ((c = jstr(def, "text_font"))) {
        const lv_font_t *f = parse_font(c);
        if (f) lv_obj_set_style_text_font(o, f, sel);
    }
    if ((c = jstr(def, "text_align"))) {
        lv_text_align_t a = LV_TEXT_ALIGN_AUTO;
        if (!strcmp(c, "left")) a = LV_TEXT_ALIGN_LEFT;
        else if (!strcmp(c, "center")) a = LV_TEXT_ALIGN_CENTER;
        else if (!strcmp(c, "right")) a = LV_TEXT_ALIGN_RIGHT;
        lv_obj_set_style_text_align(o, a, sel);
    }
    if ((n = jint(def, "text_letter_space", -1)) >= 0) lv_obj_set_style_text_letter_space(o, n, sel);
    if ((n = jint(def, "text_line_space", -1)) >= 0) lv_obj_set_style_text_line_space(o, n, sel);
    if ((c = jstr(def, "text_decor"))) {
        lv_text_decor_t d = LV_TEXT_DECOR_NONE;
        if (strstr(c, "underline")) d |= LV_TEXT_DECOR_UNDERLINE;
        if (strstr(c, "strikethrough")) d |= LV_TEXT_DECOR_STRIKETHROUGH;
        lv_obj_set_style_text_decor(o, d, sel);
    }

    if ((c = jstr(def, "shadow_color"))) lv_obj_set_style_shadow_color(o, parse_color(c), sel);
    if ((n = jint(def, "shadow_opa", -1)) >= 0) lv_obj_set_style_shadow_opa(o, (lv_opa_t)n, sel);
    if ((n = jint(def, "shadow_width", -1)) >= 0) lv_obj_set_style_shadow_width(o, n, sel);
    if ((n = jint(def, "shadow_spread", -1)) >= 0) lv_obj_set_style_shadow_spread(o, n, sel);
    if ((n = jint(def, "shadow_offset_x", -1)) >= 0) lv_obj_set_style_shadow_offset_x(o, n, sel);
    if ((n = jint(def, "shadow_offset_y", -1)) >= 0) lv_obj_set_style_shadow_offset_y(o, n, sel);

    if ((c = jstr(def, "outline_color"))) lv_obj_set_style_outline_color(o, parse_color(c), sel);
    if ((n = jint(def, "outline_opa", -1)) >= 0) lv_obj_set_style_outline_opa(o, (lv_opa_t)n, sel);
    if ((n = jint(def, "outline_width", -1)) >= 0) lv_obj_set_style_outline_width(o, n, sel);
    if ((n = jint(def, "outline_pad", -1)) >= 0) lv_obj_set_style_outline_pad(o, n, sel);

    if ((c = jstr(def, "bg_grad_color"))) lv_obj_set_style_bg_grad_color(o, parse_color(c), sel);
    if ((c = jstr(def, "bg_grad_dir"))) {
        lv_grad_dir_t d = LV_GRAD_DIR_NONE;
        if (!strcmp(c, "hor")) d = LV_GRAD_DIR_HOR;
        else if (!strcmp(c, "ver")) d = LV_GRAD_DIR_VER;
        lv_obj_set_style_bg_grad_dir(o, d, sel);
    }
    if ((n = jint(def, "bg_main_stop", -1)) >= 0) lv_obj_set_style_bg_main_stop(o, n, sel);
    if ((n = jint(def, "bg_grad_stop", -1)) >= 0) lv_obj_set_style_bg_grad_stop(o, n, sel);

    if ((c = jstr(def, "arc_color"))) lv_obj_set_style_arc_color(o, parse_color(c), sel);
    if ((n = jint(def, "arc_opa", -1)) >= 0) lv_obj_set_style_arc_opa(o, (lv_opa_t)n, sel);
    if ((n = jint(def, "arc_width", -1)) >= 0) lv_obj_set_style_arc_width(o, n, sel);

    if ((n = jint(def, "transform_angle", -1)) >= 0) lv_obj_set_style_transform_angle(o, n, sel);
    if ((n = jint(def, "transform_zoom", -1)) >= 0) lv_obj_set_style_transform_zoom(o, n, sel);

    if ((c = jstr(def, "line_color"))) lv_obj_set_style_line_color(o, parse_color(c), sel);
    if ((n = jint(def, "line_width", -1)) >= 0) lv_obj_set_style_line_width(o, n, sel);
}

/* SquareLine nests every object as style/<PART>/<STATE>/{properties} */
static void apply_style(lv_obj_t *o, const cJSON *style)
{
    if (!cJSON_IsObject(style)) return;

    const cJSON *part = NULL;
    cJSON_ArrayForEach(part, style) {
        const cJSON *state = NULL;
        cJSON_ArrayForEach(state, part) {
            if (!cJSON_IsObject(state)) continue;
            const char *pn = part->string ? part->string : "MAIN";
            const char *sn = state->string ? state->string : "DEFAULT";
            apply_style_props(o, state, parse_part(pn) | parse_state(sn));
        }
    }
}

/* Positions are computed from the JSON geometry alone: LVGL does not resolve a
 * parent's size until it is laid out, which made align-based placement
 * non-deterministic.  A widget's own size is only used when the JSON declares
 * one, so content-sized labels fall back to LVGL's align (their parent's size is
 * declared, so that is stable). */
/* Widgets whose size comes from their content (labels, one-line textareas) can
 * only be placed correctly once LVGL knows their real size, i.e. after the
 * screen has been laid out. */
typedef struct {
    lv_obj_t *obj;
    lv_obj_t *scr;
    lv_align_t align;
    int x;
    int y;
    char target[32];   /* non-empty: lv_obj_align_to() against this design name */
} content_req_t;

#define MAX_CONTENT_REQ 260
static content_req_t s_content[MAX_CONTENT_REQ];
static int s_content_n;

static void queue_content(lv_obj_t *scr, lv_obj_t *o, lv_align_t a, int x, int y)
{
    if (!o || s_content_n >= MAX_CONTENT_REQ) return;
    s_content[s_content_n].obj = o;
    s_content[s_content_n].scr = scr;
    s_content[s_content_n].align = a;
    s_content[s_content_n].x = x;
    s_content[s_content_n].y = y;
    s_content[s_content_n].target[0] = '\0';
    s_content_n++;
}

/* lv_obj_align_to() needs both objects laid out, so it is deferred like the
 * content-sized placements it usually accompanies. */
static void queue_align_to(lv_obj_t *scr, lv_obj_t *o, const char *target,
                           lv_align_t a, int x, int y)
{
    if (!o || !target || !target[0] || s_content_n >= MAX_CONTENT_REQ) return;
    if (strlen(target) >= sizeof(s_content[0].target)) return;
    queue_content(scr, o, a, x, y);
    snprintf(s_content[s_content_n - 1].target, sizeof(s_content[0].target), "%s", target);
}

/* keyboard -> textarea links, resolved after the screen has been built */
#define MAX_KBD_LINKS 8
static struct {
    lv_obj_t *kbd;
    lv_obj_t *scr;
    char target[24];
} s_kbd[MAX_KBD_LINKS];
static int s_kbd_n;

static void queue_keyboard_link(lv_obj_t *scr, lv_obj_t *kbd, const char *target)
{
    if (!kbd || !target || !target[0] || s_kbd_n >= MAX_KBD_LINKS) return;
    if (strlen(target) >= sizeof(s_kbd[0].target)) return;
    s_kbd[s_kbd_n].kbd = kbd;
    s_kbd[s_kbd_n].scr = scr;
    snprintf(s_kbd[s_kbd_n].target, sizeof(s_kbd[0].target), "%s", target);
    s_kbd_n++;
}

static void settle_content(lv_obj_t *scr)
{
    if (!scr || !lv_obj_is_valid(scr)) return;
    lv_obj_update_layout(scr);

    /* keyboard -> textarea links can only be resolved once every child of the
     * screen exists, which is why the design's "textarea" name is queued. */
    int kkeep = 0;
    for (int i = 0; i < s_kbd_n; i++) {
        if (s_kbd[i].scr != scr) {
            s_kbd[kkeep++] = s_kbd[i];
            continue;
        }
        if (!s_kbd[i].kbd || !lv_obj_is_valid(s_kbd[i].kbd)) continue;
        lv_obj_t *t = find_named(s_kbd[i].kbd, s_kbd[i].target);
        if (!t) {
            ESP_LOGW(TAG, "keyboard target %s not found", s_kbd[i].target);
            continue;
        }
        lv_keyboard_set_textarea(s_kbd[i].kbd, t);
    }
    s_kbd_n = kkeep;

    /* Two phases: a self-alignment must be resolved before anything that is
     * aligned *to* it is positioned.  home_screen chains two levels
     * (Co2Label -> OccTimerLabel -> ModeBtn), so the dependent phase runs twice.
     * Everything is done in place: a local copy of the queue would need ~13 KB of
     * stack, which this task does not have. */
    for (int i = 0; i < s_content_n; i++) {
        if (s_content[i].scr != scr || s_content[i].target[0]) continue;
        lv_obj_t *o = s_content[i].obj;
        if (!o || !lv_obj_is_valid(o)) continue;
        lv_obj_align(o, s_content[i].align, s_content[i].x, s_content[i].y);
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < s_content_n; i++) {
            if (s_content[i].scr != scr || !s_content[i].target[0]) continue;
            lv_obj_t *o = s_content[i].obj;
            if (!o || !lv_obj_is_valid(o)) continue;
            lv_obj_t *t = find_named(o, s_content[i].target);
            if (!t) {
                if (pass == 0) ESP_LOGW(TAG, "align_to: target %s not found", s_content[i].target);
                continue;
            }
            lv_obj_align_to(o, t, s_content[i].align, s_content[i].x, s_content[i].y);
        }
    }
    int keep = 0;
    for (int i = 0; i < s_content_n; i++) {
        if (s_content[i].scr != scr) s_content[keep++] = s_content[i];
    }
    s_content_n = keep;
}

/* SquareLine positions a child with lv_obj_set_align() + lv_obj_set_pos(): the
 * alignment stays *live*, so LVGL re-centres the child whenever its parent
 * changes size.  Absolute coordinates cannot follow that, and the design's
 * show/hide animations change the parent's size and y offset on every frame
 * (ui.c ShowItem_Animation animates height 100->0), which left the children of
 * such a panel behind.  LVGL resolves the alignment inside the parent's content
 * box, so the padding is applied exactly once. */
static void place_child(lv_obj_t *o, const cJSON *w)
{
    lv_obj_set_align(o, parse_align(jstr(w, "align")));
    lv_obj_set_pos(o, jint(w, "x_pos", 0), jint(w, "y_pos", 0));
}

static void apply_geometry(lv_obj_t *o, const cJSON *w, bool is_screen)
{
    if (is_screen) return;

    int wd = jint(w, "width", 0);
    int ht = jint(w, "height", 0);
    lv_obj_set_width(o, wd > 0 ? wd : LV_SIZE_CONTENT);
    lv_obj_set_height(o, ht > 0 ? ht : LV_SIZE_CONTENT);
}

static void apply_flex(lv_obj_t *o, const cJSON *w)
{
    const char *flow = jstr(w, "flex_flow");
    if (!flow) return;
    lv_flex_flow_t f = LV_FLEX_FLOW_ROW;
    if (!strcmp(flow, "column")) f = LV_FLEX_FLOW_COLUMN;
    else if (!strcmp(flow, "row_wrap")) f = LV_FLEX_FLOW_ROW_WRAP;
    else if (!strcmp(flow, "column_wrap")) f = LV_FLEX_FLOW_COLUMN_WRAP;
    lv_obj_set_flex_flow(o, f);
    lv_obj_set_flex_align(o,
                          parse_flex_align(jstr(w, "flex_main")),
                          parse_flex_align(jstr(w, "flex_cross")),
                          parse_flex_align(jstr(w, "flex_track")));
}

/* SquareLine applies the arc colour/opacity/width to the indicator as well */

/* ---------- widget creation ---------- */

static lv_obj_t *create_widget(lv_obj_t *parent, const char *sub, const cJSON *w)
{
    lv_obj_t *o = NULL;

    if (!strcmp(sub, "label")) {
        o = lv_label_create(parent);
        const char *txt = jstr(w, "obj_text");
        lv_label_set_text(o, (txt && txt[0]) ? txt : " ");
        const char *lm = jstr(w, "long_mode");
        if (lm) {
            if (!strcmp(lm, "WRAP")) lv_label_set_long_mode(o, LV_LABEL_LONG_WRAP);
            else if (!strcmp(lm, "DOT")) lv_label_set_long_mode(o, LV_LABEL_LONG_DOT);
            else if (!strcmp(lm, "SCROLL")) lv_label_set_long_mode(o, LV_LABEL_LONG_SCROLL);
            else if (!strcmp(lm, "SCROLL_CIRCULAR")) lv_label_set_long_mode(o, LV_LABEL_LONG_SCROLL_CIRCULAR);
            else if (!strcmp(lm, "CLIP")) lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
        }
    } else if (!strcmp(sub, "image") || !strcmp(sub, "imagebutton")) {
        const char *src = jstr(w, "src");
        const char *rel = jstr(w, "img_released");
        const char *prs = jstr(w, "img_pressed");
        if (!src) src = rel ? rel : prs;
        if (!src) src = prs;

        lv_image_dsc_t *d = get_image(src);
        if (!d && src) {
            ESP_LOGE(TAG, "%s: image \"%s\" could not be loaded", w->string, src);
        }

        if (!strcmp(sub, "imagebutton")) {
            /* SquareLine's imagebutton keeps a per-state source; a plain image
             * would lose the state handling (and draw nothing at all when the
             * source is unavailable). */
            o = lv_imagebutton_create(parent);
            lv_image_dsc_t *dp = get_image(prs ? prs : src);
            lv_imagebutton_set_src(o, LV_IMAGEBUTTON_STATE_RELEASED, NULL, d, NULL);
            lv_imagebutton_set_src(o, LV_IMAGEBUTTON_STATE_PRESSED, NULL, dp ? dp : d, NULL);
            lv_imagebutton_set_src(o, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, d, NULL);
            lv_imagebutton_set_src(o, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, dp ? dp : d, NULL);
        } else {
            o = lv_image_create(parent);
            if (d) lv_image_set_src(o, d);
        }
    } else if (!strcmp(sub, "button")) {
        o = lv_button_create(parent);
        const char *txt = jstr(w, "obj_text");
        if (txt && txt[0]) {
            lv_obj_t *l = lv_label_create(o);
            lv_label_set_text(l, txt);
            lv_obj_center(l);
        }
    } else if (!strcmp(sub, "textarea")) {
        o = lv_textarea_create(parent);
        const char *txt = jstr(w, "obj_text");
        const char *ph = jstr(w, "placeholder");
        if (txt && txt[0]) lv_textarea_set_text(o, txt);
        if (ph && ph[0]) lv_textarea_set_placeholder_text(o, ph);
        if (jbool(w, "one_line")) lv_textarea_set_one_line(o, true);
        lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);

        /* The JSON export omits accepted_chars / max_length, but they matter:
           e.g. the runtime writes " 21.0" and Arial80 has no space glyph, so
           only the design's accepted_chars stops a tofu box being drawn. */
        const ui_json_input_t *in = ui_json_input_lookup(w->string);
        if (in) {
            if (in->accepted && in->accepted[0]) lv_textarea_set_accepted_chars(o, in->accepted);
            if (in->max_length > 0) lv_textarea_set_max_length(o, in->max_length);
        }
    } else if (!strcmp(sub, "switch")) {
        o = lv_switch_create(parent);
    } else if (!strcmp(sub, "keyboard")) {
        o = lv_keyboard_create(parent);
        const char *km = jstr(w, "keyboard_mode");
        if (km) {
            lv_keyboard_mode_t m = LV_KEYBOARD_MODE_TEXT_LOWER;
            if (!strcmp(km, "number")) m = LV_KEYBOARD_MODE_NUMBER;
            else if (!strcmp(km, "special")) m = LV_KEYBOARD_MODE_SPECIAL;
            else if (!strcmp(km, "text_upper")) m = LV_KEYBOARD_MODE_TEXT_UPPER;
            lv_keyboard_set_mode(o, m);
        }
    } else if (!strcmp(sub, "checkbox")) {
        o = lv_checkbox_create(parent);
        const char *txt = jstr(w, "obj_text");
        if (txt) lv_checkbox_set_text(o, txt);
    } else if (!strcmp(sub, "dropdown") || !strcmp(sub, "roller")) {
        const cJSON *opts = cJSON_GetObjectItem(w, "options");
        char joined[512] = {0};
        if (cJSON_IsArray(opts)) {
            const cJSON *it = NULL;
            size_t used = 0;
            cJSON_ArrayForEach(it, opts) {
                if (!cJSON_IsString(it)) continue;
                size_t n = strlen(it->valuestring);
                if (used + n + 2 >= sizeof(joined)) break;
                if (used) joined[used++] = '\n';
                memcpy(joined + used, it->valuestring, n);
                used += n;
                joined[used] = '\0';
            }
        }
        if (!strcmp(sub, "dropdown")) {
            o = lv_dropdown_create(parent);
            if (joined[0]) lv_dropdown_set_options(o, joined);
            const char *dir = jstr(w, "dir");
            if (dir) {
                lv_dir_t d = LV_DIR_BOTTOM;
                if (!strcmp(dir, "left")) d = LV_DIR_LEFT;
                else if (!strcmp(dir, "right")) d = LV_DIR_RIGHT;
                else if (!strcmp(dir, "top")) d = LV_DIR_TOP;
                lv_dropdown_set_dir(o, d);
            }
        } else {
            o = lv_roller_create(parent);
            if (joined[0]) lv_roller_set_options(o, joined, LV_ROLLER_MODE_NORMAL);
            if (jint(w, "selected", -1) >= 0) {
                lv_roller_set_selected(o, (uint32_t)jint(w, "selected", 0), LV_ANIM_OFF);
            }
        }
    } else if (!strcmp(sub, "arc")) {
        o = lv_arc_create(parent);
        lv_arc_set_range(o, jint(w, "min", 0), jint(w, "max", 100));
        lv_arc_set_value(o, jint(w, "value", 0));
        const cJSON *bg_start = cJSON_GetObjectItem(w, "bg_start_angle");
        const cJSON *bg_end = cJSON_GetObjectItem(w, "bg_end_angle");
        if (cJSON_IsNumber(bg_start) && cJSON_IsNumber(bg_end)) {
            lv_arc_set_bg_angles(o, (uint16_t)bg_start->valueint, (uint16_t)bg_end->valueint);
        }
        const cJSON *rot = cJSON_GetObjectItem(w, "rotation");
        if (cJSON_IsNumber(rot)) lv_arc_set_rotation(o, (uint16_t)rot->valueint);
    } else if (!strcmp(sub, "keyboard")) {
        o = lv_keyboard_create(parent);
        register_keyboard(o);
    } else if (!strcmp(sub, "calendar")) {
        o = lv_calendar_create(parent);
        int ty = jint(w, "today_year", 0);
        int tm = jint(w, "today_month", 0);
        int td = jint(w, "today_day", 0);
        if (ty > 0 && tm > 0) {
            lv_calendar_set_today_date(o, ty, tm, td);
            lv_calendar_set_showed_date(o, ty, tm);
        }
        const char *hdr = jstr(w, "header");
        if (hdr && !strcmp(hdr, "Arrow")) lv_calendar_header_arrow_create(o);
        else if (hdr && !strcmp(hdr, "Dropdown")) lv_calendar_header_dropdown_create(o);
        lv_obj_add_flag(o, LV_OBJ_FLAG_CHECKABLE);
    } else {
        /* panel, screen content and anything not modelled yet */
        o = lv_obj_create(parent);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    }
    return o;
}

/* ---------- events ---------- */

static void register_name(const char *name, lv_obj_t *o)
{
    if (!name || !name[0] || !o) return;
    lv_obj_t *scr = lv_obj_get_screen(o);

    /* Only names an event can address get a slot (see collect_targets) */
    bool wanted = false;
    for (int i = 0; i < s_named_count; i++) {
        if (strcmp(s_named[i].name, name) != 0) continue;
        if (!s_named[i].obj || lv_obj_get_screen(s_named[i].obj) == scr) {
            s_named[i].obj = o;
            return;
        }
        wanted = true; /* exists on another screen: may need a second slot */
    }
    if (!wanted) return;

    if (s_named_count >= MAX_NAMED_OBJS) {
        ESP_LOGW(TAG, "name table full, %s not addressable", name);
        return;
    }
    snprintf(s_named[s_named_count].name, sizeof(s_named[0].name), "%s", name);
    s_named[s_named_count].obj = o;
    s_named_count++;
}

/* make a design name addressable by events and by "align_to" */
static void note_target(const char *target)
{
    if (!target || !target[0] || strlen(target) >= sizeof(s_named[0].name)) return;
    for (int i = 0; i < s_named_count; i++) {
        if (strcmp(s_named[i].name, target) == 0) return;
    }
    if (s_named_count >= MAX_NAMED_OBJS) {
        ESP_LOGW(TAG, "too many event targets, %s ignored", target);
        s_named_overflow = true;
        return;
    }
    snprintf(s_named[s_named_count].name, sizeof(s_named[0].name), "%s", target);
    s_named[s_named_count].obj = NULL;
    s_named_count++;
}

/* collect every widget name an event targets, so only those need a lookup entry */
static void collect_targets(const cJSON *n)
{
    if (!n) return;
    if (cJSON_IsArray(n)) {
        const cJSON *c = NULL;
        cJSON_ArrayForEach(c, n) collect_targets(c);
        return;
    }
    if (!cJSON_IsObject(n)) return;

    const cJSON *events = cJSON_GetObjectItem(n, "events");
    if (cJSON_IsObject(events)) {
        const cJSON *ev = NULL;
        cJSON_ArrayForEach(ev, events) {
            const cJSON *actions = cJSON_GetObjectItem(ev, "actions");
            const cJSON *a = NULL;
            cJSON_ArrayForEach(a, actions) {
                const char *act = jstr(a, "action");
                if (!act || strcmp(act, "flag_modify")) continue;
                note_target(jstr(a, "target"));
            }
        }
    }

    const cJSON *at = cJSON_GetObjectItem(n, "align_to");
    if (cJSON_IsObject(at)) note_target(jstr(at, "target"));

    /* a keyboard names the field it edits; that field must be addressable too */
    note_target(jstr(n, "textarea"));

    const cJSON *c = NULL;
    cJSON_ArrayForEach(c, n) {
        if (cJSON_IsObject(c) || cJSON_IsArray(c)) collect_targets(c);
    }
}
static lv_obj_t *find_named(lv_obj_t *scope, const char *name)
{
    if (!scope || !name || !name[0]) return NULL;
    lv_obj_t *scr = lv_obj_get_screen(scope);
    for (int i = 0; i < s_named_count; i++) {
        if (strcmp(s_named[i].name, name) != 0) continue;
        lv_obj_t *o = s_named[i].obj;
        if (o && lv_obj_get_screen(o) == scr) return o;
    }
    return NULL;
}

static void apply_flag_req(lv_obj_t *scope, const flag_req_t *r)
{
    lv_obj_t *t = find_named(scope, r->target);
    if (!t) {
        ESP_LOGW(TAG, "flag_modify: target %s not found", r->target);
        return;
    }
    if (!strcmp(r->mode, "add")) {
        lv_obj_add_flag(t, LV_OBJ_FLAG_HIDDEN);
    } else if (!strcmp(r->mode, "remove")) {
        lv_obj_remove_flag(t, LV_OBJ_FLAG_HIDDEN);
    } else if (lv_obj_has_flag(t, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_remove_flag(t, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(t, LV_OBJ_FLAG_HIDDEN);
    }
}

static void flag_event_cb(lv_event_t *e)
{
    flag_req_t *r = lv_event_get_user_data(e);
    if (r) apply_flag_req(lv_event_get_current_target(e), r);
}

static void flag_free_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static void flag_timer_cb(lv_timer_t *t)
{
    flag_req_t *r = lv_timer_get_user_data(t);
    if (r) {
        if (r->scope) apply_flag_req(r->scope, r);
        free(r);
    }
}

static flag_req_t *flag_req_new(const cJSON *a)
{
    const char *mode = jstr(a, "mode");
    if (!mode) mode = "toggle";
    if (strcmp(mode, "add") && strcmp(mode, "remove") && strcmp(mode, "toggle")) {
        ESP_LOGW(TAG, "flag_modify: unsupported mode %s", mode);
        return NULL;
    }
    const char *flag = jstr(a, "flag");
    if (flag && strcmp(flag, "hidden") != 0) {
        ESP_LOGW(TAG, "flag_modify: unsupported flag %s", flag);
        return NULL;
    }
    const char *target = jstr(a, "target");
    if (!target || !target[0] || strlen(target) >= sizeof(((flag_req_t *)0)->target)) return NULL;

    flag_req_t *r = calloc(1, sizeof(*r));
    if (!r) return NULL;
    snprintf(r->target, sizeof(r->target), "%s", target);
    snprintf(r->mode, sizeof(r->mode), "%s", mode);
    return r;
}

static void bind_events(lv_obj_t *o, const cJSON *w, bool bridged)
{
    const cJSON *events = cJSON_GetObjectItem(w, "events");
    if (!cJSON_IsObject(events)) return;

    const cJSON *ev = NULL;
    cJSON_ArrayForEach(ev, events) {
        const cJSON *actions = cJSON_GetObjectItem(ev, "actions");
        if (!cJSON_IsArray(actions)) continue;

        const char *when = ev->string ? ev->string : "";
        /* EEZ "ALL" only ever targets clicks in this design */
        lv_event_code_t code = LV_EVENT_CLICKED;
        bool at_build = false;
        if (!strcmp(when, "CLICKED") || !strcmp(when, "ALL")) {
            code = LV_EVENT_CLICKED;
        } else if (!strcmp(when, "VALUE_CHANGED") || !strcmp(when, "CHANGED")) {
            code = LV_EVENT_VALUE_CHANGED;
        } else if (!strcmp(when, "FOCUSED")) {
            code = LV_EVENT_FOCUSED;
        } else if (!strcmp(when, "DEFOCUSED")) {
            code = LV_EVENT_DEFOCUSED;
        } else if (!strcmp(when, "SCREEN_LOADED")) {
            /* a real LVGL event: fires every time the screen is loaded, not only
             * when it is first built (a cached splash screen must still advance) */
            code = LV_EVENT_SCREEN_LOADED;
        } else if (!strcmp(when, "READY")) {
            at_build = true;
        } else {
            ESP_LOGW(TAG, "event %s on %s ignored", when, w->string ? w->string : "?");
            continue;
        }

        const cJSON *a = NULL;
        cJSON_ArrayForEach(a, actions) {
            const char *act = jstr(a, "action");
            if (!act) continue;

            /* images, labels and screens are not clickable by default in LVGL */
            if (!at_build && code != LV_EVENT_SCREEN_LOADED) lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);

            if (!strcmp(act, "screen_change")) {
                const char *target = jstr(a, "screen");
                if (!target || !target[0]) continue;

                nav_req_t *r = malloc(sizeof(*r));
                if (!r) continue;
                snprintf(r->name, sizeof(r->name), "%s", target);
                r->anim = parse_anim(jstr(a, "anim"));
                r->speed = jint(a, "speed", 0);
                int delay = jint(a, "delay", 0);
                r->delay = delay;

                if (!at_build) {
                    lv_obj_add_event_cb(o, code == LV_EVENT_SCREEN_LOADED ? nav_loaded_cb : nav_clicked_cb,
                                        code, r);
                    lv_obj_add_event_cb(o, nav_free_cb, LV_EVENT_DELETE, r);
                } else {
                    /* always go through a timer: the screen is still being built */
                    uint32_t d = delay > 0 ? (uint32_t)delay : 1;
                    lv_timer_t *t = lv_timer_create(nav_timer_cb, d, r);
                    if (t) lv_timer_set_repeat_count(t, 1);
                    else free(r);
                }
            } else if (!strcmp(act, "flag_modify")) {
                /* a compiled handler for this widget already applies the same
                 * flags; running both would toggle twice and cancel out */
                if (bridged) continue;
                flag_req_t *r = flag_req_new(a);
                if (!r) continue;
                if (!at_build) {
                    lv_obj_add_event_cb(o, flag_event_cb, code, r);
                    lv_obj_add_event_cb(o, flag_free_cb, LV_EVENT_DELETE, r);
                } else {
                    /* deferred: the target may not exist yet while building */
                    r->scope = o;
                    lv_timer_t *t = lv_timer_create(flag_timer_cb, 1, r);
                    if (t) lv_timer_set_repeat_count(t, 1);
                    else free(r);
                }
            } else if (!strcmp(act, "call")) {
                /* forward-compatible: {"action":"call","function":"UpdateWifiConfig"} */
                const char *fname = jstr(a, "function");
                ui_json_event_fn fn = ui_json_events_lookup(fname);
                if (!fn) {
                    ESP_LOGW(TAG, "call action: unknown function %s", fname ? fname : "?");
                    continue;
                }
                lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_event_cb(o, bridge_event_cb, at_build ? LV_EVENT_CLICKED : code, (void *)fn);
            } else {
                ESP_LOGW(TAG, "action %s ignored", act);
            }
        }
    }
}

/* ---------- screen building ---------- */

static void build_children(lv_obj_t *parent, const cJSON *children)
{
    const cJSON *w = NULL;
    cJSON_ArrayForEach(w, children) {
        const char *sub = jstr(w, "sub_type");
        if (!sub) continue;

        lv_obj_t *o = create_widget(parent, sub, w);
        if (!o) continue;

        /* the design name is the JSON object key; when the runtime code knows it
         * (ui_<Name>/uic_<Name>) point those globals at this object instead */
        ui_json_bind_apply(w->string, o);
        register_name(w->string, o);

        if (jbool(w, "no_default_style")) lv_obj_remove_style_all(o);
        if (jbool(w, "hidden")) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);

        apply_geometry(o, w, false);
        apply_style(o, cJSON_GetObjectItem(w, "style"));
        inherit_flags(o, w->string);
        apply_flex(o, w);

        const bool one_line_ta = !strcmp(sub, "textarea") && jbool(w, "one_line");

        /* "align_to": SquareLine's lv_obj_align_to() against a sibling; resolved
         * after layout like every other position that depends on a size. */
        const cJSON *at = cJSON_GetObjectItem(w, "align_to");
        if (cJSON_IsObject(at)) {
            queue_align_to(lv_obj_get_screen(o), o, jstr(at, "target"),
                           parse_align_out(jstr(at, "align")),
                           jint(at, "x_pos", 0), jint(at, "y_pos", 0));
        }
        /* one-line textareas are rebuilt as LV_SIZE_CONTENT, so their declared
         * height is not the height they will have: place them after layout */
        else if (one_line_ta || jint(w, "width", 0) <= 0 || jint(w, "height", 0) <= 0) {
            queue_content(lv_obj_get_screen(o), o, parse_align(jstr(w, "align")),
                          jint(w, "x_pos", 0), jint(w, "y_pos", 0));
        } else {
            place_child(o, w);
        }
        if (!strcmp(sub, "textarea") && jbool(w, "one_line")) lv_obj_set_height(o, LV_SIZE_CONTENT);

        const cJSON *sc = cJSON_GetObjectItem(w, "scrollable");
        if (cJSON_IsBool(sc)) {
            if (cJSON_IsTrue(sc)) lv_obj_add_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            else lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
        }
        const char *sd = jstr(w, "scroll_dir");
        if (sd) {
            /* the exported JSON uses upper case (LV_DIR_*), the front end lower case */
            char dir[12];
            size_t di = 0;
            for (; sd[di] && di < sizeof(dir) - 1; di++) {
                char c = sd[di];
                dir[di] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
            }
            dir[di] = '\0';
            lv_dir_t d = LV_DIR_ALL;
            if (!strcmp(dir, "none")) d = LV_DIR_NONE;
            else if (!strcmp(dir, "hor")) d = LV_DIR_HOR;
            else if (!strcmp(dir, "ver")) d = LV_DIR_VER;
            else if (!strcmp(dir, "left")) d = LV_DIR_LEFT;
            else if (!strcmp(dir, "right")) d = LV_DIR_RIGHT;
            else if (!strcmp(dir, "top")) d = LV_DIR_TOP;
            else if (!strcmp(dir, "bottom")) d = LV_DIR_BOTTOM;
            lv_obj_set_scroll_dir(o, d);
        }

        const char *sbm = jstr(w, "scrollbar_mode");
        if (sbm) {
            lv_scrollbar_mode_t m = LV_SCROLLBAR_MODE_AUTO;
            if (!strcmp(sbm, "off")) m = LV_SCROLLBAR_MODE_OFF;
            else if (!strcmp(sbm, "on")) m = LV_SCROLLBAR_MODE_ON;
            else if (!strcmp(sbm, "active")) m = LV_SCROLLBAR_MODE_ACTIVE;
            lv_obj_set_scrollbar_mode(o, m);
        }
        const char *sny = jstr(w, "scroll_snap_y");
        if (sny) {
            lv_scroll_snap_t s = LV_SCROLL_SNAP_NONE;
            if (!strcmp(sny, "start")) s = LV_SCROLL_SNAP_START;
            else if (!strcmp(sny, "end")) s = LV_SCROLL_SNAP_END;
            else if (!strcmp(sny, "center")) s = LV_SCROLL_SNAP_CENTER;
            lv_obj_set_scroll_snap_y(o, s);
        }
        if (!strcmp(sub, "keyboard")) queue_keyboard_link(lv_obj_get_screen(o), o, jstr(w, "textarea"));

        const bool bridged = ui_json_events_lookup(w->string) != NULL;
        bind_events(o, w, bridged);

        const cJSON *kids = cJSON_GetObjectItem(w, "children");
        if (kids) build_children(o, kids);

        if (!strcmp(sub, "textarea")) {
            lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(o, textarea_clicked_cb, LV_EVENT_CLICKED, NULL);

            /* Read-only value fields: the compiled design removes
             * CLICK_FOCUSABLE, but the LVGL theme still paints a text caret
             * (a hollow box the size of one glyph) once the object has focus.
             * Hide the caret for those textareas so they look like displays. */
            lv_obj_t *twin = ui_json_bind_original(w->string);
            if (twin && !lv_obj_has_flag(twin, LV_OBJ_FLAG_CLICK_FOCUSABLE)) {
                const lv_style_selector_t cs = LV_PART_CURSOR | LV_STATE_DEFAULT;
                lv_obj_set_style_opa(o, LV_OPA_TRANSP, cs);
                lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, cs);
                lv_obj_set_style_border_width(o, 0, cs);
                lv_obj_set_style_border_opa(o, LV_OPA_TRANSP, cs);
                lv_obj_set_style_outline_width(o, 0, cs);
                lv_obj_set_style_outline_opa(o, LV_OPA_TRANSP, cs);
                lv_obj_set_style_shadow_width(o, 0, cs);
                lv_obj_clear_state(o, LV_STATE_FOCUSED);
                lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                lv_textarea_set_cursor_click_pos(o, false);
                lv_textarea_set_text_selection(o, false);
                lv_group_t *g = lv_obj_get_group(o);
                if (g) lv_group_remove_obj(o);
                watch_focus(w->string, o);
            }
        } else if (!strcmp(sub, "keyboard")) {
            lv_obj_add_event_cb(o, keyboard_done_cb, LV_EVENT_READY, NULL);
            lv_obj_add_event_cb(o, keyboard_done_cb, LV_EVENT_CANCEL, NULL);
            /* forward-compatible: JSON may name the textarea this keyboard drives */
            const char *kt = jstr(w, "textarea");
            if (!kt) kt = jstr(w, "target");
            if (kt && kt[0] && s_kb_target_count < MAX_KB_TARGETS) {
                kb_target_t *r = &s_kb_targets[s_kb_target_count++];
                r->kb = o;
                r->scope = o;
                snprintf(r->target, sizeof(r->target), "%s", kt);
                lv_timer_t *t = lv_timer_create(kb_target_timer_cb, 1, r);
                if (t) lv_timer_set_repeat_count(t, 1);
            }
        }

        /* compiled behaviour for this design name (save/apply/scan/set mode…) */
        bridge_attach(o, w->string);

#if UI_JSON_DEBUG_LAYOUT
        dbg_add(w->string, o);
#endif
    }
}

static lv_obj_t *build_screen(const char *name)
{
    char *buf = NULL;
    size_t len = 0;
    if (screen_txn_read_active("screens", name, &buf, &len) != ESP_OK || !buf) {
        ESP_LOGW(TAG, "screen %s not found", name);
        return NULL;
    }

    cJSON *root = cJSON_ParseWithLength(buf, len);
    free(buf);
    if (!root) {
        ESP_LOGE(TAG, "screen %s: invalid JSON", name);
        return NULL;
    }

    lv_obj_t *scr = lv_obj_create(NULL);
    if (!scr) {
        cJSON_Delete(root);
        return NULL;
    }
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_display_t *disp = lv_obj_get_display(scr);
    if (disp) {
        lv_obj_set_size(scr, lv_display_get_horizontal_resolution(disp),
                        lv_display_get_vertical_resolution(disp));
    }

    const char *bg = jstr(root, "bg_color");
    if (bg) {
        lv_obj_set_style_bg_color(scr, parse_color(bg), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    /* the compiled screens paint the shared Temco background image */
    lv_obj_set_style_bg_image_src(scr, &ui_img_background, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    const cJSON *widgets = cJSON_GetObjectItem(root, "widgets");
    collect_targets(root);
#if UI_JSON_DEBUG_LAYOUT
    s_dbg_n = 0;
    snprintf(s_dbg_screen, sizeof(s_dbg_screen), "%s", name);
#endif
    const cJSON *top = NULL;
    cJSON_ArrayForEach(top, widgets) {
        const cJSON *kids = cJSON_GetObjectItem(top, "children");
        if (kids) build_children(scr, kids);

        const cJSON *style = cJSON_GetObjectItem(top, "style");
        if (style) apply_style(scr, style);
        register_name(top->string, scr);
        bind_events(scr, top, ui_json_events_lookup(top->string) != NULL);
        bridge_attach(scr, top->string);
    }

    /* alignments are applied once the screen is loaded (see navigate/boot_ui) */
    cJSON_Delete(root);
    ESP_LOGI(TAG, "built screen %s", name);
#if UI_JSON_DEBUG_LAYOUT
    if (s_dbg_on) {
        /* The screen load animation is up to 600 ms and the design's own entry
         * animations start up to 1 s after SCREEN_LOADED and run for 2 s, so the
         * first dump (1200 ms) still catches a running animation while the
         * second one (6000 ms) is the settled geometry the audit compares. */
        lv_timer_t *t1 = lv_timer_create(dbg_dump_cb, 1200, scr);
        if (t1) lv_timer_set_repeat_count(t1, 1);
        lv_timer_t *t2 = lv_timer_create(dbg_dump_cb, 6000, scr);
        if (t2) lv_timer_set_repeat_count(t2, 1);
    }
#endif
    return scr;
}

static lv_obj_t *get_screen(const char *name)
{
    for (int i = 0; i < s_screen_count; i++) {
        if (!strcmp(s_screens[i].name, name)) return s_screens[i].scr;
    }
    lv_obj_t *scr = build_screen(name);
    if (!scr) return NULL;
    if (s_screen_count < MAX_SCREENS) {
        snprintf(s_screens[s_screen_count].name, sizeof(s_screens[s_screen_count].name), "%s", name);
        s_screens[s_screen_count].scr = scr;
        s_screen_count++;
    }
    return scr;
}

/* ---------- cache management ---------- */

static void free_images(void)
{
    for (int i = 0; i < s_image_count; i++) {
        lv_image_cache_drop(&s_images[i].dsc);
        lv_image_header_cache_drop(&s_images[i].dsc);
        if (s_images[i].pixels) free(s_images[i].pixels);
        s_images[i].pixels = NULL;
    }
    s_image_count = 0;
}

static void delete_screens(void)
{
    /* widgets referenced by name from the runtime code go back to the compiled
     * objects so those pointers never dangle */
    ui_json_bind_restore();
    s_named_count = 0;
    s_keyboard_count = 0;
    s_kb_target_count = 0;
    s_content_n = 0;
    s_kbd_n = 0;
#if UI_JSON_DEBUG_LAYOUT
    s_watch_n = 0;
#endif
    for (int i = 0; i < s_screen_count; i++) {
        if (s_screens[i].scr) lv_obj_delete(s_screens[i].scr);
        s_screens[i].scr = NULL;
    }
    s_screen_count = 0;
}

/* ---------- apply ---------- */

static const char *start_screen_name(void)
{
    const char *start = screen_txn_start_screen();
    if (start && start[0]) return start;
    return "start_up_screen";
}

static bool boot_ui(void)
{
    const char *start = start_screen_name();
    lv_obj_t *scr = build_screen(start);
    if (!scr) {
        set_build_ok(false);
        set_error("failed to build start screen");
        set_active(false);
        return false;
    }
    lv_screen_load(scr);
    settle_content(scr);
    snprintf(s_screens[0].name, sizeof(s_screens[0].name), "%s", start);
    s_screens[0].scr = scr;
    s_screen_count = 1;

    set_current(start);
    set_build_ok(true);
    set_error("");
    set_active(true);
    ESP_LOGI(TAG, "JSON UI active: %s", start);
    return true;
}

static void reload_ui(void)
{
    const char *start = start_screen_name();

    /* hold the display while everything is rebuilt */
    lv_obj_t *blank = lv_obj_create(NULL);
    if (blank) lv_screen_load(blank);
    delete_screens();
    free_images();

    lv_obj_t *scr = build_screen(start);
    if (!scr) {
        set_build_ok(false);
        set_error("failed to build start screen");
        set_active(false);
        if (ui_HomeScreen) lv_screen_load(ui_HomeScreen);
        if (blank) lv_obj_delete(blank);
        ESP_LOGE(TAG, "reload failed, showing compiled UI");
        return;
    }

    lv_screen_load(scr);
    settle_content(scr);
    if (blank) lv_obj_delete(blank);

    snprintf(s_screens[0].name, sizeof(s_screens[0].name), "%s", start);
    s_screens[0].scr = scr;
    s_screen_count = 1;

    set_current(start);
    set_build_ok(true);
    set_error("");
    set_active(true);
    ESP_LOGI(TAG, "JSON UI reloaded: %s", start);
}

/* ---------- hooks / public API ---------- */

static void status_cb(screen_ui_status_t *out)
{
    if (!out) return;
    taskENTER_CRITICAL(&s_lock);
    out->ui_json = s_active;
    out->build_ok = s_build_ok;
    out->forced_compiled = s_force_compiled;
    snprintf(out->screen, sizeof(out->screen), "%s", s_current);
    snprintf(out->last_error, sizeof(out->last_error), "%s", s_error);
    taskEXIT_CRITICAL(&s_lock);
}

/* ---------- JSON / compiled switch (persisted in NVS) ---------- */

#define UI_NVS_NS "ui_mode"

static void mode_load(void)
{
    nvs_handle_t h;
    if (nvs_open(UI_NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    uint8_t v = 0;
    if (nvs_get_u8(h, "compiled", &v) == ESP_OK) s_force_compiled = (v != 0);
    nvs_close(h);
}

static void mode_store(bool compiled)
{
    nvs_handle_t h;
    if (nvs_open(UI_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, "compiled", compiled ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
}

/* returns 1 when the compiled UI is in use, 0 when the JSON UI is */
static int mode_set(bool compiled)
{
    ESP_LOGW(TAG, "UI mode -> %s", compiled ? "compiled" : "json");
    mode_store(compiled);

    taskENTER_CRITICAL(&s_lock);
    s_force_compiled = compiled;
    s_reload = true;
    taskEXIT_CRITICAL(&s_lock);
    return compiled ? 1 : 0;
}

/* leave the JSON UI and go back to the compiled start screen */static void show_compiled_ui(void)
{
    delete_screens();
    free_images();
    set_active(false);
    set_current("compiled");
    set_build_ok(true);
    set_error("");
    if (ui_StartUpScreen) lv_screen_load(ui_StartUpScreen);
    else if (ui_HomeScreen) lv_screen_load(ui_HomeScreen);
}

static bool apply_cb(const char *slot, int revision)
{
    (void)slot;
    (void)revision;
    taskENTER_CRITICAL(&s_lock);
    s_reload = true;
    taskEXIT_CRITICAL(&s_lock);
    return true;
}

void ui_json_render_register(void)
{
    mode_load();
    ui_json_bind_snapshot();
    screen_deploy_set_ui_hooks(status_cb, apply_cb);
    screen_deploy_set_ui_mode_hook(mode_set);
    screen_deploy_set_ui_goto_hook(ui_json_render_goto);
#if UI_JSON_DEBUG_LAYOUT
    screen_deploy_set_ui_debug_hook(dbg_set);
#endif
}

bool ui_json_render_active(void)
{
    return s_active;
}

bool ui_json_render_goto(const char *screen_name)
{
    if (!screen_name || !screen_name[0]) return false;
    if (strlen(screen_name) >= sizeof(s_goto)) return false;
    taskENTER_CRITICAL(&s_lock);
    snprintf(s_goto, sizeof(s_goto), "%s", screen_name);
    s_goto_req = true;
    taskEXIT_CRITICAL(&s_lock);
    return true;
}

/* The per-screen data refresh in lv_UserPeram.c dispatches on pointer identity
 * (current_Screen == ui_WifiConfig, …). In JSON mode the active screen is a
 * runtime-built object, so report the compiled screen the JSON one stands in
 * for — that keeps every refresh/scan handler running. */
lv_obj_t *ui_json_render_compiled_screen(lv_obj_t *active)
{
    if (!s_active || !active) return active;

    const char *name = NULL;
    taskENTER_CRITICAL(&s_lock);
    name = s_current;
    taskEXIT_CRITICAL(&s_lock);
    if (!name[0]) return active;

    if (!strcmp(name, "home_screen")) return ui_HomeScreen;
    if (!strcmp(name, "main_menu")) return ui_MainMenu;
    if (!strcmp(name, "wifi_config")) return ui_WifiConfig;
    if (!strcmp(name, "network_config")) return ui_NetworkConfig;
    if (!strcmp(name, "protocols")) return ui_Protocols;
    if (!strcmp(name, "parameters")) return ui_Parameters;
    if (!strcmp(name, "schedule_screen")) return ui_ScheduleScreen;
    if (!strcmp(name, "schedule_edit_screen")) return ui_ScheduleEditScreen;
    if (!strcmp(name, "time")) return ui_Time;
    if (!strcmp(name, "ddns_screen")) return ui_DdnsScreen;
    if (!strcmp(name, "wireguard_screen")) return ui_WireGuardScreen;
    if (!strcmp(name, "holiday_calender_screen")) return ui_HolidayCalenderScreen;
    if (!strcmp(name, "start_up_screen")) return ui_StartUpScreen;
    return active;
}

void ui_json_render_service(void)
{
    watch_poll();
    if (!screen_txn_is_ready()) return;

    static TickType_t next_boot_try;
    static int boot_tries;

    bool reload = false;

    taskENTER_CRITICAL(&s_lock);
    if (s_reload) {
        s_reload = false;
        reload = true;
    }
    taskEXIT_CRITICAL(&s_lock);

    if (reload) {
        s_boot_done = true;
        if (s_force_compiled) {
            show_compiled_ui();
            return;
        }
        reload_ui();
        return;
    }

    if (s_force_compiled) {
        /* switched off at runtime or by a previous session */
        if (!s_boot_done) {
            s_boot_done = true;
            show_compiled_ui();
        }
        return;
    }

    /* deferred navigation requested over REST */
    if (s_goto_req) {
        char target[sizeof(s_goto)];
        taskENTER_CRITICAL(&s_lock);
        snprintf(target, sizeof(target), "%s", s_goto);
        s_goto_req = false;
        taskEXIT_CRITICAL(&s_lock);
        navigate(target, LV_SCREEN_LOAD_ANIM_NONE, 0);
    }

    /* The store becomes "ready" before the default screens are seeded, so the
     * first build may run too early; retry for a while before giving up and
     * leaving the compiled UI in place. */
    if (!s_boot_done && boot_tries < 20 &&
        (boot_tries == 0 || xTaskGetTickCount() >= next_boot_try)) {
        boot_tries++;
        if (boot_ui()) {
            s_boot_done = true;
        } else {
            next_boot_try = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
        }
    }
}
