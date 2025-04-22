#ifndef CLASSFFLOWTAKEIMAGE_H
#define CLASSFFLOWTAKEIMAGE_H

#include <string>

#include "configClass.h"
#include "ClassLogImage.h"
#include "ClassControlCamera.h"
#include "ClassFlowDefineTypes.h"


class ClassFlowTakeImage : public ClassLogImage
{
  protected:
    const CfgData::SectionTakeImage *cfgDataPtr = NULL;
    time_t timeImageTaken;

    bool takeImage();

  public:
    ClassFlowTakeImage();
    virtual ~ClassFlowTakeImage();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    time_t getTimeImageTaken() const { return timeImageTaken; };

    std::string name() { return "ClassFlowTakeImage"; };
};

#endif // CLASSFFLOWTAKEIMAGE_H
