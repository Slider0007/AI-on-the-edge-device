#ifndef CLASSFFLOWTAKEIMAGE_H
#define CLASSFFLOWTAKEIMAGE_H

#include <string>

#include "ClassFlowImage.h"
#include "ClassControllCamera.h"


class ClassFlowTakeImage : public ClassFlowImage
{
    protected:
        bool SaveAllFiles;
        time_t timeImageTaken;
        std::string namerawimage;
        int image_height, image_width;

        void SetInitialParameter();
        bool takePictureWithFlash();

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