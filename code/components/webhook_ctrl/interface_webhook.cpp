#include "interface_webhook.h"
#include "../../include/defines.h"

#ifdef ENABLE_WEBHOOK
#include <esp_http_client.h>
#include <esp_tls_errors.h>
#include <esp_crt_bundle.h>
#include <esp_log.h>

#include "ClassLogFile.h"
#include "psram.h"
#include "helper.h"
#include "time_sntp.h"


static const char *TAG = "WEBHOOK_IF";

static const CfgData::SectionWebhook *cfgDataPtr = NULL;
static std::string TLSCACert;
static std::string TLSClientCert;
static std::string TLSClientKey;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Error event");
            // ESP-IDF error codes: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html
            // mbedTLS error codes / Cert (X509) verify codes: https://github.com/wolfeidau/mbedtls/blob/master/mbedtls/x509.h
            if (cfgDataPtr->authMode == AUTH_TLS && ((esp_tls_error_handle_t)evt->data)->last_error != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "Connection refused | ESP-IDF error: " +
                                        intToHexString(((esp_tls_error_handle_t)evt->data)->last_error) +
                                        ", mbedTLS error: " + intToHexString(((esp_tls_error_handle_t)evt->data)->esp_tls_error_code) +
                                        ", Cert verify code: " + intToHexString(((esp_tls_error_handle_t)evt->data)->esp_tls_flags));
            }
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Connected");
            // ESP_LOGI(TAG, "HTTP Client Connected");
            break;
        case HTTP_EVENT_HEADERS_SENT:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Headers sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "HTTP client: Received header: key: " + std::string(evt->header_key) +
                                    " | value: " + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Received data: length: " + std::to_string(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Session finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Disconnected");
            break;
        case HTTP_EVENT_REDIRECT:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Intercepting HTTP redirect");
            break;
    }
    return ESP_OK;
}


bool webhookInit(const CfgData::SectionWebhook *_cfgDataPtr)
{
    cfgDataPtr = _cfgDataPtr;

    if (cfgDataPtr->authMode == AUTH_TLS) {
        if (cfgDataPtr->uri.substr(0, 8) != "https://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: URI parameter needs to be configured with \'https://\'");
            return false;
        }

        if (cfgDataPtr->tls.serverCertVerification != TLS_SERVER_CERT_VERIFICATION_NONE && !cfgDataPtr->tls.caCert.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: CA certificate file: /config/certs/" + cfgDataPtr->tls.caCert);

            const std::string filePath = "/sdcard/config/certs/" + cfgDataPtr->tls.caCert;
            if (!readFileToString(filePath, TLSCACert) || TLSCACert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load CA certificate");
                return false;
            }
        }
        else {
            TLSCACert = "";
        }

        if (!cfgDataPtr->tls.clientCert.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: Client certificate file: /config/certs/" + cfgDataPtr->tls.clientCert);

            const std::string filePath = "/sdcard/config/certs/" + cfgDataPtr->tls.clientCert;
            if (!readFileToString(filePath, TLSClientCert) || TLSClientCert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client certificate");
                return false;
            }
        }
        else {
            TLSClientCert = "";
        }

        if (!cfgDataPtr->tls.clientKey.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: Client key file: /config/certs/" + cfgDataPtr->tls.clientKey);

            const std::string filePath = "/sdcard/config/certs/" + cfgDataPtr->tls.clientKey;
            if (!readFileToString(filePath, TLSClientKey) || TLSClientKey.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client key");
                return false;
            }
        }
        else {
            TLSClientKey = "";
        }
    }
    else {
        if (cfgDataPtr->uri.substr(0, 7) != "http://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "URI parameter needs to be configured with \'http://\'");
            return false;
        }
    }

    return true;
}


