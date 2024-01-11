FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../../include" "miniz"
                    REQUIRES vfs spiffs esp_http_server app_update jomjol_flowcontroll jomjol_helper)


