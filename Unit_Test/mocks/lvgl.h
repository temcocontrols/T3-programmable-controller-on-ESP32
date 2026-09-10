#ifndef LVGL_H
#define LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int16_t lv_coord_t;

#define LV_COLOR_DEPTH 16

typedef struct lv_display lv_disp_t;

typedef enum {
    LV_PALETTE_RED,
    LV_PALETTE_BLUE,
} lv_palette_t;

typedef struct {
    lv_coord_t x1;
    lv_coord_t y1;
    lv_coord_t x2;
    lv_coord_t y2;
} lv_area_t;

typedef enum {
    LV_INDEV_STATE_RELEASED = 0,
    LV_INDEV_STATE_PR = 1,
    LV_INDEV_STATE_REL = 0,
} lv_indev_state_t;

#define LV_COLOR_FORMAT_RGB565_SWAPPED 0
#define LV_INDEV_TYPE_POINTER 0

#define LV_DISPLAY_RENDER_MODE_PARTIAL 0
#define LV_DISPLAY_RENDER_MODE_DIRECT 1
#define LV_DISPLAY_RENDER_MODE_FULL 2

#define LV_BORDER_SIDE_NONE 0
#define LV_BORDER_SIDE_BOTTOM 1
#define LV_BORDER_SIDE_TOP 2
#define LV_BORDER_SIDE_LEFT 4
#define LV_BORDER_SIDE_RIGHT 8
#define LV_BORDER_SIDE_INTERNAL 16
#define LV_BORDER_SIDE_FULL 31

#define LV_GRAD_DIR_NONE 0
#define LV_GRAD_DIR_VER 1
#define LV_GRAD_DIR_HOR 2

#define LV_IMAGEBUTTON_STATE_RELEASED 0
#define LV_IMAGEBUTTON_STATE_PRESSED 1
#define LV_IMAGEBUTTON_STATE_DISABLED 2
#define LV_IMAGEBUTTON_STATE_CHECKED_RELEASED 3
#define LV_IMAGEBUTTON_STATE_CHECKED_PRESSED 4
#define LV_IMAGEBUTTON_STATE_CHECKED_DISABLED 5

typedef enum {
    LV_RES_INV = 0,
    LV_RES_OK = 1,
} lv_result_t;

enum {
    LV_PART_MAIN      = 0x000000,
    LV_PART_SCROLLBAR = 0x010000,
    LV_PART_INDICATOR = 0x020000,
    LV_PART_KNOB      = 0x030000,
    LV_PART_SELECTED  = 0x040000,
    LV_PART_ITEMS     = 0x050000,
    LV_PART_CURSOR    = 0x060000,
    LV_PART_CUSTOM_FIRST = 0x080000,
    LV_PART_ANY       = 0x0F0000,
};


typedef struct {
    uint16_t blue  : 5;
    uint16_t green : 6;
    uint16_t red   : 5;
} lv_color_t;

static inline lv_color_t lv_color_hex(uint32_t hex) {
    lv_color_t c;
    c.red = (hex >> 16) & 0x1F;
    c.green = (hex >> 8) & 0x3F;
    c.blue = hex & 0x1F;
    return c;
}

static inline lv_color_t lv_palette_main(lv_palette_t p) { (void)p; lv_color_t c = {0}; return c; }

typedef enum {
    LV_ALIGN_DEFAULT = 0,
    LV_ALIGN_TOP_LEFT,
    LV_ALIGN_TOP_MID,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_BOTTOM_LEFT,
    LV_ALIGN_BOTTOM_MID,
    LV_ALIGN_BOTTOM_RIGHT,
    LV_ALIGN_LEFT_MID,
    LV_ALIGN_RIGHT_MID,
    LV_ALIGN_CENTER,
    LV_ALIGN_OUT_TOP_LEFT,
    LV_ALIGN_OUT_TOP_MID,
    LV_ALIGN_OUT_TOP_RIGHT,
    LV_ALIGN_OUT_BOTTOM_LEFT,
    LV_ALIGN_OUT_BOTTOM_MID,
    LV_ALIGN_OUT_BOTTOM_RIGHT,
    LV_ALIGN_OUT_LEFT_TOP,
    LV_ALIGN_OUT_LEFT_MID,
    LV_ALIGN_OUT_LEFT_BOTTOM,
    LV_ALIGN_OUT_RIGHT_TOP,
    LV_ALIGN_OUT_RIGHT_MID,
    LV_ALIGN_OUT_RIGHT_BOTTOM,
} lv_align_t;

