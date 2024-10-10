#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef SERVERMQTT_H
#define SERVERMQTT_H

#include "ClassFlowDefineTypes.h"
#include "ClassFlowMQTT.h"


struct strHADiscoveryData {
    std::string sequenceName = "";
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

void mqttServer_setParameter(const CfgData::SectionMqtt *_cfgDataPtr, const std::vector<SequenceData*> *_sequenceData, const float _processingInterval);
void mqttServer_setMeterType(const std::string _meterType, const std::string _valueUnit,
                             const std::string _timeUnit, const std::string _rateUnit);
std::string mqttServer_getMainTopic();
std::string mqttServer_getTimeUnit();

void mqttServer_schedulePublishDeviceInfo();

void mqttServer_schedulePublishHADiscovery();
bool mqttServer_schedulePublishHADiscoveryFromMqtt(std::string topic, char* data, int data_len);

void registerMqttUri(httpd_handle_t server);

#endif //SERVERMQTT_H
#endif //ENABLE_MQTT