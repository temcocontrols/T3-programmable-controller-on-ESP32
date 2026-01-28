SET(SOURCES ui_StartUpScreen.c
    ui_HomeScreen.c
    ui_MainMenu.c
    ui_ChangeConfig.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_wifisym_png.c
    ui_img_round_manage_accounts_black_36pt_1x_png.c
    ui_img_round_arrow_back_black_36_png.c
    fonts/ui_font_Arial80.c)

add_library(ui ${SOURCES})