typedef enum {
    LV_DIR_NONE     = 0x00,
    LV_DIR_LEFT     = 0x01,
    LV_DIR_RIGHT    = 0x02,
    LV_DIR_TOP      = 0x04,
    LV_DIR_BOTTOM   = 0x08,
    LV_DIR_HOR      = LV_DIR_LEFT | LV_DIR_RIGHT,
    LV_DIR_VER      = LV_DIR_TOP | LV_DIR_BOTTOM,
    LV_DIR_ALL      = LV_DIR_HOR | LV_DIR_VER,
} lv_dir_t;

typedef struct lv_obj {
    struct lv_obj *parent;
    void *user_data;
} lv_obj_t;

typedef struct lv_display lv_display_t;
typedef struct lv_indev lv_indev_t;
typedef struct lv_theme lv_theme_t;

typedef struct lv_font_t {
    bool (*get_glyph_dsc)(const struct lv_font_t *, void *, uint32_t, uint32_t);
    const uint8_t *(*get_glyph_bitmap)(const struct lv_font_t *, uint32_t);
    lv_coord_t line_height;
    lv_coord_t base_line;
    uint8_t subpx;
    void *dsc;
    int8_t underline_position;
    int8_t underline_thickness;
    const struct lv_font_t *fallback;
    void *user_data;
    const uint8_t *box_w;
} lv_font_t;

extern const lv_font_t lv_font_montserrat_16;
#define LV_FONT_DEFAULT (&lv_font_montserrat_16)
#define LV_ATTRIBUTE_LARGE_CONST

typedef struct lv_event lv_event_t;

typedef enum {
    LV_EVENT_ALL = 0,
    LV_EVENT_PRESSED,
    LV_EVENT_PRESSING,
    LV_EVENT_PRESS_LOST,
    LV_EVENT_SHORT_CLICKED,
    LV_EVENT_LONG_PRESSED,
    LV_EVENT_LONG_PRESSED_REPEAT,
    LV_EVENT_CLICKED,
    LV_EVENT_RELEASED,
    LV_EVENT_SCROLL_BEGIN,
    LV_EVENT_SCROLL_END,
    LV_EVENT_SCROLL,
    LV_EVENT_GESTURE,
    LV_EVENT_KEY,
    LV_EVENT_FOCUSED,
    LV_EVENT_DEFOCUSED,
    LV_EVENT_LEAVE,
    LV_EVENT_VALUE_CHANGED,
    LV_EVENT_INSERT,
    LV_EVENT_REFRESH,
    LV_EVENT_READY,
    LV_EVENT_CANCEL,
    LV_EVENT_SCREEN_LOADED,
    LV_EVENT_SCREEN_LOAD_START,
} lv_event_code_t;

typedef struct {
    int16_t year;
    int8_t month;
    int8_t day;
} lv_calendar_date_t;

typedef struct {
    struct {
        lv_coord_t x;
        lv_coord_t y;
    } point;
    lv_indev_state_t state;
} lv_indev_data_t;

typedef struct {
    void *var;
    int32_t start_value;
    int32_t end_value;
    uint32_t time;
    void *user_data;
} lv_anim_t;

#define LV_COLOR_FORMAT_NATIVE_WITH_ALPHA 0
#define LV_IMAGE_HEADER_MAGIC 0x12345678

typedef struct {
    uint32_t magic;
    uint32_t cf;
    uint32_t w;
    uint32_t h;
} lv_image_header_t;

typedef struct {
    lv_image_header_t header;
    const void *data;
    uint32_t data_size;
} lv_image_dsc_t;

typedef enum {
    LV_SCR_LOAD_ANIM_NONE,
    LV_SCR_LOAD_ANIM_OVER_LEFT,
    LV_SCR_LOAD_ANIM_OVER_RIGHT,
    LV_SCR_LOAD_ANIM_OVER_TOP,
    LV_SCR_LOAD_ANIM_OVER_BOTTOM,
    LV_SCR_LOAD_ANIM_MOVE_LEFT,
    LV_SCR_LOAD_ANIM_MOVE_RIGHT,
    LV_SCR_LOAD_ANIM_MOVE_TOP,
    LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
    LV_SCR_LOAD_ANIM_FADE_ON,
} lv_screen_load_anim_t;

