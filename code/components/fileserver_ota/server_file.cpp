#include "server_file.h"
#include "../../include/defines.h"

#include <stdio.h>
#include <cstring>
#include <unordered_set>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <cJSON.h>
#include <dirent.h>

#include <esp_partition.h>
#include <esp_core_dump.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <esp_spiffs.h>
#include <esp_http_server.h>
#include <cJSON.h>

#include "webserver.h"
#include "server_help.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "system.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif // ENABLE_MQTT


static const char *TAG = "SERVER_FILE";


// Files that should not trigger a webserver redirect
static const std::unordered_set<std::string> noRedirectFiles = {"/config/marker1.jpg",  "/config/marker2.jpg",  "/config/reference.jpg",
                                                                "/img_tmp/marker1.jpg", "/img_tmp/marker2.jpg", "/img_tmp/reference.jpg"};

// Files with sensitive content (e.g. passwords) or some content to hide completely
static const std::unordered_set<std::string> sensitiveContent = {"wlan.ini", "wlan_ini.bak", "config.ini", "config_ini.bak",
                                                                 "System Volume Information"};


// Extract parent of a given path
std::string getParentPath(const std::string &path)
{
    size_t slashPos = path.find_last_of('/');
    std::string pathPart = (slashPos != std::string::npos) ? path.substr(0, slashPos) : "";

    if (pathPart.empty()) {
        return "/fileserver/"; // Ensure trailing slash
    }
    else {
        return "/fileserver" + pathPart + "/";
    }
}


esp_err_t getDataFileList(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    DIR *dir = opendir(ROOT_FOLDER_DATA_LOG);
    if (!dir) {
        std::string msg = "Failed to open directory: " + std::string(ROOT_FOLDER_DATA_LOG);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getDataFileList: " + msg);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    cJSON *cJSONObject = cJSON_CreateObject();
    if (!cJSONObject) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return ESP_FAIL;
    }
    cJSON *files;
    if (!cJSON_AddItemToObject(cJSONObject, "files", files = cJSON_CreateArray())) {
        cJSON_Delete(cJSONObject);
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') { // ignore all files with starting dot (hidden files)
            continue;
        }

        if (getFileIsFiletype(std::string(entry->d_name), "csv")) {
            if (!cJSON_AddItemToArray(files, cJSON_CreateString(entry->d_name))) {
                retVal = ESP_FAIL;
                break;
            }
        }
    }
    closedir(dir);

    char *jsonChar = cJSON_Print(cJSONObject);
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal = httpd_resp_send(req, jsonChar, strlen(jsonChar));
        cJSON_free(jsonChar);
    }

    return retVal;
}


esp_err_t getTfliteFileList(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    DIR *dir = opendir(ROOT_FOLDER_CNN_MODELS);
    if (!dir) {
        std::string msg = "Failed to open directory: " + std::string(ROOT_FOLDER_CNN_MODELS);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getTfliteFileList: " + msg);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    cJSON *cJSONObject = cJSON_CreateObject();
    if (!cJSONObject) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return ESP_FAIL;
    }
    cJSON *files;
    if (!cJSON_AddItemToObject(cJSONObject, "files", files = cJSON_CreateArray())) {
        cJSON_Delete(cJSONObject);
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') { // ignore all files with starting dot (hidden files)
            continue;
        }

        if (getFileIsFiletype(std::string(entry->d_name), "tfl") || getFileIsFiletype(std::string(entry->d_name), "tflite")) {
            if (!cJSON_AddItemToArray(files, cJSON_CreateString(entry->d_name))) {
                retVal = ESP_FAIL;
                break;
            }
        }
    }
    closedir(dir);

    char *jsonChar = cJSON_Print(cJSONObject);
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal = httpd_resp_send(req, jsonChar, strlen(jsonChar));
        cJSON_free(jsonChar);
    }

    return retVal;
}


esp_err_t getCertFileList(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    DIR *dir = opendir(ROOT_FOLDER_CERTS);
    if (!dir) {
        std::string msg = "Failed to open directory: " + std::string(ROOT_FOLDER_CERTS);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCertFileList: " + msg);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    cJSON *cJSONObject = cJSON_CreateObject();
    if (!cJSONObject) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return ESP_FAIL;
    }
    cJSON *files;
    if (!cJSON_AddItemToObject(cJSONObject, "files", files = cJSON_CreateArray())) {
        cJSON_Delete(cJSONObject);
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') { // ignore all files with starting dot (hidden files)
            continue;
        }

        if (!cJSON_AddItemToArray(files, cJSON_CreateString(entry->d_name))) {
            retVal = ESP_FAIL;
        }
    }
    closedir(dir);

    char *jsonChar = cJSON_Print(cJSONObject);
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal = httpd_resp_send(req, jsonChar, strlen(jsonChar));
        cJSON_free(jsonChar);
    }

    return retVal;
}


