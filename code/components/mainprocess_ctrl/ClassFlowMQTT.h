#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef CLASSFFLOWMQTT_H
#define CLASSFFLOWMQTT_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"
#include "interface_mqtt.h"
#include "server_mqtt.h"


class ClassFlowMQTT : public ClassFlow
{
  protected:
    const CfgData::SectionMqtt *cfgDataPtr = NULL;
    ClassFlowPostProcessing *flowpostprocessing;
    int keepAlive;

  public:
    ClassFlowMQTT();
    virtual ~ClassFlowMQTT();
    bool initMqtt(float _processingInterval);

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string name() { return "ClassFlowMQTT"; };
};
    #endif // CLASSFFLOWMQTT_H
#endif     // ENABLE_MQTT