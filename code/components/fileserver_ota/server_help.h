#ifndef SERVERHELP_H
#define SERVERHELP_H

#include <string>

#include "esp_http_server.h"

bool endsWith(std::string const &str, std::string const &suffix);
const char* getPathFromUri(char *dest, const char *basePath, const char *uri, size_t destsize);
esp_err_t setContentTypeFromFile(httpd_req_t *req, const char *filename);

#endif //SERVERHELP_H