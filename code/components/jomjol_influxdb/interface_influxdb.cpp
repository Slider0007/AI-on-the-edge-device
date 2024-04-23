#include "interface_influxdb.h"
#include "../../include/defines.h"

#ifdef ENABLE_INFLUXDB
#include <fstream>
#include <time.h>

#include "esp_http_client.h"
#include "esp_log.h"

#include "ClassLogFile.h"
#include "psram.h"
#include "Helper.h"


static const char *TAG = "INFLUXDB_IF";

std::string influxDBURI;
std::string influxDBDatabase;
std::string influxDBUser;
std::string influxDBPassword;
bool influxDBTLSEncryption = false;
std::string influxDBTLSCACert, influxDBTLSClientCert, influxDBTLSClientKey;

std::string influxDBv2URI;
std::string influxDBv2Bucket;
std::string influxDBv2Token;
std::string influxDBv2Org;
bool influxDBv2TLSEncryption = false;
std::string influxDBv2TLSCACert, influxDBv2TLSClientCert, influxDBv2TLSClientKey;


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "HTTP client: Error encountered");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Connected");
            //ESP_LOGI(TAG, "HTTP Client Connected");
            break;
        case HTTP_EVENT_HEADERS_SENT:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Headers sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Header: key=" + std::string(evt->header_key) + ", value="  + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Data recevied: len=" + std::to_string(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: HTTP session finished");
            break;
         case HTTP_EVENT_DISCONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Disconnected");
            break;
        case HTTP_EVENT_REDIRECT:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Intercepting HTTP redirect");
            break;
    }
    return ESP_OK;
}


bool InfluxDBInit(std::string _uri, std::string _database, std::string _user, std::string _password,
                    bool _TLSEncryption, std::string _TLSCACertFilename, std::string _TLSClientCertFilename,
                    std::string _TLSClientKeyFilename)
{
    influxDBURI = _uri;
    influxDBDatabase = _database;
    influxDBUser = _user;
    influxDBPassword = _password;


    // TLS Encryption parameter
    influxDBTLSEncryption = _TLSEncryption;

    if (influxDBTLSEncryption) {
        if (influxDBURI.substr(0,8) != "https://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDB TLS: URI parameter needs to be configured with \'https://\'");
            return false;
        }

        if (!_TLSCACertFilename.empty()) { // TLS parameter activated and not empty
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDB TLS: CA certificate file: " + _TLSCACertFilename);
            std::ifstream ifs(_TLSCACertFilename);
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            influxDBTLSCACert = content;

            if (influxDBTLSCACert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDB TLS: Failed to load CA certificate");
            }
        }

        if (!_TLSClientCertFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDB TLS: Client certificate file: " + _TLSClientCertFilename);
            std::ifstream cert_ifs(_TLSClientCertFilename);
            std::string cert_content((std::istreambuf_iterator<char>(cert_ifs)), (std::istreambuf_iterator<char>()));
            influxDBTLSClientCert = cert_content;

            if (influxDBTLSClientCert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDB TLS: Failed to load client certificate");
            }
        }

        if (!_TLSClientKeyFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDB TLS: Client key file: " + _TLSClientKeyFilename);
            std::ifstream key_ifs(_TLSClientKeyFilename);
            std::string key_content((std::istreambuf_iterator<char>(key_ifs)), (std::istreambuf_iterator<char>()));
            influxDBTLSClientKey = key_content;

            if (influxDBTLSClientKey.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDB TLS: Failed to load client key");
            }
        }
    }
    else {
        if (influxDBURI.substr(0,7) != "http://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDB: URI parameter needs to be configured with \'http://\'");
            return false;
        }
    }

    return true;
}


