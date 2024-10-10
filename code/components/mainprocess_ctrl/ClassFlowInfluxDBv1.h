#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef CLASSFINFLUXDBV1_H
#define CLASSFINFLUXDBV1_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"


class ClassFlowInfluxDBv1 : public ClassFlow
{
  protected:
    const CfgData::SectionInfluxDBv1 *cfgDataPtr = NULL;
    bool InfluxDBenable;
    ClassFlowPostProcessing *flowpostprocessing;

  public:
    ClassFlowInfluxDBv1();
    virtual ~ClassFlowInfluxDBv1();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();
    bool isInfluxDBEnabled();

    std::string name() { return "ClassFlowInfluxDBv1"; };
};

    #endif // CLASSFINFLUXDBV1_H
#endif     // ENABLE_INFLUXDB