#include "interface_mqtt.h"

#ifdef ENABLE_MQTT
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#ifdef DEBUG_DETAIL_ON
#include "esp_timer.h"
#endif

#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "connect_wlan.h"
#include "server_mqtt.h"


//#define DEBUG_DETAIL_ON


static const char *TAG = "MQTT_IF";

strMqttConfig mqttConfig = {}; // Global struct for MQTT config
extern const strHADiscoveryConfig HADiscoveryConfig;

static std::map<std::string, std::function<void()>>* connectFunktionMap = NULL;  
static std::map<std::string, std::function<bool(std::string, char*, int)>>* subscribeFunktionMap = NULL;

static strMqttState mqttState = {};

static esp_mqtt_client_handle_t mqttClient = NULL;
static const esp_mqtt_event_id_t mqttEventID = MQTT_EVENT_ANY;

static std::string LWTTopic;
static std::string TLSCACert, TLSClientCert, TLSClientKey;


bool MQTTPublish(std::string _key, std::string _content, int _qos, bool _retainFlag) 
{
    if (!mqttState.mqttEnabled) { // MQTT sevice not started / configured (MQTT_Init not called before)      
        return false;
    }

    if (mqttState.failedOnCycle == getFlowCycleCounter()) { // we already failed in this cycle, do not retry until the next cycle
        return true; // Fail quietly
    }

    MQTT_Init(); // Re-Init client if not initialized yet/anymore

    if (mqttState.mqttInitialized && mqttState.mqttConnected) {
        #ifdef DEBUG_DETAIL_ON 
            long long int starttime = esp_timer_get_time();
        #endif
        int msg_id = esp_mqtt_client_publish(mqttClient, _key.c_str(), _content.c_str(), 0, _qos, _retainFlag);
        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "Publish msg_id %d in %lld ms", msg_id, (esp_timer_get_time() - starttime)/1000);
        #endif
        if (msg_id == -1) {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Failed to publish topic '" + _key + "', retry");   
            #ifdef DEBUG_DETAIL_ON 
                starttime = esp_timer_get_time();
            #endif
            msg_id = esp_mqtt_client_publish(mqttClient, _key.c_str(), _content.c_str(), 0, _qos, _retainFlag);
            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "Publish msg_id %d in %lld ms", msg_id, (esp_timer_get_time() - starttime)/1000);
            #endif
            if (msg_id == -1) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to publish topic '" + _key + "', retry in next cycle");
                mqttState.failedOnCycle = getFlowCycleCounter();
                return false;
            }
        }

        if (_content.length() > 80) { // Truncate message if too long
            _content.resize(80);
            _content.append("..");
        }

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Published topic: " + _key + ", content: " + _content + " | msg_id: " + std::to_string(msg_id));
        return true;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish skipped. Client not initalized / connected | Topic: " + _key);
        return false;
    }
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
            MQTTconnected();
            break;
        
        case MQTT_EVENT_DISCONNECTED:
            mqttState.mqttConnected = false;
            mqttState.mqttReconnectCnt++;
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Disconnected, retry to connect");

            if (mqttState.mqttReconnectCnt >= 5) {
                mqttState.mqttReconnectCnt = 0;
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Disconnected, multiple reconnect attempts failed. Retry");
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
            #endif
            topic.assign(event->topic, event->topic_len);
            if (subscribeFunktionMap != NULL) {
                if (subscribeFunktionMap->find(topic) != subscribeFunktionMap->end()) {
                    //ESP_LOGD(TAG, "call subcribe function for topic %s", topic.c_str());
                    (*subscribeFunktionMap)[topic](topic, event->data, event->data_len);
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip request, topic not subscribed");
                }
            }
            break;
        
        case MQTT_EVENT_ERROR:
            // http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718033 --> chapter 3.2.2.3 

            // The server does not support the level of the MQTT protocol requested by the client
            // NOTE: Only protocol 3.1.1 is supported (refer to setting in sdkconfig)
            if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_PROTOCOL) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, unacceptable protocol version (0x01)");  
            }
            // The client identifier is correct UTF-8 but not allowed by the server
            // e.g. clientID empty (cannot be the case -> default set in firmware)
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_ID_REJECTED) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, identifier rejected (0x02)");
            }
            // The network connection has been made but the MQTT service is unavailable
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, Server unavailable (0x03)");
            }
            // The data in the user name or password is malformed
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_BAD_USERNAME) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, malformed data in username or password (0x04)");
            }
            // The client is not authorized to connect
            else if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, not authorized. Check username/password (0x05)");
            }
            
            // Log any ESP-TLS error: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html
            if (mqttConfig.TLSEncryption && (event->error_handle->esp_tls_last_esp_err != ESP_OK)) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection refused, TLS error code: " + 
                        intToHexString(event->error_handle->esp_tls_last_esp_err) + 
                        " (More infos: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/error-codes.html");
            }

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR - esp_mqtt_error_codes:");
                ESP_LOGI(TAG, "error_type:%d", event->error_handle->error_type);
                ESP_LOGI(TAG, "connect_return_code:%d", event->error_handle->connect_return_code);
                ESP_LOGI(TAG, "esp_transport_sock_errno:%d", event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "esp_tls_last_esp_err:%d", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "esp_tls_stack_err:%d", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "esp_tls_cert_verify_flags:%d", event->error_handle->esp_tls_cert_verify_flags);
            #endif

            break;
        
        default:
            ESP_LOGD(TAG, "Other event id: %d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    mqtt_event_handler_cb((esp_mqtt_event_handle_t) event_data);
}


