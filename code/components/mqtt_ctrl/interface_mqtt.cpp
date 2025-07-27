#include "interface_mqtt.h"

#include <fstream>

#ifdef ENABLE_MQTT
#include <esp_log.h>
#include <cJSON.h>

#include <mqtt_client.h>
#include <esp_crt_bundle.h>

#ifdef DEBUG_DETAIL_ON
#include <esp_timer.h>
#endif // DEBUG_DETAIL_ON

#include "configClass.h"
#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "network_main.h"
#include "connect_wlan.h"
#include "server_mqtt.h"
#include "time_sntp.h"


static const char *TAG = "MQTT_IF";

static SemaphoreHandle_t mqttStartMutex = xSemaphoreCreateMutex();
static const CfgData::SectionMqtt *cfgDataPtr;

static std::string LWTTopic;
static std::string TLSCACert;
static std::string TLSClientCert;
static std::string TLSClientKey;

static int keepAlive;

static struct strMqttState {
    bool mqttEnabled = false;
    bool mqttInitialized = false;
    bool mqttConnected = false;

    int failedOnCycle = -1;
    int mqttReconnectCnt = 0;
} mqttState;

static esp_mqtt_client_handle_t mqttClient = NULL;
static const esp_mqtt_event_id_t mqttEventID = MQTT_EVENT_ANY;

static std::map<std::string, std::function<void()>> *connectFunctionMap = NULL;
static std::map<std::string, std::function<bool(std::string, char *, int)>> *subscribeFunctionMap = NULL;


bool publishMqttData(std::string _key, std::string _content, int _qos, bool _retainFlag)
{
    if (!mqttState.mqttEnabled) { // MQTT service disabled
        return false;
    }

    if (mqttState.failedOnCycle == getFlowCycleCounter()) { // Already a failed transmission in this cycle
        return true;                                        // Fail quietly
    }

    startMqttClient(); // Restart client if not started yet/anymore

    if (!mqttState.mqttInitialized || !mqttState.mqttConnected) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Skip publish request: Not connected to broker | Topic: " + _key);
        return false;
    }

#ifdef DEBUG_DETAIL_ON
    int64_t starttime = esp_timer_get_time();
#endif // DEBUG_DETAIL_ON
    int msg_id = esp_mqtt_client_publish(mqttClient, _key.c_str(), _content.c_str(), 0, _qos, _retainFlag);
#ifdef DEBUG_DETAIL_ON
    ESP_LOGI(TAG, "Publish msg_id %d in %lld ms", msg_id, (esp_timer_get_time() - starttime) / 1000);
#endif // DEBUG_DETAIL_ON

    if (msg_id == -1) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Failed to publish topic '" + _key + "', retry");
#ifdef DEBUG_DETAIL_ON
        starttime = esp_timer_get_time();
#endif // DEBUG_DETAIL_ON
        msg_id = esp_mqtt_client_publish(mqttClient, _key.c_str(), _content.c_str(), 0, _qos, _retainFlag);
