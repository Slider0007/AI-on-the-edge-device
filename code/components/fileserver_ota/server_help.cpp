#include "server_help.h"
#include "../../include/defines.h"

#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <esp_http_server.h>
#include <esp_err.h>
#include <esp_log.h>

#include "webserver.h"


static const char *TAG = "SERVER_HELP";

// Check file type (file extention, case-insensitive)
bool endsWith(std::string const &str, std::string const &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }

    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(), [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}


// Copies the full path into destination buffer and returns pointer to path (skipping the preceding base path)
const char *getPathFromUri(char *dest, const char *basePath, const char *uri, size_t destsize)
{
    const size_t basePathLength = strlen(basePath);
    size_t pathLength = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathLength = MIN(pathLength, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathLength = MIN(pathLength, hash - uri);
    }

    if (basePathLength + pathLength + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, basePath);
    strlcpy(dest + basePathLength, uri, pathLength + 1);

    /* Return pointer to path, skipping the base */
    return dest + basePathLength;
}


// Set HTTP response content type according to file extension
esp_err_t setContentTypeFromFile(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".jpg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}


esp_err_t receiveRequestBodyToFile(httpd_req_t *req, const char *filePath)
{
    FILE *file = fopen(filePath, "wb");
    if (!file) {
        std::string msg = "Failed to create file: " + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    setvbuf(file, NULL, _IOFBF, 512);

    ESP_LOGI(TAG, "Receiving file: %s", filePath);

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch;
    int remaining = req->content_len;
    int received = 0;

    while (remaining > 0) {
        ESP_LOGI(TAG, "Remaining size: %d", remaining);
        if ((received = httpd_req_recv(req, buffer, MIN(remaining, WEBSERVER_SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            fclose(file);
            unlink(filePath);
            std::string msg = "Failed to receive file: " + std::string(filePath);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }

        if (received && (received != fwrite(buffer, 1, received, file))) {
            fclose(file);
            unlink(filePath);
            std::string msg = "Failed to write file: " + std::string(filePath);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }

        remaining -= received;
    }

    fclose(file);
    ESP_LOGI(TAG, "File reception completed");
    return ESP_OK;
}
