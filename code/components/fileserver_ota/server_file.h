#ifndef SERVERFILE_H
#define SERVERFILE_H

#include <string>

#include <esp_http_server.h>


esp_err_t sendFile(httpd_req_t *req, const std::string &filename, bool disableCache = false);

void registerFileserverUri(httpd_handle_t server, const char *basePath);

#endif // SERVERFILE_H