#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Publish msg_id %d in %lld ms", msg_id, (esp_timer_get_time() - starttime) / 1000);
#endif // DEBUG_DETAIL_ON

        if (msg_id == -1) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to publish topic '" + _key + "', retry in next cycle");
            mqttState.failedOnCycle = getFlowCycleCounter();
            return false;
        }
    }

    if (_content.length() > 80) { // Truncate message if too long
        _content.resize(80);
        _content.append("..");
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Published topic: " + _key + ", content: " + _content + " | msg_id: " + std::to_string(msg_id));

    return true;
}


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    std::string topic = "";
    switch (event->event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            mqttState.mqttInitialized = true;
            break;

        case MQTT_EVENT_CONNECTED:
            mqttState.mqttReconnectCnt = 0;
            mqttState.mqttInitialized = true;
            mqttState.mqttConnected = true;
            isConnectedState();
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqttState.mqttConnected = false;
            mqttState.mqttReconnectCnt++;
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected. Retry to connect");

            if (mqttState.mqttReconnectCnt >= 5) {
                mqttState.mqttReconnectCnt = 0;
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Multiple reconnect attempts failed. Retry to connect");
            }
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
#ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
#endif // DEBUG_DETAIL_ON
            topic.assign(event->topic, event->topic_len);
            if (subscribeFunctionMap != NULL) {
                if (subscribeFunctionMap->find(topic) != subscribeFunctionMap->end()) {
                    // ESP_LOGD(TAG, "call subcribe function for topic %s", topic.c_str());
                    (*subscribeFunctionMap)[topic](topic, event->data, event->data_len);
                }
                else {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip request, topic not subscribed");
                }
            }
            break;

        case MQTT_EVENT_ERROR:
            // http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033 --> chapter 3.2.2.3

            // The server does not support the level of the MQTT protocol requested by the client
            // NOTE: Only protocol 3.1.1 is supported (refer to setting in sdkconfig)
            if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_PROTOCOL) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Connection refused, unacceptable protocol version (0x01)");
            }
            // The client identifier is correct UTF-8 but not allowed by the server
            // e.g. clientID empty (cannot be the case -> default set in firmware)
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_ID_REJECTED) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Connection refused, identifier rejected (0x02)");
            }
            // The network connection has been made but the MQTT service is unavailable
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Connection refused, server unavailable (0x03)");
            }
            // The data in the username name or password is malformed
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_BAD_USERNAME) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Connection refused, malformed data in username or password (0x04)");
            }
            // The client is not authorized to connect
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Connection refused, not authorized (0x05)");
            }

            // ESP-IDF error codes: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html
            // mbedTLS error codes / Cert (X509) verify codes: https://github.com/wolfeidau/mbedtls/blob/master/mbedtls/x509.h
            if (cfgDataPtr->authMode == AUTH_TLS && event->error_handle->esp_tls_last_esp_err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "Connection refused | ESP-IDF error: " + intToHexString(event->error_handle->esp_tls_last_esp_err) +
                                        ", mbedTLS error: " + intToHexString(event->error_handle->esp_tls_stack_err) +
                                        ", Cert verify code: " + intToHexString(event->error_handle->esp_tls_cert_verify_flags));
            }

#ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR - esp_mqtt_error_codes:");
            ESP_LOGI(TAG, "error_type:%d", event->error_handle->error_type);
            ESP_LOGI(TAG, "connect_return_code:%d", event->error_handle->connect_return_code);
            ESP_LOGI(TAG, "esp_transport_sock_errno:%d", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "esp_tls_last_esp_err:%d", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "esp_tls_stack_err:%d", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "esp_tls_cert_verify_flags:%d", event->error_handle->esp_tls_cert_verify_flags);
#endif // DEBUG_DETAIL_ON

            break;

        default:
            ESP_LOGD(TAG, "Other event id: %d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}


bool configureMqttClient(const CfgData::SectionMqtt *_param, int keepAlive)
{
    cfgDataPtr = _param;
    keepAlive = keepAlive;

    if ((cfgDataPtr->uri.empty()) || (cfgDataPtr->mainTopic.empty()) || (cfgDataPtr->clientID.empty())) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init aborted! Config error (URI, MainTopic or ClientID missing)");
        return false;
    }

    LWTTopic = cfgDataPtr->mainTopic + MQTT_STATUS_TOPIC;

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "URI: " + cfgDataPtr->uri + ", clientID: " + cfgDataPtr->clientID + ", username: " + cfgDataPtr->username +
                            ", password: *****, mainTopic: " + cfgDataPtr->mainTopic + ", last-will-topic: " + LWTTopic + ", keepAlive: " +
                            std::to_string(keepAlive) + ", RetainProcessData: " + std::to_string(cfgDataPtr->retainProcessData) +
                            ", AuthMode: " + std::to_string(cfgDataPtr->authMode));

    if (cfgDataPtr->authMode == AUTH_TLS) {
        if (cfgDataPtr->uri.substr(0, 8) != "mqtts://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: URI parameter needs to be configured with \'mqtts://\'");
            return false;
        }

        if (cfgDataPtr->uri.substr(cfgDataPtr->uri.find_last_of(":") + 1, 4) != "8883") {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: URI parameter not using default MQTT TLS port \'8883\'");
        }

        if (cfgDataPtr->tls.serverCertVerification != TLS_SERVER_CERT_VERIFICATION_NONE && !cfgDataPtr->tls.caCert.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TLS: CA certificate file: /config/certs/" + cfgDataPtr->tls.caCert);
            std::ifstream ifs("/sdcard/config/certs/" + cfgDataPtr->tls.caCert);
            TLSCACert = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            if (TLSCACert.empty()) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load CA certificate");
                return false;
            }
        }
        else {
            TLSCACert = "";
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
        else {
            TLSClientCert = "";
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
        else {
            TLSClientKey = "";
        }
    }
    else {
        if (cfgDataPtr->uri.substr(0, 7) != "mqtt://") {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "URI parameter needs to be configured with \'mqtt://\'");
            return false;
        }

        if (cfgDataPtr->uri.substr(cfgDataPtr->uri.find_last_of(":") + 1, 4) != "1883") {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "URI parameter not using default MQTT port \'1883\'");
        }
    }

    mqttState.mqttEnabled = true;
    return true;
}


