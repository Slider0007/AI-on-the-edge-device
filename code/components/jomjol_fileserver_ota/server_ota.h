#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <string>

#include <esp_http_server.h>


void CheckOTAUpdate();
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void CheckOTAPartitionState();
#endif

std::string unzip_ota(std::string _in_zip_file, std::string _root_folder = "/sdcard/");
void unzip(std::string _in_zip_file, std::string _target_directory);

void doReboot();
void doRebootOTA();
void hard_restart();

void register_server_ota_sdcard_uri(httpd_handle_t server);

#endif //SERVEROTA_H