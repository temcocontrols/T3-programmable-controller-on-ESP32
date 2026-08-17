#include "ui.h"
#include "define.h"
#include "user_data.h"
#include "flash.h"
#include "wifi.h"
#include "WireGuard_App.h"
#include <stdio.h>

lv_obj_t *ui_WireGuardScreen;
static lv_obj_t *wg_enable, *wg_local, *wg_peer, *wg_port, *wg_status;

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
    wireguard_point.reg.wireguard_enable = lv_obj_has_state(wg_enable, LV_STATE_CHECKED) ? 1 : 0;
    if(sscanf(lv_textarea_get_text(wg_local), "%u.%u.%u.%u", &a,&b,&c,&d) == 4)
        { wireguard_point.reg.wireguard_local_ip[0]=a; wireguard_point.reg.wireguard_local_ip[1]=b; wireguard_point.reg.wireguard_local_ip[2]=c; wireguard_point.reg.wireguard_local_ip[3]=d; }
    if(sscanf(lv_textarea_get_text(wg_peer), "%u.%u.%u.%u", &a,&b,&c,&d) == 4)
        { wireguard_point.reg.wireguard_peer_ip[0]=a; wireguard_point.reg.wireguard_peer_ip[1]=b; wireguard_point.reg.wireguard_peer_ip[2]=c; wireguard_point.reg.wireguard_peer_ip[3]=d; }
    wireguard_point.reg.wireguard_port = (uint16_t)atoi(lv_textarea_get_text(wg_port));
    lv_label_set_text(wg_status, save_wireguard_config_to_flash() == ESP_OK ? "Saved. Restart WireGuard to apply." : "Save failed");
}

static void wg_test(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
        lv_label_set_text(wg_status, wireguard_app_start_ping() == ESP_OK ? "WireGuard test started" : "WireGuard test unavailable");
}

void ui_WireGuardScreen_screen_init(void)
{
    char local[20], peer[20], port[8];
    ui_WireGuardScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_WireGuardScreen, LV_OBJ_FLAG_SCROLLABLE);
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
void ui_WireGuardScreen_screen_destroy(void) { if(ui_WireGuardScreen) lv_obj_del(ui_WireGuardScreen); ui_WireGuardScreen=NULL; }

#if 1
lv_obj_t *ui_DdnsScreen;
static lv_obj_t *ddns_enable, *ddns_status;
static void ddns_save(lv_event_t *e) { if(lv_event_get_code(e)==LV_EVENT_CLICKED) { Modbus.en_dyndns=lv_obj_has_state(ddns_enable,LV_STATE_CHECKED)?2:1; save_block(FLASH_BLOCK2_PN);
    lv_label_set_text(ddns_status,"Saved"); } }
static void ddns_test(lv_event_t *e) { if(lv_event_get_code(e)==LV_EVENT_CLICKED) { lv_label_set_text(ddns_status, SSID_Info.IP_Wifi_Status==WIFI_NORMAL ? "DDNS update will run shortly" : "Connect WiFi first"); } }
void ui_DdnsScreen_screen_init(void)
{
    ui_DdnsScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_DdnsScreen, LV_OBJ_FLAG_SCROLLABLE);
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
void ui_DdnsScreen_screen_destroy(void) { if(ui_DdnsScreen)lv_obj_del(ui_DdnsScreen);ui_DdnsScreen=NULL; }
#endif
