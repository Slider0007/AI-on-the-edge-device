#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <string>

#include <esp_http_server.h>


void checkOtaStaged();

#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void checkOtaPartitionState();
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

void doReboot();
void doRebootOTA();

void registerOtaRebootUri(httpd_handle_t server);

#endif // SERVEROTA_H
