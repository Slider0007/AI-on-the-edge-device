#ifndef HTTP_AUTH_H
#define HTTP_AUTH_H

#include <esp_http_server.h>

esp_err_t handleHttpAuthBasic(httpd_req_t *req, esp_err_t httpHandler(httpd_req_t *));

#define HTTP_AUTH_BASIC(httpHandlerFunc) [](httpd_req_t *req) { return handleHttpAuthBasic(req, httpHandlerFunc); }

#endif // HTTP_AUTH_H
