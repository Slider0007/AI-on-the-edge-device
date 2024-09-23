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
    int image_height;
    int image_width;
    time_t timeImageTaken;

    bool takePictureWithFlash();

  public:
    CImageBasis *rawImage;

    ClassFlowTakeImage();
    virtual ~ClassFlowTakeImage();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    time_t getTimeImageTaken();
    std::string getFileNameRawImage();

    ImageData *sendRawImage();
    esp_err_t sendRawJPG(httpd_req_t *req);

    std::string name() { return "ClassFlowTakeImage"; };
};


#endif // CLASSFFLOWTAKEIMAGE_H