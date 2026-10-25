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


// Copies the full path into destination buffer and returns pointer to path (skipping the preceding base path)
const char *getPathFromUri(char *dest, const char *basePath, const char *uri, size_t destsize)
{
    if (!dest || !basePath || !uri || destsize == 0) {
        return NULL;
    }

    const size_t basePathLength = strlen(basePath);
    size_t pathLength = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathLength = MIN(pathLength, static_cast<size_t>(quest - uri));
    }

    const char *hash = strchr(uri, '#');
    if (hash) {
        pathLength = MIN(pathLength, static_cast<size_t>(hash - uri));
    }

    if (basePathLength + pathLength + 1 > destsize) {
        return NULL;
    }

    strcpy(dest, basePath);

    char *out = dest + basePathLength;
    const char *in = uri;
    const char *end = uri + pathLength;

    while (in < end) {
        if (*in == '%') {
            if (in + 2 >= end) {
                return NULL;
            }

            const char h1 = in[1];
            const char h2 = in[2];

            auto hexValue = [](char c) -> int {
                if (c >= '0' && c <= '9') {
                    return c - '0';
                }
                if (c >= 'a' && c <= 'f') {
                    return c - 'a' + 10;
                }
                if (c >= 'A' && c <= 'F') {
                    return c - 'A' + 10;
                }
                return -1;
            };

            const int high = hexValue(h1);
            const int low = hexValue(h2);

            if (high < 0 || low < 0) {
                return NULL;
            }

            const char decoded = static_cast<char>((high << 4) | low);

            // Reject URL decoding path separators
            if (decoded == '/' || decoded == '\\' || static_cast<unsigned char>(decoded) < 0x20 || decoded == 0x7F) {
                return NULL;
            }

            *out++ = decoded;
            in += 3;
            continue;
        }

        *out++ = *in++;
    }

    *out = '\0';

    const char *rel = dest + basePathLength;
    while (*rel != '\0') {
        while (*rel == '/') {
            ++rel;
        }

        const char *end = strchr(rel, '/');
        const size_t segmentLength = end ? static_cast<size_t>(end - rel) : strlen(rel);
        if (segmentLength == 2 && rel[0] == '.' && rel[1] == '.') {
            return NULL;
        }

        if (!end) {
            break;
        }
        rel = end + 1;
    }


    return dest + basePathLength;
}


// Check file type (file extention, case-insensitive)
bool endsWith(const char *filename, const char *suffix)
{
    const size_t filenameLen = strlen(filename);
    const size_t suffixLen = strlen(suffix);

    return filenameLen >= suffixLen && strcasecmp(filename + filenameLen - suffixLen, suffix) == 0;
}


bool endsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }

    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}


