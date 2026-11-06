#include "ui.h"
#include "ui_IrBlaster.h"
#include "define.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#if CONFIG_IDF_TARGET_ESP32S3
#include "ir_blaster.h"
#endif

#define IR_UI_OBJ_READY(obj) ((obj) != NULL && lv_obj_is_valid(obj))

lv_obj_t *ui_IrBlaster = NULL;
lv_obj_t *ui_IrSlotDetail = NULL;

#if CONFIG_IDF_TARGET_ESP32S3
static lv_obj_t *s_slot_btn[IR_BLASTER_SLOT_COUNT];
static lv_obj_t *s_slot_lbl[IR_BLASTER_SLOT_COUNT];
static lv_obj_t *s_status_lbl;
static lv_obj_t *s_lib_popup;
static lv_obj_t *s_detail_title;
static lv_obj_t *s_detail_status;
static lv_obj_t *s_detail_grid;
static uint8_t s_open_slot;
static uint8_t s_rename_kind;
static uint8_t s_rename_index;
static uint8_t s_suppress_click;
static int s_grid_lib_n = -1;
static int s_grid_custom_n = -1;
static uint8_t s_grid_learn = 0xFF;

bool ui_ir_blaster_visible(void)
{
    return (Modbus.mini_type == MINI_TSTAT11) && ir_blaster_is_supported();
}

static bool ir_ui_ok(void)
{
    return ui_ir_blaster_visible();
}

#define IR_SLOT_BTN_W   344  /* 80% of previous 430 */
#define IR_SLOT_BTN_H   36
#define IR_KEY_W        98   /* 70% of previous 140 */
#define IR_KEY_H        32
#define IR_ACTION_W     130

#define IR_BTN_ACTION  0
#define IR_BTN_LIB     1
#define IR_BTN_CUSTOM  2
#define IR_BTN_POWER   3

#define IR_RENAME_SLOT    0
#define IR_RENAME_CUSTOM  1

static void style_menu_btn(lv_obj_t *btn)
{
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x349EF9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0x81BAEA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void style_power_btn(lv_obj_t *btn)
{
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xB03A2E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0xD46A64), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Key colors ~160% darker than the menu blue; custom is a slight teal shift. */
static void style_key_btn(lv_obj_t *btn, int kind)
{
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (kind == IR_BTN_POWER) {
        style_power_btn(btn);
        return;
    }
    if (kind == IR_BTN_CUSTOM) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x17687A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0x2F8A9A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x0B3E4A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x145A96), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0x2E7AB8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void style_clear_container(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);
}

static void slot_text(uint8_t slot, char *buf, size_t len)
{
    ir_blaster_slot_info_t info;
    if (ir_blaster_get_slot_info(slot, &info) != ESP_OK) {
        snprintf(buf, len, "Empty  (tap to learn)");
        return;
    }
    if (info.kind == IR_SLOT_EMPTY) {
        snprintf(buf, len, "Empty  (tap to learn)");
        return;
    }
    const char *name = info.name[0] ? info.name : info.proto_name;
    if (info.custom_count > 0) {
        snprintf(buf, len, "%s    custom %u", name, info.custom_count);
    } else {
        snprintf(buf, len, "%s", name);
    }
}

static void close_lib_popup(void)
{
    if (s_lib_popup && lv_obj_is_valid(s_lib_popup)) {
        lv_obj_del(s_lib_popup);
    }
    s_lib_popup = NULL;
}

static void lib_popup_answer(uint8_t accept)
{
    close_lib_popup();
    (void)ir_blaster_confirm_library(accept);
    ui_IrBlaster_refresh();
    ui_IrSlotDetail_refresh();
}

static void ui_event_lib_yes(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lib_popup_answer(1);
    }
}

static void ui_event_lib_no(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lib_popup_answer(0);
    }
}

