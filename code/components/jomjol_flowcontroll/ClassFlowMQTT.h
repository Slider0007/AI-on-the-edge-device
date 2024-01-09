#include "../../include/defines.h"
#ifdef ENABLE_MQTT

#ifndef CLASSFFLOWMQTT_H
#define CLASSFFLOWMQTT_H

#include <string>

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"


class ClassFlowMQTT : public ClassFlow
{
    protected:
        ClassFlowPostProcessing* flowpostprocessing; 
        std::string uri, maintopic, clientname; 
        std::string user, password;
        bool TLSEncryption;
        std::string TLSCACertFilename, TLSClientCertFilename, TLSClientKeyFilename;
        bool SetRetainFlag;
        int keepAlive; // Seconds
        
        void SetInitialParameter(void);        

    public:
        ClassFlowMQTT();
        ClassFlowMQTT(std::vector<ClassFlow*>* lfc);
        ClassFlowMQTT(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
        virtual ~ClassFlowMQTT();
        bool Start(float AutoInterval);

        bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
        bool doFlow(std::string time);
        void doPostProcessEventHandling();
        std::string name() {return "ClassFlowMQTT";};
};
#endif //CLASSFFLOWMQTT_H
#endif //ENABLE_MQTT