esp_err_t startMqttClient(void)
{
    // Return if already started or service is not enabled
    if (mqttState.mqttInitialized || !mqttState.mqttEnabled) {
        return ESP_ERR_NOT_ALLOWED;
    }

    // Start only with established network connection
    if (!getNetworkConnectionState()) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init postponed: Network connection not yet established");
        return ESP_ERR_NOT_ALLOWED;
    }

    if (xSemaphoreTake(mqttStartMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        esp_mqtt_client_config_t mqtt_cfg = {};
        if (cfgDataPtr->authMode == AUTH_BASIC) {
            mqtt_cfg.credentials.username = cfgDataPtr->username.c_str();
            mqtt_cfg.credentials.authentication.password = cfgDataPtr->password.c_str();
        }
        else if (cfgDataPtr->authMode == AUTH_TLS) {
            mqtt_cfg.credentials.username = cfgDataPtr->username.c_str();
            mqtt_cfg.credentials.authentication.password = cfgDataPtr->password.c_str();

            if (cfgDataPtr->tls.serverCertVerification == TLS_SERVER_CERT_VERIFICATION_NONE) {
                // Warning: Server certificate verification disabled, use only for testing purpose
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Server certificate verification disabled");
            }
            else {
                // Skip request if no valid time is set to verify server certificate
                // Note: A warning message will be set in flow state "Publish to MQTT" (ClassFlowMQTT.cpp)
                //       to give the system some time to set proper time
                if (!getTimeIsSet()) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init postponed: No valid time for server certificate verification");
                    xSemaphoreGive(mqttStartMutex);
                    return ESP_ERR_NOT_ALLOWED;
                }

                if (!TLSCACert.empty()) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification enabled | Use user provided certificate");
                    mqtt_cfg.broker.verification.certificate = TLSCACert.c_str();
                    mqtt_cfg.broker.verification.certificate_len = TLSCACert.length() + 1;
                }
                else {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification enabled | Use built-in certificate bundle");
                    mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
                }

                // Configure validation of certificate name to identify server
                // - 1. Only if available: Subject alternative names (SAN fields: DNS or IP) for multi-domain usage: Aliases for server name
                // - 2. Check common name (CN field): Server name
                // Warning: If name validation is disabled, MITM attacks are possible (fake server)
                if (cfgDataPtr->tls.serverCertVerification == TLS_SERVER_CERT_VERIFICATION_NO_NAME_VALIDATION) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Server certificate verification | Name validation disabled");
                    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
                }
            }

            if (!TLSClientCert.empty()) {
                mqtt_cfg.credentials.authentication.certificate = TLSClientCert.c_str();
                mqtt_cfg.credentials.authentication.certificate_len = TLSClientCert.length() + 1;
            }

            if (!TLSClientKey.empty()) {
                mqtt_cfg.credentials.authentication.key = TLSClientKey.c_str();
                mqtt_cfg.credentials.authentication.key_len = TLSClientKey.length() + 1;
            }
        }

        mqtt_cfg.broker.address.uri = cfgDataPtr->uri.c_str();
        mqtt_cfg.credentials.client_id = cfgDataPtr->clientID.c_str();
        mqtt_cfg.network.disable_auto_reconnect = false;    // Reconnection routine active (Default: false)
        mqtt_cfg.network.reconnect_timeout_ms = 15000;      // Try to reconnect to broker (Default: 10000ms)
        mqtt_cfg.network.timeout_ms = 10000;                // Network Timeout (Default: 10000ms)
        mqtt_cfg.session.message_retransmit_timeout = 3000; // Time after message resent when broker not acknowledged (QoS1, QoS2)
        mqtt_cfg.session.last_will.topic = LWTTopic.c_str();
        mqtt_cfg.session.last_will.retain = 1;
        mqtt_cfg.session.last_will.msg = std::string(MQTT_STATUS_OFFLINE).c_str();
        mqtt_cfg.session.keepalive = keepAlive;
        mqtt_cfg.buffer.size = 1024; // size of MQTT send/receive buffer (Default: 1024)


        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init MQTT client");

        deinitMqttClient();

        mqttClient = esp_mqtt_client_init(&mqtt_cfg);
        if (mqttClient == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init failed: No handle created");
            mqttState.mqttInitialized = false;
            xSemaphoreGive(mqttStartMutex);
            return ESP_FAIL;
        }

        esp_err_t ret = esp_mqtt_client_register_event(mqttClient, mqttEventID, mqtt_event_handler, mqttClient);
        if (ret != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Could not register event | Error: " + intToHexString(ret));
            mqttState.mqttInitialized = false;
            xSemaphoreGive(mqttStartMutex);
            return ESP_FAIL;
        }

        ret = esp_mqtt_client_start(mqttClient);
        if (ret != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Client start failed | Error: " + intToHexString(ret));
            mqttState.mqttInitialized = false;
            xSemaphoreGive(mqttStartMutex);
            return ESP_FAIL;
        }

        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Client started: Waiting for established connection");
        mqttState.mqttInitialized = true;
        xSemaphoreGive(mqttStartMutex);
        return ESP_OK;
    }

    // Return if semaphore cannot be taken
    return ESP_ERR_NOT_FINISHED;
}


