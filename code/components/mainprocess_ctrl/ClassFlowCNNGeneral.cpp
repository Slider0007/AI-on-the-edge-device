#include "ClassFlowCNNGeneral.h"
#include "../../include/defines.h"

#include <math.h>
#include <iomanip>
#include <sys/types.h>
#include <sstream> // std::stringstream

#include <esp_log.h>

#include "configClass.h"
#include "ClassLogFile.h"
#include "ClassControlCamera.h"
#include "CImageMod.h"


static const char *TAG = "CNN";


ClassFlowCNNGeneral::ClassFlowCNNGeneral(std::string _cnnName, CNNType _cnnType) : ClassLogImage(TAG)
{
    cnnName = _cnnName;
    cnnType = _cnnType;
    tflite = new CTfLiteClass;
    modelWidth = 32;
    modelHeight = 32;
    modelChannel = STBI_rgb;
    saveAllFiles = false;
    presetFlowStateHandler(true);
}


bool roiPositionPlausibilityCheck(RoiData *roiEl)
{
    // ROI position plausibility check
    int imgWidth = CAMERA_OUTPUT_WINDOW_SIZE_WIDTH;
    int imgHeight = CAMERA_OUTPUT_WINDOW_SIZE_HEIGHT;
    cameraCtrl.getOutputFrameSize(imgWidth, imgHeight);

    if (roiEl->param->x < 1 || (roiEl->param->x > (imgWidth - 1 - roiEl->param->dx))) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more ROI out of image area (x). Check ROI config");
        return false;
    }

    if (roiEl->param->y < 1 || (roiEl->param->y > (imgHeight - 1 - roiEl->param->dy))) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "One or more ROI out of image area (y). Check ROI config");
        return false;
    }

    return true;
}


bool ClassFlowCNNGeneral::loadParameter()
{
    if (cnnName != "Digit" && cnnName != "Analog") {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown CNN class name");
        return false;
    }

    // Assign pointers based on cnnName
    const bool isDigit = (cnnName == "Digit");
    sectionDigitPtr = isDigit ? &ConfigClass::getInstance()->get()->sectionDigit : nullptr;
    sectionAnalogPtr = !isDigit ? &ConfigClass::getInstance()->get()->sectionAnalog : nullptr;

    cnnModelFile = "/sdcard/config/models/" + (isDigit ? sectionDigitPtr->model : sectionAnalogPtr->model);

    saveImagesEnabled = isDigit ? sectionDigitPtr->debug.saveRoiImages : sectionAnalogPtr->debug.saveRoiImages;
    imagesLocation = "/sdcard" + (isDigit ? sectionDigitPtr->debug.roiImagesLocation : sectionAnalogPtr->debug.roiImagesLocation);
    imagesRetention = isDigit ? sectionDigitPtr->debug.roiImagesRetention : sectionAnalogPtr->debug.roiImagesRetention;
    saveAllFiles = isDigit ? sectionDigitPtr->debug.saveAllFiles : sectionAnalogPtr->debug.saveAllFiles;

    const auto &sequences = isDigit ? sectionDigitPtr->sequence : sectionAnalogPtr->sequence;
    for (size_t i = 0; i < sequences.size(); i++) {
        if (i >= sequenceData.size()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid sequence index");
            return false;
        }

        for (size_t j = 0; j < sequences[i].roi.size(); j++) {
            auto *roiEl = new RoiData{};
            roiEl->param = &sequences[i].roi[j];

            if (!roiPositionPlausibilityCheck(roiEl)) {
                return false;
            }

            if (isDigit) {
                sequenceData[i]->digitRoi.push_back(roiEl);
            }
            else {
                sequenceData[i]->analogRoi.push_back(roiEl);
            }
        }
    }

    if (!resolveNetworkParameter()) {
        return false;
    }

    for (const auto &sequence : sequenceData) {
        // Save sequence length
        sequence->sequenceLength = sequence->digitRoi.size() + sequence->analogRoi.size();

        // Allocate ROI images
        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (const auto &roi : roiList) {
            roi->imageRoiResized = new CImage(roi->param->roiName, modelWidth, modelHeight, modelChannel);
            roi->imageRoi = new CImage(roi->param->roiName + "_org", roi->param->dx, roi->param->dy, STBI_rgb);
        }
    }

    return true;
}


bool ClassFlowCNNGeneral::doFlow(std::string time)
{
    presetFlowStateHandler(false, time);

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Process ROI extraction");
    if (!doExtractRoi(time)) {
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Process neural network");
    if (!doInvokeCnn(time)) {
        return false;
    }

    removeOldLogs();

    return getFlowState()->isSuccessful;
}


void ClassFlowCNNGeneral::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
}


bool ClassFlowCNNGeneral::doExtractRoi(const std::string time)
{
    for (const auto &sequence : sequenceData) {
        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (const auto &roi : roiList) {
            CImageMod::crop(*flowImageData->imgProcess, roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, *roi->imageRoi);

            if (saveAllFiles) {
                roi->imageRoi->saveJpgToFile(formatFileName("/sdcard/img_tmp/" + roi->param->roiName + "_org.jpg"));
            }

            CImageMod::resize(*roi->imageRoi, modelWidth, modelHeight, *roi->imageRoiResized);

            if (saveAllFiles) {
                roi->imageRoiResized->saveJpgToFile(formatFileName("/sdcard/img_tmp/" + roi->param->roiName + ".jpg"));
            }
        }
    }

    return true;
}


