# ================================================================== #
# Detect OS — Windows_NT is set by both cmd.exe and PowerShell
# ================================================================== #
ifeq ($(OS),Windows_NT)
    TARGET   = run_tests.exe
    RM       = del /Q
    PATHSEP  = $(strip \)
    RUN      = $(TARGET)
    # MinGW uses the same --allow-multiple-definition but via -Xlinker
    MULTI_DEF = -Xlinker --allow-multiple-definition
else
    TARGET   = run_tests
    RM       = rm -f
    PATHSEP  = /
    RUN      = ./$(TARGET)
    MULTI_DEF = -Wl,--allow-multiple-definition
endif

# ================================================================== #
# Compiler and Flags
# ================================================================== #
CC = gcc

CFLAGS = -Wall -Wextra -std=gnu99 -g \
         -Wno-incompatible-pointer-types \
         -Wno-unused-parameter \
         -Wno-unused-variable \
         -Wno-unused-function \
         -Wno-sign-compare \
         -Wno-maybe-uninitialized \
         -Wno-implicit-fallthrough \
         -Wno-format \
         -Wno-pedantic \
         -include stdint.h \
         -include stdbool.h \
         -include sdkconfig.h \
         -DALL_COV=0 \
         -DBACNET_SUB_COV=1 \
         -DMINI_NEW_TINY=4 \
         -DMINI_TINY_ARM=7 \
         -DMINI_TINY_11I=12 \
         -DMINI_SMALL_ARM=6 \
         -DMINI_BIG_ARM=5 \
         -DMINI_BIG=1 \
         -DMINI_SMALL=2 \
         -DMINI_TINY=3 \
         $(EXTRA_CFLAGS)

# Linker Flags
# EXTRA_LDFLAGS is injected by CI (e.g. -fsanitize=address,undefined on Ubuntu)
# MULTI_DEF is platform-specific (set above)
LDFLAGS = $(MULTI_DEF) -lm $(EXTRA_LDFLAGS)

# ================================================================== #
# Include Directories
# ================================================================== #
INC = -Iunity \
      -Imocks \
      -Imocks/freertos \
      -I../main \
      -I../main/LCD_Driver \
      -I../main/TemcoScreen \
      -I../main/Mqtt_Handler \
      -I../main/WireGuard_App \
      -I../temco_IO_control/include \
      -I../temco_bacnet/include \
      -I../temco_bacnet/private

# ================================================================== #
# Source Files
# ================================================================== #

# Test Source Files
TEST_SRCS = test_runner.c \
            test_io_control.c \
            test_mqtt.c \
            test_modbus.c \
            test_sensors.c \
            test_rtc_time.c \
            test_bacnet_codecs.c \
            test_math_drivers.c \
            test_bacnet_objects.c \
            test_system_data.c \
            test_extra_apis.c

# Mock Source Files
MOCK_SRCS = mocks/mock_freertos.c \
            mocks/mock_user_data.c \
            mocks/mock_hardware.c \
            mocks/mock_esp_idf.c \
            mocks/mock_lvgl.c \
            mocks/mock_itoa.c \
            mocks/cJSON.c \
            unity/unity.c

# Main Application C files active in CMakeLists
MAIN_SRCS = app_main.c \
            modbus.c \
            Mqtt_Handler/Mqtt_Handler.c \
            i2c_task.c \
            ota.c \
            wifi.c \
            sgp30_featureset.c \
            sensirion_common.c \
            sgp30.c \
            ethernet_task.c \
            flash.c \
            pro_info.c \
            store.c \
            led_pwm.c \
            inputs.c \
            outputs.c \
            io.c \
            mlx90632.c \
            mlx90632_extended_meas.c \
            rtc.c \
            timegm.c \
            sht4x.c \
            sht3x.c \
            scd4x_i2c.c \
            scan.c \
            commsub.c \
            ade7953.c \
            lcd.c \
            smtp_client.c \
            airlab.c \
            LightSwitch.c \
            LS_LED.c \
            INA228.c \
            mppt_task.c \
            Menu.c \
            menuIdle.c \
            menuMain.c \
            menuSet.c \
            sntp_app.c \
            multiMeter.c \
            mm_spi.c \
            hy3131_sw.c \
            Light_pwm.c \
            co2.c \
            lora.c \
            lcdTheme.c \
            DisHomeScreen.c \
            DisFonts.c \
            DisSymbol.c \
            LCD_Driver/lcd_drv.c \
            LCD_Driver/lcd_task.c \
            LCD_Driver/touch_drv.c \
            LCD_Driver/lv_UserPeram.c \
            WireGuard_App/WireGuard_App.c

