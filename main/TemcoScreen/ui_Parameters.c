#include "ui.h"
#include <stdlib.h>  // atoi

// ── Column definitions ────────────────────────────────────────
#define COL_NO    0
#define COL_DESC  1
#define COL_LABEL 2
#define COL_VALUE 3
#define COL_AM    4
#define COL_DA    5
#define COL_CTRL  6
#define COL_SW    7   // OUTPUT only
#define COL_RANGE 8   // col 7 for INPUT/VAR, col 8 for OUTPUT

// ── Static state ──────────────────────────────────────────────
static lv_obj_t  * s_param_table  = NULL;
static lv_obj_t  * s_edit_popup   = NULL;
static lv_obj_t  * s_edit_ta      = NULL;
static uint16_t    s_edit_row     = 0;
static uint16_t    s_edit_col     = 0;


// ─────────────────────────────────────────────────────────────
// Existing UI screen – unchanged from SquareLine output
// ─────────────────────────────────────────────────────────────
lv_obj_t * uic_Parameters;
lv_obj_t * ui_Parameters         = NULL;
lv_obj_t * ui_ChangeConfigTitle2  = NULL;
lv_obj_t * ui_NetworkSetupConfig4 = NULL;
lv_obj_t * ui_GotoMenuButton8     = NULL;
lv_obj_t * ui_Container1          = NULL;
lv_obj_t * ui_Panel4              = NULL;
lv_obj_t * ui_ParameterUpdateBtn  = NULL;
lv_obj_t * ui_Label15             = NULL;

// ─────────────────────────────────────────────────────────────
// Event callbacks
// ─────────────────────────────────────────────────────────────
void ui_event_Parameters(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_SCREEN_LOAD_START)
        UpdateParameterTableFunc(e);
}

void ui_event_GotoMenuButton8(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ParameterClearTableFunc(e);
        _ui_screen_change(&ui_MainMenu, LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                          500, 100, &ui_MainMenu_screen_init);
    }
}

void ui_event_ParameterUpdateBtn(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        ParameterUpdateFunc(e);
        _ui_screen_change(&ui_MainMenu, LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                          1000, 100, &ui_MainMenu_screen_init);
    }
}

// ─────────────────────────────────────────────────────────────
// Screen init – unchanged from SquareLine output
// ─────────────────────────────────────────────────────────────
void ui_Parameters_screen_init(void)
{
    ui_Parameters = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Parameters, LV_OBJ_FLAG_SCROLLABLE);

    ui_ChangeConfigTitle2 = lv_obj_create(ui_Parameters);
    lv_obj_set_width(ui_ChangeConfigTitle2, 450);
    lv_obj_set_height(ui_ChangeConfigTitle2, 40);
    lv_obj_set_align(ui_ChangeConfigTitle2, LV_ALIGN_TOP_MID);
    lv_obj_remove_flag(ui_ChangeConfigTitle2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_ChangeConfigTitle2, lv_color_hex(0x2D70A0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ChangeConfigTitle2, 0,                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_ChangeConfigTitle2, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NetworkSetupConfig4 = lv_label_create(ui_ChangeConfigTitle2);
    lv_obj_set_width(ui_NetworkSetupConfig4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_NetworkSetupConfig4, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_NetworkSetupConfig4, -80);
    lv_obj_set_y(ui_NetworkSetupConfig4, 0);
    lv_obj_set_align(ui_NetworkSetupConfig4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_NetworkSetupConfig4, "   Parameter Setup");
    lv_obj_set_style_text_font(ui_NetworkSetupConfig4, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_GotoMenuButton8 = lv_imagebutton_create(ui_ChangeConfigTitle2);
    lv_imagebutton_set_src(ui_GotoMenuButton8, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_imagebutton_set_src(ui_GotoMenuButton8, LV_IMAGEBUTTON_STATE_PRESSED,  NULL, &ui_img_backsmallarrow_png, NULL);
    lv_obj_set_width(ui_GotoMenuButton8, 35);
    lv_obj_set_height(ui_GotoMenuButton8, 35);
    lv_obj_set_align(ui_GotoMenuButton8, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_radius(ui_GotoMenuButton8, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GotoMenuButton8, lv_color_hex(0x3C3C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_GotoMenuButton8, 255,                      LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Container1 = lv_obj_create(ui_Parameters);
    lv_obj_remove_style_all(ui_Container1);
    lv_obj_set_width(ui_Container1, 480);
    lv_obj_set_height(ui_Container1, 280);
    lv_obj_set_x(ui_Container1, 0);
    lv_obj_set_y(ui_Container1, 20);
    lv_obj_set_align(ui_Container1, LV_ALIGN_CENTER);
    lv_obj_remove_flag(ui_Container1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    // ui_Panel4 — made scrollable so the table can scroll inside it
    ui_Panel4 = lv_obj_create(ui_Container1);
    lv_obj_set_width(ui_Panel4, 480);
    lv_obj_set_height(ui_Panel4, 230);
    lv_obj_set_x(ui_Panel4, 0);
    lv_obj_set_y(ui_Panel4, -20);
    lv_obj_set_align(ui_Panel4, LV_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ui_Panel4, 0,                      LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Panel4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Panel4, 0,                        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_Panel4, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    // Keep scroll enabled so table rows are reachable
    lv_obj_add_flag(ui_Panel4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_Panel4, LV_DIR_HOR | LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_Panel4, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_snap_y(ui_Panel4, LV_SCROLL_SNAP_NONE);

    ui_ParameterUpdateBtn = lv_button_create(ui_Container1);
    lv_obj_set_width(ui_ParameterUpdateBtn, 100);
    lv_obj_set_height(ui_ParameterUpdateBtn, 30);
    lv_obj_set_x(ui_ParameterUpdateBtn, 180);
    lv_obj_set_y(ui_ParameterUpdateBtn, 110);
    lv_obj_set_align(ui_ParameterUpdateBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ParameterUpdateBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_flag(ui_ParameterUpdateBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_ParameterUpdateBtn, lv_color_hex(0x2971A4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ParameterUpdateBtn, 255,                      LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label15 = lv_label_create(ui_ParameterUpdateBtn);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Update");
    lv_obj_set_style_text_font(ui_Label15, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_GotoMenuButton8,    ui_event_GotoMenuButton8,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ParameterUpdateBtn, ui_event_ParameterUpdateBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Parameters,         ui_event_Parameters,         LV_EVENT_ALL, NULL);
    uic_Parameters = ui_Parameters;
}

// ─────────────────────────────────────────────────────────────
// Screen destroy – NULL the table pointer too
// ─────────────────────────────────────────────────────────────
void ui_Parameters_screen_destroy(void)
{
    if(ui_Parameters) lv_obj_del(ui_Parameters);

    uic_Parameters        = NULL;
    ui_Parameters         = NULL;
    ui_ChangeConfigTitle2  = NULL;
    ui_NetworkSetupConfig4 = NULL;
    ui_GotoMenuButton8     = NULL;
    ui_Container1          = NULL;
    ui_Panel4              = NULL;
    ui_ParameterUpdateBtn  = NULL;
    ui_Label15             = NULL;
    s_param_table          = NULL;   // ← lv_obj_del(ui_Parameters) frees it
    s_edit_popup           = NULL;
    s_edit_ta              = NULL;
}

/* End of the file  ----------------*/
