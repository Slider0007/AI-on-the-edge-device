#ifndef CLASSFLOWALIGNMENT_H
#define CLASSFLOWALIGNMENT_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "helper.h"
#include "CAlignAndCutImage.h"
#include "ClassFlowDefineTypes.h"


class ClassFlowAlignment : public ClassFlow
{
  protected:
    const CfgData::SectionImageAlignment *cfgDataPtr = NULL;
    CImageBasis *imageTemp;
    CAlignAndCutImage *alignAndCutImage;
    AlignmentMarker alignmentMarker[2];
    int alignFastSADThreshold;

    bool useAntialiasing;

    void drawAlignmentMarker(CImageBasis *image);
    bool loadAlignmentMarkerData(void);
    bool saveAlignmentMarkerData(void);

    void doPostProcessEventHandling();

  public:
    CImageBasis *ImageBasis;
    ImageData *AlgROI;

    ClassFlowAlignment();
    //ClassFlowAlignment(ClassFlowTakeImage *_flowTakeImage);
    virtual ~ClassFlowAlignment();

    bool loadParameter();
    bool doFlow(std::string time);

    CAlignAndCutImage *getAlignAndCutImage() { return alignAndCutImage; };
    bool getFlipImageSize();

    std::string name() { return "ClassFlowAlignment"; };
};


#endif // CLASSFLOWALIGNMENT_H