bool MQTT_Configure()
{
    if ((mqttConfig.uri.length() == 0) || (mqttConfig.mainTopic.length() == 0) || (mqttConfig.clientID.length() == 0)) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Init aborted! Config error (URI, MainTopic or ClientID missing)");
        return false;
    }

    LWTTopic = mqttConfig.mainTopic + MQTT_STATUS_TOPIC;

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "URI: " + mqttConfig.uri + ", clientID: " + mqttConfig.clientID + 
                        ", user: " + mqttConfig.user + ", password: *****, mainTopic: " + mqttConfig.mainTopic + 
                        ", last-will-topic: " + LWTTopic + ", keepAlive: " + std::to_string(mqttConfig.keepAlive) + 
                        ", RetainProcessData: " + std::to_string(mqttConfig.retainProcessData) + 
                        ", TLSEncryption: " + std::to_string(mqttConfig.TLSEncryption)); 

    if (mqttConfig.TLSEncryption) {
        if (mqttConfig.uri.substr(0,8) != "mqtts://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TLS: URI parameter needs to be configured with \'mqtts://\'");
            return false;
        }

        if (mqttConfig.uri.substr(mqttConfig.uri.find_last_of(":")+1, 4) != "8883") {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "TLS: URI parameter not using default MQTT TLS port \'8883\'");
        }

        if (!mqttConfig.TLSCACertFilename.empty()) { // TLS parameter activated and not empty
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "TLS: CA certificate file: " + mqttConfig.TLSCACertFilename);
            std::ifstream ifs(mqttConfig.TLSCACertFilename);
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            TLSCACert = content;

            if (TLSCACert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load CA certificate");
            }
        }

        if (!mqttConfig.TLSClientCertFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "TLS: Client certificate file: " + mqttConfig.TLSClientCertFilename);
            std::ifstream cert_ifs(mqttConfig.TLSClientCertFilename);
            std::string cert_content((std::istreambuf_iterator<char>(cert_ifs)), (std::istreambuf_iterator<char>()));
            TLSClientCert = cert_content;

            if (TLSClientCert.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client certificate");
            }
        }

        if (!mqttConfig.TLSClientKeyFilename.empty()) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "TLS: Client key file: " + mqttConfig.TLSClientKeyFilename);
            std::ifstream key_ifs(mqttConfig.TLSClientKeyFilename);
            std::string key_content((std::istreambuf_iterator<char>(key_ifs)), (std::istreambuf_iterator<char>()));
            TLSClientKey = key_content;

            if (TLSClientKey.empty()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TLS: Failed to load client key");
            }
        }
    }
    else {
        if (mqttConfig.uri.substr(0,7) != "mqtt://") {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "URI parameter needs to be configured with \'mqtt://\'");
            return false;
        }
        
        if (mqttConfig.uri.substr(mqttConfig.uri.find_last_of(":")+1, 4) != "1883") {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "URI parameter not using default MQTT port \'1883\'");
        }
    }

    mqttState.mqttConfigOK = true;
    return true;
}


