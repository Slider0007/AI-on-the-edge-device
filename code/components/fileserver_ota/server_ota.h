#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <string>

#include <esp_http_server.h>


void checkOTAUpdate();
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void checkOTAPartitionState();
#endif

std::string unzipOTA(std::string _in_zip_file, std::string _root_folder = "/sdcard/");
void unzip(std::string _in_zip_file, std::string _target_directory);

void doReboot();
void doRebootOTA();
void forceReboot();

void registerOtaRebootUri(httpd_handle_t server);

#endif //SERVEROTA_H