#ifdef ENABLE_INFLUXDB

#pragma once

#ifndef CLASSFINFLUXDBv2_H
#define CLASSFINFLUXDBv2_H

#include "ClassFlow.h"

#include "ClassFlowPostProcessing.h"

#include <string>

class ClassFlowInfluxDBv2 : public ClassFlow
{
protected:
	ClassFlowPostProcessing* flowpostprocessing;
    std::string uri, bucket;
    std::string dborg, dbtoken, dbfield;
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
    std::string name() {return "ClassFlowInfluxDBv2";};
};

#endif //CLASSFINFLUXDBv2_H
#endif //ENABLE_INFLUXDB