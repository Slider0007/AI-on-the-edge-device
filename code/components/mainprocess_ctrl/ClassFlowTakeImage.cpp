#include "ClassFlowTakeImage.h"
#include "../../include/defines.h"

#include <time.h>

#include <esp_wifi.h>
#include <esp_log.h>

#include "configClass.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "CImage.h"
#include "CImageJpg.h"
#include "MainFlowControl.h"


static const char *TAG = "TAKEIMAGE";


ClassFlowTakeImage::ClassFlowTakeImage() : ClassLogImage(TAG)
{
    presetFlowStateHandler(true);
    timeImageTaken = 0;
    //  Init of ClassLogImage variables --> ClassLogImage.cpp
}


bool ClassFlowTakeImage::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionTakeImage;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    ConfigClass::getInstance()->get()->sectionOperationMode.useDemoImages ? cameraCtrl.enableDemoMode() : cameraCtrl.disableDemoMode();

    cameraCtrl.initFlashlight();

    cameraCtrl.setCameraParameter(&cfgDataPtr->camera);
    cameraCtrl.setFlashlightParameter(&cfgDataPtr->flashlight);

    int imgWidth = 640;
    int imgHeight = 480;
    cameraCtrl.getOutputFrameSize(imgWidth, imgHeight);

    flowImageData->imgProcess = new CImage("imgProcess", imgWidth, imgHeight, STBI_rgb, true);
    if (!flowImageData->imgProcess) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to initialize CImage imgProcess");
        return false;
    }

    flowImageData->imgVisu = new CImageJpg("imgVisu", IMAGE_JPG_MAX_SIZE);
    if (!flowImageData->imgVisu) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to initialize CImageJpg imgVisu");
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
#endif // DEBUG_DETAIL_ON

    if (!takeImage()) {
        setFlowStateHandlerEvent(-1); // Set error code for post cycle error handler 'doPostProcessEventHandling' (error level)
        return false;
    }

    flowImageData->imgProcess->saveJpgToContainer(flowImageData->imgVisu);

    if (cfgDataPtr->debug.saveAllFiles) {
        flowImageData->imgVisu->saveJpgToFile("/sdcard/img_tmp/raw.jpg");
    }

#ifdef DEBUG_DETAIL_ON
    LogFile.writeHeapInfo("ClassFlowTakeImage::doFlow - After takeImage");
#endif // DEBUG_DETAIL_ON

    logImage(logPath, "raw", CNNTYPE_NONE, -1, zwtime, flowImageData->imgProcess);

    removeOldLogs();

#ifdef DEBUG_DETAIL_ON
    LogFile.writeHeapInfo("ClassFlowTakeImage::doFlow - Done");
#endif // DEBUG_DETAIL_ON

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
    if (!flowImageData->imgProcess) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "takeImage: imgProcess not initialized");
        return false;
    }

    if (cameraCtrl.captureToBasisImage(flowImageData->imgProcess) != ESP_OK) {
        return false;
    }

    time(&timeImageTaken);

    return true;
}


ClassFlowTakeImage::~ClassFlowTakeImage()
{
    cameraCtrl.freeDemoMemoryOnly();

    delete flowImageData->imgProcess;
    delete flowImageData->imgVisu;
}
