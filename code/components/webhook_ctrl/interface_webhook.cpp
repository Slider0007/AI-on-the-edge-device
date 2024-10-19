#include "interface_webhook.h"
#include "../../include/defines.h"

#ifdef ENABLE_WEBHOOK
#include <fstream>
//#include <time.h>

#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_log.h>

#include "ClassLogFile.h"
#include "psram.h"
#include "helper.h"


static const char *TAG = "WEBHOOK_IF";

static const CfgData::SectionWebhook *cfgDataPtr = NULL;
static std::string TLSCACert;
static std::string TLSClientCert;
static std::string TLSClientKey;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Error event");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Connected");
            //ESP_LOGI(TAG, "HTTP Client Connected");
            break;
        case HTTP_EVENT_HEADERS_SENT:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Headers sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Received header: key: " + std::string(evt->header_key) +
                                                    " | value: " + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Received data: length:" + std::to_string(evt->data_len));
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
        if (cfgDataPtr->uri.substr(0,8) != "https://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: URI parameter needs to be configured with \'https://\'");
            return false;
        }

        if (!cfgDataPtr->tls.caCert.empty()) { // TLS parameter activated and not empty
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: CA certificate file: /config/certs/" + cfgDataPtr->tls.caCert);
            std::ifstream ifs("/sdcard/config/certs/" + cfgDataPtr->tls.caCert);
            TLSCACert = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            if (TLSCACert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load CA certificate");
                return false;
            }
        }

        if (!cfgDataPtr->tls.clientCert.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: Client certificate file: /config/certs/" + cfgDataPtr->tls.clientCert);
            std::ifstream cert_ifs("/sdcard/config/certs/" + cfgDataPtr->tls.clientCert);
            TLSClientCert = std::string(std::istreambuf_iterator<char>(cert_ifs), std::istreambuf_iterator<char>());
            if (TLSClientCert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client certificate");
                return false;
            }
        }

        if (!cfgDataPtr->tls.clientKey.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: Client key file: /config/certs/" + cfgDataPtr->tls.clientKey);
            std::ifstream key_ifs("/sdcard/config/certs/" + cfgDataPtr->tls.clientKey);
            TLSClientKey = std::string(std::istreambuf_iterator<char>(key_ifs), std::istreambuf_iterator<char>());
            if (TLSClientKey.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client key");
                return false;
            }
        }
    }
    else {
        if (cfgDataPtr->uri.substr(0,7) != "http://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "URI parameter needs to be configured with \'http://\'");
            return false;
        }
    }

    return true;
}


esp_err_t webhookPublish(const char* _jsonData, ImageData *_imgData, time_t _imageTimeProcessed)
{
    esp_http_client_config_t httpConfig = {
        .url = cfgDataPtr->uri.c_str(),
        .user_agent = "AI-on-the-Edge Device",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .buffer_size = MAX_HTTP_OUTPUT_BUFFER
    };

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

        if (!TLSCACert.empty()) {
            httpConfig.cert_pem = TLSCACert.c_str();
            httpConfig.cert_len = TLSCACert.length() + 1;
            httpConfig.skip_cert_common_name_check = true;    // Skip any validation of server certificate CN field
        }
        else {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CA Certificate empty, use certification bundle for server verfication");
            httpConfig.crt_bundle_attach = esp_crt_bundle_attach;
        }

        if (!TLSClientCert.empty()) {
            httpConfig.client_cert_pem = TLSClientCert.c_str();
            httpConfig.client_cert_len = TLSClientCert.length() + 1;
        }

        if (!TLSClientKey.empty()) {
            httpConfig.client_key_pem = TLSClientKey.c_str();
            httpConfig.client_key_len = TLSClientKey.length() + 1;
        }
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "webhookPublish: Publish data: " + std::string(_jsonData, strlen(_jsonData)));

    esp_err_t retVal = ESP_OK;
    esp_http_client_handle_t httpClient = esp_http_client_init(&httpConfig);
    if (httpClient == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "HTTP client: Initialization failed");
        return ESP_FAIL;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Initialized");

    esp_http_client_set_header(httpClient, "APIKEY", cfgDataPtr->apiKey.c_str());
    esp_http_client_set_header(httpClient, "Content-Type", "application/json");
    esp_http_client_set_post_field(httpClient, _jsonData, strlen(_jsonData));

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

    if (_imgData) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "webhookPublish: Publish image");
        esp_http_client_set_url(httpClient, (cfgDataPtr->uri + std::string("?timestamp=") + std::to_string(_imageTimeProcessed)).c_str());
        esp_http_client_set_method(httpClient, HTTP_METHOD_PUT);
        esp_http_client_set_header(httpClient, "Content-Type", "image/jpeg");
        esp_http_client_set_post_field(httpClient, (const char *)_imgData->data, _imgData->size);

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
    if (cfgDataPtr != NULL && cfgDataPtr->authMode == AUTH_TLS)
        return true;

    return false;
}

#endif //ENABLE_WEBHOOK
