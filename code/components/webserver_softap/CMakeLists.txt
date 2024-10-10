FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../../include"
                    REQUIRES esp_http_server json misc_helper mqtt_ctrl influxdb_ctrl mainprocess_ctrl)