static void show_lib_popup(void)
{
    if (s_lib_popup && lv_obj_is_valid(s_lib_popup)) {
        return;
    }
    char proto[IR_BLASTER_NAME_MAX];
    proto[0] = 0;
    int slot = ir_blaster_learn_slot();
    ir_blaster_slot_info_t info;
    if ((slot >= 0) && (ir_blaster_get_slot_info((uint8_t)slot, &info) == ESP_OK) && info.name[0]) {
        strncpy(proto, info.name, sizeof(proto) - 1);
        proto[sizeof(proto) - 1] = 0;
    }

    lv_obj_t *shade = lv_obj_create(lv_layer_top());
    lv_obj_set_size(shade, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(shade, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(shade, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(shade, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(shade, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(shade, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *box = lv_obj_create(shade);
    lv_obj_set_size(box, 300, 150);
    lv_obj_center(box);
    lv_obj_set_style_radius(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(box, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(box);
    lv_label_set_text(title, "Accept as Library?");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *body = lv_label_create(box);
    lv_label_set_text(body, proto[0] ? proto : "Known protocol");
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *yes = lv_button_create(box);
    lv_obj_set_size(yes, 110, 36);
    lv_obj_align(yes, LV_ALIGN_BOTTOM_LEFT, 8, -4);
    style_menu_btn(yes);
    lv_obj_t *yes_lbl = lv_label_create(yes);
    lv_label_set_text(yes_lbl, "Yes");
    lv_obj_center(yes_lbl);
    lv_obj_set_style_text_color(yes_lbl, lv_color_hex(0x09007D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(yes, ui_event_lib_yes, LV_EVENT_CLICKED, NULL);

    lv_obj_t *no = lv_button_create(box);
    lv_obj_set_size(no, 110, 36);
    lv_obj_align(no, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
    style_menu_btn(no);
    lv_obj_t *no_lbl = lv_label_create(no);
    lv_label_set_text(no_lbl, "No");
    lv_obj_center(no_lbl);
    lv_obj_set_style_text_color(no_lbl, lv_color_hex(0x09007D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(no, ui_event_lib_no, LV_EVENT_CLICKED, NULL);

    s_lib_popup = shade;
}

static void maybe_lib_popup(void)
{
    if (ir_blaster_learn_status() == IR_LEARN_ASK_LIB) {
        show_lib_popup();
    } else {
        close_lib_popup();
    }
}

static void ui_event_IrBack(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ir_blaster_cancel_learn();
        close_lib_popup();
        _ui_screen_change(&ui_MainMenu, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 100, &ui_MainMenu_screen_init);
    }
}

static void ui_event_IrDetailBack(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ir_blaster_cancel_learn();
        close_lib_popup();
        _ui_screen_change(&ui_IrBlaster, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 400, 50, &ui_IrBlaster_screen_init);
    }
}

static void ui_event_slot(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    if (!ir_ui_ok()) {
        return;
    }
    uint8_t slot = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    ir_blaster_slot_info_t info;
    if (ir_blaster_get_slot_info(slot, &info) != ESP_OK) {
        return;
    }
    if (ir_blaster_learn_status() == IR_LEARN_ASK_LIB) {
        return;
    }
    if (info.kind == IR_SLOT_EMPTY) {
        (void)ir_blaster_begin_learn(slot, 0);
        ui_IrBlaster_refresh();
        return;
    }
    if (ir_blaster_learn_status() == IR_LEARN_WAIT) {
        ir_blaster_cancel_learn();
        ui_IrBlaster_refresh();
    }
    ui_IrSlotDetail_set_slot(slot);
    _ui_screen_change(&ui_IrSlotDetail, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 50, &ui_IrSlotDetail_screen_init);
}

void ui_IrBlaster_refresh(void)
{
    if (!IR_UI_OBJ_READY(ui_IrBlaster) || !ir_ui_ok()) {
        return;
    }
    char buf[96];
    for (uint8_t i = 0; i < IR_BLASTER_SLOT_COUNT; i++) {
        if (IR_UI_OBJ_READY(s_slot_lbl[i])) {
            slot_text(i, buf, sizeof(buf));
            lv_label_set_text(s_slot_lbl[i], buf);
        }
    }
    if (IR_UI_OBJ_READY(s_status_lbl)) {
        snprintf(buf, sizeof(buf), "%s   custom %u/%u free",
                 ir_blaster_learn_message(),
                 ir_blaster_custom_used(), ir_blaster_custom_free());
        lv_label_set_text(s_status_lbl, buf);
    }
    maybe_lib_popup();
}

void ui_IrBlaster_screen_init(void)
{
    if (!ir_ui_ok()) {
        return;
    }
    if (IR_UI_OBJ_READY(ui_IrBlaster)) {
        ui_IrBlaster_refresh();
        return;
    }
    ui_IrBlaster = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_IrBlaster, LV_OBJ_FLAG_SCROLLABLE);
    ui_set_screen_background(ui_IrBlaster);

    lv_obj_t *title = lv_obj_create(ui_IrBlaster);
    lv_obj_set_size(title, 450, 40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back = lv_imagebutton_create(title);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_obj_set_size(back, 35, 35);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(back, ui_event_IrBack, LV_EVENT_CLICKED, NULL);

    lv_obj_t *tlabel = lv_label_create(title);
    lv_label_set_text(tlabel, "IR Blaster");
    lv_obj_align(tlabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(tlabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_status_lbl = lv_label_create(ui_IrBlaster);
    lv_obj_set_width(s_status_lbl, 440);
    lv_obj_align(s_status_lbl, LV_ALIGN_TOP_MID, 0, 44);
    lv_label_set_text(s_status_lbl, "TSTAT11 IR");
    lv_label_set_long_mode(s_status_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *list = lv_obj_create(ui_IrBlaster);
    lv_obj_set_size(list, 460, 250);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_style_bg_opa(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    style_clear_container(list);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (uint8_t i = 0; i < IR_BLASTER_SLOT_COUNT; i++) {
        s_slot_btn[i] = lv_button_create(list);
        lv_obj_set_size(s_slot_btn[i], IR_SLOT_BTN_W, IR_SLOT_BTN_H);
        style_menu_btn(s_slot_btn[i]);
        s_slot_lbl[i] = lv_label_create(s_slot_btn[i]);
        lv_obj_center(s_slot_lbl[i]);
        lv_obj_set_style_text_color(s_slot_lbl[i], lv_color_hex(0x09007D), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(s_slot_btn[i], ui_event_slot, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }
    ui_IrBlaster_refresh();
}

void ui_IrBlaster_screen_destroy(void)
{
    if (ui_IrBlaster) {
        lv_obj_del(ui_IrBlaster);
    }
    ui_IrBlaster = NULL;
    s_status_lbl = NULL;
    close_lib_popup();
    memset(s_slot_btn, 0, sizeof(s_slot_btn));
    memset(s_slot_lbl, 0, sizeof(s_slot_lbl));
}

static void show_rename(const char *cur, uint8_t kind, uint8_t index);

static void ui_event_lib_btn(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int idx = (int)(uintptr_t)lv_event_get_user_data(e);
        (void)ir_blaster_send_lib(s_open_slot, idx);
    }
}

static void ui_event_custom_send(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (code == LV_EVENT_LONG_PRESSED) {
        s_suppress_click = 1;
        char name[IR_BLASTER_NAME_MAX];
        if (ir_blaster_custom_name(s_open_slot, idx, name, sizeof(name)) == ESP_OK) {
            show_rename(name, IR_RENAME_CUSTOM, (uint8_t)idx);
        }
        return;
    }
    if (code == LV_EVENT_CLICKED) {
        if (s_suppress_click) {
            s_suppress_click = 0;
            return;
        }
        (void)ir_blaster_send_custom(s_open_slot, idx);
    }
}

static void rename_done(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if ((code != LV_EVENT_READY) && (code != LV_EVENT_CANCEL)) {
        return;
    }
    if (code == LV_EVENT_READY) {
        lv_obj_t *ta = lv_keyboard_get_textarea(kb);
        const char *txt = ta ? lv_textarea_get_text(ta) : "";
        if (s_rename_kind == IR_RENAME_CUSTOM) {
            (void)ir_blaster_rename_custom(s_open_slot, (int)s_rename_index, txt);
        } else {
            (void)ir_blaster_rename_slot(s_open_slot, txt);
        }
    }
    lv_obj_t *popup = lv_obj_get_parent(kb);
    lv_obj_del(popup);
    s_grid_lib_n = -1;
    ui_IrSlotDetail_refresh();
}

static void show_rename(const char *cur, uint8_t kind, uint8_t index)
{
    s_rename_kind = kind;
    s_rename_index = index;
    lv_obj_t *popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(popup, 440, 230);
    lv_obj_center(popup);
    lv_obj_remove_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(popup, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(popup, lv_color_hex(0xF4F7FB), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(popup, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ta = lv_textarea_create(popup);
    lv_obj_set_size(ta, 420, 52);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(ta, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ta, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ta, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ta, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ta, lv_color_hex(0x111111), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_opa(ta, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x349EF9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x111111), LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_border_width(ta, 2, LV_PART_CURSOR);
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_LEFT, LV_PART_CURSOR);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x111111), LV_PART_CURSOR);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, IR_BLASTER_NAME_MAX - 1);
    lv_textarea_set_text(ta, cur ? cur : "");
    lv_obj_add_state(ta, LV_STATE_FOCUSED);

    lv_obj_t *kb = lv_keyboard_create(popup);
    lv_obj_set_size(kb, 424, 154);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(kb, LV_OBJ_FLAG_SCROLLABLE);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, rename_done, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, rename_done, LV_EVENT_CANCEL, NULL);
}

static void ui_event_rename_slot(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    ir_blaster_slot_info_t info;
    if (ir_blaster_get_slot_info(s_open_slot, &info) == ESP_OK) {
        show_rename(info.name, IR_RENAME_SLOT, 0);
    }
}

static void ui_event_learn_custom(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        (void)ir_blaster_begin_learn(s_open_slot, 1);
        ui_IrSlotDetail_refresh();
    }
}

static void ui_event_clear_slot(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        (void)ir_blaster_clear_slot(s_open_slot);
        _ui_screen_change(&ui_IrBlaster, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 400, 50, &ui_IrBlaster_screen_init);
    }
}

void ui_IrSlotDetail_set_slot(uint8_t slot)
{
    if (s_open_slot != slot) {
        s_grid_lib_n = -1;
    }
    s_open_slot = slot;
}

static lv_obj_t *add_grid_btn(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, void *ud,
                             int32_t w, int32_t h, int kind)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, w, h);
    if (kind == IR_BTN_ACTION) {
        style_menu_btn(b);
    } else {
        style_key_btn(b, kind);
    }
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    if (kind == IR_BTN_ACTION) {
        lv_obj_set_style_text_color(l, lv_color_hex(0x09007D), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    if (kind == IR_BTN_CUSTOM) {
        lv_obj_add_event_cb(b, cb, LV_EVENT_LONG_PRESSED, ud);
    }
    return b;
}

void ui_IrSlotDetail_refresh(void)
{
    if (!IR_UI_OBJ_READY(ui_IrSlotDetail) || !ir_ui_ok()) {
        return;
    }
    ir_blaster_slot_info_t info;
    if (ir_blaster_get_slot_info(s_open_slot, &info) != ESP_OK) {
        return;
    }
    char buf[80];
    const char *title = info.name[0] ? info.name : info.proto_name;
    snprintf(buf, sizeof(buf), "%s", title);
    if (IR_UI_OBJ_READY(s_detail_title)) {
        lv_label_set_text(s_detail_title, buf);
    }
    if (IR_UI_OBJ_READY(s_detail_status)) {
        snprintf(buf, sizeof(buf), "%s  custom %u/%u",
                 ir_blaster_learn_message(),
                 ir_blaster_custom_used(), ir_blaster_custom_free());
        lv_label_set_text(s_detail_status, buf);
    }
    if (!IR_UI_OBJ_READY(s_detail_grid)) {
        return;
    }

    maybe_lib_popup();

    int lib_n = ir_blaster_lib_button_count(s_open_slot);
    int cn = ir_blaster_custom_count(s_open_slot);
    uint8_t st = ir_blaster_learn_status();
    if ((lib_n == s_grid_lib_n) && (cn == s_grid_custom_n) && (st == s_grid_learn)) {
        return;
    }

    s_grid_lib_n = lib_n;
    s_grid_custom_n = cn;
    s_grid_learn = st;

    lv_obj_clean(s_detail_grid);

    for (int i = 0; i < lib_n; i++) {
        int kind = ((lib_n > 1) && (i == IR_LIB_BTN_POWER)) ? IR_BTN_POWER : IR_BTN_LIB;
        add_grid_btn(s_detail_grid, ir_blaster_lib_button_name(s_open_slot, i),
                     ui_event_lib_btn, (void *)(uintptr_t)i, IR_KEY_W, IR_KEY_H, kind);
    }
    for (int i = 0; i < cn; i++) {
        char name[IR_BLASTER_NAME_MAX];
        if (ir_blaster_custom_name(s_open_slot, i, name, sizeof(name)) != ESP_OK) {
            continue;
        }
        add_grid_btn(s_detail_grid, name, ui_event_custom_send, (void *)(uintptr_t)i,
                     IR_KEY_W, IR_KEY_H, IR_BTN_CUSTOM);
    }
}

void ui_IrSlotDetail_screen_init(void)
{
    if (!ir_ui_ok()) {
        return;
    }
    if (IR_UI_OBJ_READY(ui_IrSlotDetail)) {
        ui_IrSlotDetail_refresh();
        return;
    }
    ui_IrSlotDetail = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_IrSlotDetail, LV_OBJ_FLAG_SCROLLABLE);
    ui_set_screen_background(ui_IrSlotDetail);

    lv_obj_t *title = lv_obj_create(ui_IrSlotDetail);
    lv_obj_set_size(title, 450, 40);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(title, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *back = lv_imagebutton_create(title);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_obj_set_size(back, 35, 35);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(back, ui_event_IrDetailBack, LV_EVENT_CLICKED, NULL);

    s_detail_title = lv_label_create(title);
    lv_obj_align(s_detail_title, LV_ALIGN_CENTER, 20, 0);
    lv_obj_set_style_text_font(s_detail_title, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_detail_status = lv_label_create(ui_IrSlotDetail);
    lv_obj_set_width(s_detail_status, 440);
    lv_obj_align(s_detail_status, LV_ALIGN_TOP_MID, 0, 44);

    lv_obj_t *actions = lv_obj_create(ui_IrSlotDetail);
    lv_obj_set_size(actions, 460, 48);
    lv_obj_align(actions, LV_ALIGN_TOP_MID, 0, 68);
    lv_obj_set_style_bg_opa(actions, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    style_clear_container(actions);
    lv_obj_set_scroll_dir(actions, LV_DIR_NONE);
    lv_obj_remove_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    add_grid_btn(actions, "Add Custom", ui_event_learn_custom, NULL, IR_ACTION_W, IR_KEY_H, IR_BTN_ACTION);
    add_grid_btn(actions, "Rename", ui_event_rename_slot, NULL, IR_ACTION_W, IR_KEY_H, IR_BTN_ACTION);
    add_grid_btn(actions, "Clear slot", ui_event_clear_slot, NULL, IR_ACTION_W, IR_KEY_H, IR_BTN_ACTION);

    s_detail_grid = lv_obj_create(ui_IrSlotDetail);
    lv_obj_set_size(s_detail_grid, 460, 178);
    lv_obj_align(s_detail_grid, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_scroll_dir(s_detail_grid, LV_DIR_VER);
    lv_obj_set_flex_flow(s_detail_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(s_detail_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    style_clear_container(s_detail_grid);
    lv_obj_set_style_pad_all(s_detail_grid, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(s_detail_grid, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(s_detail_grid, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_detail_grid, LV_OPA_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *hint = lv_label_create(ui_IrSlotDetail);
    lv_label_set_text(hint, "Long press custom key to rename");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_RIGHT, -8, -4);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(hint, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_grid_lib_n = -1;
    s_grid_custom_n = -1;
    s_grid_learn = 0xFF;
    ui_IrSlotDetail_refresh();
}

void ui_IrSlotDetail_screen_destroy(void)
{
    if (ui_IrSlotDetail) {
        lv_obj_del(ui_IrSlotDetail);
    }
    ui_IrSlotDetail = NULL;
    s_detail_title = NULL;
    s_detail_status = NULL;
    s_detail_grid = NULL;
    s_grid_lib_n = -1;
    s_grid_custom_n = -1;
    s_grid_learn = 0xFF;
}

#else /* !CONFIG_IDF_TARGET_ESP32S3 */

bool ui_ir_blaster_visible(void) { return false; }
void ui_IrBlaster_screen_init(void) {}
void ui_IrBlaster_screen_destroy(void) { ui_IrBlaster = NULL; }
void ui_IrBlaster_refresh(void) {}
void ui_IrSlotDetail_screen_init(void) {}
void ui_IrSlotDetail_screen_destroy(void) { ui_IrSlotDetail = NULL; }
void ui_IrSlotDetail_refresh(void) {}
void ui_IrSlotDetail_set_slot(uint8_t slot) { (void)slot; }

#endif
