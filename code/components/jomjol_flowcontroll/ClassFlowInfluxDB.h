#ifdef ENABLE_INFLUXDB

#pragma once

#ifndef CLASSFINFLUXDB_H
#define CLASSFINFLUXDB_H

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"

#include <string>

class ClassFlowInfluxDB : public ClassFlow
{
protected:
    ClassFlowPostProcessing* flowpostprocessing;
    std::string uri, database, measurement;
    std::string user, password;
    bool TLSEncryption;
    std::string TLSCACertFilename, TLSClientCertFilename, TLSClientKeyFilename;
    bool InfluxDBenable;

    void SetInitialParameter(void);    
    
    void handleFieldname(std::string _decsep, std::string _value);   
    void handleMeasurement(std::string _decsep, std::string _value);   

public:
    ClassFlowInfluxDB();
    ClassFlowInfluxDB(std::vector<ClassFlow*>* lfc);
    ClassFlowInfluxDB(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
    virtual ~ClassFlowInfluxDB();

    bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
    bool doFlow(std::string time);
    void doPostProcessEventHandling();
    std::string name() {return "ClassFlowInfluxDB";};
};

#endif //CLASSFINFLUXDB_H
#endif //ENABLE_INFLUXDB