esp_err_t IRAM_ATTR sendFile(httpd_req_t *req, std::string filename, bool disableCache)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, get404());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    // Files listed are never be cached (declared static to avoid reallocation)
    static const std::unordered_set<std::string> specialNoCacheFiles = {"/sdcard/html/index.html", "/sdcard/html/setup.html"};
    bool doNotCacheFile = specialNoCacheFiles.count(filename) > 0;

    // Default cache behaviour -> disable caching
    const char *cacheControl = "max-age=0";

    if (!disableCache && !doNotCacheFile) {
        // Files types listed shall be cached (Declared static to avoid reallocation)
        static const std::unordered_set<std::string> fileTypes = {".html", ".htm", ".css", ".js",  ".map", ".jpg",         ".jpeg",
                                                                  ".ico",  ".gif", ".svg", ".png", ".md",  ".webmanifest", ".txt"};

        // Check if file type is matching
        for (const auto &type : fileTypes) {
            if (endsWith(filename, type)) {
                cacheControl = "max-age=31536000"; // Cache for a long time (1 year)
                break;
            }
        }
    }

    httpd_resp_set_hdr(req, "Cache-Control", cacheControl); // Set cache control header
    setContentTypeFromFile(req, filename.c_str());          // Set content-type header based on file type (extention)

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, 1, WEBSERVER_SCRATCH_BUFSIZE, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytesRead) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            std::string msg = "Failed to send file: " + filename;
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }

    httpd_resp_sendstr_chunk(req, NULL);
    fclose(file);

    return ESP_OK;
}


static esp_err_t sendLogfile(httpd_req_t *req, bool sendFullFile)
{
    FILE *file = fopen(LogFile.getCurrentFileName().c_str(), "r");
    if (!file) {
        httpd_resp_send(req, "No recent log entries", HTTPD_RESP_USE_STRLEN); // Respond with a positive feedback, no logs from today
        return ESP_OK;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    if (!sendFullFile) { // Send only last part of file
        ESP_LOGD(TAG, "Sending last %d bytes of the actual logfile", LOGFILE_LAST_PART_BYTES);
        long pos = 0;

        // Adapted from https://www.geeksforgeeks.org/implement-your-own-tail-read-last-n-lines-of-a-huge-file/
        if (fseek(file, 0, SEEK_END)) {
            fclose(file);
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendLogfile: Failed to get to end of file");
            return ESP_FAIL;
        }
        else {
            pos = ftell(file); // Number of bytes in the file
            ESP_LOGD(TAG, "File contains %ld bytes", pos);

            // Calc start position -> either beginning of LAST PART (EOF - LAST_PART_BYTES) or beginning of file (pos = 0)
            pos = pos - std::min((long)LOGFILE_LAST_PART_BYTES, pos);

            if (fseek(file, pos, SEEK_SET)) { // Go to start position
                fclose(file);
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "sendLogfile: Failed to go back " + std::to_string(std::min((long)LOGFILE_LAST_PART_BYTES, pos)) +
                                        " bytes within the file");
                return ESP_FAIL;
            }
        }

        // Find end of line
        while (pos > 0) { // Only search end of line if pos is pointing to "beginning of LAST PART"
                          // (skip if start is from beginning of file to ensure first line is included)
            if (fgetc(file) == '\n') {
                break;
            }
        }
    }

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, 1, WEBSERVER_SCRATCH_BUFSIZE, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytesRead) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            std::string msg = "Failed to send log file: " + LogFile.getCurrentFileName();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);

    return ESP_OK;
}


static esp_err_t handler_logfiles(httpd_req_t *req)
{
    const char *APIName = "log:v2"; // API name and version
    char query[100];
    char valuechar[30];
    std::string type;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "type", valuechar, sizeof(valuechar)) == ESP_OK) {
            type = std::string(valuechar);
        }
    }

    if (type.empty()) {
        return sendLogfile(req, false);
    }
    else if (type.compare("full") == 0) {
        return sendLogfile(req, true);
    }
    else if (type.compare("api_name") == 0) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E91: Parameter not found");
        return ESP_FAIL;
    }
}


