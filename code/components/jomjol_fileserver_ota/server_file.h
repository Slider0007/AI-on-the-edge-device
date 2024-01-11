#ifndef SERVERFILE_H
#define SERVERFILE_H

#include <string>

#include <esp_http_server.h>


esp_err_t get_numbers_file_handler(httpd_req_t *req);
esp_err_t get_data_file_handler(httpd_req_t *req);
esp_err_t get_tflite_file_handler(httpd_req_t *req);

void register_server_file_uri(httpd_handle_t server, const char *base_path);

#endif //SERVERFILE_H