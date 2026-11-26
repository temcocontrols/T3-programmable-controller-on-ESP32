#include "ui.h"
#include "define.h"
#include "user_data.h"
#include "flash.h"
#include "wifi.h"
#include "WireGuard_App.h"
#include "ui_json_render.h"
#include <stdio.h>
#include <string.h>

/* lv_UserPeram.h owns UI_OBJ_READY; keep this file independent of it. */
#define REMOTE_OBJ_READY(obj)   ((obj) != NULL && lv_obj_is_valid(obj))

lv_obj_t *ui_WireGuardScreen;
static lv_obj_t *wg_enable, *wg_local, *wg_peer, *wg_port, *wg_status;

/* This screen is hand-written rather than exported from SquareLine, so its
 * handlers hold file-static widget pointers.  Once the deployed JSON UI owns the
 * display those pointers still refer to the compiled screen, so they have to be
 * re-resolved by design name before being read or written. */
static void wg_resolve_widgets(void)
{
    lv_obj_t *o;

    if(!ui_json_render_active()) {
        return;
    }
    if((o = ui_json_render_find("wg_enable")) != NULL) wg_enable = o;
    if((o = ui_json_render_find("wg_local")) != NULL)  wg_local = o;
    if((o = ui_json_render_find("wg_peer")) != NULL)   wg_peer = o;
    if((o = ui_json_render_find("wg_port")) != NULL)   wg_port = o;
    if((o = ui_json_render_find("wg_status")) != NULL) wg_status = o;
}

static void remote_back(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_MainMenu, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_MainMenu_screen_init);
}

static lv_obj_t *remote_field(lv_obj_t *parent, const char *name, int y, const char *value)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, name);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y);
    lv_obj_t *field = lv_textarea_create(parent);
    lv_obj_set_size(field, 285, 34);
    lv_obj_align(field, LV_ALIGN_TOP_RIGHT, -18, y - 8);
    lv_textarea_set_one_line(field, true);
    lv_textarea_set_text(field, value);
    return field;
}

static lv_obj_t *remote_action_button(lv_obj_t *parent, const char *text, int x)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_size(button, 100, 30);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, x, -20);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2971A4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    return button;
}

static void remote_title(lv_obj_t *screen, const char *text)
{
    lv_obj_t *panel = lv_obj_create(screen);
    lv_obj_set_size(panel, 450, 40);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(panel, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, text);
    lv_obj_center(title);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *back = lv_imagebutton_create(panel);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_imagebutton_set_src(back, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &ui_img_backsmallarrow_png, NULL);
    lv_obj_set_size(back, 35, 35);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(back, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x3C3C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(back, remote_back, LV_EVENT_CLICKED, NULL);
}

static void wg_save(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    unsigned a,b,c,d;
    wg_resolve_widgets();
    wireguard_point.reg.wireguard_enable = lv_obj_has_state(wg_enable, LV_STATE_CHECKED) ? 1 : 0;
    if(sscanf(lv_textarea_get_text(wg_local), "%u.%u.%u.%u", &a,&b,&c,&d) == 4)
    {
        wireguard_point.reg.wireguard_local_ip[0]=a;
        wireguard_point.reg.wireguard_local_ip[1]=b;
        wireguard_point.reg.wireguard_local_ip[2]=c;
        wireguard_point.reg.wireguard_local_ip[3]=d;
    }
    if(sscanf(lv_textarea_get_text(wg_peer), "%u.%u.%u.%u", &a,&b,&c,&d) == 4)
        {
            wireguard_point.reg.wireguard_peer_ip[0]=a;
            wireguard_point.reg.wireguard_peer_ip[1]=b;
            wireguard_point.reg.wireguard_peer_ip[2]=c;
            wireguard_point.reg.wireguard_peer_ip[3]=d;
        }
    wireguard_point.reg.wireguard_port = (uint16_t)atoi(lv_textarea_get_text(wg_port));
    lv_label_set_text(wg_status, save_wireguard_config_to_flash() == ESP_OK ? "Saved. Restart WireGuard to apply." : "Save failed");
}

static void wg_test(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        wg_resolve_widgets();
        lv_label_set_text(wg_status, wireguard_app_start_ping() == ESP_OK ? "WireGuard test started" : "WireGuard test unavailable");
    }
}

