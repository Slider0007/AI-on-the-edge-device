#include "interface_influxdbv1.h"
#include "../../include/defines.h"

#ifdef ENABLE_INFLUXDB
#include <fstream>
#include <time.h>

#include <esp_http_client.h>
#include <esp_log.h>

#include "ClassLogFile.h"
#include "psram.h"
#include "helper.h"


static const char *TAG = "INFLUXDBV1_IF";

static const CfgData::SectionInfluxDBv1 *cfgDataPtr = NULL;
static std::string TLSCACert;
static std::string TLSClientCert;
static std::string TLSClientKey;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
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
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Header: key=" + std::string(evt->header_key) + ", value="  + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Data recevied: len=" + std::to_string(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: HTTP session finished");
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


bool influxDBv1Init(const CfgData::SectionInfluxDBv1 *_cfgDataPtr)
{
    cfgDataPtr = _cfgDataPtr;

    if (cfgDataPtr->authMode == AUTH_TLS) {
        if (cfgDataPtr->uri.substr(0,8) != "https://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1 TLS: URI parameter needs to be configured with \'https://\'");
            return false;
        }

        if (!cfgDataPtr->tls.caCert.empty()) { // TLS parameter activated and not empty
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv1 TLS: CA certificate file: /sdcard" + cfgDataPtr->tls.caCert);
            std::ifstream ifs("/sdcard" + cfgDataPtr->tls.caCert);
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            TLSCACert = content;
            if (TLSCACert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1 TLS: Failed to load CA certificate");
            }
        }

        if (!cfgDataPtr->tls.clientCert.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv1 TLS: Client certificate file: " + cfgDataPtr->tls.clientCert);
            std::ifstream cert_ifs("/sdcard" + cfgDataPtr->tls.clientCert);
            std::string cert_content((std::istreambuf_iterator<char>(cert_ifs)), (std::istreambuf_iterator<char>()));
            TLSClientCert = cert_content;
            if (TLSClientCert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1 TLS: Failed to load client certificate");
            }
        }

        if (!cfgDataPtr->tls.clientKey.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv1 TLS: Client key file: " + cfgDataPtr->tls.clientKey);
            std::ifstream key_ifs("/sdcard" + cfgDataPtr->tls.clientKey);
            std::string key_content((std::istreambuf_iterator<char>(key_ifs)), (std::istreambuf_iterator<char>()));
            TLSClientKey = key_content;
            if (TLSClientKey.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1 TLS: Failed to load client key");
            }
        }
    }
    else {
        if (cfgDataPtr->uri.substr(0,7) != "http://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1: URI parameter needs to be configured with \'http://\'");
            return false;
        }
    }

    return true;
}


esp_err_t influxDBv1Publish(const std::string &_measurement, const std::string &_key, const std::string &_content, const std::string &_timestamp)
{
    char* response_buffer = (char*) calloc_psram_heap(std::string(TAG) + "->response_buffer", 1,
                            sizeof(char) * MAX_HTTP_OUTPUT_BUFFER, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

    esp_http_client_config_t http_config = {
       .user_agent = "ESP32 Meter reader",
       .method = HTTP_METHOD_POST,
       .event_handler = http_event_handler,
       .buffer_size = MAX_HTTP_OUTPUT_BUFFER,
       .user_data = response_buffer
    };

    if (cfgDataPtr->authMode != AUTH_NONE) {
       http_config.username = cfgDataPtr->username.c_str();
       http_config.password = cfgDataPtr->password.c_str();
       http_config.auth_type = HTTP_AUTH_TYPE_BASIC;
    }

    if (cfgDataPtr->authMode == AUTH_TLS) {
        if (!TLSCACert.empty()) {
            http_config.cert_pem = TLSCACert.c_str();
            http_config.cert_len = TLSCACert.length() + 1;
            http_config.skip_cert_common_name_check = true;    // Skip any validation of server certificate CN field
        }

        if (!TLSClientCert.empty()) {
            http_config.client_cert_pem = TLSClientCert.c_str();
            http_config.client_cert_len = TLSClientCert.length() + 1;
        }

        if (!TLSClientKey.empty()) {
            http_config.client_key_pem = TLSClientKey.c_str();
            http_config.client_key_len = TLSClientKey.length() + 1;
        }
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "influxDBv1Publish: Key: " + _key + ", Content: " + _content + ", Timestamp: " + _timestamp);

    esp_err_t retVal = ESP_OK;
    std::string payload;
    char nowTimestamp[21];

    if (_timestamp.length() > 0) {
        struct tm tm;

        time_t t;
        time(&t);
        localtime_r(&t, &tm); // Extract DST setting from actual time to consider it for timestamp evaluation

        strptime(_timestamp.c_str(), TIME_FORMAT_OUTPUT, &tm);
        t = mktime(&tm);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Timestamp: " + _timestamp + ", Timestamp (UTC): " + std::to_string(t));

        sprintf(nowTimestamp,"%ld000000000", (long) t);           // UTC
        payload = _measurement + " " + _key + "=" + _content + " " + nowTimestamp;
    }
    else {
        payload = _measurement + " " + _key + "=" + _content;
    }

    payload.shrink_to_fit();
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Payload: " + payload);

    // use the default retention policy of the database
    std::string apiURI = cfgDataPtr->uri + "/write?db=" + cfgDataPtr->database;
    apiURI.shrink_to_fit();
    http_config.url = apiURI.c_str();
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "URI: " + apiURI);

    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Initialized");

    esp_http_client_set_header(http_client, "Content-Type", "text/plain");
    //LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Header setting done");

    ESP_ERROR_CHECK(esp_http_client_set_post_field(http_client, payload.c_str(), payload.length()));
    //LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Payload post completed");

    retVal = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(http_client));

    if (retVal == ESP_OK) {
        int status_code = esp_http_client_get_status_code(http_client);
        if (status_code < 300) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Writing data successful. HTTP response status: " + std::to_string(status_code));
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Writing data rejected. HTTP response status: " + std::to_string(status_code));
            retVal = ESP_FAIL;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "HTTP client: Request failed. Error: " + intToHexString(retVal));
    }
    esp_http_client_cleanup(http_client);
    free_psram_heap(std::string(TAG) + "->response_buffer", response_buffer);

    return retVal;
}


bool getInfluxDBv1isEncrypted()
{
    if (cfgDataPtr != NULL && cfgDataPtr->authMode == AUTH_TLS)
        return true;

    return false;
}

#endif //ENABLE_INFLUXDB
