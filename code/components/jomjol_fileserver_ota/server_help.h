#ifndef SERVERHELP_H
#define SERVERHELP_H

#include <string>

#include "esp_http_server.h"


esp_err_t send_file(httpd_req_t *req, std::string filename);
const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

void unzip(std::string _in_zip_file, std::string _target_directory);
std::string unzip_new(std::string _in_zip_file, std::string _target_zip, std::string _target_bin, std::string _main = "/sdcard/", bool _initial_setup = false);

#endif //SERVERHELP_H