enum {
    LV_STATE_DEFAULT  =  0x0000,
    LV_STATE_CHECKED  =  0x0001,
    LV_STATE_FOCUSED  =  0x0002,
    LV_STATE_FOCUS_KEY = 0x0004,
    LV_STATE_EDITED   =  0x0008,
    LV_STATE_HOVERED  =  0x0010,
    LV_STATE_PRESSED  =  0x0020,
    LV_STATE_DISABLED =  0x0040,
    LV_STATE_USER_1   =  0x1000,
    LV_STATE_USER_2   =  0x2000,
    LV_STATE_USER_3   =  0x4000,
    LV_STATE_USER_4   =  0x8000,
    LV_STATE_ANY      =  0xFFFF,
};

enum {
    LV_OBJ_FLAG_HIDDEN          = (1L << 0),
    LV_OBJ_FLAG_CLICKABLE       = (1L << 1),
    LV_OBJ_FLAG_CLICK_FOCUSABLE = (1L << 2),
    LV_OBJ_FLAG_SCROLLABLE      = (1L << 3),
    LV_OBJ_FLAG_SCROLL_ELASTIC  = (1L << 4),
    LV_OBJ_FLAG_SCROLL_MOMENTUM = (1L << 5),
    LV_OBJ_FLAG_SCROLL_ONE      = (1L << 6),
    LV_OBJ_FLAG_SCROLL_CHAIN_HOR = (1L << 7),
    LV_OBJ_FLAG_SCROLL_CHAIN_VER = (1L << 8),
    LV_OBJ_FLAG_SCROLL_CHAIN    = (LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_SCROLL_CHAIN_VER),
    LV_OBJ_FLAG_GESTURE_BUBBLE  = (1L << 9),
    LV_OBJ_FLAG_EVENT_BUBBLE    = (1L << 10),
    LV_OBJ_FLAG_RECLAIM_KEYPAD  = (1L << 11),
    LV_OBJ_FLAG_OVERFLOW_VISIBLE = (1L << 12),
    LV_OBJ_FLAG_CHECKABLE       = (1L << 13),
    LV_OBJ_FLAG_SCROLL_ON_FOCUS = (1L << 14),
    LV_OBJ_FLAG_SCROLL_WITH_ARROW = (1L << 15),
};

#define LV_SIZE_CONTENT 20001

typedef enum {
    LV_ANIM_OFF = 0,
    LV_ANIM_ON = 1,
} lv_anim_enable_t;

enum {
    LV_KEYBOARD_MODE_TEXT_LOWER = 0,
    LV_KEYBOARD_MODE_TEXT_UPPER,
    LV_KEYBOARD_MODE_SPECIAL,
    LV_KEYBOARD_MODE_NUMBER,
    LV_KEYBOARD_MODE_USER_1,
    LV_KEYBOARD_MODE_USER_2,
    LV_KEYBOARD_MODE_USER_3,
    LV_KEYBOARD_MODE_USER_4,
};

extern const lv_font_t lv_font_montserrat_10;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_16;
extern const lv_font_t lv_font_montserrat_18;
extern const lv_font_t lv_font_montserrat_20;
extern const lv_font_t lv_font_montserrat_30;
extern const lv_font_t lv_font_montserrat_36;
extern const lv_font_t lv_font_montserrat_40;

#define LV_IMG_DECLARE(name) extern const lv_image_dsc_t name
#define LV_FONT_DECLARE(name) extern const lv_font_t name

void lv_init(void);
void lv_tick_set_cb(uint32_t (*cb)(void));
void lv_timer_handler(void);
void *lv_malloc(size_t size);
void lv_free(void *ptr);

lv_display_t *lv_display_create(int32_t hor_res, int32_t ver_res);
void lv_display_set_buffers(lv_display_t *disp, void *buf1, void *buf2, uint32_t buf_size, int mode);
void lv_display_set_color_format(lv_display_t *disp, int fmt);
void lv_display_set_flush_cb(lv_display_t *disp, void (*flush_cb)(lv_display_t *, const lv_area_t *, uint8_t *));
void lv_display_set_default(lv_display_t *disp);
lv_display_t *lv_display_get_default(void);
void lv_disp_flush_ready(lv_display_t *disp);


lv_indev_t *lv_indev_create(void);
void lv_indev_set_type(lv_indev_t *indev, int type);
void lv_indev_set_read_cb(lv_indev_t *indev, void (*read_cb)(lv_indev_t *, lv_indev_data_t *));
lv_indev_t *lv_indev_active(void);
void lv_indev_wait_release(lv_indev_t *indev);
int lv_indev_get_gesture_dir(lv_indev_t *indev);