static esp_err_t sendDatafile(httpd_req_t *req, bool sendFullFile)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    FILE *file = fopen(LogFile.getCurrentFileNameData().c_str(), "r");
    if (!file) {
        httpd_resp_send(req, "No recent data entries", HTTPD_RESP_USE_STRLEN); // Respond with a positive feedback, no data from today
        return ESP_OK;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    if (!sendFullFile) { // Send only last part of file
        ESP_LOGD(TAG, "Sending last %d bytes of the actual datafile", LOGFILE_LAST_PART_BYTES);
        long pos = 0;

        // Adapted from https://www.geeksforgeeks.org/implement-your-own-tail-read-last-n-lines-of-a-huge-file/
        if (fseek(file, 0, SEEK_END)) {
            fclose(file);
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendDatafile: Failed to get to end of file");
            return ESP_FAIL;
        }
        else {
            pos = ftell(file); // Number of bytes in the file
            ESP_LOGD(TAG, "File contains %ld bytes", pos);

            // Calc start position -> either beginning of LAST PART (EOF - LAST_PART_BYTES) or beginning of file (pos = 0)
            pos = pos - std::min((long)LOGFILE_LAST_PART_BYTES, pos);

            if (fseek(file, pos, SEEK_SET)) { // Go to start position
                fclose(file);
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "sendDatafile: Failed to go back " + std::to_string(std::min((long)LOGFILE_LAST_PART_BYTES, pos)) +
                                        " bytes within the file");
                return ESP_FAIL;
            }
        }

        // Find end of line
        while (pos > 0) { // Only search end of line if pos is pointing to "beginning of LAST PART"
                          // (skip if start is from beginning of file to ensure first line is included)
            if (fgetc(file) == '\n') {
                break;
            }
        }
    }

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, 1, WEBSERVER_SCRATCH_BUFSIZE, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytesRead) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            std::string msg = "Failed to send data file: " + LogFile.getCurrentFileNameData();
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);

    return ESP_OK;
}


static esp_err_t handler_datafiles(httpd_req_t *req)
{
    const char *APIName = "data:v2"; // API name and version
    char query[100];
    char valuechar[30];
    std::string type;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "type", valuechar, sizeof(valuechar)) == ESP_OK) {
            type = std::string(valuechar);
        }
    }

    if (type.empty()) {
        return sendDatafile(req, false);
    }
    else if (type.compare("full") == 0) {
        return sendDatafile(req, true);
    }
    else if (type.compare("api_name") == 0) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E91: Parameter not found");
        return ESP_FAIL;
    }
}


