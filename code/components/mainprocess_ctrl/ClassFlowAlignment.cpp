#include "ClassFlowAlignment.h"
#include "../../include/defines.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <esp_log.h>

#include "ClassFlowTakeImage.h"
#include "ClassFlow.h"
#include "MainFlowControl.h"
#include "time_sntp.h"
#include "CRotateImage.h"
#include "ClassLogFile.h"
#include "psram.h"


static const char *TAG = "ALIGN";


ClassFlowAlignment::ClassFlowAlignment()
{
    presetFlowStateHandler(true);
    alignFastSADThreshold = 10;  // FAST ALIGN ALGO: SADNorm -> if smaller than threshold use same alignment values as last cycle
    alignAndCutImage = NULL;
    useAntialiasing = false; // @TODO: Remove or check

    ImageBasis = flowctrl.getRawImage();
    imageTemp = NULL;
    AlgROI = (ImageData*)heap_caps_malloc(sizeof(ImageData), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


bool ClassFlowAlignment::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionImageAlignment;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    // Configure two alignemnt marker
    for (int i = 0; i < 2; i++) {
        int x = 0, y = 0, channel = 0;
        std::string sIndex = std::to_string(i+1);

        alignmentMarker[i].alignment_algo = cfgDataPtr->alignmentAlgo;
        alignmentMarker[i].search_x = cfgDataPtr->searchField.x;
        alignmentMarker[i].search_y = cfgDataPtr->searchField.y;
        alignmentMarker[i].fastalg_SADThreshold = alignFastSADThreshold;


        alignmentMarker[i].image_file = "/sdcard/config/marker" + sIndex + ".jpg";
        stbi_info(alignmentMarker[i].image_file.c_str(), &x, &y, &channel);

        alignmentMarker[i].refImage = new CImageBasis("marker" + sIndex);
        if (alignmentMarker[i].refImage) {
            if(!alignmentMarker[i].refImage->createEmptyImage(x, y, channel, 1)) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create alignment marker image");
                return false;
            }
        }
        STBIObjectPSRAM.name = "marker" + sIndex;
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.PreallocatedMemory = alignmentMarker[i].refImage->getRgbImage();
        STBIObjectPSRAM.PreallocatedMemorySize = alignmentMarker[i].refImage->getMemsize();

        if (!alignmentMarker[i].refImage->loadFromFilePreallocated("marker" + sIndex, alignmentMarker[i].image_file.c_str())) {
            return false;
        }

        alignmentMarker[i].target_x = cfgDataPtr->marker[i].x;
        alignmentMarker[i].target_y = cfgDataPtr->marker[i].y;

        // ROI position plausibilty check - Check Flip Image Size
        int img_width = cameraCtrl.image_width;
        int img_height = cameraCtrl.image_height;
        if (cfgDataPtr->flipImageSize) {
            img_width = cameraCtrl.image_height;
            img_height = cameraCtrl.image_width;
        }

        if (alignmentMarker[i].target_x < 1 || (alignmentMarker[i].target_x > (img_width - 1 - alignmentMarker[i].refImage->width))) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more alignment marker out of image area (x). Check alignment marker");
            return false;
        }

        if (alignmentMarker[i].target_y < 1 || (alignmentMarker[i].target_y > (img_height - 1 - alignmentMarker[i].refImage->height))) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more alignment marker out of image area (y). Check alignment marker");
            return false;
        }
    }

    if (cfgDataPtr->alignmentAlgo == ALIGNALGO_FAST) // Load AlignmentMarker if "fast" algo is used
        loadAlignmentMarkerData();

    return true;
}


