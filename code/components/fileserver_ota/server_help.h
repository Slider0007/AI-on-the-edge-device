#ifndef SERVERHELP_H
#define SERVERHELP_H

#include <string>

#include <esp_http_server.h>

std::string getParentPath(const std::string &path);
const char *getPathFromUri(char *dest, const char *basePath, const char *uri, size_t destsize);
bool endsWith(const char *filename, const char *suffix);
bool endsWith(std::string const &str, std::string const &suffix);
esp_err_t setContentTypeFromFile(httpd_req_t *req, const char *filename);

esp_err_t receiveRequestBodyToFile(httpd_req_t *req, const char *filePath);

bool isValidIsoTimestamp(const char *isoStr);
long getOffsetForTime(FILE *filePtr, const char *targetIso, long fileSize);

#endif // SERVERHELP_H
