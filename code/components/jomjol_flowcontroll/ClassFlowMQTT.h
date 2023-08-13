#ifdef ENABLE_MQTT

#pragma once

#ifndef CLASSFFLOWMQTT_H
#define CLASSFFLOWMQTT_H

#include "ClassFlow.h"

#include "ClassFlowPostProcessing.h"

#include <string>

class ClassFlowMQTT : public ClassFlow
{
protected:
	ClassFlowPostProcessing* flowpostprocessing; 
    std::string uri, topic, topicError, clientname, topicRate, topicTimeStamp, topicUptime, topicFreeMem;
    std::string user, password;
    std::string maintopic; 
    float roundInterval; // Minutes
    int keepAlive; // Seconds
    bool SetRetainFlag;

	void SetInitialParameter(void);        

public:
    ClassFlowMQTT();
    ClassFlowMQTT(std::vector<ClassFlow*>* lfc);
    ClassFlowMQTT(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
    virtual ~ClassFlowMQTT();
    bool Start(float AutoInterval);

    bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
    bool doFlow(std::string time);
    std::string name() {return "ClassFlowMQTT";};
};
#endif //CLASSFFLOWMQTT_H
#endif //ENABLE_MQTT