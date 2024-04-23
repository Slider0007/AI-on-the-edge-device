#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef CLASSFINFLUXDBv2_H
#define CLASSFINFLUXDBv2_H

#include <string>

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"


class ClassFlowInfluxDBv2 : public ClassFlow
{
    protected:
        ClassFlowPostProcessing* flowpostprocessing;
        std::string uri, bucket;
        std::string dborg, dbtoken;
        bool TLSEncryption;
        std::string TLSCACertFilename, TLSClientCertFilename, TLSClientKeyFilename;
        bool InfluxDBenable;

        void SetInitialParameter(void);

        void handleFieldname(std::string _decsep, std::string _value);
        void handleMeasurement(std::string _decsep, std::string _value);


    public:
        ClassFlowInfluxDBv2();
        ClassFlowInfluxDBv2(std::vector<ClassFlow*>* lfc);
        ClassFlowInfluxDBv2(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
        virtual ~ClassFlowInfluxDBv2();

    bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
    bool doFlow(std::string time);
    void doPostProcessEventHandling();
    bool isInfluxDBEnabled();
    std::string name() {return "ClassFlowInfluxDBv2";};
};

#endif //CLASSFINFLUXDBv2_H
#endif //ENABLE_INFLUXDB