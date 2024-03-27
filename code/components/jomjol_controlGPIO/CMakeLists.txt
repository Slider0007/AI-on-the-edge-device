FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../../include"
                    REQUIRES smartleds esp_http_server jomjol_logfile jomjol_configfile jomjol_mqtt jomjol_flowcontroll)


