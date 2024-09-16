FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../../include" "miniz"
                    REQUIRES vfs spiffs esp_http_server webserver_softap app_update mainprocess_ctrl misc_helper)