bool ClassFlowCNNGeneral::resolveNetworkParameter()
{
    if (!tflite->loadModel(formatFileName(cnnModelFile))) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to load model: " + cnnModelFile);
        LogFile.writeHeapInfo("resolveNetworkParameter-LoadModel");
        return false;
    }

    if (!tflite->makeAllocate()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Allocation of tensors failed");
        LogFile.writeHeapInfo("resolveNetworkParameter-MakeAllocate");
        return false;
    }

    if (!tflite->parseInputDimension()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to parse input dimensions from model");
        return false;
    }

    modelWidth = tflite->getInputDimension(0);
    modelHeight = tflite->getInputDimension(1);
    modelChannel = tflite->getInputDimension(2);

    int outputDims = tflite->getOutputDimension();
    if (outputDims == -1) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to load output dimensions");
        return false;
    }

    // Auto-detect model type
    if (cnnType == CNNTYPE_AUTODETECT) {
        switch (outputDims) {
            case 100:
                if (cnnModelFile.contains("dig-class100")) {
                    cnnType = CNNTYPE_DIGIT_CLASS100;
                }
                else if (cnnModelFile.contains("ana-class100")) {
                    cnnType = CNNTYPE_ANALOG_CLASS100;
                }
                else {
                    cnnType = CNNTYPE_DIGIT_CLASS100;
                    LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                        "Model not officially supported. Fallback to 100 class raw logits output processing");
                }
                break;

            case 11:
                cnnType = CNNTYPE_NONE;
                LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                    "Model type \'dig-class11\' is not supported anymore. Please select another model type");
                return false;

            case 10:
                if (cnnModelFile.contains("dig-cont")) {
                    cnnType = CNNTYPE_DIGIT_DOUBLE_HYBRID10;
                }
                else {
                    cnnType = CNNTYPE_DIGIT_DOUBLE_HYBRID10;
                    LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                        "Model not officially supported. Fallback to 10 class raw logits output processing");
                }
                break;

            case 2:
                if (cnnModelFile.contains("ana-cont")) {
                    cnnType = CNNTYPE_ANALOG_CONT;
                }
                else {
                    cnnType = CNNTYPE_NONE;
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown model type (Output dimension: " + std::to_string(outputDims) + ")");
                    return false;
                }
                break;

            default:
                cnnType = CNNTYPE_NONE;
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown model type (Output dimension: " + std::to_string(outputDims) + ")");
                return false;
        }
    }

    // Print CNN type
    switch (cnnType) {
        case CNNTYPE_DIGIT_CLASS100:
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Type: Digit Rolling / LCD | 100 classes (dig-class100)");
            break;
        case CNNTYPE_ANALOG_CLASS100:
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Type: Analog Dial | 100 classes (ana-class100)");
            break;
        case CNNTYPE_DIGIT_DOUBLE_HYBRID10:
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Type: Digit Rolling / LCD | 10/100 classes (dig-cont)");
            break;
        case CNNTYPE_ANALOG_CONT:
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Type: Analog Dial | 2 classes (ana-cont)");
            break;
        default:
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown model type (Output dimension: " + std::to_string(outputDims) + ")");
            return false;
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Network parameter loaded: " + cnnModelFile);

    tflite->deleteInterpreter();

    return true;
}