bool ClassFlowAlignment::doFlow(std::string time)
{
    presetFlowStateHandler(false, time);
    if (AlgROI == NULL) { // AlgROI needs to be allocated before imageTemp to avoid heap fragmentation
        AlgROI = (ImageData*)heap_caps_realloc(AlgROI, sizeof(ImageData), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (AlgROI == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate AlgROI");
            LogFile.writeHeapInfo("ClassFlowAlignment-doFlow");
        }
    }

    if (AlgROI) {
        ImageBasis->writeToMemoryAsJPG((ImageData*)AlgROI, 90);
    }

    if (imageTemp == NULL) {
        imageTemp = new CImageBasis("imageTemp", ImageBasis, 1);
        if (imageTemp == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate imageTemp");
            LogFile.writeHeapInfo("ClassFlowAlignment-doFlow");
            return false;
        }
    }

    delete alignAndCutImage;
    alignAndCutImage = new CAlignAndCutImage("AlignAndCutImage", ImageBasis, imageTemp);
    if (alignAndCutImage == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate AlignAndCutImage");
        LogFile.writeHeapInfo("ClassFlowAlignment-doFlow");
        return false;
    }

    CRotateImage rt("rawImageRT", alignAndCutImage, imageTemp, cfgDataPtr->flipImageSize);
    if (cfgDataPtr->flipImageSize) {
        int _zw = ImageBasis->height;
        ImageBasis->height = ImageBasis->width;
        ImageBasis->width = _zw;

        _zw = imageTemp->width;
        imageTemp->width = imageTemp->height;
        imageTemp->height = _zw;
    }

    float rotation = cfgDataPtr->initialRotation;
    if ((rotation != 0) || cfgDataPtr->flipImageSize) {
        if (alignmentMarker[0].alignment_algo == ALIGNALGO_OFF)  // alignment off: no initial rotation and no additional alignment algo
            rotation = 0.0;

        if (useAntialiasing)
            rt.rotateImageAntiAliasing(rotation);
        else
            rt.rotateImage(rotation);

        if (cfgDataPtr->debug.saveAllFiles)
            alignAndCutImage->saveToFile(formatFileName("/sdcard/img_tmp/rot.jpg"));
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Initial rotation: " + to_stringWithPrecision(rotation, 1));

    if(alignmentMarker[0].alignment_algo <= ALIGNALGO_FAST) { // Only if any additional alignment algo is used: "default", "highaccuracy" or "fast"
        int AlignRetval = alignAndCutImage->alignImage(&alignmentMarker[0], &alignmentMarker[1]);

        if (AlignRetval >= 0) {
            saveAlignmentMarkerData();
        }
        else if (AlignRetval == -1) {   // Alignment failed
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Alignment by algorithm failed. Verify image rotation and alignment marker");
            setFlowStateHandlerEvent(-1); // Set error event code for post cycle error handler 'doPostProcessEventHandling'
        }
    }

    if (AlgROI) {
        if(alignmentMarker[0].alignment_algo <= ALIGNALGO_FAST) { // Only if any additional alignment algo is used: "default", "highaccuracy" or "fast"
            drawAlignmentMarker(imageTemp);
        }
        if (getFlowState()->isSuccessful) {
            flowctrl.drawDigitRoi(imageTemp);
            flowctrl.drawAnalogRoi(imageTemp);
        }
        imageTemp->writeToMemoryAsJPG((ImageData*)AlgROI, 90);
    }

    if (cfgDataPtr->debug.saveAllFiles) {
        alignAndCutImage->saveToFile(formatFileName("/sdcard/img_tmp/alg.jpg"));
        imageTemp->saveToFile(formatFileName("/sdcard/img_tmp/alg_roi.jpg"));
    }

    // must be deleted to have memory space for loading tflite
    delete imageTemp;
    imageTemp = NULL;

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


void ClassFlowAlignment::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    for (int i = 0; i < getFlowState()->EventCode.size(); i++) {
        if (cfgDataPtr->debug.saveDebugInfo && getFlowState()->EventCode[i] == -1) {  // If saving error logs enabled and alignment failed event
            time_t actualtime;
            time(&actualtime);

            // Define path, e.g. /sdcard/log/debug/20230814/20230814-125528/ClassFlowAlignment
            std::string destination = std::string(LOG_DEBUG_ROOT_FOLDER) + "/" + getFlowState()->ExecutionTime.DEFAULT_TIME_FORMAT_DATE_EXTR + "/" +
                                        getFlowState()->ExecutionTime + "/" + getFlowState()->ClassName;

            if (!makeDir(destination))
                return;

            // Save algo results in file
            std::string resultFileName = "/alignment_failed.txt";
            FILE* fpResult = fopen((destination + resultFileName).c_str(), "w");
            fwrite(alignmentMarker[0].error_details.c_str(), (alignmentMarker[0].error_details).length(), 1, fpResult);
            fclose(fpResult);

            // Draw alignment marker and save image
            drawAlignmentMarker(alignAndCutImage);
            alignAndCutImage->saveToFile(formatFileName(destination + "/alg_misalign.jpg"));

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

    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_x", alignmentMarker[0].fastalg_x);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_y", alignmentMarker[0].fastalg_y);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_x", alignmentMarker[1].fastalg_x);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_y", alignmentMarker[1].fastalg_y);
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
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND ) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_x", (int32_t*)&alignmentMarker[0].fastalg_x);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_y", (int32_t*)&alignmentMarker[0].fastalg_y);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_x", (int32_t*)&alignmentMarker[1].fastalg_x);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_y", (int32_t*)&alignmentMarker[1].fastalg_y);
        if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    nvs_close(align_nvshandle);

    return true;
}


std::string ClassFlowAlignment::getHTMLSingleStep(std::string host)
{
    std::string result;

    result =          "<p>Rotated Image: </p> <p><img src=\"" + host + "/img_tmp/rot.jpg\"></p>\n";
    result = result + "<p>Found Alignment: </p> <p><img src=\"" + host + "/img_tmp/rot_roi.jpg\"></p>\n";
    result = result + "<p>Aligned Image: </p> <p><img src=\"" + host + "/img_tmp/alg.jpg\"></p>\n";
    return result;
}


bool ClassFlowAlignment::getFlipImageSize()
{
    if (cfgDataPtr->flipImageSize)
        return true;

    return false;
}


void ClassFlowAlignment::drawAlignmentMarker(CImageBasis *image)
{
    if (!image->imageOkay())
        return;

    image->drawRect(alignmentMarker[0].target_x, alignmentMarker[0].target_y, alignmentMarker[0].width, alignmentMarker[0].height, 255, 51, 51, 2);
    image->drawRect(alignmentMarker[1].target_x, alignmentMarker[1].target_y, alignmentMarker[1].width, alignmentMarker[1].height, 255, 51, 51, 2);
}


ClassFlowAlignment::~ClassFlowAlignment()
{
    free_psram_heap("AlgROI", AlgROI);
    delete alignmentMarker[0].refImage;
    delete alignmentMarker[1].refImage;
    delete imageTemp;
    delete alignAndCutImage;
}