// Set HTTP response content type according to file extension
esp_err_t setContentTypeFromFile(httpd_req_t *req, const char *filename)
{
    if (endsWith(filename, ".html") || endsWith(filename, ".htm")) {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (endsWith(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    else if (endsWith(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    }
    else if (endsWith(filename, ".json")) {
        return httpd_resp_set_type(req, "application/json");
    }
    else if (endsWith(filename, ".png")) {
        return httpd_resp_set_type(req, "image/png");
    }
    else if (endsWith(filename, ".jpg") || endsWith(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (endsWith(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    else if (endsWith(filename, ".csv")) {
        return httpd_resp_set_type(req, "text/csv");
    }
    else if (endsWith(filename, ".txt") || endsWith(filename, ".log") || endsWith(filename, ".md") || endsWith(filename, ".ini")) {
        return httpd_resp_set_type(req, "text/plain");
    }
    else if (endsWith(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (endsWith(filename, ".zip")) {
        return httpd_resp_set_type(req, "application/zip");
    }
    else if (endsWith(filename, ".gz")) {
        return httpd_resp_set_type(req, "application/gzip");
    }
    else if (endsWith(filename, ".xml")) {
        return httpd_resp_set_type(req, "text/xml");
    }
    else if (endsWith(filename, ".gif")) {
        return httpd_resp_set_type(req, "image/gif");
    }

    return httpd_resp_set_type(req, "application/octet-stream");
}


esp_err_t receiveRequestBodyToFile(httpd_req_t *req, const char *filePath)
{
    ESP_LOGI(TAG, "File upload started: %s", filePath);

    FILE *file = fopen(filePath, "wb");
    if (!file) {
        std::string msg = "Failed to create file: " + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
        return ESP_FAIL;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    setvbuf(file, NULL, _IOFBF, 512);

    char *buffer = ((HttpServerData *)req->user_ctx)->scratch;
    int received = 0;
    int remaining = req->content_len;
    int totalSize = remaining;
    int lastLoggedPercent = -1;

    constexpr uint8_t MAX_TIMEOUT_RETRIES = 5;
    uint8_t timeoutRetries = 0;

    while (remaining > 0) {
        int percent = (int)(((totalSize - remaining) * 100ULL) / totalSize);
        if (percent / 10 != lastLoggedPercent / 10) { // Logs every 10%
            ESP_LOGI(TAG, "Upload Progress: %d%% (%d bytes remaining)", percent, remaining);
            lastLoggedPercent = percent;
        }

        if ((received = httpd_req_recv(req, buffer, MIN(remaining, WEBSERVER_SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                if (++timeoutRetries <= MAX_TIMEOUT_RETRIES) {
                    continue;
                }
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

    if (fclose(file) != 0) {
        unlink(filePath);
        std::string msg = "Failed to finalize file: " + std::string(filePath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg.c_str());
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File upload completed: 100%%");
    return ESP_OK;
}


bool isValidIsoTimestamp(const char *isoStr)
{
    if (!isoStr) {
        return false;
    }

    // Strictly accept 16 chars (YYYY-MM-DDTHH:MM) or 19 chars (YYYY-MM-DDTHH:MM:SS)
    size_t len = strlen(isoStr);
    if (len != 16 && len != 19) {
        return false;
    }

    // Delimiters check
    if (isoStr[4] != '-' || isoStr[7] != '-') {
        return false;
    }
    if (isoStr[10] != 'T' && isoStr[10] != ' ') {
        return false;
    }
    if (isoStr[13] != ':') {
        return false;
    }
    if (len == 19 && isoStr[16] != ':') {
        return false;
    }

    // Digits check for all character positions
    for (int i = 0; i < static_cast<int>(len); i++) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || (len == 19 && i == 16)) {
            continue;
        }
        if (!isdigit(static_cast<unsigned char>(isoStr[i]))) {
            return false;
        }
    }

    // Parse components
    int year = (isoStr[0] - '0') * 1000 + (isoStr[1] - '0') * 100 + (isoStr[2] - '0') * 10 + (isoStr[3] - '0');
    int month = (isoStr[5] - '0') * 10 + (isoStr[6] - '0');
    int day = (isoStr[8] - '0') * 10 + (isoStr[9] - '0');
    int hour = (isoStr[11] - '0') * 10 + (isoStr[12] - '0');
    int minute = (isoStr[14] - '0') * 10 + (isoStr[15] - '0');
    int second = 0;
    if (len == 19) {
        second = (isoStr[17] - '0') * 10 + (isoStr[18] - '0');
    }

    // Range checks: Time (Hours: 0-23, Minutes: 0-59, Seconds: 0-59)
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }

    // Range checks: Date (Year: 1-9999, Month: 1-12)
    if (year < 1 || year > 9999 || month < 1 || month > 12) {
        return false;
    }

    // Days in month validation (including leap year check for February)
    int daysInMonth = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        daysInMonth = 30;
    }
    else if (month == 2) {
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        daysInMonth = isLeapYear ? 29 : 28;
    }

    if (day < 1 || day > daysInMonth) {
        return false;
    }

    return true;
}


long getOffsetForTime(FILE *filePtr, const char *targetIso, long fileSize, bool includeTimeAtBoundery)
{
    // Validate input: ensure target string exists and is long enough for a date
    if (!targetIso || strlen(targetIso) < 10) {
        return -1;
    }

    long low = 0;
    long high = fileSize;
    long resultOffset = 0;
    char lineBuffer[256];

    while (low <= high) {
        // Find the middle of the current search range
        long mid = low + (high - low) / 2;
        fseek(filePtr, mid, SEEK_SET);

        // If we landed in the middle of a line, skip to the start of the NEXT line
        if (mid > 0) {
            while (fgetc(filePtr) != '\n' && !feof(filePtr))
                ;
        }

        // Read the first full line available at this position
        if (fgets(lineBuffer, sizeof(lineBuffer), filePtr)) {
            // Log format: "[uptime] YYYY-MM-DD HH:MM:SS"
            // We search for the first space to find the start of the date string
            const char *timestampPtr = strchr(lineBuffer, ' ');

            if (timestampPtr && timestampPtr[1] != '\0') {
                timestampPtr++; // Increment past the space to the actual date

                const size_t targetLen = strlen(targetIso);
                const int compareLen = (int)std::min(targetLen, (size_t)19);
                bool isBefore = includeTimeAtBoundery; // A prefix-equal line counts as "before" for an inclusive bound

                // Compare the first 16 characters (YYYY-MM-DD HH:MM)
                for (int i = 0; i < compareLen; i++) {
                    // Skip index 10: 'T' in ISO vs ' ' in log file
                    if (i == 10) {
                        continue;
                    }

                    if (timestampPtr[i] < targetIso[i]) {
                        isBefore = true;
                        break;
                    }
                    if (timestampPtr[i] > targetIso[i]) {
                        isBefore = false;
                        break;
                    }
                }

                if (isBefore) {
                    // Time at 'mid' is too early (or includeTimeAtBoundery match); search the upper half
                    low = mid + 1;
                    resultOffset = ftell(filePtr); // Store current position as best match so far
                }
                else {
                    // Time at 'mid' is target or later; search the lower half
                    high = mid - 1;
                }
            }
            else {
                // Could not find a valid timestamp on this line; back up
                high = mid - 1;
            }
        }
        else {
            // Reached EOF or read error
            high = mid - 1;
        }
    }
    return resultOffset;
}