bool ClassFlowCNNGeneral::doInvokeCnn(const std::string time)
{
    const auto roiSavingSize = sectionDigitPtr ? sectionDigitPtr->debug.roiSavingSize : sectionAnalogPtr->debug.roiSavingSize;
    const std::string logPath = createLogFolder(time, roiSavingSize == ROI_SAVE_FULL_SIZE_AND_RESIZED);

    if (!tflite->makeAllocate()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Allocation of TFLite tensors failed");
        LogFile.writeHeapInfo("invokeCnn-MakeAllocate");
        return false;
    }

    for (const auto &sequence : sequenceData) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Processing number sequence: " + sequence->sequenceName);

        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (const auto &roi : roiList) {
            const std::string &roiName = roi->param->roiName;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Processing ROI: " + roiName);

            bool success = false;

            switch (cnnType) {
                case CNNTYPE_DIGIT_CLASS100:
                case CNNTYPE_ANALOG_CLASS100: {
                    success = tflite->loadInputImage(*(roi->imageRoiResized)) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    const int classIdx = tflite->getHighestScoringClass();
                    roi->CNNResult = std::clamp(roi->param->ccw ? (100 - classIdx) % 100 : classIdx, 0, 99);
                    roi->CNNResultConfidence = tflite->getOutputValue(classIdx, true);

                    break;
                }

                case CNNTYPE_DIGIT_DOUBLE_HYBRID10: { // jomjol model: dig-cont*
                    success = tflite->loadInputImage(*(roi->imageRoiResized)) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    // Helper for circular arithmetic (handles negative steps)
                    auto circularIncrement = [](int num, int step, int max) -> int { return (num + step + max) % max; };

                    constexpr int maxClass = 10;
                    const int classIdx = tflite->getHighestScoringClass(0, maxClass - 1);

                    // Neighbor indices
                    const int nextClass = circularIncrement(classIdx, 1, maxClass);
                    const int prevClass = circularIncrement(classIdx, -1, maxClass);

                    // Fetch output values
                    const float val = tflite->getOutputValue(classIdx);
                    const float valNext = tflite->getOutputValue(nextClass);
                    const float valPrev = tflite->getOutputValue(prevClass);

                    // Weighted circular adjustment
                    const float adjustment = (valNext - valPrev) / (val + std::max(valNext, valPrev));
                    float result = static_cast<float>(classIdx) + adjustment;
                    result = fmod(result + 10, 10);

                    roi->CNNResult = std::clamp((int)(roundf(result * 10.0f)), 0, 99); // Normalize to 0-99
                    roi->CNNResultConfidence = tflite->getOutputValue(classIdx, true);

                    break;
                }

                case CNNTYPE_ANALOG_CONT: { // jomjol model: ana-cont*
                    success = tflite->loadInputImage(*(roi->imageRoiResized)) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    const int result =
                        (int)roundf(fmod(atan2(tflite->getOutputValue(0), tflite->getOutputValue(1)) / (2 * M_PI) + 1.0f, 1.0f) * 100.0f);
                    roi->CNNResult = std::clamp(roi->param->ccw ? (100 - result) % 100 : result, 0, 99);
                    roi->CNNResultConfidence = -1.0f; // No confidence available

                    break;
                }

                default:
                    break;
            }

            // Abort if any error occured
            if (!success) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                tflite->deleteInterpreter();
                return false;
            }

            // Store result as string in floating-point interpretation
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%d.%d", roi->CNNResult / 10, roi->CNNResult % 10);
            roi->sCNNResult = std::string(buffer);

            // Log results
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "Result: " + roi->sCNNResult + ", Confidence: " +
                                    (roi->CNNResultConfidence > -1.0f ? to_stringWithPrecision(roi->CNNResultConfidence, 2) : "N/A"));

            // Save images
            if (saveImagesEnabled) {
                const std::string roiNameResized = roiName + "_resized";

                if (roiSavingSize == ROI_SAVE_FULL_SIZE_AND_RESIZED) {
                    logImage(logPath, roiName, cnnType, roi->CNNResult, time, roi->imageRoi, 100);
                    logImage(logPath + "/resized", roiNameResized, cnnType, roi->CNNResult, time, roi->imageRoiResized, 100);
                }
                else if (roiSavingSize == ROI_SAVE_RESIZED) { // Special case: Save to default log path for backward compatibility
                    logImage(logPath, roiNameResized, cnnType, roi->CNNResult, time, roi->imageRoiResized, 100);
                }
                else {
                    logImage(logPath, roiName, cnnType, roi->CNNResult, time, roi->imageRoi, 100);
                }
            }
        }
    }

    tflite->deleteInterpreter();

    return true;
}


void ClassFlowCNNGeneral::drawROI(CImage &image)
{
    if (!image.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawROI: Invalid image");
        return;
    }

    static const std::vector<std::array<int, 3>> colors = {
        {0, 255, 0},   // Green
        {0, 0, 255},   // Blue
        {0, 255, 255}, // Cyan
        {255, 0, 255}, // Pink
        {255, 255, 0}  // Yellow
    };

    int colorIndex = 0;
    for (const auto &sequence : sequenceData) {
        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;
        const auto &color = colors[colorIndex % colors.size()];

        for (const auto &roi : roiList) {
            if (cnnType == CNNTYPE_ANALOG_CLASS100 || cnnType == CNNTYPE_ANALOG_CONT) {
                const int centerX = roi->param->x + roi->param->dx / 2;
                const int centerY = roi->param->y + roi->param->dy / 2;

                CImageMod::drawEllipse(image, centerX, centerY, roi->param->dx / 2, roi->param->dy / 2, color[0], color[1], color[2], 2);
                CImageMod::drawLine(image, centerX, roi->param->y, centerX, roi->param->y + roi->param->dy, color[0], color[1], color[2],
                                    1);
                CImageMod::drawLine(image, roi->param->x, centerY, roi->param->x + roi->param->dx, centerY, color[0], color[1], color[2],
                                    1);
            }
            else {
                CImageMod::drawRect(image, roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, color[0], color[1], color[2], 2);
            }
        }
        colorIndex++;
    }
}


ClassFlowCNNGeneral::~ClassFlowCNNGeneral()
{
    delete tflite;
    tflite = nullptr;

    for (const auto &sequence : sequenceData) {
        auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (auto &roi : roiList) {
            delete roi->imageRoiResized;
            delete roi->imageRoi;
            roi->imageRoiResized = nullptr;
            roi->imageRoi = nullptr;
        }

        roiList.clear();
        std::vector<RoiData *>().swap(roiList); // Force deallocation
    }
}