static esp_err_t getDirectory(httpd_req_t *req, const char *dirpath, const char *uripath, bool readonly, bool showAllFiles)
{
    char entryPath[FILE_PATH_MAX];
    char entrySize[16];
    const char *entryType;

    struct dirent *entry;
    struct stat entryStat;

    char dirpathCorrected[FILE_PATH_MAX];
    strlcpy(dirpathCorrected, dirpath, sizeof(dirpathCorrected));

    // Strip trailing slash if necessary
    if ((strlen(dirpathCorrected) - 1) > strlen(((HttpServerData *)req->user_ctx)->basePathFileserver)) {
        dirpathCorrected[strlen(dirpathCorrected) - 1] = '\0';
    }

    DIR *dir = opendir(dirpathCorrected);
    if (!dir) {
        std::string msg = "Failed to open directory: " + std::string(dirpath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

    // Send static HTML file
    FILE *file = fopen(HTML_FILE_FILESERVER_STATIC, "rb");
    if (!file) {
        std::string msg = "Failed to read file: " + std::string(HTML_FILE_FILESERVER_STATIC);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    size_t bufSize = WEBSERVER_SCRATCH_BUFSIZE;
    size_t bufSizeUsed = 0;

    while ((bufSizeUsed = fread(buffer, 1, WEBSERVER_SCRATCH_BUFSIZE, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bufSizeUsed) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            std::string msg = "Failed to send file: " + std::string(HTML_FILE_FILESERVER_STATIC);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }

    fclose(file);
    bufSizeUsed = 0;

    // Build table content
    auto sendChunk = [&]() {
        if (bufSizeUsed > 0) {
            httpd_resp_send_chunk(req, buffer, bufSizeUsed);
            bufSizeUsed = 0;
            buffer[0] = '\0';
        }
    };

    // Table header
    bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed,
                            "<table id=\"files_table\" class=\"sortable\">"
                            "<col style=\"width:800px\"><col style=\"width:300px\"><col style=\"width:300px\"><col style=\"width:100px\">"
                            "<thead><tr><th>Name</th><th>Type</th><th>Size</th>");

    if (!readonly) {
        bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed, "<th>Action</th>");
    }

    bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed, "</tr></thead><tbody>\n");

    sendChunk();

    const size_t dirPathLen = strlen(dirpath);
    strlcpy(entryPath, dirpath, sizeof(entryPath));

    int fileCount = 0;
    const int maxFiles = 100;
    bool truncated = false;

    while ((entry = readdir(dir)) != NULL) {
        // Stopp after max files
        if (!showAllFiles && fileCount >= maxFiles) {
            truncated = true;
            sendChunk();
            break;
        }

        // Skip sensitive files
        if (sensitiveContent.count(entry->d_name) > 0) {
            continue;
        }

        // Parse entry type
        entryType = (entry->d_type == DT_DIR ? "directory" : "file");

        // Check availability
        strlcpy(entryPath + dirPathLen, entry->d_name, sizeof(entryPath) - dirPathLen);

        if (stat(entryPath, &entryStat) == -1) {
            continue;
        }

        // Format size output
        if (entry->d_type == DT_DIR) {
            strcpy(entrySize, "-");
        }
        else {
            if (entryStat.st_size >= 1024) {
                sprintf(entrySize, "%ld KB", entryStat.st_size / 1024);
            }
            else {
                sprintf(entrySize, "%ld B", entryStat.st_size);
            }
        }

        // Write table row
        bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed,
                                "<tr><td><a href=\"/fileserver%s%s%s\">%s</a></td>"
                                "<td>%s</td>"
                                "<td>%s</td>",
                                uripath, entry->d_name, (entry->d_type == DT_DIR ? "/?full=false" : ""), entry->d_name, entryType,
                                entrySize);

        // Handle action column
        if (!readonly) {
            static const std::unordered_set<std::string> protectedRootFolder = {"/sdcard/", "/sdcard/config", "/sdcard/log"};

            // Handle protected directories
            if (protectedRootFolder.count(dirpathCorrected) > 0 && entry->d_type == DT_DIR) {
                bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed, "<td>-</td>");
            }
            else {
                bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed,
                                        "<td><button onclick=\"deleteEntry('%s','%s%s%s')\">Delete</button></td>", entry->d_name, uripath,
                                        entry->d_name, (entry->d_type == DT_DIR) ? "?task=deldir" : "");
            }
        }

        bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed, "</tr>\n");

        if (bufSize - bufSizeUsed < 768) {
            sendChunk();
        }

        fileCount++;
    }

    closedir(dir);

    // Finalize table
    bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed, "</tbody></table>");

    // If truncated, show message and button to load all item of directory
    if (truncated) {
        bufSizeUsed += snprintf(buffer + bufSizeUsed, bufSize - bufSizeUsed,
                                "<div style=\"margin-top: 20px; color: red;\">Directory listing is truncated to %d items.<br><form "
                                "method=\"get\" action=\"/fileserver%s\"><input type=\"hidden\" name=\"full\" value=\"true\"><button "
                                "class=\"button\" style=\"padding:4px 10px;width:250px\" type=\"submit\">Load all items (SLOW!)</button>"
                                "</form></div>",
                                (int)maxFiles, uripath);
    }

    sendChunk();

    // Final empty chunk signals response complete
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}