# BACnet Stack C files active in CMakeLists
BACNET_SRCS = src/abort.c \
              src/address.c \
              src/ai.c \
              src/ao.c \
              src/apdu.c \
              src/av.c \
              src/bacaddr.c \
              src/bacapp.c \
              src/bacdcode.c \
              src/bacdevobjpropref.c \
              src/bacerror.c \
              src/bacfile.c \
              src/bacint.c \
              src/bacprop.c \
              src/bacpropstates.c \
              src/bacreal.c \
              src/bacstr.c \
              src/bactext.c \
              src/bactimevalue.c \
              src/bi.c \
              src/bo.c \
              src/bv.c \
              src/bvlc.c \
              src/calendar.c \
              src/cov.c \
              src/s_cov.c \
              src/h_cov.c \
              src/h_ucov.c \
              src/h_ccov.c \
              src/crc.c \
              src/datalink.c \
              src/dlmstp.c \
              src/datetime.c \
              src/dcc.c \
              src/device.c \
              src/fifo.c \
              src/h_iam.c \
              src/h_npdu.c \
              src/h_rp.c \
              src/h_rpm.c \
              src/h_ts.c \
              src/h_whohas.c \
              src/h_whois.c \
              src/h_wp.c \
              src/h_wpm.c \
              src/iam.c \
              src/ihave.c \
              src/indtext.c \
              src/memcopy.c \
              src/ms-input.c \
              src/msv.c \
              src/noserv.c \
              src/npdu.c \
              src/proplist.c \
              src/proprietary.c \
              src/reject.c \
              src/ringbuf.c \
              src/rp.c \
              src/rpm.c \
              src/s_dcc.c \
              src/s_iam.c \
              src/s_ihave.c \
              src/s_readrange.c \
              src/s_rp.c \
              src/s_ts.c \
              src/s_upt.c \
              src/s_whois.c \
              src/s_wp.c \
              src/schedule.c \
              src/timer.c \
              src/timesync.c \
              src/trendlog.c \
              src/tsm.c \
              src/txbuf.c \
              src/whohas.c \
              src/whois.c \
              src/wp.c \
              src/wpm.c \
              private/alarm.c \
              private/decode.c \
              private/bac_control.c \
              private/ptransfer.c \
              private/rs485.c \
              private/user_data.c \
              private/bac_interface.c \
              private/bip.c \
              src/h_rr.c \
              src/h_rr_a.c \
              src/readrange.c

# IO Control C files active in CMakeLists
IO_CONTROL_SRCS = src/inputs.c \
                  src/outputs.c

# ================================================================== #
# Build source lists with path prefixes
# ================================================================== #
APP_SRCS = $(addprefix ../main/, $(MAIN_SRCS)) \
           $(wildcard ../main/TemcoScreen/*.c) \
           $(addprefix ../temco_bacnet/, $(BACNET_SRCS)) \
           $(addprefix ../temco_IO_control/, $(IO_CONTROL_SRCS))

SRCS = $(TEST_SRCS) $(MOCK_SRCS) $(APP_SRCS)
OBJS = $(SRCS:.c=.o)

# ================================================================== #
# Targets
# ================================================================== #
.PHONY: all clean run

all: $(TARGET) run

$(TARGET): $(OBJS)
	$(CC) $(INC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

run: $(TARGET)
	$(RUN)

clean:
ifeq ($(OS),Windows_NT)
	$(RM) $(TARGET) 2>nul || true
	for /r %%f in (*.o) do $(RM) "%%f" 2>nul || true
else
	$(RM) $(TARGET) $(OBJS)
endif