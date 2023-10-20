#include "ClassFlowTakeImage.h"
#include "Helper.h"
#include "ClassLogFile.h"

#include "CImageBasis.h"
#include "ClassControllCamera.h"
#include "MainFlowControl.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "../../include/defines.h"

#include <time.h>

// #define DEBUG_DETAIL_ON 

static const char* TAG = "TAKEIMAGE";


void ClassFlowTakeImage::SetInitialParameter(void)
{
    presetFlowStateHandler(true);

    disabled = false;
    rawImage = NULL;
    namerawimage = "/sdcard/img_tmp/raw.jpg";
    waitbeforepicture = 2.0; // Flash duration in s
    flash_duration = (int)(waitbeforepicture * 1000); // Flash duration in ms
    isImageSize = false;
    ImageSize = FRAMESIZE_VGA;
    TimeImageTaken = 0;
    CameraFrequency = 20; // Mhz
    ImageQuality = 12;
    brightness = 0;
    contrast = 0;
    saturation = 0;
    FixedExposure = false;
    SaveAllFiles = false;
}     


ClassFlowTakeImage::ClassFlowTakeImage(std::vector<ClassFlow*>* lfc) : ClassFlowImage(lfc, TAG)
{
    SetInitialParameter();
    // Init of ClassFlowImage variables --> ClassFlowImage.cpp
}


bool ClassFlowTakeImage::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;
    int ledintensity = 50;

    aktparamgraph = trim(aktparamgraph);
    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (aktparamgraph.compare("[TakeImage]") != 0)       // Paragraph does not fit TakeImage
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        splitted = ZerlegeZeile(aktparamgraph);
        if ((toUpper(splitted[0]) ==  "RAWIMAGESLOCATION") && (splitted.size() > 1)) {
            imagesLocation = "/sdcard" + splitted[1];
            isLogImage = true;
        }

        if ((toUpper(splitted[0]) == "RAWIMAGESRETENTION") && (splitted.size() > 1)) {
            this->imagesRetention = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "WAITBEFORETAKINGPICTURE") && (splitted.size() > 1)) {
            waitbeforepicture = stof(splitted[1]);
            flash_duration = (int)(waitbeforepicture * 1000);
        }

        if ((toUpper(splitted[0]) == "CAMERAFREQUENCY") && (splitted.size() > 1)) {
            CameraFrequency = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "IMAGEQUALITY") && (splitted.size() > 1)) {
            ImageQuality = std::stod(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "IMAGESIZE") && (splitted.size() > 1)) {
            ImageSize = Camera.TextToFramesize(splitted[1].c_str());
            isImageSize = true;
        }

        if ((toUpper(splitted[0]) == "LEDINTENSITY") && (splitted.size() > 1)) {
            ledintensity = stoi(splitted[1]);
            ledintensity = std::min(100, ledintensity);
            ledintensity = std::max(0, ledintensity);
        }

        if ((toUpper(splitted[0]) == "BRIGHTNESS") && (splitted.size() > 1)) {
            brightness = stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "CONTRAST") && (splitted.size() > 1)) {
            contrast = stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "SATURATION") && (splitted.size() > 1)) {
            saturation = stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "FIXEDEXPOSURE") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                FixedExposure = true;
            else
                FixedExposure = false;
        }

        if ((toUpper(splitted[0]) == "DEMO") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                Camera.EnableDemoMode();
            else
                Camera.DisableDemoMode();
        }

        if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                SaveAllFiles = true;
            else
                SaveAllFiles = false;
        }  
    }

    Camera.ledc_init(); // PWM init needs to be done here due to parameter reload (camera class not to be deleted completely)
    Camera.SetLEDIntensity(ledintensity);
    Camera.SetCameraFrequency(CameraFrequency);
    Camera.SetQualitySize(ImageQuality, ImageSize);
    Camera.SetBrightnessContrastSaturation(brightness, contrast, saturation);

    image_width = Camera.image_width;
    image_height = Camera.image_height;
    rawImage = new CImageBasis("rawImage");
    if (rawImage) {
        if(!rawImage->CreateEmptyImage(image_width, image_height, 3, 1)) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create rawimage");
            return false;
        }
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ReadParameter: Can't create CImageBasis for rawImage");
        return false;
    }

    if (FixedExposure && (flash_duration > 0)) {
        //ESP_LOGD(TAG, "Fixed Exposure enabled");
        Camera.EnableAutoExposure(flash_duration);
    }

    return true;
}