// Handler to download a file from server (sd card)
static esp_err_t download_get_handler(httpd_req_t *req)
{
    char filePath[FILE_PATH_MAX];
    struct stat fileStat;

    const char *filename = getPathFromUri(filePath, ((HttpServerData *)req->user_ctx)->basePathFileserver,
                                          req->uri + sizeof("/fileserver") - 1, sizeof(filePath));
    if (!filename) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: Path malformed or too long");
        return ESP_FAIL;
    }

    // List directory HTML formatted (If URI has trailing slash '/')
    if (filename[strlen(filename) - 1] == '/') {
        bool readonly = false;
        bool showAllFiles = true;
        char query[64];

        if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
            char param[16];
            if (httpd_query_key_value(query, "readonly", param, sizeof(param)) == ESP_OK) {
                readonly = (strcmp(param, "true") == 0);
            }
            if (httpd_query_key_value(query, "full", param, sizeof(param)) == ESP_OK) {
                showAllFiles = (strcmp(param, "true") == 0);
            }
        }

        return getDirectory(req, filePath, filename, readonly, showAllFiles);
    }

    // Check: File not available
    if (stat(filePath, &fileStat) == -1) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid path: File not found");
        return ESP_FAIL;
    }

    //  Deny access for special files with sensitive data
    if (sensitiveContent.count(filePath) > 0) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Access denied (Sensitive data)");
        return ESP_FAIL;
    }

    FILE *file = fopen(filePath, "rb");
    if (!file) {
        std::string msg = "Failed to read file :" + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg.c_str());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    setContentTypeFromFile(req, filename);

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, 1, WEBSERVER_SCRATCH_BUFSIZE, file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytesRead) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            std::string msg = "Failed to send file :" + std::string(filePath);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }

    httpd_resp_sendstr_chunk(req, NULL);
    fclose(file);

    return ESP_OK;
}


// Handler to upload a file to server (sd card)
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    char filePath[FILE_PATH_MAX];
    struct stat fileStat;

    const char *filename = getPathFromUri(filePath, ((HttpServerData *)req->user_ctx)->basePathFileserver, req->uri + sizeof("/upload") - 1,
                                          sizeof(filePath));

    if (!filename) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: Path malformed or too long");
        return ESP_FAIL;
    }

    // Filename cannot have a trailing '/'
    if (filename[strlen(filename) - 1] == '/') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: Trailing slash");
        return ESP_FAIL;
    }

    // File cannot be larger than a limit
    if (req->content_len > MAX_FILE_SIZE) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File size must be less than " MAX_FILE_SIZE_STR);
        return ESP_FAIL;
    }

    if (stat(filePath, &fileStat) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    FILE *file = fopen(filePath, "wb");
    if (!file) {
        std::string msg = "Failed to create file: " + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(file, NULL, _IOFBF, 512);

    ESP_LOGI(TAG, "Receiving file: %s", filename);

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
    int remaining = req->content_len;                          // Content length of the request gives the size of the file being uploaded
    int received = 0;

    while (remaining > 0) {
        ESP_LOGI(TAG, "Remaining size: %d", remaining);
        // Receive the file part by part into a buffer
        if ((received = httpd_req_recv(req, buffer, MIN(remaining, WEBSERVER_SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; // Retry if timeout occurred
            }

            // In case of unrecoverable error, close and delete the unfinished file
            fclose(file);
            unlink(filePath);

            std::string msg = "Failed to receive file: " + std::string(filePath);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }

        // Write buffer content to file on storage
        if (received && (received != fwrite(buffer, 1, received, file))) {
            fclose(file);
            unlink(filePath);
            std::string msg = "Failed to write file: " + std::string(filePath);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }

        // Keep track of remaining size of the file left to be uploaded
        remaining -= received;
    }

    // Close file upon upload completion
    fclose(file);
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "File saved: " + std::string(filename));
    ESP_LOGI(TAG, "File reception completed");

    // Redirect to parent folder (exception: special files)
    if (noRedirectFiles.count(filename) > 0) {
        httpd_resp_set_status(req, HTTPD_200); // Response without redirection request -> Avoid reloading of folder content
        std::string msg = "File saved successfully: " + std::string(filename);
        httpd_resp_sendstr(req, msg.c_str());
        return ESP_OK;
    }
    else {
        std::string redirectPath = getParentPath(filename) + "?full=false";
        httpd_resp_set_hdr(req, "Location", redirectPath.c_str()); // Redirect to parent directory
        httpd_resp_set_status(req, HTTPD_303_SEE_OTHER);
        httpd_resp_sendstr(req, "");
        return ESP_OK;
    }
}


// Handler to delete a file or folder content from server (sd card)
static esp_err_t delete_post_handler(httpd_req_t *req)
{
    char filePath[FILE_PATH_MAX];
    struct stat fileStat;
    char query[200];
    char valuechar[30];
    std::string task;

    // Get filename from URI
    const char *filename = getPathFromUri(filePath, ((HttpServerData *)req->user_ctx)->basePathFileserver, req->uri + sizeof("/delete") - 1,
                                          sizeof(filePath));

    if (!filename) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: Malformed or too long");
        return ESP_FAIL;
    }

    std::string filenameStr(filename);

    // Parse query string
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "task", valuechar, sizeof(valuechar)) == ESP_OK) {
            task = std::string(valuechar);
        }
    }

    // Handle directory deletion
    if (task.compare("deldir") == 0) {
        // Remove trailing slash
        if (filenameStr.back() == '/') {
            filenameStr.pop_back();
        }

        if (deleteAllFilesInDirectory(filePath, true, true) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to delete files in directory");
            return ESP_FAIL;
        }

        std::string redirectPath = getParentPath(filenameStr) + "?full=false";
        httpd_resp_set_hdr(req, "Location", redirectPath.c_str()); // Redirect to parent directory
        httpd_resp_set_status(req, HTTPD_303_SEE_OTHER);
        httpd_resp_sendstr(req, "");
        return ESP_OK;
    }

    // Handle single file deletion
    if (filenameStr.back() == '/') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: Trailing slash");
        return ESP_FAIL;
    }

    // Check for file presence
    if (stat(filePath, &fileStat) == -1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path: File not found");
        return ESP_FAIL;
    }

    // Delete file
    if (unlink(filePath) != 0) {
        std::string msg = "Failed to delete file: " + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
        return ESP_FAIL;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "File deleted: " + filenameStr);

    if (noRedirectFiles.count(filenameStr) > 0) {
        httpd_resp_set_status(req, HTTPD_200);
        std::string msg = "File deleted successfully: " + filenameStr;
        httpd_resp_sendstr(req, msg.c_str());
        return ESP_OK;
    }
    else {
        std::string redirectPath = getParentPath(filenameStr) + "?full=false";
        httpd_resp_set_hdr(req, "Location", redirectPath.c_str()); // Redirect to parent directory
        httpd_resp_set_status(req, HTTPD_303_SEE_OTHER);
        httpd_resp_sendstr(req, "");
        return ESP_OK;
    }
}


