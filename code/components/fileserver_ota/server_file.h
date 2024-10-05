#ifndef SERVERFILE_H
#define SERVERFILE_H

#include <string>

#include <esp_http_server.h>


esp_err_t getDataFileList(httpd_req_t *req);
esp_err_t getTfliteFileList(httpd_req_t *req);
esp_err_t getCertFileList(httpd_req_t *req);
esp_err_t sendFile(httpd_req_t *req, std::string filename);

void registerFileserverUri(httpd_handle_t server, const char *basePath);

#endif //SERVERFILE_H