bool ClassFlowTakeImage::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);
    std::string logPath = CreateLogFolder(zwtime);
 
    #ifdef DEBUG_DETAIL_ON  
        LogFile.WriteHeapInfo("ClassFlowTakeImage::doFlow - Start");
    #endif

    if (!takePictureWithFlash(flash_duration)) {
        setFlowStateHandlerEvent(-1); // Set error code for post cycle error handler 'doPostProcessEventHandling' (error level)
        return false;
    }

    #ifdef DEBUG_DETAIL_ON  
        LogFile.WriteHeapInfo("ClassFlowTakeImage::doFlow - After takePictureWithFlash");
    #endif

    LogImage(logPath, "raw", None, -1, zwtime, rawImage);

    RemoveOldLogs();

    #ifdef DEBUG_DETAIL_ON  
        LogFile.WriteHeapInfo("ClassFlowTakeImage::doFlow - Done");
    #endif

    return true;
}


void ClassFlowTakeImage::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    for (int i = 0; i < getFlowState()->EventCode.size(); i++) {
        if (getFlowState()->EventCode[i] == -1) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Camera init or framebuffer access failed, reinit camera");
            Camera.DeinitCam(); // Reinit will be done here: MainFlowControl.cpp -> DoInit()
            setTaskAutoFlowState(FLOW_TASK_STATE_INIT); // Do fully init process to avoid SPIRAM fragmentation
        }
    }
}


std::string ClassFlowTakeImage::getHTMLSingleStep(std::string host)
{
    std::string result = "Raw Image: <br>\n<img src=\"" + host + "/img_tmp/raw.jpg\">\n";
    return result;
}


esp_err_t ClassFlowTakeImage::camera_capture()
{
    std::string nm =  namerawimage;

    if (Camera.CaptureToFile(nm) != ESP_OK)
        return ESP_FAIL;

    time(&TimeImageTaken);

    return ESP_OK;
}


bool ClassFlowTakeImage::takePictureWithFlash(int _flash_duration)
{
    if (rawImage == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "takePictureWithFlash: rawImage not initialized");
        return false;
    }

    // in case the image is flipped, it must be reset here //
    rawImage->width = image_width;          
    rawImage->height = image_height;

    ESP_LOGD(TAG, "flash_duration: %d", _flash_duration);
    if (Camera.CaptureToBasisImage(rawImage, _flash_duration) != ESP_OK)
        return false;

    time(&TimeImageTaken);

    if (SaveAllFiles)
        rawImage->SaveToFile(namerawimage);

    return true;
}


esp_err_t ClassFlowTakeImage::SendRawJPG(httpd_req_t *req)
{
    time(&TimeImageTaken);

    return Camera.CaptureToHTTP(req, flash_duration);
}


ImageData* ClassFlowTakeImage::SendRawImage()
{
    CImageBasis *zw = new CImageBasis("SendRawImage", rawImage);
    ImageData *id;

    if (Camera.CaptureToBasisImage(rawImage, flash_duration) != ESP_OK)
        return NULL;

    time(&TimeImageTaken);

    id = zw->writeToMemoryAsJPG();    
    delete zw;
    return id;  
}


time_t ClassFlowTakeImage::getTimeImageTaken()
{
    return TimeImageTaken;
}


std::string ClassFlowTakeImage::getFileNameRawImage(void)
{
    return namerawimage;
}


ClassFlowTakeImage::~ClassFlowTakeImage()
{
    delete rawImage;
}
