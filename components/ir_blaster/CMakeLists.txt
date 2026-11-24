idf_component_register(
    SRCS "src/ir_blaster.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash driver esp_driver_uart esp_timer freertos heap
)