lv_obj_t *lv_obj_create(lv_obj_t *parent);
void lv_obj_del(lv_obj_t *obj);
bool lv_obj_is_valid(const lv_obj_t *obj);
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_center(lv_obj_t *obj);
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
lv_coord_t lv_obj_get_width(lv_obj_t *obj);
lv_coord_t lv_obj_get_height(lv_obj_t *obj);
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y);
lv_coord_t lv_obj_get_x_aligned(lv_obj_t *obj);
lv_coord_t lv_obj_get_y_aligned(lv_obj_t *obj);
void lv_obj_add_flag(lv_obj_t *obj, uint32_t flag);
void lv_obj_clear_flag(lv_obj_t *obj, uint32_t flag);
bool lv_obj_has_flag(const lv_obj_t *obj, uint32_t flag);
void lv_obj_remove_flag(lv_obj_t *obj, uint32_t flag);
void lv_obj_add_state(lv_obj_t *obj, uint32_t state);
void lv_obj_clear_state(lv_obj_t *obj, uint32_t state);
bool lv_obj_has_state(const lv_obj_t *obj, uint32_t state);
void lv_obj_remove_state(lv_obj_t *obj, uint32_t state);
void lv_obj_remove_style_all(lv_obj_t *obj);

void lv_obj_set_style_pad_all(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_pad_top(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_pad_bottom(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_pad_left(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_pad_right(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_pad_row(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_radius(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_border_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_border_opa(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_border_side(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *value, uint32_t selector);
void lv_obj_set_style_text_align(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_text_opa(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_text_letter_space(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_text_line_space(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_opa(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_arc_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_arc_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_arc_opa(lv_obj_t *obj, int value, uint32_t selector);

void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_outline_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_outline_pad(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_outline_opa(lv_obj_t *obj, int value, uint32_t selector);

void lv_obj_set_style_shadow_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_shadow_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_shadow_offset_x(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_shadow_offset_y(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_shadow_spread(lv_obj_t *obj, lv_coord_t value, uint32_t selector);
void lv_obj_set_style_shadow_opa(lv_obj_t *obj, int value, uint32_t selector);

void lv_obj_set_style_bg_grad_color(lv_obj_t *obj, lv_color_t value, uint32_t selector);
void lv_obj_set_style_bg_grad_dir(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_bg_main_stop(lv_obj_t *obj, int value, uint32_t selector);
void lv_obj_set_style_bg_grad_stop(lv_obj_t *obj, int value, uint32_t selector);

int lv_obj_get_style_opa(const lv_obj_t *obj, uint32_t selector);

lv_obj_t *lv_label_create(lv_obj_t *parent);
void lv_label_set_text(lv_obj_t *obj, const char *text);
void lv_label_set_long_mode(lv_obj_t *obj, int mode);

lv_obj_t *lv_button_create(lv_obj_t *parent);
lv_obj_t *lv_checkbox_create(lv_obj_t *parent);
void lv_checkbox_set_text(lv_obj_t *obj, const char *text);

lv_obj_t *lv_switch_create(lv_obj_t *parent);

lv_obj_t *lv_slider_create(lv_obj_t *parent);
void lv_slider_set_value(lv_obj_t *obj, int32_t value, int anim);
int32_t lv_slider_get_value(const lv_obj_t *obj);

lv_obj_t *lv_arc_create(lv_obj_t *parent);
void lv_arc_set_value(lv_obj_t *obj, int16_t value);
void lv_arc_set_range(lv_obj_t *obj, int16_t min, int16_t max);
void lv_arc_set_rotation(lv_obj_t *obj, uint16_t rotation);
void lv_arc_set_bg_angles(lv_obj_t *obj, uint16_t start, uint16_t end);
int16_t lv_arc_get_value(const lv_obj_t *obj);

lv_obj_t *lv_bar_create(lv_obj_t *parent);
void lv_bar_set_value(lv_obj_t *obj, int32_t value, int anim);
int32_t lv_bar_get_value(const lv_obj_t *obj);

lv_obj_t *lv_dropdown_create(lv_obj_t *parent);
void lv_dropdown_set_options(lv_obj_t *obj, const char *options);
void lv_dropdown_set_selected(lv_obj_t *obj, uint16_t sel_opt);
uint16_t lv_dropdown_get_selected(const lv_obj_t *obj);
void lv_dropdown_get_selected_str(const lv_obj_t *obj, char *buf, uint32_t buf_size);
void lv_dropdown_set_dir(lv_obj_t *obj, lv_dir_t dir);

lv_obj_t *lv_roller_create(lv_obj_t *parent);
void lv_roller_set_options(lv_obj_t *obj, const char *options, int mode);
void lv_roller_set_selected(lv_obj_t *obj, uint16_t sel_opt, int anim);
uint16_t lv_roller_get_selected(const lv_obj_t *obj);

lv_obj_t *lv_keyboard_create(lv_obj_t *parent);
void lv_keyboard_set_textarea(lv_obj_t *obj, lv_obj_t *ta);
void lv_keyboard_set_mode(lv_obj_t *obj, int mode);

lv_obj_t *lv_textarea_create(lv_obj_t *parent);
void lv_textarea_set_text(lv_obj_t *obj, const char *text);
const char *lv_textarea_get_text(const lv_obj_t *obj);
void lv_textarea_set_placeholder_text(lv_obj_t *obj, const char *txt);
void lv_textarea_set_one_line(lv_obj_t *obj, bool en);
void lv_textarea_set_max_length(lv_obj_t *obj, uint32_t max);
void lv_textarea_set_accepted_chars(lv_obj_t *obj, const char *list);
void lv_textarea_cursor_right(lv_obj_t *obj);
void lv_textarea_cursor_left(lv_obj_t *obj);
void lv_textarea_cursor_up(lv_obj_t *obj);
void lv_textarea_cursor_down(lv_obj_t *obj);

lv_obj_t *lv_table_create(lv_obj_t *parent);
void lv_table_set_row_count(lv_obj_t *obj, uint16_t row_cnt);
void lv_table_set_column_count(lv_obj_t *obj, uint16_t col_cnt);
void lv_table_set_column_width(lv_obj_t *obj, uint16_t col, lv_coord_t w);
void lv_table_set_cell_value(lv_obj_t *obj, uint16_t row, uint16_t col, const char *txt);
const char *lv_table_get_cell_value(const lv_obj_t *obj, uint16_t row, uint16_t col);
void lv_table_get_selected_cell(const lv_obj_t *obj, uint32_t *row, uint32_t *col);

lv_obj_t *lv_calendar_create(lv_obj_t *parent);
void lv_calendar_set_today_date(lv_obj_t *obj, uint16_t year, uint8_t month, uint8_t day);
void lv_calendar_set_showed_date(lv_obj_t *obj, uint16_t year, uint8_t month);
lv_result_t lv_calendar_get_pressed_date(const lv_obj_t *obj, lv_calendar_date_t *date);
lv_obj_t *lv_calendar_header_arrow_create(lv_obj_t *parent);

lv_obj_t *lv_image_create(lv_obj_t *parent);
void lv_image_set_src(lv_obj_t *obj, const void *src);
void lv_image_set_rotation(lv_obj_t *obj, int32_t angle);
int32_t lv_image_get_rotation(const lv_obj_t *obj);
void lv_image_set_scale(lv_obj_t *obj, uint16_t zoom);
uint16_t lv_image_get_scale(const lv_obj_t *obj);

lv_obj_t *lv_imagebutton_create(lv_obj_t *parent);
void lv_imagebutton_set_src(lv_obj_t *obj, int state, const void *src_mid, const void *src_left, const void *src_right);

lv_obj_t *lv_screen_active(void);
lv_obj_t *lv_layer_top(void);
void lv_disp_load_scr(lv_obj_t *scr);
void lv_screen_load_anim(lv_obj_t *scr, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del);

void lv_obj_add_event_cb(lv_obj_t *obj, void (*event_cb)(lv_event_t *), lv_event_code_t filter, void *user_data);
void lv_obj_send_event(lv_obj_t *obj, lv_event_code_t code, void *param);
lv_event_code_t lv_event_get_code(lv_event_t *e);
lv_obj_t *lv_event_get_target(lv_event_t *e);
lv_obj_t *lv_event_get_current_target(lv_event_t *e);
void *lv_event_get_user_data(lv_event_t *e);

lv_theme_t *lv_theme_default_init(lv_display_t *disp, lv_color_t color_primary, lv_color_t color_secondary, bool dark, const lv_font_t *font);
void lv_disp_set_theme(lv_display_t *disp, lv_theme_t *theme);

void lv_anim_init(lv_anim_t *a);
void lv_anim_set_var(lv_anim_t *a, void *var);
void lv_anim_set_values(lv_anim_t *a, int32_t start, int32_t end);
void lv_anim_set_time(lv_anim_t *a, uint32_t duration);
void lv_anim_set_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_playback_time(lv_anim_t *a, uint32_t duration);
void lv_anim_set_playback_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_repeat_count(lv_anim_t *a, uint16_t count);
void lv_anim_set_repeat_delay(lv_anim_t *a, uint32_t delay);
void lv_anim_set_early_apply(lv_anim_t *a, bool en);
void lv_anim_set_user_data(lv_anim_t *a, void *user_data);
void lv_anim_set_custom_exec_cb(lv_anim_t *a, void (*exec_cb)(lv_anim_t *, int32_t));
void lv_anim_set_path_cb(lv_anim_t *a, void *path_cb);
void lv_anim_set_deleted_cb(lv_anim_t *a, void (*deleted_cb)(lv_anim_t *));
void lv_anim_set_get_value_cb(lv_anim_t *a, int32_t (*get_value_cb)(lv_anim_t *));
lv_anim_t * lv_anim_start(lv_anim_t *a);
int32_t lv_anim_path_linear(const void *a);

void lv_obj_set_flex_flow(lv_obj_t *obj, int flow);
void lv_obj_set_flex_align(lv_obj_t *obj, int main_place, int cross_place, int track_place);
void lv_obj_set_scroll_dir(lv_obj_t *obj, lv_dir_t dir);
void lv_obj_set_scroll_snap_y(lv_obj_t *obj, int snap);
void lv_obj_set_scrollbar_mode(lv_obj_t *obj, int mode);
void lv_obj_set_align(lv_obj_t *obj, lv_align_t align);
void lv_spinbox_increment(lv_obj_t *obj);
void lv_spinbox_decrement(lv_obj_t *obj);

#define LV_SCROLLBAR_MODE_OFF 0
#define LV_SCROLLBAR_MODE_AUTO 1
#define LV_SCROLL_SNAP_NONE 0
#define LV_ROLLER_MODE_INFINITE 0
#define LV_FLEX_FLOW_ROW 0
#define LV_FLEX_FLOW_COLUMN 1
#define LV_FLEX_ALIGN_START 0
#define LV_FLEX_ALIGN_SPACE_BETWEEN 1
#define LV_LABEL_LONG_SCROLL 0
#define LV_TEXT_DECOR_NONE 0
#define LV_OPA_COVER 255
#define LV_OPA_40 102
#define LV_TEXT_ALIGN_CENTER 1

void lv_obj_set_style_text_decor(lv_obj_t *obj, int value, uint32_t selector);

#define LV_PCT(x) (x)
#define lv_snprintf snprintf

#define LV_FONT_SUBPX_NONE 0
#define LV_VERSION_CHECK(x,y,z) (((x) << 16) | ((y) << 8) | (z))
#define LVGL_VERSION_MAJOR 8

#define LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL 0

typedef struct {
    uint16_t range_start;
    uint16_t range_length;
    uint16_t glyph_id_start;
    const uint16_t *unicode_list;
    const void *glyph_id_ofs_list;
    uint16_t list_length;
    uint8_t type;
} lv_font_fmt_txt_cmap_t;

typedef struct {
    uint32_t bitmap_index;
    uint32_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
} lv_font_fmt_txt_glyph_dsc_t;

typedef struct {
    int dummy;
} lv_font_fmt_txt_glyph_cache_t;

typedef struct {
    const uint8_t *glyph_bitmap;
    const lv_font_fmt_txt_glyph_dsc_t *glyph_dsc;
    const lv_font_fmt_txt_cmap_t *cmaps;
    const void *kern_dsc;
    uint8_t kern_scale;
    uint8_t cmap_num;
    uint8_t bpp;
    uint8_t kern_classes;
    uint8_t bitmap_format;
    lv_font_fmt_txt_glyph_cache_t *cache;
} lv_font_fmt_txt_dsc_t;

typedef struct {
    int dummy;
} lv_font_fmt_txt_glyph_dsc_t_dummy;

const uint8_t *lv_font_get_bitmap_fmt_txt(const lv_font_t *font, uint32_t unicode_letter);
bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t *font, void *dsc_out, uint32_t unicode_letter, uint32_t next_letter);

#ifdef __cplusplus
}
#endif

#endif // LVGL_H