void deinitMqttClient(bool disable)
{
    if (mqttClient) {
        unregisterMqttSubscribeFunction();
        esp_mqtt_client_stop(mqttClient);
        esp_mqtt_client_unregister_event(mqttClient, mqttEventID, mqtt_event_handler);
        esp_mqtt_client_destroy(mqttClient);
        mqttClient = NULL;
    }

    if (disable) {
        mqttState.mqttEnabled = false;
    }

    mqttState.mqttInitialized = false;
    mqttState.mqttConnected = false;
}


bool getMqttIsEnabled(void)
{
    return mqttState.mqttEnabled;
}


bool getMqttIsConnected(void)
{
    return mqttState.mqttConnected;
}


bool getMqttIsEncrypted(void)
{
    if (cfgDataPtr != NULL && cfgDataPtr->authMode == AUTH_TLS) {
        return true;
    }

    return false;
}


bool getMqttTlsCertVerifyRequiresTime()
{
    if (cfgDataPtr == NULL ||
        (cfgDataPtr->authMode == AUTH_TLS && cfgDataPtr->tls.serverCertVerification != TLS_SERVER_CERT_VERIFICATION_NONE)) {
        return true;
    }

    return false;
}


bool mqtt_handler_flow_start(std::string _topic, char *_data, int _data_len)
{
    // ESP_LOGD(TAG, "Handler called: topic %s, data %.*s", _topic.c_str(), _data_len, _data);

    if (_data_len > 0) {
        triggerFlowStartByMqtt(_topic);
    }
    else {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "handler_flow_start: handler called, but no data");
    }

    return true;
}


bool mqtt_handler_set_fallbackvalue(std::string _topic, char *_data, int _data_len)
{
    // ESP_LOGD(TAG, "Handler called: topic %s, data %.*s", _topic.c_str(), _data_len, _data);
    // example: {"sequence": "main", "value": 12345.1234567}

    if (_data_len > 0) { // Check if data length > 0
        cJSON *jsonData = cJSON_Parse(_data);
        cJSON *sequenceName = cJSON_GetObjectItemCaseSensitive(jsonData, "sequence");
        cJSON *value = cJSON_GetObjectItemCaseSensitive(jsonData, "value");

        if (cJSON_IsString(sequenceName) && (sequenceName->valuestring != NULL)) { // Check if sequenceName is valid
            if (cJSON_IsNumber(value)) {                                           // Check if value is a number
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                    "handler_set_fallbackvalue called: sequence: " + std::string(sequenceName->valuestring) +
                                        ", value: " + std::to_string(value->valuedouble));
                if (flowctrl.setFallbackValue(std::string(sequenceName->valuestring), std::to_string(value->valuedouble))) {
                    cJSON_Delete(jsonData);
                    return true;
                }
            }
            else {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: value not a valid number (\"value\": 12345.12345)");
            }
        }
        else {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: sequence not a valid string (\"sequence\": \"main\")");
        }
        cJSON_Delete(jsonData);
    }
    else {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: handler called, but no data received");
    }

    return false;
}


