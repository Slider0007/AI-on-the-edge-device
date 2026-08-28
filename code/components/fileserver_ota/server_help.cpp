#include "server_help.h"
#include "../../include/defines.h"

#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_http_server.h"
#include "esp_err.h"
#include <esp_log.h>


// static const char *TAG = "SERVER_HELP"; // Unsed

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