void InfluxDBPublish(std::string _measurement, std::string _key, std::string _content, std::string _timestamp)
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

    if (influxDBUser.length() && influxDBPassword.length()) {
       http_config.username = influxDBUser.c_str();
       http_config.password = influxDBPassword.c_str();
       http_config.auth_type = HTTP_AUTH_TYPE_BASIC;
    }

    if (influxDBTLSEncryption) {
        if (!influxDBTLSCACert.empty()) {
            http_config.cert_pem = influxDBTLSCACert.c_str();
            http_config.cert_len = influxDBTLSCACert.length() + 1;
            http_config.skip_cert_common_name_check = true;    // Skip any validation of server certificate CN field
        }

        if (!influxDBTLSClientCert.empty()) {
            http_config.client_cert_pem = influxDBTLSClientCert.c_str();
            http_config.client_cert_len = influxDBTLSClientCert.length() + 1;
        }

        if (!influxDBTLSClientKey.empty()) {
            http_config.client_key_pem = influxDBTLSClientKey.c_str();
            http_config.client_key_len = influxDBTLSClientKey.length() + 1;
        }
    }

    std::string payload;
    char nowTimestamp[21];

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDBPublish: Key: " + _key + ", Content: " + _content + ", Timestamp: " + _timestamp);

    if (_timestamp.length() > 0) {
        struct tm tm;

        time_t t;
        time(&t);
        localtime_r(&t, &tm); // Extract DST setting from actual time to consider it for timestamp evaluation

        strptime(_timestamp.c_str(), TIME_FORMAT_OUTPUT, &tm);
        t = mktime(&tm);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Timestamp: " + _timestamp + ", Timestamp (UTC): " + std::to_string(t));

        sprintf(nowTimestamp,"%ld000000000", (long) t);           // UTC
        payload = _measurement + " " + _key + "=" + _content + " " + nowTimestamp;
    }
    else {
        payload = _measurement + " " + _key + "=" + _content;
    }

    payload.shrink_to_fit();
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Payload: " + payload);


    // use the default retention policy of the database
    std::string apiURI = influxDBURI + "/write?db=" + influxDBDatabase;
    apiURI.shrink_to_fit();
    http_config.url = apiURI.c_str();
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "URI: " + apiURI);

    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Initialized");

    esp_http_client_set_header(http_client, "Content-Type", "text/plain");
    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Header setting done");

    ESP_ERROR_CHECK(esp_http_client_set_post_field(http_client, payload.c_str(), payload.length()));
    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Payload post completed");

    esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(http_client));

    if( err == ESP_OK ) {
        int status_code = esp_http_client_get_status_code(http_client);
        if (status_code < 300)
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Writing data successful. HTTP response status: " + std::to_string(status_code));
        else
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Writing data rejected. HTTP response status: " + std::to_string(status_code));
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "HTTP client: Request failed. Error: " + intToHexString(err));
    }
    esp_http_client_cleanup(http_client);
    free_psram_heap(std::string(TAG) + "->response_buffer", response_buffer);
}


bool getInfluxDBisEncrypted()
{
    return influxDBTLSEncryption;
}


bool InfluxDBv2Init(std::string _uri, std::string _bucket, std::string _org, std::string _token,
                        bool _TLSEncryption, std::string _TLSCACertFilename, std::string _TLSClientCertFilename,
                        std::string _TLSClientKeyFilename)
{
    influxDBv2URI = _uri;
    influxDBv2Bucket = _bucket;
    influxDBv2Org = _org;
    influxDBv2Token = _token;

    // TLS Encryption parameter
    influxDBv2TLSEncryption = _TLSEncryption;

    if (influxDBv2TLSEncryption) {
        if (influxDBv2URI.substr(0,8) != "https://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2 TLS: URI parameter needs to be configured with \'https://\'");
            return false;
        }

        if (!_TLSCACertFilename.empty()) { // TLS parameter activated and not empty
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv2 TLS: CA certificate file: " + _TLSCACertFilename);
            std::ifstream ifs(_TLSCACertFilename);
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            influxDBv2TLSCACert = content;

            if (influxDBv2TLSCACert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2 TLS: Failed to load CA certificate");
            }
        }

        if (!_TLSClientCertFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv2 TLS: Client certificate file: " + _TLSClientCertFilename);
            std::ifstream cert_ifs(_TLSClientCertFilename);
            std::string cert_content((std::istreambuf_iterator<char>(cert_ifs)), (std::istreambuf_iterator<char>()));
            influxDBv2TLSClientCert = cert_content;

            if (influxDBv2TLSClientCert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2 TLS: Failed to load client certificate");
            }
        }

        if (!_TLSClientKeyFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv2 TLS: Client key file: " + _TLSClientKeyFilename);
            std::ifstream key_ifs(_TLSClientKeyFilename);
            std::string key_content((std::istreambuf_iterator<char>(key_ifs)), (std::istreambuf_iterator<char>()));
            influxDBv2TLSClientKey = key_content;

            if (influxDBv2TLSClientKey.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2 TLS: Failed to load client key");
            }
        }
    }
    else {
        if (influxDBv2URI.substr(0,7) != "http://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2: URI parameter needs to be configured with \'http://\'");
            return false;
        }
    }

    return true;
}


