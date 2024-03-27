#include "ClassFlowTakeImage.h"
#include "../../include/defines.h"

#include <time.h>

#include "esp_wifi.h"
#include "esp_log.h"

#include "Helper.h"
#include "ClassLogFile.h"
#include "CImageBasis.h"
#include "MainFlowControl.h"

// #define DEBUG_DETAIL_ON 

static const char* TAG = "TAKEIMAGE";


void ClassFlowTakeImage::SetInitialParameter(void)
{
    presetFlowStateHandler(true);

    disabled = false;
    SaveAllFiles = false;

    timeImageTaken = 0;
    namerawimage = "/sdcard/img_tmp/raw.jpg";
    rawImage = NULL;
}     


ClassFlowTakeImage::ClassFlowTakeImage(std::vector<ClassFlow*>* lfc) : ClassFlowImage(lfc, TAG)
{
    SetInitialParameter();
    // Init of ClassFlowImage variables --> ClassFlowImage.cpp
}


bool ClassFlowTakeImage::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    int flashTime = 2000; // FlashTime in ms
    int flashIntensity = 50;
    int cameraFrequency = 20; // Mhz
    framesize_t imageSize = FRAMESIZE_VGA;
    int imageQuality = 12;
    int brightness = 0;
    int contrast = 0;
    int saturation = 0;
    int sharpness = 0;
    int exposureControlMode = 1;
    int autoExposureLevel = 0;
    int manualExposureValue = 300;
    int gainControlMode = 1;
    int manualGainValue = 0;
    int specialEffect = 0;
    bool mirrorImage = false;
    bool flipImage = false;
    int zoomMode = 0;
    int zoomOffsetX = 0;
    int zoomOffsetY = 0;

    std::vector<std::string> splitted;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (aktparamgraph.compare("[TakeImage]") != 0)       // Paragraph does not fit TakeImage
        return false;

    while (getNextLine(pfile, &aktparamgraph) && !isNewParagraph(aktparamgraph)) {
        splitted = ZerlegeZeile(aktparamgraph);

        if ((toUpper(splitted[0]) ==  "RAWIMAGESLOCATION") && (splitted.size() > 1)) {
            imagesLocation = "/sdcard" + splitted[1];
            isLogImage = true;
        }

        if ((toUpper(splitted[0]) == "RAWIMAGESRETENTION") && (splitted.size() > 1)) {
            imagesRetention = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "FLASHTIME") && (splitted.size() > 1)) {
            flashTime = (int)(stof(splitted[1]) * 1000); // Flashtime in ms
        }

        if ((toUpper(splitted[0]) == "FLASHINTENSITY") && (splitted.size() > 1)) {
            flashIntensity = std::max(0, std::min(stoi(splitted[1]), 100));
        }

        if ((toUpper(splitted[0]) == "CAMERAFREQUENCY") && (splitted.size() > 1)) {
            cameraFrequency = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "IMAGEQUALITY") && (splitted.size() > 1)) {
            imageQuality = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "IMAGESIZE") && (splitted.size() > 1)) {
            imageSize = Camera.textToFramesize(splitted[1].c_str());
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

        if ((toUpper(splitted[0]) == "SHARPNESS") && (splitted.size() > 1)) {
            sharpness = stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "EXPOSURECONTROLMODE") && (splitted.size() > 1)) {
            exposureControlMode = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "AUTOEXPOSURELEVEL") && (splitted.size() > 1)) {
            autoExposureLevel = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "MANUALEXPOSUREVALUE") && (splitted.size() > 1)) {
            manualExposureValue = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "GAINCONTROLMODE") && (splitted.size() > 1)) {
            gainControlMode = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "MANUALGAINVALUE") && (splitted.size() > 1)) {
            manualGainValue = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "SPECIALEFFECT") && (splitted.size() > 1)) {
            specialEffect = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "MIRRORIMAGE") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                mirrorImage = true;
            else
                mirrorImage = false;
        }

        if ((toUpper(splitted[0]) == "FLIPIMAGE") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                flipImage = true;
            else
                flipImage = false;
        }

        if ((toUpper(splitted[0]) == "ZOOMMODE") && (splitted.size() > 1)) {
            zoomMode = std::stoi(splitted[1]);
        }
        
        if ((toUpper(splitted[0]) == "ZOOMOFFSETX") && (splitted.size() > 1)) {
            zoomOffsetX = std::stoi(splitted[1]);
        }
        
        if ((toUpper(splitted[0]) == "ZOOMOFFSETY") && (splitted.size() > 1)) {
            zoomOffsetY = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "DEMO") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                Camera.enableDemoMode();
            else
                Camera.disableDemoMode();
        }

        if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                SaveAllFiles = true;
            else
                SaveAllFiles = false;
        }  
    }

    #ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    Camera.ledcInitFlashlightDefault(); // PWM init needs to be done here due to parameter reload (camera class not to be deleted completely)
    #endif
    Camera.setFlashIntensity(flashIntensity);
    Camera.setFlashTime(flashTime);
    Camera.setCameraFrequency(cameraFrequency);
    Camera.setSizeQuality(imageQuality, imageSize, zoomMode, zoomOffsetX, zoomOffsetY);
    Camera.setImageManipulation(brightness, contrast, saturation, sharpness, exposureControlMode, autoExposureLevel, 
                                manualExposureValue, gainControlMode, manualGainValue, specialEffect, mirrorImage, flipImage);
    
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

    return true;
}


bool ClassFlowTakeImage::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);
    std::string logPath = CreateLogFolder(zwtime);
 
    #ifdef DEBUG_DETAIL_ON  
        LogFile.WriteHeapInfo("ClassFlowTakeImage::doFlow - Start");
    #endif

    if (!takePictureWithFlash()) {
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
            Camera.deinitCam(); // Reinit will be done here: MainFlowControl.cpp -> DoInit()
            setTaskAutoFlowState(FLOW_TASK_STATE_INIT); // Do fully init process to avoid SPIRAM fragmentation
        }
    }
}


std::string ClassFlowTakeImage::getHTMLSingleStep(std::string host)
{
    std::string result = "Raw Image: <br>\n<img src=\"" + host + "/img_tmp/raw.jpg\">\n";
    return result;
}


bool ClassFlowTakeImage::takePictureWithFlash()
{
    if (rawImage == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "takePictureWithFlash: rawImage not initialized");
        return false;
    }

    // in case the image is flipped, it must be reset here //
    rawImage->width = image_width;          
    rawImage->height = image_height;

    if (Camera.captureToBasisImage(rawImage) != ESP_OK)
        return false;

    time(&timeImageTaken);

    if (SaveAllFiles)
        rawImage->SaveToFile(namerawimage);

    return true;
}


esp_err_t ClassFlowTakeImage::SendRawJPG(httpd_req_t *req)
{
    time(&timeImageTaken);

    return Camera.captureToHTTP(req);   // Capture with configured flash time
}


ImageData* ClassFlowTakeImage::SendRawImage()
{
    CImageBasis *zw = new CImageBasis("SendRawImage", rawImage);
    ImageData *id;

    if (Camera.captureToBasisImage(rawImage) != ESP_OK)
        return NULL;

    time(&timeImageTaken);

    id = zw->writeToMemoryAsJPG();    
    delete zw;
    return id;  
}


time_t ClassFlowTakeImage::getTimeImageTaken()
{
    return timeImageTaken;
}


std::string ClassFlowTakeImage::getFileNameRawImage(void)
{
    return namerawimage;
}


ClassFlowTakeImage::~ClassFlowTakeImage()
{
    delete rawImage;
}
