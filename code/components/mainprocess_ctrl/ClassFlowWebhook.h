#include "../../include/defines.h"
#ifdef ENABLE_WEBHOOK

#ifndef CLASSFLOWWEBHOOK_H
#define CLASSFLOWWEBHOOK_H

#include <string>

#include "cfgDataStruct.h"
#include "ClassFlow.h"

#include "configClass.h"
class ClassFlowWebhook : public ClassFlow
{
  protected:
    const CfgData::SectionWebhook *cfgDataPtr = NULL;
    bool webhookEnable;

  public:
    ClassFlowWebhook();
    virtual ~ClassFlowWebhook();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string name() { return "ClassFlowWebhook"; };
};

#endif // CLASSFLOWWEBHOOK_H
#endif // ENABLE_WEBHOOK
