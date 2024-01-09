#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include <esp_http_server.h>

extern httpd_handle_t server;

httpd_handle_t start_webserver(void);
void register_server_main_uri(httpd_handle_t server, const char *base_path);

#endif
