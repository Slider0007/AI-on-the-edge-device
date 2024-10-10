#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef CLASSFINFLUXDBv2_H
#define CLASSFINFLUXDBv2_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"


class ClassFlowInfluxDBv2 : public ClassFlow
{
  protected:
    const CfgData::SectionInfluxDBv2 *cfgDataPtr = NULL;
    bool InfluxDBenable;
    ClassFlowPostProcessing *flowpostprocessing;

  public:
    ClassFlowInfluxDBv2();
    virtual ~ClassFlowInfluxDBv2();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();
    bool isInfluxDBEnabled();

    std::string name() { return "ClassFlowInfluxDBv2"; };
};

    #endif // CLASSFINFLUXDBv2_H
#endif     // ENABLE_INFLUXDB