FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

list(APPEND EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer esp32-camera esp_http_server logfile_handling image_manipulation nvs_flash fileserver_ota gpio_ctrl)