/* The JSON UI bridges a button to ui_event_<design name>().  Test/Update exist on
 * both remote screens, so they carry screen-unique names in the deployed JSON.
 * The resolve happens inside wg_test()/wg_save(), which the compiled screen also
 * calls, so it stays correct in compiled mode. */
void ui_event_WgTestButton(lv_event_t *e)
{
    wg_test(e);
}

void ui_event_WgUpdateButton(lv_event_t *e)
{
    wg_save(e);
}

/* Called once when the screen becomes active: show the stored configuration.
 * Not run periodically - the fields are user editable and must not be
 * overwritten while typing.  Only for the JSON UI: the compiled screen fills
 * itself in ui_WireGuardScreen_screen_init() and keeps any unsaved edits when
 * it is re-entered. */
void ui_refresh_WireGuardScreen_Data(void)
{
    char buf[20];

    if(!ui_json_render_active()) {
        return;
    }
    wg_resolve_widgets();
    if(!REMOTE_OBJ_READY(wg_local) || !REMOTE_OBJ_READY(wg_peer) || !REMOTE_OBJ_READY(wg_port)) {
        return;
    }

    lv_snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                (unsigned)wireguard_point.reg.wireguard_local_ip[0],
                (unsigned)wireguard_point.reg.wireguard_local_ip[1],
                (unsigned)wireguard_point.reg.wireguard_local_ip[2],
                (unsigned)wireguard_point.reg.wireguard_local_ip[3]);
    if(strcmp(lv_textarea_get_text(wg_local), buf) != 0) lv_textarea_set_text(wg_local, buf);

    lv_snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                (unsigned)wireguard_point.reg.wireguard_peer_ip[0],
                (unsigned)wireguard_point.reg.wireguard_peer_ip[1],
                (unsigned)wireguard_point.reg.wireguard_peer_ip[2],
                (unsigned)wireguard_point.reg.wireguard_peer_ip[3]);
    if(strcmp(lv_textarea_get_text(wg_peer), buf) != 0) lv_textarea_set_text(wg_peer, buf);

    lv_snprintf(buf, sizeof(buf), "%u", (unsigned)wireguard_point.reg.wireguard_port);
    if(strcmp(lv_textarea_get_text(wg_port), buf) != 0) lv_textarea_set_text(wg_port, buf);

    if(REMOTE_OBJ_READY(wg_enable)) {
        if(wireguard_point.reg.wireguard_enable) lv_obj_add_state(wg_enable, LV_STATE_CHECKED);
        else lv_obj_clear_state(wg_enable, LV_STATE_CHECKED);
    }
}

void ui_WireGuardScreen_screen_init(void)
{
    char local[20], peer[20], port[8];
    ui_WireGuardScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_WireGuardScreen, LV_OBJ_FLAG_SCROLLABLE);
    ui_set_screen_background(ui_WireGuardScreen);
    remote_title(ui_WireGuardScreen, "WireGuard Settings");
    wg_enable=lv_switch_create(ui_WireGuardScreen);
    lv_obj_align(wg_enable,LV_ALIGN_TOP_MID,-20,48);
    if(wireguard_point.reg.wireguard_enable) lv_obj_add_state(wg_enable,LV_STATE_CHECKED);
    lv_obj_t *en=lv_label_create(ui_WireGuardScreen);
    lv_label_set_text(en,"Enable");
    lv_obj_align(en,LV_ALIGN_TOP_LEFT,20,52);
    lv_snprintf(local,sizeof(local),"%u.%u.%u.%u",wireguard_point.reg.wireguard_local_ip[0],wireguard_point.reg.wireguard_local_ip[1],wireguard_point.reg.wireguard_local_ip[2],wireguard_point.reg.wireguard_local_ip[3]);
    lv_snprintf(peer,sizeof(peer),"%u.%u.%u.%u",wireguard_point.reg.wireguard_peer_ip[0],wireguard_point.reg.wireguard_peer_ip[1],wireguard_point.reg.wireguard_peer_ip[2],wireguard_point.reg.wireguard_peer_ip[3]);
    lv_snprintf(port,sizeof(port),"%u",wireguard_point.reg.wireguard_port);
    wg_local=remote_field(ui_WireGuardScreen,"Local IP",88,local);
    wg_peer=remote_field(ui_WireGuardScreen,"Peer IP",132,peer);
    wg_port=remote_field(ui_WireGuardScreen,"Port",176,port);
    wg_status=lv_label_create(ui_WireGuardScreen);
    lv_label_set_text(wg_status,"Keys are configured through Modbus.");
    lv_obj_align(wg_status,LV_ALIGN_TOP_MID,0,220);
    lv_obj_t *test=remote_action_button(ui_WireGuardScreen,"Test",55);
    lv_obj_add_event_cb(test,wg_test,LV_EVENT_CLICKED,NULL);
    lv_obj_t *save=remote_action_button(ui_WireGuardScreen,"Update",180);
    lv_obj_add_event_cb(save,wg_save,LV_EVENT_CLICKED,NULL);
}