static std::string printCoreDumpBacktraceInfo(const esp_core_dump_summary_t *summary)
{
    if (!summary) {
        return "No core dump available";
    }

    char results[256]; // Assuming a maximum of 256 characters for the backtrace string
    int offset = 0;

    for (int i = 0; i < summary->exc_bt_info.depth; i++) {
        uintptr_t pc = summary->exc_bt_info.bt[i]; // Program Counter (PC)
        int len = snprintf(results + offset, sizeof(results) - offset, " 0x%08X", pc);
        if (len >= 0 && offset + len < sizeof(results)) {
            offset += len;
        }
        else {
            break; // Reached the limit of the results buffer
        }
    }

    return std::string("Backtrace: " + std::string(results) + "\nDepth: " + std::to_string((int)summary->exc_bt_info.depth) +
                       "\nCorrupted: " + std::to_string(summary->exc_bt_info.corrupted) + "\nPC: " + std::to_string((int)summary->exc_pc) +
                       "\nFirmware version: " + getFwVersion());
}


static esp_err_t coredump_handler(httpd_req_t *req)
{
    const char *APIName = "coredump:v1"; // API name and version
    char query[200];
    char valuechar[30];
    std::string task;

    httpd_resp_set_type(req, "text/plain");

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "task", valuechar, sizeof(valuechar)) == ESP_OK) {
            task = std::string(valuechar);
        }
    }

    // Check if coredump partition is available
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");
    if (!partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Partition 'coredump' not found");
        return ESP_FAIL;
    }

    // Get core dump summary to check if core dump is available
    esp_core_dump_summary_t summary;
    esp_err_t coreDumpGetSummaryRetVal = esp_core_dump_get_summary(&summary);

    // Save core dump file
    // Debug with esp-coredump (https://github.com/espressif/esp-coredump) --> e.g. install with "pip install esp-coredump"
    // Generic: esp-coredump info_corefile --gdb <path_to_gdb_bin> --rom-elf <soc_specific_rom_elf_file> --core-format raw
    //  --core <downloaded coredump file (FIRMWARE_BOARDTYPE-coredump-elf.bin)> firmware.elf (firmware debug zip --> firmware.elf)
    // Example: esp-coredump info_corefile --gdb <tool-xtensa-esp-elf-gdb/bin/xtensa-esp32-elf-gdb.exe>
    //  --rom-elf esp32_rev0_rom.elf --core-format raw --core firmware_ESP32CAM_coredump-elf.bin firmware.elf
    if (task.compare("save") == 0) {
        if (coreDumpGetSummaryRetVal != ESP_OK) { // Skip save request if no core dump is available (empty partition)
            httpd_resp_sendstr(req, "Skip request, no core dump available");
            return ESP_OK;
        }

        // Get firmware and cleanup name to have proper filename
        std::string firmware = getFwVersion();
        replaceAll(firmware, ":", "_");
        replaceAll(firmware, " ", "_");
        replaceAll(firmware, "(", "_");
        replaceAll(firmware, ")", "_");

        std::string attachmentFile = "attachment;filename=" + firmware + "_" + getBoardType() + "_coredump-elf.bin";
        httpd_resp_set_type(req, "application/octet-stream");
        httpd_resp_set_hdr(req, "Content-Disposition", attachmentFile.c_str());

        char *buffer = ((HttpServerData *)req->user_ctx)->scratch; // Retrieve the pointer to scratch buffer for temporary storage
        int i = 0;

        for (i = 0; i < (partition->size / WEBSERVER_SCRATCH_BUFSIZE); i++) {
            esp_partition_read(partition, i * WEBSERVER_SCRATCH_BUFSIZE, buffer, WEBSERVER_SCRATCH_BUFSIZE);
            httpd_resp_send_chunk(req, buffer, WEBSERVER_SCRATCH_BUFSIZE);
        }

        int pendingSize = partition->size - (i * WEBSERVER_SCRATCH_BUFSIZE);
        if (pendingSize > 0) {
            ESP_ERROR_CHECK(esp_partition_read(partition, i * WEBSERVER_SCRATCH_BUFSIZE, buffer, pendingSize));
            httpd_resp_send_chunk(req, buffer, pendingSize);
        }
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }
    else if (task.compare("clear") == 0) { // Format partition 'coredump'
        esp_err_t err = esp_partition_erase_range(partition, 0, partition->size);
        if (err == ESP_OK) {
            httpd_resp_sendstr(req, "Partition 'coredump' cleared");
            return ESP_OK;
        }
        else {
            std::string msg = "Failed to format partition 'coredump'. Error: " + intToHexString(err);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
            return ESP_FAIL;
        }
    }
    else if (task.compare("force_exception") == 0) { // Only for testing purpose, and ESP exception crash can be forced
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "coredump_handler: Software exception triggered manually");
        httpd_resp_send_chunk(req, NULL, 0);
        assert(0);
        return ESP_OK;
    }
    else if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }

    // Default action: Print backtrace summary
    if (coreDumpGetSummaryRetVal == ESP_OK) {
        httpd_resp_sendstr(req, printCoreDumpBacktraceInfo(&summary).c_str());
    }
    else {
        httpd_resp_sendstr(req, "No core dump available");
    }

    return ESP_OK;
}


