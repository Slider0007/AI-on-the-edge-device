#ifndef CLASSFFLOWTAKEIMAGE_H
#define CLASSFFLOWTAKEIMAGE_H

#include <string>

#include "ClassFlowImage.h"
#include "ClassControllCamera.h"


class ClassFlowTakeImage : public ClassFlowImage
{
    protected:
        time_t TimeImageTaken;
        std::string namerawimage;
        float waitbeforepicture;
        int flash_duration;
        framesize_t ImageSize;
        bool isImageSize;
        int CameraFrequency;
        int ImageQuality;
        int brightness;
        int contrast;
        int saturation;
        int image_height, image_width;
        bool SaveAllFiles;
        bool FixedExposure;

        void SetInitialParameter(void);    
        esp_err_t camera_capture();
        bool takePictureWithFlash(int flash_duration);

    public:
        CImageBasis *rawImage;

        ClassFlowTakeImage(std::vector<ClassFlow*>* lfc);
        virtual ~ClassFlowTakeImage();

        bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
        bool doFlow(std::string time);
        std::string getHTMLSingleStep(std::string host);
        time_t getTimeImageTaken();
        std::string getFileNameRawImage();
        void doPostProcessEventHandling();
        std::string name() {return "ClassFlowTakeImage";};

        ImageData* SendRawImage();
        esp_err_t SendRawJPG(httpd_req_t *req);
};


#endif //CLASSFFLOWTAKEIMAGE_H