void ui_WireGuardScreen_screen_destroy(void)
{
    if(ui_WireGuardScreen)
    {
        lv_obj_del(ui_WireGuardScreen);
        ui_WireGuardScreen=NULL;
    }
}

#if 1
lv_obj_t *ui_DdnsScreen;
static lv_obj_t *ddns_enable, *ddns_status;

/* same re-resolution as the WireGuard screen (see wg_resolve_widgets) */
static void ddns_resolve_widgets(void)
{
    lv_obj_t *o;

    if(!ui_json_render_active()) {
        return;
    }
    if((o = ui_json_render_find("ddns_enable")) != NULL) ddns_enable = o;
    if((o = ui_json_render_find("ddns_status")) != NULL) ddns_status = o;
}

static void ddns_save(lv_event_t *e)
{
    if(lv_event_get_code(e)==LV_EVENT_CLICKED)
    {
        ddns_resolve_widgets();
        Modbus.en_dyndns=lv_obj_has_state(ddns_enable,LV_STATE_CHECKED)?2:1; save_block(FLASH_BLOCK2_PN);
        lv_label_set_text(ddns_status,"Saved");
    }
}

static void ddns_test(lv_event_t *e)
{
    if(lv_event_get_code(e)==LV_EVENT_CLICKED)
    {
        ddns_resolve_widgets();
        lv_label_set_text(ddns_status, SSID_Info.IP_Wifi_Status==WIFI_NORMAL ? "DDNS update will run shortly" : "Connect WiFi first");
    }
}

void ui_event_DdnsTestButton(lv_event_t *e)
{
    ddns_test(e);
}

void ui_event_DdnsUpdateButton(lv_event_t *e)
{
    ddns_save(e);
}

/* Called once when the screen becomes active (see ui_refresh_WireGuardScreen_Data). */
void ui_refresh_DdnsScreen_Data(void)
{
    if(!ui_json_render_active()) {
        return;
    }
    ddns_resolve_widgets();
    if(!REMOTE_OBJ_READY(ddns_enable)) {
        return;
    }
    if(Modbus.en_dyndns == 2) lv_obj_add_state(ddns_enable, LV_STATE_CHECKED);
    else lv_obj_clear_state(ddns_enable, LV_STATE_CHECKED);
}

void ui_DdnsScreen_screen_init(void)
{
    ui_DdnsScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_DdnsScreen, LV_OBJ_FLAG_SCROLLABLE);
    ui_set_screen_background(ui_DdnsScreen);
    remote_title(ui_DdnsScreen, "DDNS Settings");
    ddns_enable = lv_switch_create(ui_DdnsScreen);
    lv_obj_align(ddns_enable, LV_ALIGN_TOP_MID, 80, 75);
    if(Modbus.en_dyndns == 2) lv_obj_add_state(ddns_enable, LV_STATE_CHECKED);
    lv_obj_t *label = lv_label_create(ui_DdnsScreen);
    lv_label_set_text(label, "Enable fixed DDNS service");
    lv_obj_align(label, LV_ALIGN_TOP_MID, -45, 78);
    ddns_status = lv_label_create(ui_DdnsScreen);
    lv_label_set_text(ddns_status, "Hostname and credentials are fixed.");
    lv_obj_align(ddns_status, LV_ALIGN_TOP_MID, 0, 135);
    lv_obj_t *test = remote_action_button(ui_DdnsScreen, "Test", 55);
    lv_obj_add_event_cb(test, ddns_test, LV_EVENT_CLICKED, NULL);
    lv_obj_t *update = remote_action_button(ui_DdnsScreen, "Update", 180);
    lv_obj_add_event_cb(update, ddns_save, LV_EVENT_CLICKED, NULL);
}

void ui_DdnsScreen_screen_destroy(void)
{
    if(ui_DdnsScreen)
    {
        lv_obj_del(ui_DdnsScreen);
        ui_DdnsScreen=NULL;
    }
}

#endif