void registerFileserverUri(httpd_handle_t server, const char *basePath)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    // Validate file storage base path
    if (!basePath) {
        // if (!basePath || strcmp(basePath, "/spiffs") != 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "basePath for fileserver root folder not set");
        // return ESP_ERR_INVALID_ARG;
    }

    strlcpy(httpServerData->basePathFileserver, basePath, sizeof(httpServerData->basePathFileserver));

    httpd_uri_t file_download = {
        .uri = "/fileserver*", // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = HTTP_AUTH_BASIC(download_get_handler),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_download);

    httpd_uri_t file_upload = {
        .uri = "/upload/*", // Match all URIs of type /upload/path/to/file
        .method = HTTP_POST,
        .handler = HTTP_AUTH_BASIC(upload_post_handler),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_upload);

    httpd_uri_t file_delete = {
        .uri = "/delete/*", // Match all URIs of type /delete/path/to/file
        .method = HTTP_POST,
        .handler = HTTP_AUTH_BASIC(delete_post_handler),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_delete);

    httpd_uri_t coredump = {
        .uri = "/coredump",
        .method = HTTP_GET,
        .handler = HTTP_AUTH_BASIC(coredump_handler),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &coredump);

    httpd_uri_t handler_logfile = {
        .uri = "/log",
        .method = HTTP_GET,
        .handler = HTTP_AUTH_BASIC(handler_logfiles),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &handler_logfile);

    httpd_uri_t handler_datafile = {
        .uri = "/data",
        .method = HTTP_GET,
        .handler = HTTP_AUTH_BASIC(handler_datafiles),
        .user_ctx = httpServerData // Pass server data as context
    };
    httpd_register_uri_handler(server, &handler_datafile);
}