int MQTT_Init()
{ 
    if (mqttState.mqttInitialized) {
        return 0;
    }

    if (mqttState.mqttConfigOK) {                           
        mqttState.mqttEnabled = true;
    } else {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Init called, but client is not yet configured.");
        return 0;
    }

    if (!getWIFIisConnected()) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Init called, but WIFI is not yet connected.");
        return 0;
    }

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init");
    MQTTdestroy_client(false);

    esp_mqtt_client_config_t mqtt_cfg = { };

    mqtt_cfg.broker.address.uri = mqttConfig.uri.c_str();
    mqtt_cfg.credentials.client_id = mqttConfig.clientID.c_str();
    mqtt_cfg.network.disable_auto_reconnect = false;     // Reconnection routine active (Default: false)
    mqtt_cfg.network.reconnect_timeout_ms = 15000;       // Try to reconnect to broker (Default: 10000ms)
    mqtt_cfg.network.timeout_ms = 10000;                 // Network Timeout (Default: 10000ms)
    mqtt_cfg.session.message_retransmit_timeout = 3000;  // Time after message resent when broker not acknowledged (QoS1, QoS2)
    mqtt_cfg.session.last_will.topic = LWTTopic.c_str();
    mqtt_cfg.session.last_will.retain = 1;
    mqtt_cfg.session.last_will.msg = std::string(MQTT_STATUS_OFFLINE).c_str();
    //mqtt_cfg.session.last_will.msg_len = (int)(std::string(MQTT_STATUS_OFFLINE).length());
    mqtt_cfg.session.keepalive = mqttConfig.keepAlive;
    mqtt_cfg.buffer.size = 2048;                         // size of MQTT send/receive buffer (Default: 1024)

    if (!mqttConfig.user.empty()) {
        mqtt_cfg.credentials.username = mqttConfig.user.c_str();
    }

    if (!mqttConfig.password.empty()) {
        mqtt_cfg.credentials.authentication.password = mqttConfig.password.c_str();
    }

    if (mqttConfig.TLSEncryption) {
        if (!TLSCACert.empty()) {
            mqtt_cfg.broker.verification.certificate = TLSCACert.c_str();
            mqtt_cfg.broker.verification.certificate_len = TLSCACert.length() + 1;  
            mqtt_cfg.broker.verification.skip_cert_common_name_check = true;    // Skip any validation of server certificate CN field
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

    mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (mqttClient) {
        esp_err_t ret = esp_mqtt_client_register_event(mqttClient, mqttEventID, mqtt_event_handler, mqttClient);
        if (ret != ESP_OK)
        {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Could not register event (ret=" + std::to_string(ret) + ")");
            mqttState.mqttInitialized = false;
            return -1;
        }

        ret = esp_mqtt_client_start(mqttClient);
        if (ret != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Client start failed (retval=" + std::to_string(ret) + ")");
            mqttState.mqttInitialized = false;
            return -1;
        }
        else {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Client started, waiting for established connection");
            mqttState.mqttInitialized = true;
            return 1;
        }
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Init failed, no handle created");
        mqttState.mqttInitialized = false;
        return -1;
    }
}


void MQTTdestroy_client(bool _disable = false)
{
    if (mqttClient) {
        if (mqttState.mqttConnected) {
            MQTTdestroySubscribeFunction();      
            esp_mqtt_client_disconnect(mqttClient);
            mqttState.mqttConnected = false;
        }
        esp_mqtt_client_stop(mqttClient);
        esp_mqtt_client_destroy(mqttClient);
        mqttClient = NULL;
        mqttState.mqttInitialized = false;
        mqttState.mqttEnabled = false;
    }

    if (_disable) // Disable MQTT service, avoid restart with MQTTPublish
        mqttState.mqttConfigOK = false;
}


bool getMQTTisEnabled()
{
    return mqttState.mqttEnabled;
}


bool getMQTTisConnected()
{
    return mqttState.mqttConnected;
}


bool getMQTTisEncrypted()
{
    return mqttConfig.TLSEncryption;
}


bool mqtt_handler_flow_start(std::string _topic, char* _data, int _data_len) 
{
    //ESP_LOGD(TAG, "Handler called: topic %s, data %.*s", _topic.c_str(), _data_len, _data);

    if (_data_len > 0) {
        MQTTCtrlFlowStart(_topic);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "handler_flow_start: handler called, but no data");
    }

    return true;
}


bool mqtt_handler_set_fallbackvalue(std::string _topic, char* _data, int _data_len) 
{
    //ESP_LOGD(TAG, "Handler called: topic %s, data %.*s", _topic.c_str(), _data_len, _data);
    //example: {"sequence": "main", "value": 12345.1234567}

    if (_data_len > 0) {    // Check if data length > 0
        cJSON *jsonData = cJSON_Parse(_data);
        cJSON *sequenceName = cJSON_GetObjectItemCaseSensitive(jsonData, "sequence");
        cJSON *value = cJSON_GetObjectItemCaseSensitive(jsonData, "value");

        if (cJSON_IsString(sequenceName) && (sequenceName->valuestring != NULL)) {    // Check if sequenceName is valid
            if (cJSON_IsNumber(value)) {   // Check if value is a number
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "handler_set_fallbackvalue called: sequence: " + std::string(sequenceName->valuestring) + 
                                                                                         ", value: " + std::to_string(value->valuedouble));
                if (flowctrl.UpdateFallbackValue(std::to_string(value->valuedouble), std::string(sequenceName->valuestring))) {
                    cJSON_Delete(jsonData);
                    return true;
                }
            }
            else {
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: value not a valid number (\"value\": 12345.12345)");
            }
        }
        else {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: sequence not a valid string (\"sequence\": \"main\")");
        }
        cJSON_Delete(jsonData);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "handler_set_fallbackvalue: handler called, but no data received");
    }

    return false;
}


