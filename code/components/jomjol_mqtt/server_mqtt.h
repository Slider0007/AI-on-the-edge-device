#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef SERVERMQTT_H
#define SERVERMQTT_H

#include "ClassFlowDefineTypes.h"
#include "ClassFlowMQTT.h"


struct strHADiscoveryConfig {
    bool HADiscoveryEnabled = false;
    std::string HADiscoveryPrefix = "homeassistant";
    std::string HAStatusTopic = "homeassistant/status";
    bool HARetainDiscoveryTopics = false;
};


struct strHADiscoveryData {
    int numberSequenceID = -1;
    bool isTopicJSONNotation = false;
    std::string topic = "";
    std::string topicName = "";
    std::string friendlyName = "";
    std::string icon = "";
    std::string unit = "";
    std::string deviceClass = "";
    std::string stateClass = "";
    std::string entityCategory = "";
};


bool mqttServer_publishHADiscovery(int _qos);
bool mqttServer_publishDeviceInfo(int _qos);
bool mqttServer_publishDeviceStatus(int _qos);

void mqttServer_setParameter(std::vector<NumberPost*>* _NUMBERS, float _processingInterval);
void mqttServer_setMeterType(std::string _meterType, std::string _valueUnit, 
                             std::string _timeUnit, std::string _rateUnit);
std::string mqttServer_getMainTopic();
std::string mqttServer_getTimeUnit();

void mqttServer_schedulePublishDeviceInfo();

void mqttServer_schedulePublishHADiscovery();
bool mqttServer_schedulePublishHADiscoveryFromMqtt(std::string topic, char* data, int data_len);

void register_server_mqtt_uri(httpd_handle_t server);

#endif //SERVERMQTT_H
#endif //ENABLE_MQTT