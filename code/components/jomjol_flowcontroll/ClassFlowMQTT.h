#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef CLASSFFLOWMQTT_H
#define CLASSFFLOWMQTT_H

#include <string>

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"
#include "interface_mqtt.h"
#include "server_mqtt.h"


enum eDataNotation {
    JSON = 0,
    TOPICS,
    JSON_AND_TOPICS
};


class ClassFlowMQTT : public ClassFlow
{
    protected:
        void SetInitialParameter(void);
        ClassFlowPostProcessing* flowpostprocessing;

        int processDataNotation;

    public:
        ClassFlowMQTT(ClassFlowPostProcessing* _flowpostprocessing);
        virtual ~ClassFlowMQTT();
        bool Start(float AutoInterval);

        bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
        bool doFlow(std::string time);
        void doPostProcessEventHandling();
        std::string name() {return "ClassFlowMQTT";};
};
#endif //CLASSFFLOWMQTT_H
#endif //ENABLE_MQTT