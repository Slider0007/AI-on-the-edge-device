#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include <esp_http_server.h>
#include <esp_vfs.h>
#include <string>

#include "../../include/defines.h"

struct HttpServerData {
    char basePathRoot[ESP_VFS_PATH_MAX + 1]; // Base path of main storage
    char basePathFileserver[ESP_VFS_PATH_MAX + 1]; // Base path of fileserver storage
    char scratch[WEBSERVER_SCRATCH_BUFSIZE]; // Scratch buffer for temporary storage
};
extern struct HttpServerData *httpServerData;

extern httpd_handle_t server;
extern std::string getFwVersion(void);

void allocateWebserverHelperMemory(void);
httpd_handle_t startWebserver(void);
void registerWebserverUri(httpd_handle_t server, const char *basePath);

#endif
