#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <string>

#include <esp_http_server.h>


void checkOTAUpdate();
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void checkOTAPartitionState();
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

std::string unzipOTA(std::string inputZipFile, std::string rootFolder = "/sdcard/");

void doReboot();
void doRebootOTA();
void forceReboot();

void registerOtaRebootUri(httpd_handle_t server);

#endif // SERVEROTA_H
