#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef INTERFACE_MQTT_H
#define INTERFACE_MQTT_H

#include <string>
#include <map>
#include <functional>

#include "read_wlanini.h"


struct strMqttConfig {
    std::string uri = "";
    std::string clientID = wlan_config.hostname;
    std::string mainTopic = wlan_config.hostname;
    std::string user = "";
    std::string password = "";
    bool TLSEncryption = false;
    std::string TLSCACertFilename = "";
    std::string TLSClientCertFilename = "";
    std::string TLSClientKeyFilename = "";
    bool retainProcessData = false;
    int keepAlive = 25*60;
};


struct strMqttState {
    bool mqttEnabled = false;
    bool mqttConfigOK = false;
    bool mqttInitialized = false;
    bool mqttConnected = false;

    int failedOnCycle = -1;
    int mqttReconnectCnt = 0;
};


bool MQTT_Configure();
int MQTT_Init();
void MQTTdestroy_client(bool _disable);

bool MQTTPublish(std::string _key, std::string _content, int qos, bool _retainFlag = false);

bool getMQTTisEnabled();
bool getMQTTisConnected();
bool getMQTTisEncrypted();

void MQTTregisterConnectFunction(std::string name, std::function<void()> func);
void MQTTunregisterConnectFunction(std::string name);
void MQTTregisterSubscribeFunction(std::string topic, std::function<bool(std::string, char*, int)> func);
void MQTTdestroySubscribeFunction();
void MQTTconnected();

#endif //INTERFACE_MQTT_H
#endif //#ENABLE_MQTT