esp_err_t webhookPublish(const char *_jsonData, CImageJpg *_imgData, time_t _imageTimeProcessed)
{
    esp_http_client_config_t httpConfig = {};
    httpConfig.url = cfgDataPtr->uri.c_str();
    httpConfig.user_agent = "AI-on-the-Edge Device";
    httpConfig.method = HTTP_METHOD_POST;
    httpConfig.event_handler = http_event_handler;
    httpConfig.buffer_size = MAX_HTTP_OUTPUT_BUFFER; // Receive buffer

    if (cfgDataPtr->authMode == AUTH_BASIC) {
        httpConfig.auth_type = HTTP_AUTH_TYPE_BASIC;
        httpConfig.username = cfgDataPtr->username.c_str();
        httpConfig.password = cfgDataPtr->password.c_str();
    }
    else if (cfgDataPtr->authMode == AUTH_TLS) {
        httpConfig.auth_type = HTTP_AUTH_TYPE_BASIC;
        httpConfig.username = cfgDataPtr->username.c_str();
        httpConfig.password = cfgDataPtr->password.c_str();
        httpConfig.transport_type = HTTP_TRANSPORT_OVER_SSL;

        if (cfgDataPtr->tls.serverCertVerification == TLS_SERVER_CERT_VERIFICATION_NONE) {
            // Warning: Server certificate verification disabled, use only for testing purpose
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Server certificate verification disabled");
        }
        else {
            // Skip request if no valid time is set to verify server certificate
            if (!getTimeIsSet()) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip publish request: No valid time for server certificate verification");
                return ESP_FAIL;
            }

            if (!TLSCACert.empty()) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification enabled | Use user provided certificate");
                httpConfig.cert_pem = TLSCACert.c_str();
                httpConfig.cert_len = TLSCACert.length() + 1;
            }
            else {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification enabled | Use built-in certificate bundle");
                httpConfig.crt_bundle_attach = esp_crt_bundle_attach;
            }

            // Configure validation of certificate name to identify server
            // - 1. Only if available: Subject alternative names (SAN fields: DNS or IP) for multi-domain usage: Aliases for server name
            // - 2. Check common name (CN field): Server name
            // Warning: If name validation is disabled, MITM attacks are possible (fake server)
            if (cfgDataPtr->tls.serverCertVerification == TLS_SERVER_CERT_VERIFICATION_NO_NAME_VALIDATION) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification | Name validation disabled");
                httpConfig.skip_cert_common_name_check = true;
            }
        }
    }

    esp_http_client_handle_t httpClient = esp_http_client_init(&httpConfig);
    if (httpClient == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "HTTP client: Initialization failed");
        return ESP_FAIL;
    }
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Initialized");

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Publish data: " + std::string(_jsonData, strlen(_jsonData)));

    esp_http_client_set_header(httpClient, "APIKEY", cfgDataPtr->apiKey.c_str());
    esp_http_client_set_header(httpClient, "Content-Type", "application/json");
    esp_http_client_set_post_field(httpClient, _jsonData, strlen(_jsonData));

    esp_err_t retVal = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(httpClient));
    if (retVal == ESP_OK) {
        int statusCode = esp_http_client_get_status_code(httpClient);
        if (statusCode < 300) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Writing data successful. HTTP response status: " + std::to_string(statusCode));
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Writing data rejected. HTTP response status: " + std::to_string(statusCode));
            retVal = ESP_FAIL;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "HTTP client: Request failed. Error: " + intToHexString(retVal));
    }

    if (_imgData) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "webhookPublish: Publish image");
        esp_http_client_set_url(httpClient, (cfgDataPtr->uri + std::string("?timestamp=") + std::to_string(_imageTimeProcessed)).c_str());
        esp_http_client_set_method(httpClient, HTTP_METHOD_PUT);
        esp_http_client_set_header(httpClient, "Content-Type", "image/jpeg");
        esp_http_client_set_post_field(httpClient, (const char *)_imgData->getImgData(), _imgData->getImgDataSize());

        retVal = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(httpClient));
        if (retVal == ESP_OK) {
            int statusCode = esp_http_client_get_status_code(httpClient);
            if (statusCode < 300) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Writing data successful. HTTP response status: " + std::to_string(statusCode));
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Writing data rejected. HTTP response status: " + std::to_string(statusCode));
                retVal = ESP_FAIL;
            }
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "HTTP client: Request failed. Error: " + intToHexString(retVal));
        }
    }

    esp_http_client_cleanup(httpClient);

    return retVal;
}


bool getWebhookIsEncrypted()
{
    if (cfgDataPtr != NULL && cfgDataPtr->authMode == AUTH_TLS) {
        return true;
    }

    return false;
}

#endif // ENABLE_WEBHOOK
