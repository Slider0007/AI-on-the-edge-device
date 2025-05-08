#include "ClassFlowAlignment.h"
#include "../../include/defines.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <esp_log.h>

#include "CImage.h"
#include "CImageMod.h"
#include "CImageTplMatch.h"
#include "ClassFlowTakeImage.h"
#include "ClassLogFile.h"
#include "MainFlowControl.h"
#include "time_sntp.h"
#include "psram.h"


static const char *TAG = "ALIGN";


ClassFlowAlignment::ClassFlowAlignment()
{
    presetFlowStateHandler(true);
    alignSimilarityCheckSADThreshold = 10; // Alignment image template similarity check threshold
                                           // If result smaller than threshold use alignment values of last cycle
}


bool ClassFlowAlignment::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionImageAlignment;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    // Configure two alignemnt marker
    if (cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH ||
        cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_3CH ||
        cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH_SIMILAR) {
        for (int i = 0; i < 2; i++) {
            int x = 0, y = 0, channels = 0;
            std::string sIndex = std::to_string(i + 1);

            // Check availability of marker image before usage
            if (!fileExists("/sdcard/config/marker" + sIndex + ".jpg")) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "Alignmant marker image missing: '/sdcard/config/marker" + sIndex +
                                        ".jpg' > Please update alignment marker");
                return false;
            }

            alignmentMarker[i].alignmentAlgo = cfgDataPtr->alignmentAlgo;
            alignmentMarker[i].searchX = cfgDataPtr->searchField.x;
            alignmentMarker[i].searchY = cfgDataPtr->searchField.y;
            alignmentMarker[i].similarityCheckSADThreshold = alignSimilarityCheckSADThreshold;

            alignmentMarker[i].markerImageFilename = "/sdcard/config/marker" + sIndex + ".jpg";
            stbi_info(alignmentMarker[i].markerImageFilename.c_str(), &x, &y, &channels);

            alignmentMarker[i].markerImage = new CImage("marker" + sIndex, x, y, channels, true);
            if (!alignmentMarker[i].markerImage) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create alignment marker image");
                return false;
            }

            if (alignmentMarker[i].markerImage->loadJpgFromFile(alignmentMarker[i].markerImageFilename.c_str(), true) != ESP_OK) {
                return false;
            }

            alignmentMarker[i].targetX = cfgDataPtr->marker[i].x;
            alignmentMarker[i].targetY = cfgDataPtr->marker[i].y;
            alignmentMarker[i].width = alignmentMarker[i].markerImage->getWidth();
            alignmentMarker[i].height = alignmentMarker[i].markerImage->getHeight();

            // ROI position plausibilty check
            int imgWidth = 640;
            int imgHeight = 480;
            cameraCtrl.getOutputFrameSize(imgWidth, imgHeight);

            if (alignmentMarker[i].targetX < 1 || (alignmentMarker[i].targetX > (imgWidth - 1 - alignmentMarker[i].width))) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more alignment marker out of image area (x). Check alignment marker");
                return false;
            }

            if (alignmentMarker[i].targetY < 1 || (alignmentMarker[i].targetY > (imgHeight - 1 - alignmentMarker[i].height))) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more alignment marker out of image area (y). Check alignment marker");
                return false;
            }
        }
    }

    if (cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH_SIMILAR) { // Load alignment marker if "similarity check" is enabled
        loadAlignmentMarkerData();
    }

    return true;
}


