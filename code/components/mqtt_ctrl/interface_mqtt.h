#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef INTERFACE_MQTT_H
#define INTERFACE_MQTT_H

#include <string>
#include <map>
#include <functional>

#include "cfgDataStruct.h"


struct strMqttState {
    bool mqttEnabled = false;
    bool mqttConfigOK = false;
    bool mqttInitialized = false;
    bool mqttConnected = false;

    int failedOnCycle = -1;
    int mqttReconnectCnt = 0;
};


bool MQTTPublish(std::string _key, std::string _content, int qos, bool _retainFlag = false);
bool MQTT_Configure(const CfgData::SectionMqtt *cfgDataPtr, int keepAlive);
int MQTT_Init();

bool getMQTTisEnabled();
bool getMQTTisConnected();
bool getMQTTisEncrypted();

void MQTTregisterConnectFunction(std::string name, std::function<void()> func);
void MQTTunregisterConnectFunction(std::string name);
void MQTTregisterSubscribeFunction(std::string topic, std::function<bool(std::string, char*, int)> func);
void MQTTdestroySubscribeFunction();
void MQTTconnected();

void MQTTdestroy_client(bool _disable);

#endif //INTERFACE_MQTT_H
#endif //#ENABLE_MQTT
