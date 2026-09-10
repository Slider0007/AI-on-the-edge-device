#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <esp_http_server.h>


void checkOtaUpdate();

void doReboot();
void doRebootOTA();

void registerOtaRebootUri(httpd_handle_t server);

#endif // SERVEROTA_H
