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
    std::string uri, database;
    std::string dborg, dbtoken, dbfield;
    std::string OldValue;
    bool InfluxDBenable;
    bool SaveErrorLog;

    void SetInitialParameter(void);     

    void handleFieldname(string _decsep, string _value);   
    void handleMeasurement(string _decsep, string _value);


public:
    ClassFlowInfluxDBv2();
    ClassFlowInfluxDBv2(std::vector<ClassFlow*>* lfc);
    ClassFlowInfluxDBv2(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
    virtual ~ClassFlowInfluxDBv2();

    bool ReadParameter(FILE* pfile, string& aktparamgraph);
    bool doFlow(string time);
    void doAutoErrorHandling();
    string name(){return "ClassFlowInfluxDBv2";};
};

#endif //CLASSFINFLUXDBv2_H
#endif //ENABLE_INFLUXDB