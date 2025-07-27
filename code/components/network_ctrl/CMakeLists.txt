FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "." "../esp-protocols/components/mdns/include"
                    REQUIRES esp_driver_uart esp_driver_usb_serial_jtag esp_wifi esp_eth nvs_flash wpa_supplicant mdns misc_helper mqtt_ctrl)


