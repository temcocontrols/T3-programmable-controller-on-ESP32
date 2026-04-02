cmake_minimum_required(VERSION 3.16)

# 1. Set the target early
set(IDF_TARGET esp32)

# 2. Setup Matter Paths explicitly
set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

# 3. FIX: Hardcode the device path as a CACHE variable
# This prevents the "/esp_matter_device.cmake" not found error
set(ESP_MATTER_DEVICE_PATH "${ESP_MATTER_PATH}/device_hal/device/esp32_devkit_c" CACHE PATH "Matter Device Path")
set(ENV{ESP_MATTER_DEVICE_PATH} "${ESP_MATTER_DEVICE_PATH}")

# 4. Include ESP-IDF and Matter helper files
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${ESP_MATTER_PATH}/examples/common/cmake_common/components_include.cmake)
include(${ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)

# Override button_type to hollow_button after device cmake loads
set(button_type "hollow_button" CACHE STRING "Button type" FORCE)

# 5. Set Component Search Paths
set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/device_hal/device"
    "${ESP_MATTER_PATH}/device_hal/led_driver"    # <--- ADD THIS
    "${ESP_MATTER_PATH}/device_hal/button_driver" # <--- ADD THIS (just in case)
    "${CMAKE_CURRENT_LIST_DIR}/main"
    ${extra_components_dirs_append})

project(temco_app)

# Matter specific compiler flags
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)