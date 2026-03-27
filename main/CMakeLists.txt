# Get all your .c and .cpp files
file(GLOB SOURCES "*.c" "*.cpp")

# Remove problematic files that cause build errors
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Adafruit_ADS1X15.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/LiquidCrystal_I2C.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/LS_LED.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ble_mesh.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ethernet_task.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mppt_lcd.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/matter_light.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/veml7700.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sntp.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ota.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/menuDaySet.c")

idf_component_register(
    SRCS ${SOURCES}
    PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
    PRIV_REQUIRES esp_matter button led_strip temco_IO_control temco_bacnet esp_http_client bt
)

if (CONFIG_ENABLE_SET_CERT_DECLARATION_API)
    target_add_binary_data(${COMPONENT_TARGET} "certification_declaration/certification_declaration.der" BINARY)
endif()

# Keep your special link flag for pro_info
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u pro_info")

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")