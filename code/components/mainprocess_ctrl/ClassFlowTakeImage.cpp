#include "ClassFlowTakeImage.h"
#include "../../include/defines.h"

#include <time.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include "configClass.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "CImageBasis.h"
#include "MainFlowControl.h"


static const char *TAG = "TAKEIMAGE";


ClassFlowTakeImage::ClassFlowTakeImage() : ClassLogImage(TAG)
{
    presetFlowStateHandler(true);
    timeImageTaken = 0;
    rawImageFilename = "/sdcard/img_tmp/raw.jpg";
    rawImage = NULL;
    // Init of ClassLogImage variables --> ClassLogImage.cpp
}


bool ClassFlowTakeImage::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionTakeImage;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    ConfigClass::getInstance()->get()->sectionOperationMode.useDemoImages ? cameraCtrl.enableDemoMode() : cameraCtrl.disableDemoMode();

#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    cameraCtrl.ledcInitFlashlightDefault(); // PWM init needs to be done here due to parameter reload
                                            // (Camera class not to be deleted completely)
#endif

    cameraCtrl.setCameraParameter(&cfgDataPtr->camera);
    cameraCtrl.setFlashlightParameter(&cfgDataPtr->flashlight);

    int imgWidth = 640;
    int imgHeight = 480;
    cameraCtrl.getOutputFrameSize(imgWidth, imgHeight);

    rawImage = new CImageBasis("rawImage");
    if (rawImage) {
        if (!rawImage->createEmptyImage(imgWidth, imgHeight, STBI_rgb, 1)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create rawimage");
            return false;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ReadParameter: Can't create CImageBasis for rawImage");
        return false;
    }

    // Parameter used in ClassLogImage
    saveImagesEnabled = cfgDataPtr->debug.saveRawImages;
    imagesLocation = "/sdcard" + cfgDataPtr->debug.rawImagesLocation;
    imagesRetention = cfgDataPtr->debug.rawImagesRetention;

    return true;
}


bool ClassFlowTakeImage::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);
    std::string logPath = createLogFolder(zwtime);

#ifdef DEBUG_DETAIL_ON
    LogFile.writeHeapInfo("ClassFlowTakeImage::doFlow - Start");
#endif

    if (!takeImage()) {
        setFlowStateHandlerEvent(-1); // Set error code for post cycle error handler 'doPostProcessEventHandling' (error level)
        return false;
    }

#ifdef DEBUG_DETAIL_ON
    LogFile.writeHeapInfo("ClassFlowTakeImage::doFlow - After takeImage");
#endif

    logImage(logPath, "raw", CNNTYPE_NONE, -1, zwtime, rawImage);

    removeOldLogs();

#ifdef DEBUG_DETAIL_ON
    LogFile.writeHeapInfo("ClassFlowTakeImage::doFlow - Done");
#endif

    return true;
}


void ClassFlowTakeImage::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    for (int i = 0; i < getFlowState()->EventCode.size(); i++) {
        if (getFlowState()->EventCode[i] == -1) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Camera init or framebuffer access failed, reinit camera");
            cameraCtrl.deinitCam();                     // Reinit will be done here: MainFlowControl.cpp -> DoInit()
            setTaskAutoFlowState(FLOW_TASK_STATE_INIT); // Do fully init process to avoid SPIRAM fragmentation
        }
    }
}


bool ClassFlowTakeImage::takeImage()
{
    if (rawImage == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "takeImage: rawImage not initialized");
        return false;
    }

    if (cameraCtrl.captureToBasisImage(rawImage) != ESP_OK) {
        return false;
    }

    time(&timeImageTaken);

    if (cfgDataPtr->debug.saveAllFiles) {
        rawImage->saveToFile(rawImageFilename);
    }

    return true;
}


esp_err_t ClassFlowTakeImage::sendRawJPG(httpd_req_t *req)
{
    time(&timeImageTaken);

    return cameraCtrl.captureToHTTP(req); // Capture with configured flash time
}


ImageData *ClassFlowTakeImage::sendRawImage()
{
    CImageBasis *imgTmp = new CImageBasis("sendRawImage", rawImage);
    ImageData *id = NULL;

    if (cameraCtrl.captureToBasisImage(rawImage) != ESP_OK) {
        return NULL;
    }

    time(&timeImageTaken);

    if (imgTmp != NULL) {
        id = imgTmp->writeToMemoryAsJPG();
        delete imgTmp;
    }

    return id;
}


time_t ClassFlowTakeImage::getTimeImageTaken()
{
    return timeImageTaken;
}


ClassFlowTakeImage::~ClassFlowTakeImage()
{
    delete rawImage;
}
