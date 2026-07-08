#include "lvgl.h"
#include <stdlib.h>

static lv_obj_t dummy_obj = {0};
static lv_display_t* dummy_disp = (lv_display_t*)1;
static lv_indev_t* dummy_indev = (lv_indev_t*)1;
static lv_theme_t* dummy_theme = (lv_theme_t*)1;
static lv_calendar_date_t dummy_date = {0};

const lv_font_t lv_font_montserrat_10 = {0};
const lv_font_t lv_font_montserrat_12 = {0};
const lv_font_t lv_font_montserrat_16 = {0};
const lv_font_t lv_font_montserrat_18 = {0};
const lv_font_t lv_font_montserrat_20 = {0};
const lv_font_t lv_font_montserrat_30 = {0};
const lv_font_t lv_font_montserrat_36 = {0};
const lv_font_t lv_font_montserrat_40 = {0};

void lv_init(void) {}
void lv_tick_set_cb(uint32_t (*cb)(void)) { (void)cb; }
void lv_timer_handler(void) {}
void *lv_malloc(size_t size) { return malloc(size); }
void lv_free(void *ptr) { free(ptr); }

lv_display_t *lv_display_create(int32_t hor_res, int32_t ver_res) { (void)hor_res; (void)ver_res; return dummy_disp; }
void lv_display_set_buffers(lv_display_t *disp, void *buf1, void *buf2, uint32_t buf_size, int mode) { (void)disp; (void)buf1; (void)buf2; (void)buf_size; (void)mode; }
void lv_display_set_color_format(lv_display_t *disp, int fmt) { (void)disp; (void)fmt; }
void lv_display_set_flush_cb(lv_display_t *disp, void (*flush_cb)(lv_display_t *, const lv_area_t *, uint8_t *)) { (void)disp; (void)flush_cb; }
void lv_display_set_default(lv_display_t *disp) { (void)disp; }
lv_display_t *lv_display_get_default(void) { return dummy_disp; }

lv_indev_t *lv_indev_create(void) { return dummy_indev; }
void lv_indev_set_type(lv_indev_t *indev, int type) { (void)indev; (void)type; }
void lv_indev_set_read_cb(lv_indev_t *indev, void (*read_cb)(lv_indev_t *, lv_indev_data_t *)) { (void)indev; (void)read_cb; }
lv_indev_t *lv_indev_active(void) { return dummy_indev; }
void lv_indev_wait_release(lv_indev_t *indev) { (void)indev; }
int lv_indev_get_gesture_dir(lv_indev_t *indev) { (void)indev; return 0; }

lv_obj_t *lv_obj_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_obj_del(lv_obj_t *obj) { (void)obj; }
bool lv_obj_is_valid(const lv_obj_t *obj) { return obj != NULL; }
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) { (void)obj; (void)align; (void)x_ofs; (void)y_ofs; }
void lv_obj_center(lv_obj_t *obj) { (void)obj; }
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h) { (void)obj; (void)w; (void)h; }
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w) { (void)obj; (void)w; }
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h) { (void)obj; (void)h; }
lv_coord_t lv_obj_get_width(lv_obj_t *obj) { (void)obj; return 100; }
lv_coord_t lv_obj_get_height(lv_obj_t *obj) { (void)obj; return 100; }
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x) { (void)obj; (void)x; }
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y) { (void)obj; (void)y; }
lv_coord_t lv_obj_get_x_aligned(lv_obj_t *obj) { (void)obj; return 0; }
lv_coord_t lv_obj_get_y_aligned(lv_obj_t *obj) { (void)obj; return 0; }
void lv_obj_add_flag(lv_obj_t *obj, uint32_t flag) { (void)obj; (void)flag; }
void lv_obj_clear_flag(lv_obj_t *obj, uint32_t flag) { (void)obj; (void)flag; }
bool lv_obj_has_flag(const lv_obj_t *obj, uint32_t flag) { (void)obj; (void)flag; return false; }
void lv_obj_remove_flag(lv_obj_t *obj, uint32_t flag) { (void)obj; (void)flag; }
void lv_obj_add_state(lv_obj_t *obj, uint32_t state) { (void)obj; (void)state; }
void lv_obj_clear_state(lv_obj_t *obj, uint32_t state) { (void)obj; (void)state; }
bool lv_obj_has_state(const lv_obj_t *obj, uint32_t state) { (void)obj; (void)state; return false; }
void lv_obj_remove_state(lv_obj_t *obj, uint32_t state) { (void)obj; (void)state; }
void lv_obj_remove_style_all(lv_obj_t *obj) { (void)obj; }

