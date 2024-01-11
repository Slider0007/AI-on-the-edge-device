#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <esp_http_server.h>


void CheckUpdate();
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void CheckOTAUpdate();
#endif

void doReboot();
void doRebootOTA();
void hard_restart();

void register_server_ota_sdcard_uri(httpd_handle_t server);

#endif //SERVEROTA_H