void isConnectedState(void)
{
    if (mqttState.mqttConnected) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Connected to broker");
        publishMqttData(cfgDataPtr->mainTopic + MQTT_STATUS_TOPIC, MQTT_STATUS_ONLINE, 1, false); // Send MQTT birth message "online"

        if (connectFunctionMap != NULL) {
            for (std::map<std::string, std::function<void()>>::iterator it = connectFunctionMap->begin(); it != connectFunctionMap->end();
                 ++it) {
                it->second();
                // ESP_LOGD(TAG, "call connect function %s", it->first.c_str());
            }
        }

        // Subcribe to topics
        // Note: Further subsriptions are handled in GPIO class
        //*****************************************
        // Subcribe to [mainTopic]/process/ctrl/flow_start
        std::function<bool(std::string topic, char *data, int data_len)> subHandler1 = mqtt_handler_flow_start;
        registerMqttSubscribeFunction(cfgDataPtr->mainTopic + "/process/ctrl/cycle_start", subHandler1);

        // Subcribe to [mainTopic]/process/ctrl/set_fallbackvalue
        std::function<bool(std::string topic, char *data, int data_len)> subHandler2 = mqtt_handler_set_fallbackvalue;
        registerMqttSubscribeFunction(cfgDataPtr->mainTopic + "/process/ctrl/set_fallbackvalue", subHandler2);

        // Subcribe to /homeassistant/status
        if (cfgDataPtr->homeAssistant.discoveryEnabled) {
            std::function<bool(std::string topic, char *data, int data_len)> subHandler3 = mqttServer_schedulePublishHADiscoveryFromMqtt;
            registerMqttSubscribeFunction(cfgDataPtr->homeAssistant.statusTopic, subHandler3);
        }

        if (subscribeFunctionMap != NULL) {
            for (std::map<std::string, std::function<bool(std::string, char *, int)>>::iterator it = subscribeFunctionMap->begin();
                 it != subscribeFunctionMap->end(); ++it) {
                int retVal = esp_mqtt_client_subscribe(mqttClient, it->first.c_str(), 0);
                if (retVal >= 0) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Topic subscribed: " + it->first);
                }
                else {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to subscribe topic: " + it->first);
                }
            }
        }
    }
}


void registerMqttConnectFunction(std::string name, std::function<void()> func)
{
    // ESP_LOGD(TAG, "MQTTregisteronnectFunction %s\r\n", name.c_str());
    if (connectFunctionMap == NULL) {
        connectFunctionMap = new std::map<std::string, std::function<void()>>();
    }

    if ((*connectFunctionMap)[name] != NULL) {
        ESP_LOGD(TAG, "Connect function %s already registred", name.c_str());
        return;
    }

    (*connectFunctionMap)[name] = func;

    if (mqttState.mqttConnected) {
        func();
    }
}


void unregisterMqttConnectFunction(std::string name)
{
    ESP_LOGD(TAG, "unregisterConnnectFunction %s\r\n", name.c_str());
    if ((connectFunctionMap != NULL) && (connectFunctionMap->find(name) != connectFunctionMap->end())) {
        connectFunctionMap->erase(name);
    }
}


void registerMqttSubscribeFunction(std::string topic, std::function<bool(std::string, char *, int)> func)
{
    // ESP_LOGD(TAG, "registerSubscribeFunction %s", topic.c_str());
    if (subscribeFunctionMap == NULL) {
        subscribeFunctionMap = new std::map<std::string, std::function<bool(std::string, char *, int)>>();
    }

    if ((*subscribeFunctionMap)[topic] != NULL) {
        ESP_LOGD(TAG, "Topic %s already registered for subscription", topic.c_str());
        return;
    }

    (*subscribeFunctionMap)[topic] = func;
}


void unregisterMqttSubscribeFunction()
{
    if (subscribeFunctionMap != NULL) {
        if (mqttState.mqttConnected) {
            for (std::map<std::string, std::function<bool(std::string, char *, int)>>::iterator it = subscribeFunctionMap->begin();
                 it != subscribeFunctionMap->end(); ++it) {
                int retVal = esp_mqtt_client_unsubscribe(mqttClient, it->first.c_str());
                if (retVal >= 0) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Topic unsubscribed: " + it->first);
                }
                else {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to unsubscribe topic: " + it->first);
                }
            }
        }

        subscribeFunctionMap->clear();
        delete subscribeFunctionMap;
        subscribeFunctionMap = NULL;
    }
}
#endif // ENABLE_MQTT
