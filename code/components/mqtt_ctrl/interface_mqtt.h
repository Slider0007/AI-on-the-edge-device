#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef INTERFACE_MQTT_H
#define INTERFACE_MQTT_H

#include <string>
#include <map>
#include <functional>

#include "cfgDataStruct.h"


bool publishMqttData(std::string _key, std::string _content, int qos, bool _retainFlag = false);
bool configureMqttClient(const CfgData::SectionMqtt *cfgDataPtr, int keepAlive);
int startMqttClient(void);

bool getMqttIsEnabled(void);
bool getMqttIsConnected(void);
bool getMqttIsEncrypted(void);

void registerMqttConnectFunction(std::string name, std::function<void()> func);
void unregisterMqttConnectFunction(std::string name);
void registerMqttSubscribeFunction(std::string topic, std::function<bool(std::string, char *, int)> func);
void unregisterMqttSubscribeFunction();
void isConnectedState(void);

void deinitMqttClient(bool disable = false);

#endif // INTERFACE_MQTT_H
#endif // ENABLE_MQTT