void InfluxDBv2Publish(std::string _measurement, std::string _key, std::string _content, std::string _timestamp)
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

    if (influxDBv2TLSEncryption) {
        if (!influxDBv2TLSCACert.empty()) {
            http_config.cert_pem = influxDBv2TLSCACert.c_str();
            http_config.cert_len = influxDBv2TLSCACert.length() + 1;
            http_config.skip_cert_common_name_check = true;    // Skip any validation of server certificate CN field
        }

        if (!influxDBv2TLSClientCert.empty()) {
            http_config.client_cert_pem = influxDBv2TLSClientCert.c_str();
            http_config.client_cert_len = influxDBv2TLSClientCert.length() + 1;
        }

        if (!influxDBv2TLSClientKey.empty()) {
            http_config.client_key_pem = influxDBv2TLSClientKey.c_str();
            http_config.client_key_len = influxDBv2TLSClientKey.length() + 1;
        }
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "InfluxDBv2Publish: Key: " + _key + ", Content: " + _content + ", Timestamp: " + _timestamp);

    std::string payload;
    char nowTimestamp[21];

    if (_timestamp.length() > 0) {
        struct tm tm;

        time_t t;
        time(&t);
        localtime_r(&t, &tm); // Extract DST setting from actual time to consider it for timestamp evaluation

        strptime(_timestamp.c_str(), TIME_FORMAT_OUTPUT, &tm);
        t = mktime(&tm);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Timestamp: " + _timestamp + ", Timestamp (UTC): " + std::to_string(t));

        sprintf(nowTimestamp,"%ld000000000", (long) t);           // UTC
        payload = _measurement + " " + _key + "=" + _content + " " + nowTimestamp;
    }
    else {
        payload = _measurement + " " + _key + "=" + _content;
    }

    payload.shrink_to_fit();
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Payload: " + payload);

    std::string apiURI = influxDBv2URI + "/api/v2/write?org=" + influxDBv2Org + "&bucket=" + influxDBv2Bucket;
    apiURI.shrink_to_fit();
    http_config.url = apiURI.c_str();
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "URI: " + apiURI);

    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP client: Initialized");

    esp_http_client_set_header(http_client, "Content-Type", "text/plain");
    std::string _zw = "Token " + influxDBv2Token;
    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Tokenheader: %s\n", _zw.c_str());
    esp_http_client_set_header(http_client, "Authorization", _zw.c_str());
    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Header setting done");

    ESP_ERROR_CHECK(esp_http_client_set_post_field(http_client, payload.c_str(), payload.length()));
    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Payload post completed");

    esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(http_client));

    if( err == ESP_OK ) {
        int status_code = esp_http_client_get_status_code(http_client);
        if (status_code < 300)
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Writing data successful. HTTP response status: " + std::to_string(status_code));
        else
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Writing data rejected. HTTP response status: " + std::to_string(status_code));
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "HTTP client: Request failed. Error: " + intToHexString(err));
    }
    esp_http_client_cleanup(http_client);
    free_psram_heap(std::string(TAG) + "->response_buffer", response_buffer);
}


bool getInfluxDBv2isEncrypted()
{
    return influxDBv2TLSEncryption;
}


void InfluxDBdestroy()
{

}

#endif //ENABLE_INFLUXDB
