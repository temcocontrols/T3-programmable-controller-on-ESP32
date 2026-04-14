idf_component_register(SRCS "matter_tstat.cpp"
                       INCLUDE_DIRS "."
                       "${CMAKE_CURRENT_SOURCE_DIR}/../"
                       REQUIRES esp_matter
                                temco_bacnet)