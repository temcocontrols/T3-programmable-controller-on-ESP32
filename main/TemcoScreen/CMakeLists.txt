SET(SOURCES ui_StartUpScreen.c
    ui_HomeScreen.c
    ui_MainMenu.c
    ui_WifiConfig.c
    ui_NetworkConfig.c
    ui_Protocols.c
    ui_Parameters.c
    ui_Time.c
    ui_ScheduleScreen.c
    ui_ScheduleEditScreen.c
    ui_HolidayCalenderScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_wifisym_png.c
    ui_img_rightarrow_png.c
    ui_img_displayhomescreendot_png.c
    ui_img_fan_auto_new_png.c
    ui_img_fan_small_png.c
    ui_img_fan_withtime_png.c
    ui_img_1679221590.c
    ui_img_1020677502.c
    ui_img_1869774352.c
    ui_img_leftarrow_png.c
    ui_img_displayscreendotformenuscreen_gray_png.c
    ui_img_1984394067.c
    ui_img_444508050.c
    ui_img_1589547977.c
    ui_img_1207302323.c
    ui_img_backsmallarrow_png.c
    fonts/ui_font_Arial80.c)

add_library(ui ${SOURCES})