bool ClassFlowAlignment::doFlow(std::string time)
{
    presetFlowStateHandler(false, time);
    if (!flowImageData->imgProcess) {
        return false;
    }

    CImage imgAlgRoi(*flowImageData->imgProcess);
    if (!imgAlgRoi.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create imgAlgRoi");
        return false;
    }

    imgAlgRoi.setName("imgAlgRoi");

    // 1. Perform basic rotation
    // *******************************************
    float rotation = cfgDataPtr->imageRotation;
    if (rotation != 0.0f && cfgDataPtr->alignmentAlgo != ALIGNALGO_OFF) {
        CImageMod::rotate(*flowImageData->imgProcess, rotation, imgAlgRoi, true);

        if (cfgDataPtr->debug.saveAllFiles) {
            flowImageData->imgProcess->saveJpgToFile(formatFileName("/sdcard/img_tmp/rot.jpg"));
        }
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Initial rotation: " + to_stringWithPrecision(rotation, 1));

    // 2. Perform alignment algorythm (template match)
    // Note: Only if any additional alignment algo is configured
    // *******************************************
    if (cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH ||
        cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_3CH ||
        cfgDataPtr->alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH_SIMILAR) {
        TplMatchStatus AlignRetval = CImageTplMatch::invokeTplMatch(*flowImageData->imgProcess, imgAlgRoi, alignmentMarker[0],
                                                                    alignmentMarker[1]);
        if (AlignRetval == TPL_MATCH_OK_SIMILAR) { // Alignment with similarity check successful
            saveAlignmentMarkerData();
        }
        else if (AlignRetval == TPL_MATCH_FAILED) { // Alignment failed
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Alignment by algorithm failed. Verify image rotation and alignment marker");
            setFlowStateHandlerEvent(-1); // Set error event code for post cycle error handler 'doPostProcessEventHandling'
        }

        drawAlignmentMarker(imgAlgRoi);
    }

    if (getFlowState()->isSuccessful) {
        flowctrl.drawDigitRoi(imgAlgRoi);
        flowctrl.drawAnalogRoi(imgAlgRoi);
    }

    // 3. Save aligned image with overlays
    // *******************************************
    imgAlgRoi.saveJpgToContainer(flowImageData->imgVisu);

    if (cfgDataPtr->debug.saveAllFiles) {
        flowImageData->imgVisu->saveJpgToFile(formatFileName("/sdcard/img_tmp/alg_roi.jpg"));
    }

    if (!getFlowState()->isSuccessful) {
        return false;
    }

    return true;
}


void ClassFlowAlignment::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    for (int i = 0; i < getFlowState()->EventCode.size(); i++) {
        if (cfgDataPtr->debug.saveDebugInfo && getFlowState()->EventCode[i] == -1) { // If saving error logs enabled and alignment failed
                                                                                     // event
            time_t actualtime;
            time(&actualtime);

            // Define path, e.g. /sdcard/log/debug/20230814/20230814-125528/ClassFlowAlignment
            std::string destination = std::string(LOG_DEBUG_ROOT_FOLDER) + "/" +
                                      getFlowState()->ExecutionTime.DEFAULT_TIME_FORMAT_DATE_EXTR + "/" + getFlowState()->ExecutionTime +
                                      "/" + getFlowState()->ClassName;

            if (!makeDir(destination)) {
                return;
            }

            // Save algo results in file
            std::string resultFileName = "/alignment_failed.txt";
            FILE *fpResult = fopen((destination + resultFileName).c_str(), "w");
            fwrite(alignmentMarker[0].errorMsg.c_str(), (alignmentMarker[0].errorMsg).length(), 1, fpResult);
            fclose(fpResult);

            // Draw alignment marker and save image
            drawAlignmentMarker(*flowImageData->imgProcess);
            flowImageData->imgProcess->saveJpgToFile(formatFileName(destination + "/alg_misalign.jpg"));

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Alignment failed, debug infos saved: " + destination);
        }
    }
}


bool ClassFlowAlignment::saveAlignmentMarkerData()
{
    esp_err_t err = ESP_OK;

    nvs_handle_t align_nvshandle;
    err = nvs_open("align", NVS_READWRITE, &align_nvshandle);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_x", alignmentMarker[0].similarityCheckX);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_y", alignmentMarker[0].similarityCheckY);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_x", alignmentMarker[1].similarityCheckX);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_y", alignmentMarker[1].similarityCheckY);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref1fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_commit(align_nvshandle);
    nvs_close(align_nvshandle);

    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: nvs_commit - error code: " + std::to_string(err));
        return false;
    }

    return true;
}


bool ClassFlowAlignment::loadAlignmentMarkerData(void)
{
    esp_err_t err = ESP_OK;

    nvs_handle_t align_nvshandle;
    err = nvs_open("align", NVS_READONLY, &align_nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_x", (int32_t *)&alignmentMarker[0].similarityCheckX);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_y", (int32_t *)&alignmentMarker[0].similarityCheckY);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_x", (int32_t *)&alignmentMarker[1].similarityCheckX);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_y", (int32_t *)&alignmentMarker[1].similarityCheckY);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    nvs_close(align_nvshandle);

    return true;
}


void ClassFlowAlignment::drawAlignmentMarker(CImage &image)
{
    if (!image.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawAlignmentMarker: Invalid image");
        return;
    }

    CImageMod::drawRect(image, alignmentMarker[0].targetX, alignmentMarker[0].targetY, alignmentMarker[0].width, alignmentMarker[0].height,
                        255, 51, 51, 2);
    CImageMod::drawRect(image, alignmentMarker[1].targetX, alignmentMarker[1].targetY, alignmentMarker[1].width, alignmentMarker[1].height,
                        255, 51, 51, 2);
}


ClassFlowAlignment::~ClassFlowAlignment()
{
    for (int i = 0; i < 2; ++i) {
        delete alignmentMarker[i].markerImage;
    }
}
