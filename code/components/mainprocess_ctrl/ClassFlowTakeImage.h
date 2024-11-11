#ifndef CLASSFFLOWTAKEIMAGE_H
#define CLASSFFLOWTAKEIMAGE_H

#include <string>

#include "configClass.h"
#include "ClassLogImage.h"
#include "ClassControlCamera.h"


class ClassFlowTakeImage : public ClassLogImage
{
  protected:
    const CfgData::SectionTakeImage *cfgDataPtr = NULL;
    std::string rawImageFilename;
    time_t timeImageTaken;

    bool takeImage();

  public:
    CImageBasis *rawImage;

    ClassFlowTakeImage();
    virtual ~ClassFlowTakeImage();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    time_t getTimeImageTaken();

    ImageData *sendRawImage();
    esp_err_t sendRawJPG(httpd_req_t *req);

    std::string name() { return "ClassFlowTakeImage"; };
};


#endif // CLASSFFLOWTAKEIMAGE_H