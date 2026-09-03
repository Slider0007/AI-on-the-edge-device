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


static esp_err_t sendAuthRequired(httpd_req_t *req, const char *message)
{
    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"AI-on-the-Edge\"");
    return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, message);
}


esp_err_t handleHttpAuthBasic(httpd_req_t *req, esp_err_t httpHandler(httpd_req_t *))
{
    const auto *config = ConfigClass::getInstance()->get();
    const auto &auth = config->sectionWebUi.httpAuth;
    const bool authActive = auth.authMode != HTTP_AUTH_DISABLED && getSystemStatus() == 0;

    // CORS handling (permissive cross-origin requests by design)
    char originBuf[128] = {};
    const size_t originLen = httpd_req_get_hdr_value_len(req, "Origin");

    if (originLen > 0 && originLen < sizeof(originBuf) &&
        httpd_req_get_hdr_value_str(req, "Origin", originBuf, sizeof(originBuf)) == ESP_OK) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", originBuf);
        httpd_resp_set_hdr(req, "Vary", "Origin");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", authActive ? "Content-Type, Authorization" : "Content-Type");
    }

    // CORS preflight
    if (req->method == HTTP_OPTIONS) {
        return httpd_resp_send(req, NULL, 0);
    }

    // Skip authorization check if
    // 1. HTTP authorization is disabled
    // 2. At critical system state (reduced web interface)
    if (!authActive) {
        return httpHandler(req);
    }

    // Authorization header
    constexpr size_t MAX_AUTH_HEADER_LEN = 128;
    char authHeader[MAX_AUTH_HEADER_LEN + 1] = {};
    const size_t authLen = httpd_req_get_hdr_value_len(req, "Authorization");

    if (authLen == 0 || authLen > MAX_AUTH_HEADER_LEN) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Access denied. No or invalid authorization data");
        return sendAuthRequired(req, "Access denied");
    }

    if (httpd_req_get_hdr_value_str(req, "Authorization", authHeader, sizeof(authHeader)) != ESP_OK) {
        return sendAuthRequired(req, "Access denied");
    }

    char *expectedAuth = getAuthBase64Encoded(auth.username, auth.password);

    if (expectedAuth == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    const size_t expectedLen = strlen(expectedAuth);
    const bool authorized = authLen == expectedLen && memcmp(expectedAuth, authHeader, expectedLen) == 0;

    free(expectedAuth);

    if (!authorized) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Access denied. Wrong username or password");
        return sendAuthRequired(req, "Access denied");
    }

    return httpHandler(req);
}
