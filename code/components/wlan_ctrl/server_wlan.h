#ifndef SERVER_WLAN_H
#define SERVER_WLAN_H

#include <esp_log.h>
#include <esp_http_server.h>


void registerWlanUri(httpd_handle_t server);

#endif // SERVER_WLAN_H