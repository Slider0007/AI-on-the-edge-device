#ifndef CLASSFLOWALIGNMENT_H
#define CLASSFLOWALIGNMENT_H

#include <string>

#include "ClassFlowDefineTypes.h"
#include "cfgDataStruct.h"
#include "ClassFlow.h"


class ClassFlowAlignment : public ClassFlow
{
  protected:
    const CfgData::SectionImageAlignment *cfgDataPtr = NULL;
    AlignmentMarker alignmentMarker[2];
    int alignSimilarityCheckSADThreshold;

    void drawAlignmentMarker(CImage &image);
    bool loadAlignmentMarkerData(void);
    bool saveAlignmentMarkerData(void);

    void doPostProcessEventHandling();

  public:
    ClassFlowAlignment();
    virtual ~ClassFlowAlignment();

    bool loadParameter();
    bool doFlow(std::string time);

    std::string name() { return "ClassFlowAlignment"; };
};

#endif // CLASSFLOWALIGNMENT_H
