#ifndef OPENMETRICS_H
#define OPENMETRICS_H

#include <esp_http_server.h>


void register_openmetrics_uri(httpd_handle_t server);

#endif // OPENMETRICS_H
