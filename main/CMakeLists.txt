# Get all your .c and .cpp files

if(NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "ESP_MATTER_PATH is not set")
endif()

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})

set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/device_hal/device"
    "${ESP_MATTER_PATH}/device_hal/led_driver"
    ${extra_components_dirs_append}
)

idf_component_register(SRCS
							"AppMain.c"
							"modbus.c"
#							"deviceparams.c"
							"i2c_task.c"
							"ota.c"
#							"microphone.c"
							"wifi.c"
							"sgp30_featureset.c"
							"sensirion_common.c"
							"sgp30.c"
							"ethernet_task.c"
							"flash.c"
							"pro_info.c"
#							"scd40.c"
							"store.c"
							"led_pwm.c"
							"inputs.c"
							"outputs.c"
							"io.c"
							"mlx90632.c"
							"mlx90632_extended_meas.c"
							"rtc.c"
							"timegm.c"
							"sht4x.c"
							"sht3x.c"
							"scd4x_i2c.c"
							"scan.c"
							"commsub.c"
							"ade7953.c"
							"lcd.c"
#							"pic.c"
							"smtp_client.c"
							"airlab.c"
							"LightSwitch.c"
							"LS_LED.c"
							"INA228.c"
							"mppt_task.c"
#							"spi_master_arm.c"
#							"ble_mesh.c"
							"Menu.c"
							"menuIdle.c"
							"menuMain.c"
						#	"Constcode.c"
							"menuSet.c"
						#	"menuDaySet.c"
							"sntp_app.c"
							"multiMeter.c"
							"mm_spi.c"
							"hy3131_sw.c"
							"Light_pwm.c"
					#		"ddns.c"
						#	"lowPower.c"
							"co2.c"
							"lcdTheme.c"
							"DisHomeScreen.c"
							"DisFonts.c"
							"DisSymbol.c"
                            "matter_light.cpp"
							"app_driver.cpp"
    PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
    PRIV_REQUIRES
                esp_matter
                button
                led_strip
				led_driver
                temco_IO_control
                temco_bacnet
                esp_https_ota
                esp_http_client
                app_update
                esp_eth
                esp_netif
                lwip
                bt
				app_reset
)

if (CONFIG_ENABLE_SET_CERT_DECLARATION_API)
    target_add_binary_data(${COMPONENT_TARGET} "certification_declaration/certification_declaration.der" BINARY)
endif()

# Keep your special link flag for pro_info
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u pro_info")

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")