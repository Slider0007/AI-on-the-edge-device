# #######################################################################
# Component Registration
# #######################################################################

FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_app_format
                    REQUIRES esp_driver_sdmmc nvs_flash esp_psram esp_netif esp_hw_support config_handling camera_ctrl
                                logfile_handling mainprocess_ctrl network_ctrl webserver_softap misc_helper openmetrics_ctrl)