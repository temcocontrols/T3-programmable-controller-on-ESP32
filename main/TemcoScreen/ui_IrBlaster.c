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
static lv_obj_t *s_detail_title;
static lv_obj_t *s_detail_status;
static lv_obj_t *s_detail_grid;
static uint8_t s_open_slot;
static uint8_t s_rename_custom_index = 0xFF;

bool ui_ir_blaster_visible(void)
{
    return (Modbus.mini_type == MINI_TSTAT11) && ir_blaster_is_supported();
}

static bool ir_ui_ok(void)
{
    return ui_ir_blaster_visible();
}

static void style_menu_btn(lv_obj_t *btn)
{
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x349EF9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0x81BAEA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void slot_text(uint8_t slot, char *buf, size_t len)
{
    ir_blaster_slot_info_t info;
    if (ir_blaster_get_slot_info(slot, &info) != ESP_OK) {
        snprintf(buf, len, "Slot %u  Empty", (unsigned)(slot + 1));
        return;
    }
    if (info.kind == IR_SLOT_EMPTY) {
        snprintf(buf, len, "Slot %u  Empty  (tap to learn)", (unsigned)(slot + 1));
    } else if (info.kind == IR_SLOT_LIBRARY) {
        snprintf(buf, len, "Slot %u  Lib:%s  custom %u  %s",
                 (unsigned)(slot + 1), info.proto_name, info.custom_count, info.name);
    } else {
        snprintf(buf, len, "Slot %u  Custom  keys %u  %s",
                 (unsigned)(slot + 1), info.custom_count, info.name);
    }
}

static void ui_event_IrBack(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ir_blaster_cancel_learn();
        _ui_screen_change(&ui_MainMenu, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 100, &ui_MainMenu_screen_init);
    }
}

static void ui_event_IrDetailBack(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ir_blaster_cancel_learn();
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
    if (info.kind == IR_SLOT_EMPTY) {
        (void)ir_blaster_begin_learn(slot, 0);
        ui_IrBlaster_refresh();
        return;
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
    lv_obj_set_style_border_side(list, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < IR_BLASTER_SLOT_COUNT; i++) {
        s_slot_btn[i] = lv_button_create(list);
        lv_obj_set_size(s_slot_btn[i], 430, 36);
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
    memset(s_slot_btn, 0, sizeof(s_slot_btn));
    memset(s_slot_lbl, 0, sizeof(s_slot_lbl));
}

static void ui_event_lib_btn(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int idx = (int)(uintptr_t)lv_event_get_user_data(e);
        (void)ir_blaster_send_lib(s_open_slot, idx);
    }
}

static void ui_event_custom_send(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int idx = (int)(uintptr_t)lv_event_get_user_data(e);
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
        if (s_rename_custom_index == 0xFF) {
            (void)ir_blaster_rename_slot(s_open_slot, txt);
        } else {
            (void)ir_blaster_rename_custom(s_open_slot, s_rename_custom_index, txt);
        }
    }
    lv_obj_t *popup = lv_obj_get_parent(kb);
    lv_obj_del(popup);
    s_rename_custom_index = 0xFF;
    ui_IrSlotDetail_refresh();
}

static void show_rename(const char *cur, uint8_t custom_index)
{
    s_rename_custom_index = custom_index;
    lv_obj_t *popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(popup, 300, 180);
    lv_obj_center(popup);
    lv_obj_t *ta = lv_textarea_create(popup);
    lv_obj_set_size(ta, 280, 40);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 4);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, IR_BLASTER_NAME_MAX - 1);
    lv_textarea_set_text(ta, cur ? cur : "");
    lv_obj_t *kb = lv_keyboard_create(popup);
    lv_obj_set_size(kb, 284, 120);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
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
        show_rename(info.name, 0xFF);
    }
}

static void ui_event_rename_custom(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    char name[IR_BLASTER_NAME_MAX];
    if (ir_blaster_custom_name(s_open_slot, idx, name, sizeof(name)) == ESP_OK) {
        show_rename(name, (uint8_t)idx);
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
    s_open_slot = slot;
}

static lv_obj_t *add_grid_btn(lv_obj_t *parent, const char *txt, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, 140, 36);
    style_menu_btn(b);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    lv_obj_set_style_text_color(l, lv_color_hex(0x09007D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
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
    snprintf(buf, sizeof(buf), "Slot %u  %s", (unsigned)(s_open_slot + 1),
             info.kind == IR_SLOT_LIBRARY ? info.proto_name : info.name);
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
    lv_obj_clean(s_detail_grid);

    int lib_n = ir_blaster_lib_button_count(s_open_slot);
    for (int i = 0; i < lib_n; i++) {
        add_grid_btn(s_detail_grid, ir_blaster_lib_button_name(s_open_slot, i),
                     ui_event_lib_btn, (void *)(uintptr_t)i);
    }
    int cn = ir_blaster_custom_count(s_open_slot);
    for (int i = 0; i < cn; i++) {
        char name[IR_BLASTER_NAME_MAX];
        if (ir_blaster_custom_name(s_open_slot, i, name, sizeof(name)) != ESP_OK) {
            continue;
        }
        lv_obj_t *row = lv_obj_create(s_detail_grid);
        lv_obj_set_size(row, 290, 40);
        lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        add_grid_btn(row, name, ui_event_custom_send, (void *)(uintptr_t)i);
        lv_obj_t *rn = lv_button_create(row);
        lv_obj_set_size(rn, 70, 36);
        style_menu_btn(rn);
        lv_obj_t *rl = lv_label_create(rn);
        lv_label_set_text(rl, "Name");
        lv_obj_center(rl);
        lv_obj_add_event_cb(rn, ui_event_rename_custom, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
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
    lv_obj_set_size(actions, 460, 44);
    lv_obj_align(actions, LV_ALIGN_TOP_MID, 0, 68);
    lv_obj_set_style_bg_opa(actions, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(actions, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    add_grid_btn(actions, "Learn custom", ui_event_learn_custom, NULL);
    add_grid_btn(actions, "Rename", ui_event_rename_slot, NULL);
    add_grid_btn(actions, "Clear slot", ui_event_clear_slot, NULL);

    s_detail_grid = lv_obj_create(ui_IrSlotDetail);
    lv_obj_set_size(s_detail_grid, 460, 190);
    lv_obj_align(s_detail_grid, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_scroll_dir(s_detail_grid, LV_DIR_VER);
    lv_obj_set_flex_flow(s_detail_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(s_detail_grid, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(s_detail_grid, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_detail_grid, LV_OPA_20, LV_PART_MAIN | LV_STATE_DEFAULT);

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