void lv_obj_set_style_pad_all(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_pad_top(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_pad_bottom(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_pad_left(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_pad_right(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_pad_row(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_radius(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_bg_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_border_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_border_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_border_side(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_align(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_letter_space(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_text_line_space(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_arc_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_arc_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_arc_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }

void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_outline_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_outline_pad(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_outline_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }

void lv_obj_set_style_shadow_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_shadow_width(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_shadow_offset_x(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_shadow_offset_y(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_shadow_spread(lv_obj_t *obj, lv_coord_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_shadow_opa(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }

void lv_obj_set_style_bg_grad_color(lv_obj_t *obj, lv_color_t value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_bg_grad_dir(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_bg_main_stop(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }
void lv_obj_set_style_bg_grad_stop(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }

int lv_obj_get_style_opa(const lv_obj_t *obj, uint32_t selector) { (void)obj; (void)selector; return 255; }

lv_obj_t *lv_label_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_label_set_text(lv_obj_t *obj, const char *text) { (void)obj; (void)text; }
void lv_label_set_long_mode(lv_obj_t *obj, int mode) { (void)obj; (void)mode; }

lv_obj_t *lv_button_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
lv_obj_t *lv_checkbox_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_checkbox_set_text(lv_obj_t *obj, const char *text) { (void)obj; (void)text; }

lv_obj_t *lv_switch_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }

lv_obj_t *lv_slider_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_slider_set_value(lv_obj_t *obj, int32_t value, int anim) { (void)obj; (void)value; (void)anim; }
int32_t lv_slider_get_value(const lv_obj_t *obj) { (void)obj; return 0; }

lv_obj_t *lv_arc_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_arc_set_value(lv_obj_t *obj, int16_t value) { (void)obj; (void)value; }
void lv_arc_set_range(lv_obj_t *obj, int16_t min, int16_t max) { (void)obj; (void)min; (void)max; }
void lv_arc_set_rotation(lv_obj_t *obj, uint16_t rotation) { (void)obj; (void)rotation; }
void lv_arc_set_bg_angles(lv_obj_t *obj, uint16_t start, uint16_t end) { (void)obj; (void)start; (void)end; }
int16_t lv_arc_get_value(const lv_obj_t *obj) { (void)obj; return 0; }

lv_obj_t *lv_bar_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_bar_set_value(lv_obj_t *obj, int32_t value, int anim) { (void)obj; (void)value; (void)anim; }
int32_t lv_bar_get_value(const lv_obj_t *obj) { (void)obj; return 0; }

lv_obj_t *lv_dropdown_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_dropdown_set_options(lv_obj_t *obj, const char *options) { (void)obj; (void)options; }
void lv_dropdown_set_selected(lv_obj_t *obj, uint16_t sel_opt) { (void)obj; (void)sel_opt; }
uint16_t lv_dropdown_get_selected(const lv_obj_t *obj) { (void)obj; return 0; }
void lv_dropdown_get_selected_str(const lv_obj_t *obj, char *buf, uint32_t buf_size) { (void)obj; if(buf && buf_size > 0) buf[0] = '\0'; }
void lv_dropdown_set_dir(lv_obj_t *obj, lv_dir_t dir) { (void)obj; (void)dir; }

lv_obj_t *lv_roller_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_roller_set_options(lv_obj_t *obj, const char *options, int mode) { (void)obj; (void)options; (void)mode; }
void lv_roller_set_selected(lv_obj_t *obj, uint16_t sel_opt, int anim) { (void)obj; (void)sel_opt; (void)anim; }
uint16_t lv_roller_get_selected(const lv_obj_t *obj) { (void)obj; return 0; }

lv_obj_t *lv_keyboard_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_keyboard_set_textarea(lv_obj_t *obj, lv_obj_t *ta) { (void)obj; (void)ta; }
void lv_keyboard_set_mode(lv_obj_t *obj, int mode) { (void)obj; (void)mode; }

lv_obj_t *lv_textarea_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_textarea_set_text(lv_obj_t *obj, const char *text) { (void)obj; (void)text; }
const char *lv_textarea_get_text(const lv_obj_t *obj) { (void)obj; return ""; }
void lv_textarea_set_placeholder_text(lv_obj_t *obj, const char *txt) { (void)obj; (void)txt; }
void lv_textarea_set_one_line(lv_obj_t *obj, bool en) { (void)obj; (void)en; }
void lv_textarea_set_max_length(lv_obj_t *obj, uint32_t max) { (void)obj; (void)max; }
void lv_textarea_set_accepted_chars(lv_obj_t *obj, const char *list) { (void)obj; (void)list; }
void lv_textarea_cursor_right(lv_obj_t *obj) { (void)obj; }
void lv_textarea_cursor_left(lv_obj_t *obj) { (void)obj; }
void lv_textarea_cursor_up(lv_obj_t *obj) { (void)obj; }
void lv_textarea_cursor_down(lv_obj_t *obj) { (void)obj; }

lv_obj_t *lv_table_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_table_set_row_count(lv_obj_t *obj, uint16_t row_cnt) { (void)obj; (void)row_cnt; }
void lv_table_set_column_count(lv_obj_t *obj, uint16_t col_cnt) { (void)obj; (void)col_cnt; }
void lv_table_set_column_width(lv_obj_t *obj, uint16_t col, lv_coord_t w) { (void)obj; (void)col; (void)w; }
void lv_table_set_cell_value(lv_obj_t *obj, uint16_t row, uint16_t col, const char *txt) { (void)obj; (void)row; (void)col; (void)txt; }
const char *lv_table_get_cell_value(const lv_obj_t *obj, uint16_t row, uint16_t col) { (void)obj; (void)row; (void)col; return ""; }
void lv_table_get_selected_cell(const lv_obj_t *obj, uint32_t *row, uint32_t *col) { (void)obj; if(row) *row = 0; if(col) *col = 0; }

lv_obj_t *lv_calendar_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_calendar_set_today_date(lv_obj_t *obj, uint16_t year, uint8_t month, uint8_t day) { (void)obj; (void)year; (void)month; (void)day; }
void lv_calendar_set_showed_date(lv_obj_t *obj, uint16_t year, uint8_t month) { (void)obj; (void)year; (void)month; }
lv_result_t lv_calendar_get_pressed_date(const lv_obj_t *obj, lv_calendar_date_t *date) { (void)obj; if(date) *date = dummy_date; return LV_RES_OK; }
lv_obj_t *lv_calendar_header_arrow_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }

lv_obj_t *lv_image_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_image_set_src(lv_obj_t *obj, const void *src) { (void)obj; (void)src; }
void lv_image_set_rotation(lv_obj_t *obj, int32_t angle) { (void)obj; (void)angle; }
int32_t lv_image_get_rotation(const lv_obj_t *obj) { (void)obj; return 0; }
void lv_image_set_scale(lv_obj_t *obj, uint16_t zoom) { (void)obj; (void)zoom; }
uint16_t lv_image_get_scale(const lv_obj_t *obj) { (void)obj; return 0; }

lv_obj_t *lv_imagebutton_create(lv_obj_t *parent) { dummy_obj.parent = parent; return &dummy_obj; }
void lv_imagebutton_set_src(lv_obj_t *obj, int state, const void *src_mid, const void *src_left, const void *src_right) { (void)obj; (void)state; (void)src_mid; (void)src_left; (void)src_right; }

lv_obj_t *lv_screen_active(void) { return &dummy_obj; }
lv_obj_t *lv_layer_top(void) { return &dummy_obj; }
void lv_disp_load_scr(lv_obj_t *scr) { (void)scr; }
void lv_screen_load_anim(lv_obj_t *scr, lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool auto_del) { (void)scr; (void)anim_type; (void)time; (void)delay; (void)auto_del; }

void lv_obj_add_event_cb(lv_obj_t *obj, void (*event_cb)(lv_event_t *), lv_event_code_t filter, void *user_data) { (void)obj; (void)event_cb; (void)filter; (void)user_data; }
void lv_obj_send_event(lv_obj_t *obj, lv_event_code_t code, void *param) { (void)obj; (void)code; (void)param; }
lv_event_code_t lv_event_get_code(lv_event_t *e) { (void)e; return 0; }
lv_obj_t *lv_event_get_target(lv_event_t *e) { (void)e; return &dummy_obj; }
lv_obj_t *lv_event_get_current_target(lv_event_t *e) { (void)e; return &dummy_obj; }
void *lv_event_get_user_data(lv_event_t *e) { (void)e; return NULL; }

lv_theme_t *lv_theme_default_init(lv_display_t *disp, lv_color_t color_primary, lv_color_t color_secondary, bool dark, const lv_font_t *font) { (void)disp; (void)color_primary; (void)color_secondary; (void)dark; (void)font; return dummy_theme; }
void lv_disp_set_theme(lv_display_t *disp, lv_theme_t *theme) { (void)disp; (void)theme; }

void lv_anim_init(lv_anim_t *a) { if(a) { a->var = NULL; a->start_value = 0; a->end_value = 0; a->time = 0; a->user_data = NULL; } }
void lv_anim_set_var(lv_anim_t *a, void *var) { if(a) a->var = var; }
void lv_anim_set_values(lv_anim_t *a, int32_t start, int32_t end) { if(a) { a->start_value = start; a->end_value = end; } }
void lv_anim_set_time(lv_anim_t *a, uint32_t duration) { if(a) a->time = duration; }
void lv_anim_set_delay(lv_anim_t *a, uint32_t delay) { (void)a; (void)delay; }
void lv_anim_set_playback_time(lv_anim_t *a, uint32_t duration) { (void)a; (void)duration; }
void lv_anim_set_playback_delay(lv_anim_t *a, uint32_t delay) { (void)a; (void)delay; }
void lv_anim_set_repeat_count(lv_anim_t *a, uint16_t count) { (void)a; (void)count; }
void lv_anim_set_repeat_delay(lv_anim_t *a, uint32_t delay) { (void)a; (void)delay; }
void lv_anim_set_early_apply(lv_anim_t *a, bool en) { (void)a; (void)en; }
void lv_anim_set_user_data(lv_anim_t *a, void *user_data) { if(a) a->user_data = user_data; }
void lv_anim_set_custom_exec_cb(lv_anim_t *a, void (*exec_cb)(lv_anim_t *, int32_t)) { (void)a; (void)exec_cb; }
void lv_anim_set_path_cb(lv_anim_t *a, void *path_cb) { (void)a; (void)path_cb; }
void lv_anim_set_deleted_cb(lv_anim_t *a, void (*deleted_cb)(lv_anim_t *)) { (void)a; (void)deleted_cb; }
void lv_anim_set_get_value_cb(lv_anim_t *a, int32_t (*get_value_cb)(lv_anim_t *)) { (void)a; (void)get_value_cb; }
lv_anim_t * lv_anim_start(lv_anim_t *a) { (void)a; return a; }
int32_t lv_anim_path_linear(const void *a) { (void)a; return 0; }

void lv_obj_set_flex_flow(lv_obj_t *obj, int flow) { (void)obj; (void)flow; }
void lv_obj_set_flex_align(lv_obj_t *obj, int main_place, int cross_place, int track_place) { (void)obj; (void)main_place; (void)cross_place; (void)track_place; }
void lv_obj_set_scroll_dir(lv_obj_t *obj, lv_dir_t dir) { (void)obj; (void)dir; }
void lv_obj_set_scroll_snap_y(lv_obj_t *obj, int snap) { (void)obj; (void)snap; }
void lv_obj_set_scrollbar_mode(lv_obj_t *obj, int mode) { (void)obj; (void)mode; }
void lv_obj_set_align(lv_obj_t *obj, lv_align_t align) { (void)obj; (void)align; }

const uint8_t *lv_font_get_bitmap_fmt_txt(const lv_font_t *font, uint32_t unicode_letter) { (void)font; (void)unicode_letter; return NULL; }
bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t *font, void *dsc_out, uint32_t unicode_letter, uint32_t next_letter) { (void)font; (void)dsc_out; (void)unicode_letter; (void)next_letter; return false; }

void lv_obj_set_style_text_decor(lv_obj_t *obj, int value, uint32_t selector) { (void)obj; (void)value; (void)selector; }

void lv_spinbox_increment(lv_obj_t *obj) { (void)obj; }
void lv_spinbox_decrement(lv_obj_t *obj) { (void)obj; }

void lv_disp_flush_ready(lv_display_t *disp) { (void)disp; }