void MQTTconnected()
{
    if (mqttState.mqttConnected) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Connected to broker");
        MQTTPublish(mqttConfig.mainTopic + MQTT_STATUS_TOPIC, MQTT_STATUS_ONLINE, 1, false); // Send MQTT birth message "online"

        if (connectFunktionMap != NULL) {
            for(std::map<std::string, std::function<void()>>::iterator it = connectFunktionMap->begin(); it != connectFunktionMap->end(); ++it) {
                it->second();
                //ESP_LOGD(TAG, "call connect function %s", it->first.c_str());
            }
        }

        // Subcribe to topics
        // Note: Further subsriptions are handled in GPIO class
        //*****************************************
        // Subcribe to [mainTopic]/process/ctrl/flow_start
        std::function<bool(std::string topic, char* data, int data_len)> subHandler1 = mqtt_handler_flow_start;     
        MQTTregisterSubscribeFunction(mqttConfig.mainTopic + "/process/ctrl/cycle_start", subHandler1);

        // Subcribe to [mainTopic]/process/ctrl/set_fallbackvalue
        std::function<bool(std::string topic, char* data, int data_len)> subHandler2 = mqtt_handler_set_fallbackvalue;     
        MQTTregisterSubscribeFunction(mqttConfig.mainTopic + "/process/ctrl/set_fallbackvalue", subHandler2);
        
        // Subcribe to /homeassistant/status
        if (HADiscoveryConfig.HADiscoveryEnabled) {
            std::function<bool(std::string topic, char* data, int data_len)> subHandler3 = mqttServer_schedulePublishHADiscoveryFromMqtt;     
            MQTTregisterSubscribeFunction(HADiscoveryConfig.HAStatusTopic, subHandler3);
        }

       if (subscribeFunktionMap != NULL) {
            for(std::map<std::string, std::function<bool(std::string, char*, int)>>::iterator it = subscribeFunktionMap->begin(); 
                                                                                        it != subscribeFunktionMap->end(); ++it)
            {
                int retVal = esp_mqtt_client_subscribe(mqttClient, it->first.c_str(), 0);
                if (retVal >= 0)
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Topic subscribed: " + it->first);
                else
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to subscribe topic: " + it->first);
            }
        }
    }
}


void MQTTregisterConnectFunction(std::string name, std::function<void()> func)
{
    //ESP_LOGD(TAG, "MQTTregisteronnectFunction %s\r\n", name.c_str());
    if (connectFunktionMap == NULL) {
        connectFunktionMap = new std::map<std::string, std::function<void()>>();
    }

    if ((*connectFunktionMap)[name] != NULL) {
        ESP_LOGD(TAG, "Connect function %s already registred", name.c_str());
        return;
    }

    (*connectFunktionMap)[name] = func;

    if (mqttState.mqttConnected) {
        func();
    }
}


void MQTTunregisterConnectFunction(std::string name)
{
    ESP_LOGD(TAG, "unregisterConnnectFunction %s\r\n", name.c_str());
    if ((connectFunktionMap != NULL) && (connectFunktionMap->find(name) != connectFunktionMap->end())) {
        connectFunktionMap->erase(name);
    }
}


void MQTTregisterSubscribeFunction(std::string topic, std::function<bool(std::string, char*, int)> func)
{
    //ESP_LOGD(TAG, "registerSubscribeFunction %s", topic.c_str());
    if (subscribeFunktionMap == NULL) {
        subscribeFunktionMap = new std::map<std::string, std::function<bool(std::string, char*, int)>>();
    }

    if ((*subscribeFunktionMap)[topic] != NULL) {
        ESP_LOGD(TAG, "Topic %s already registered for subscription", topic.c_str());
        return;
    }

    (*subscribeFunktionMap)[topic] = func;
}


void MQTTdestroySubscribeFunction()
{
    if (subscribeFunktionMap != NULL) {
        if (mqttState.mqttConnected) {
            for(std::map<std::string, std::function<bool(std::string, char*, int)>>::iterator it = subscribeFunktionMap->begin(); 
                                                                                        it != subscribeFunktionMap->end(); ++it)
            {
                int retVal = esp_mqtt_client_unsubscribe(mqttClient, it->first.c_str());
                if (retVal >= 0)
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Topic unsubscribed: " + it->first);
                else
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to unsubscribe topic: " + it->first);
            }
        }

        subscribeFunktionMap->clear();
        delete subscribeFunktionMap;
        subscribeFunktionMap = NULL;
    }
}
#endif //ENABLE_MQTT
