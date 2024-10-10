FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer esp_wifi json tflite_ctrl misc_helper camera_ctrl mqtt_ctrl influxdb_ctrl fileserver_ota image_manipulation wlan_ctrl config_handling)


