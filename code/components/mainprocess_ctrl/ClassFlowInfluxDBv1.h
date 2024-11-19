#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef CLASSFLOWINFLUXDBV1_H
#define CLASSFLOWINFLUXDBV1_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"


class ClassFlowInfluxDBv1 : public ClassFlow
{
  protected:
    const CfgData::SectionInfluxDBv1 *cfgDataPtr = NULL;
    bool InfluxDBenable;

  public:
    ClassFlowInfluxDBv1();
    virtual ~ClassFlowInfluxDBv1();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string name() { return "ClassFlowInfluxDBv1"; };
};

    #endif // CLASSFLOWINFLUXDBV1_H
#endif     // ENABLE_INFLUXDB