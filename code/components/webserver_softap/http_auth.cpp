#include "http_auth.h"
#include "../../include/defines.h"

#include <esp_tls_crypto.h>
#include <esp_log.h>

#include "configClass.h"
#include "ClassLogFile.h"
#include "system.h"

static const char *TAG = "HTTPAUTH";


static char *getAuthBase64Encoded(const std::string username, const std::string password)
{
    size_t userAuthEncodedLen = 0;
    const std::string userAuth = username + ":" + password;

    esp_crypto_base64_encode(NULL, 0, &userAuthEncodedLen, (const unsigned char *)userAuth.c_str(), userAuth.length());

    // 6: The length of the "Basic " string
    // userAuthEncodedLen: Number of bytes for base64 encoded user auth string
    // 1: Number of bytes which be used to fill zero
    char *authBase64EncodedData = (char *)calloc(1, 6 + userAuthEncodedLen + 1);
    if (authBase64EncodedData == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory (digest)");
        return NULL;
    }

    size_t authBase64EncodedDataLen = 0;
    strcpy(authBase64EncodedData, "Basic ");
    esp_crypto_base64_encode((unsigned char *)authBase64EncodedData + 6, userAuthEncodedLen, &authBase64EncodedDataLen,
                             (const unsigned char *)userAuth.c_str(), userAuth.length());

    return authBase64EncodedData;
}


esp_err_t handleHttpAuthBasic(httpd_req_t *req, esp_err_t httpHandler(httpd_req_t *))
{
    // Cross origin handling (e.g. allow access from localhost (test environment))
    size_t bufLen = httpd_req_get_hdr_value_len(req, "Origin") + 1;
    if (bufLen > 1) {
        char *buf = (char *)calloc(1, bufLen);
        if (buf == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory (buf1)");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Origin", buf, bufLen) == ESP_OK) {
            static const std::string origin(buf, bufLen);
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", origin.c_str()); //@TODO FEATURE: Origin whitelist?
            httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
        }

        free(buf);
    }

    // Skip authorization check if
    // 1. HTTP authorization is disabled
    // 2. At critical system state (reduced web interface)
    if (ConfigClass::getInstance()->get()->sectionWebUi.httpAuth.authMode == HTTP_AUTH_DISABLED || getSystemStatus() > 0) {
        return httpHandler(req);
    }

    // Perform authorization check
    esp_err_t retVal = ESP_FAIL;
    bufLen = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (bufLen > 1) {
        char *buf = (char *)calloc(1, bufLen);
        if (buf == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory (buf)");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, bufLen) == ESP_OK) {
            char *userAuthEncoded = getAuthBase64Encoded(ConfigClass::getInstance()->get()->sectionWebUi.httpAuth.username,
                                                         ConfigClass::getInstance()->get()->sectionWebUi.httpAuth.password);

            if (strncmp((const char *)userAuthEncoded, (const char *)buf, bufLen) == 0) {
                retVal = httpHandler(req);
            }
            else {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Access denied. Wrong username or password");
                httpd_resp_set_status(req, "401 Unauthorized");
                httpd_resp_set_type(req, "text/html");
                httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"AIOTE\"");
                httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Access denied. Wrong username or password");
            }

            free(userAuthEncoded);
        }
        else {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Access denied. No or invalid authorization data");
            httpd_resp_set_status(req, "401 Unauthorized");
            httpd_resp_set_type(req, "text/html");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"AIOTE\"");
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Access denied. No or invalid authorization data");
        }

        free(buf);
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Access denied. No authorization header");
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"AIOTE\"");
        httpd_resp_send(req, NULL, 0);
    }

    return retVal;
}
