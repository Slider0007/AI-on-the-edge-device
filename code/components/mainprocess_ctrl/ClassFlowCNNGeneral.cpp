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


static const char *TAG = "CNN";


ClassFlowCNNGeneral::ClassFlowCNNGeneral(ClassFlowAlignment *_flowalignment, const std::string _cnnname, const CNNType _cnntype)
    : ClassLogImage(TAG)
{
    flowalignment = _flowalignment;
    cnnname = _cnnname;
    cnnType = _cnntype;
    tflite = new CTfLiteClass;
    cnnmodelfile = "";
    modelxsize = 32;
    modelysize = 32;
    modelchannel = STBI_rgb;
    CNNGoodThreshold = 0.50;
    saveAllFiles = false;
    presetFlowStateHandler(true);
}


bool roiPositionPlausibilityCheck(RoiData *roiEl)
{
    // ROI position plausibilty check
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
    if (cnnname != "Digit" && cnnname != "Analog") {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown CNN class name");
        return false;
    }

    // Assign pointers based on cnnname
    const bool isDigit = (cnnname == "Digit");
    sectionDigitPtr = isDigit ? &ConfigClass::getInstance()->get()->sectionDigit : nullptr;
    sectionAnalogPtr = !isDigit ? &ConfigClass::getInstance()->get()->sectionAnalog : nullptr;

    cnnmodelfile = "/sdcard/config/models/" + (isDigit ? sectionDigitPtr->model : sectionAnalogPtr->model);
    CNNGoodThreshold = isDigit ? sectionDigitPtr->cnnGoodThreshold : 0.0;

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
        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (const auto &roi : roiList) {
            roi->imageRoiResized = new CImageBasis(roi->param->roiName, modelxsize, modelysize, modelchannel);
            roi->imageRoi = new CImageBasis(roi->param->roiName + "_org", roi->param->dx, roi->param->dy, STBI_rgb);
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
    CAlignAndCutImage *caic = flowalignment->getAlignAndCutImage();

    if (caic == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "doAlignAndCut: Aligned image not available");
        return false;
    }

    for (const auto &sequence : sequenceData) {
        const auto &roiList = sectionDigitPtr ? sequence->digitRoi : sequence->analogRoi;

        for (const auto &roi : roiList) {
            caic->cutAndSaveImage(roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, roi->imageRoi);

            if (saveAllFiles) {
                roi->imageRoi->saveToFile(formatFileName("/sdcard/img_tmp/" + roi->param->roiName + "_org.jpg"));
            }

            roi->imageRoi->resizeImage(modelxsize, modelysize, roi->imageRoiResized);

            if (saveAllFiles) {
                roi->imageRoiResized->saveToFile(formatFileName("/sdcard/img_tmp/" + roi->param->roiName + ".jpg"));
            }
        }
    }

    return true;
}


bool ClassFlowCNNGeneral::resolveNetworkParameter()
{
    if (!tflite->loadModel(formatFileName(cnnmodelfile))) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to load model: " + cnnmodelfile);
        LogFile.writeHeapInfo("resolveNetworkParameter-LoadModel");
        return false;
    }

    if (!tflite->makeAllocate()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Allocation of tensors failed");
        LogFile.writeHeapInfo("resolveNetworkParameter-MakeAllocate");
        return false;
    }

    if (cnnType == CNNTYPE_AUTODETECT) {
        if (!tflite->parseInputDimension()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to parse input dimensions from model");
            return false;
        }

        modelxsize = tflite->getInputDimension(0);
        modelysize = tflite->getInputDimension(1);
        modelchannel = tflite->getInputDimension(2);

        int outputDims = tflite->getOutputDimension();
        if (outputDims == -1) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLite: Failed to load output dimensions");
            return false;
        }

        switch (outputDims) {
            case 2:
                cnnType = CNNTYPE_ANALOG_CONT;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Analog (ana-cont)");
                break;

            case 10:
                cnnType = CNNTYPE_DIGIT_DOUBLE_HYBRID10;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Digit (dig-cont)");
                break;

            case 11:
                cnnType = CNNTYPE_DIGIT_CLASS11;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Digit (dig-class11)");
                break;

            case 100:
                cnnType = (modelxsize == 32 && modelysize == 32) ? CNNTYPE_ANALOG_CLASS100 : CNNTYPE_DIGIT_CLASS100;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                    "Type: " + std::string((cnnType == CNNTYPE_ANALOG_CLASS100) ? "Analog (ana-class100)"
                                                                                                : "Digit (dig-class100)"));
                break;

            default:
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Type does not fit the firmware (outputDims=" + std::to_string(outputDims) + ")");
                return false;
        }
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Network parameter loaded: " + cnnmodelfile);

    tflite->deleteInterpreter();

    return true;
}


bool ClassFlowCNNGeneral::doInvokeCnn(const std::string time)
{
    const std::string logPath = createLogFolder(time);

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
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + roiName);

            bool success = false;
            std::string modelTypeMsg;
            int logImageResult = 0;

            switch (cnnType) {
                case CNNTYPE_DIGIT_CLASS100:    // Models dig-class100*
                case CNNTYPE_ANALOG_CLASS100: { // Models ana-class100*
                    modelTypeMsg = "Type: " +
                                   std::string(cnnType == CNNTYPE_DIGIT_CLASS100 ? "Digit (dig-class100)" : "Analog (ana-class100)");
                    success = tflite->loadInputImage(roi->imageRoiResized) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    int num = tflite->getOutClassification();
                    roi->CNNResult = roi->param->ccw ? (100 - num) % 100 : num;
                    roi->CNNResult = std::clamp(roi->CNNResult, 0, 99);
                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);
                    logImageResult = roi->CNNResult;
                    break;
                }

                case CNNTYPE_DIGIT_DOUBLE_HYBRID10: { // Models dig-cont*
                    modelTypeMsg = "Type: Digit (dig-cont)";
                    success = tflite->loadInputImage(roi->imageRoiResized) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    int num = tflite->getOutClassification(0, 9);
                    int numplus = (num + 1) % 10;
                    int numminus = (num + 9) % 10;

                    float val = tflite->getOutputValue(num);
                    float valplus = tflite->getOutputValue(numplus);
                    float valminus = tflite->getOutputValue(numminus);

                    float result = num;
                    float fit;

                    if (valplus > valminus) {
                        result += valplus / (val + valplus);
                        fit = val + valplus;
                    }
                    else {
                        result -= valminus / (val + valminus);
                        fit = val + valminus;
                    }

                    result = fmod(result + 10, 10); // Normalize
                    roi->CNNResult = (int)std::clamp(result * 10.0f, 0.0f, 99.0f);
                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);

                    roi->isRejected = (fit < CNNGoodThreshold);
                    logImageResult = roi->isRejected ? -roi->CNNResult : roi->CNNResult;

#ifdef DEBUG_DETAIL_ON
                    std::string logData = "num (p, m): " + std::to_string(num) + " (" + std::to_string(numplus) + " , " +
                                          std::to_string(numminus) + "), val (p, m): " + std::to_string(val) + " (" +
                                          std::to_string(valplus) + " , " + std::to_string(valminus) + "), result: " + roi->sCNNResult +
                                          ", fit: " + std::to_string(fit);
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, logData);
#endif

                    if (roi->isRejected) {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                            "Result rejected - bad fit (Fit: " + std::to_string(fit) +
                                                ", Threshold: " + std::to_string(CNNGoodThreshold) + ")");
                    }

                    break;
                }

                case CNNTYPE_ANALOG_CONT: { // Models ana-cont*
                    modelTypeMsg = "Type: Analog (ana-cont)";
                    success = tflite->loadInputImage(roi->imageRoiResized) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    float result = fmod(atan2(tflite->getOutputValue(0), tflite->getOutputValue(1)) / (2 * M_PI) + 2, 1.0f);
                    float scaled = roi->param->ccw ? 100.0f - result * 100.0f : result * 100.0f;

                    roi->CNNResult = std::clamp(static_cast<int>(scaled), 0, 99);
                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);
                    logImageResult = roi->CNNResult;
                    break;
                }

                case CNNTYPE_DIGIT_CLASS11: { // Models dig-class11*
                    modelTypeMsg = "Type: Digit (dig-class11)";
                    success = tflite->loadInputImage(roi->imageRoiResized) && tflite->invoke();

                    if (!success) {
                        break;
                    }

                    roi->CNNResult = tflite->getOutClassification();
                    roi->sCNNResult = (roi->CNNResult == 10) ? "N" : std::to_string(roi->CNNResult);
                    logImageResult = roi->CNNResult;
                    break;
                }

                default:
                    break;
            }

            if (!success) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                return false;
            }

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, modelTypeMsg);
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Result: " + roi->sCNNResult);

            if (saveImagesEnabled) {
                logImage(logPath, roiName, cnnType, logImageResult, time, roi->imageRoi);
            }
        }
    }

    tflite->deleteInterpreter();
    return true;
}


std::string ClassFlowCNNGeneral::getReadout(SequenceData *sequence, int valuePreviousNumber, int resultPreviousNumber)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "getReadout: Number sequence: " + sequence->sequenceName +
                            ", extendedResolution: " + std::to_string(sequence->paramPostProc->extendedResolution) +
                            ", valuePreviousNumber: " + to_stringWithPrecision(valuePreviousNumber / 10.0, 1) +
                            ", resultPreviousNumber: " + std::to_string(resultPreviousNumber) +
                            ", analogDigitSyncValue: " + to_stringWithPrecision(sequence->paramPostProc->analogDigitSyncValue, 1));

    if (cnnType == CNNTYPE_ANALOG_CONT || cnnType == CNNTYPE_ANALOG_CLASS100) { // Class-analog-model, ana-class100-model
        if (sequence->analogRoi.size() == 0) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: No analog ROI in processed number sequence");
            return std::string("");
        }

        std::string result = "";
        int resultTemp = -1;
        int lastROI = sequence->analogRoi.size() - 1;

        // Evaluate last ROI of number sequence (and potentially correct)
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->analogRoi[lastROI]->param->roiName);
        resultTemp = evalAnalogNumber(sequence->analogRoi[lastROI]->CNNResult, -1);

        if (sequence->paramPostProc->extendedResolution) {
            int resultDecimalPlace = sequence->analogRoi[lastROI]->CNNResult % 10; // Decimal place of number result
            result = std::to_string(resultTemp) + std::to_string(resultDecimalPlace);
        }
        else {
            result = std::to_string(resultTemp);
        }

        // Evaluate all remaining ROI of number sequence (and potentially correct)
        for (int i = lastROI - 1; i >= 0; i--) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->analogRoi[i]->param->roiName);
            resultTemp = evalAnalogNumber(sequence->analogRoi[i]->CNNResult, resultTemp);
            result = std::to_string(resultTemp) + result;
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: Analog (ana-cont/ana-class100) Result: " + result);
        return result;
    }
    else if ((cnnType == CNNTYPE_DIGIT_DOUBLE_HYBRID10) || (cnnType == CNNTYPE_DIGIT_CLASS100)) { // dig-cont-model, dig-class100-model
        if (sequence->digitRoi.size() == 0) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: No digit ROI in processed number sequence");
            return std::string("");
        }

        std::string result = "";
        int resultTemp = -1;
        int lastROI = sequence->digitRoi.size() - 1;

        // Evaluate last ROI of number sequence (and potentially correct)
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->digitRoi[lastROI]->param->roiName);
        resultTemp = sequence->digitRoi[lastROI]->CNNResult;

        // Valid result (e.g. model 'dig-cont*' --> bad fit) or not used for other models (ensure isRejected is not set)
        if (!sequence->digitRoi[lastROI]->isRejected) {
            // NOTE: Ensure that this flag is only set if no analog previous number is available
            if (sequence->paramPostProc->extendedResolution && valuePreviousNumber == -1) {
                result = std::to_string(resultTemp);
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                    "Digit Number (No previous number, Extended Resolution): Result: " + result +
                                        ", Value: " + to_stringWithPrecision((resultTemp / 10.0), 1));
                resultTemp = resultTemp / 10; // resultIntergerPart to hand over a previous result to next digit evaluation
            }
            else {
                if (valuePreviousNumber >= 0) { // If previous number available (analog value should be handed over)
                    resultTemp = evalDigitNumber(sequence->digitRoi[lastROI]->CNNResult, valuePreviousNumber, resultPreviousNumber, true,
                                                 int(sequence->paramPostProc->analogDigitSyncValue * 10.0));
                }
                else {
                    resultTemp = evalDigitNumber(sequence->digitRoi[lastROI]->CNNResult, -1, -1); // No previous number
                }

                result = std::to_string(resultTemp);
            }
        }
        else {
            result = "N";
            if (sequence->paramPostProc->extendedResolution) {
                result = "NN";
            }

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont): Rejected, substitude with N");
        }

        // Evaluate all remaining ROI of number sequence (and potentially correct)
        for (int i = lastROI - 1; i >= 0; i--) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->digitRoi[i]->param->roiName);
            if (!sequence->digitRoi[i]->isRejected) { // valid result (e.g. model 'dig-cont*' --> bad fit)?
                resultTemp = evalDigitNumber(sequence->digitRoi[i]->CNNResult, sequence->digitRoi[i + 1]->CNNResult, resultTemp);
                result = std::to_string(resultTemp) + result;
            }
            else {
                resultTemp = -1;
                result = "N" + result;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont/dig-class100): Rejected, substitude with N");
            }
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont/dig-class100): Result: " + result);
        return result;
    }
    else if (cnnType == CNNTYPE_DIGIT_CLASS11) { // Class-11-model (1-0 + NaN)
        if (sequence->digitRoi.size() == 0) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: No digit ROI in processed number sequence");
            return std::string("");
        }

        std::string result = "";

        // Evaluate all ROI of number sequence (and potentially correct)
        for (int i = 0; i < sequence->digitRoi.size(); i++) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->digitRoi[i]->param->roiName);

            if (sequence->digitRoi[i]->CNNResult == 10) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: Digit (dig-class11): Result ambiguous, substitude with N");
                result = result + "N";
            }
            else {
                result = result + std::to_string(sequence->digitRoi[i]->CNNResult);
            }
        }
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: Digit (dig-class11) Result: " + result);
        return result;
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getReadout: CNN Type unknown");
        return std::string("");
    }
}


/* Evaluate analog number pointer */
int ClassFlowCNNGeneral::evalAnalogNumber(int _value, int _resultPreviousNumber)
{
    int result = -1;

    if (_resultPreviousNumber <= -1) {
        result = _value / 10; // Return IntegerPart, remove decimal place (73 -> 7)
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalAnalogNumber (No previous number): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1));
        return result;
    }

    int valueMax = _value + Analog_error;
    if (valueMax >= 100) { // e.g. 10.2 -> 0.2 (value = 02)
        valueMax = valueMax - 100;
    }

    int valueMin = _value - Analog_error;
    if (valueMin < 0) { // e.g. -0.3 -> 9.7 (value = 97)
        valueMin = 100 + valueMin;
    }

    if (((valueMin / 10 - valueMax / 10)) != 0) {
        if (_resultPreviousNumber <= Analog_error) {
            result = valueMax / 10;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "evalAnalogNumber (Ambiguous, use value + corretion): Result: " + std::to_string(result) +
                                    ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
        else if (_resultPreviousNumber >= 10 - Analog_error) {
            result = valueMin / 10;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "evalAnalogNumber (Ambiguous, use value - corretion): Result: " + std::to_string(result) +
                                    ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
    }

    result = _value / 10;
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "evalAnalogNumber (Unambiguous, use value): Result: " + std::to_string(result) + ", Value: " +
                            to_stringWithPrecision(_value / 10.0, 1) + ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    return result;
}


/* Evaluate digit number */
int ClassFlowCNNGeneral::evalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, bool _isPreviousAnalog,
                                         int digitalAnalogTransitionStart)
{
    int result = -1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    if (_resultPreviousNumber <= -1) { // no previous number -> no correction logic for transition needed, use value as is (integer part)
        result = resultIntergerPart;
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalDigitNumber (No previous number): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1));
        return result;
    }

    // previous number is analog (_valuePreviousNumber: 0-99), special transistion check needed
    if (_isPreviousAnalog) {
        result = evalAnalogToDigitTransition(_value, _valuePreviousNumber, _resultPreviousNumber, digitalAnalogTransitionStart);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalDigitNumber (Analog previous number): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // Previous number is digit (_valuePreviousNumber: 0-99) No digit change, because predecessor is far enough away (+/-
    // Digital_Transition_Area_Predecessor)
    if ((_valuePreviousNumber >= Digital_Transition_Area_Predecessor) &&
        (_valuePreviousNumber <= (100 - Digital_Transition_Area_Predecessor))) {
        // Band around the digit --> Round, as digit reaches inaccuracy in the frame
        if ((resultDecimalPlace <= DigitalBand) || (resultDecimalPlace >= (10 - DigitalBand))) {
            if (resultDecimalPlace >= 5) {
                result = resultIntergerPart + 1; // "Round"

                if (result >= 10) {
                    result = 0;
                }
            }
            else {
                result = resultIntergerPart; // "Trunc"
            }
        }
        else {
            result = resultIntergerPart; // "Trunc"
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalDigitNumber (Digit previous number: no zero crossing, \'safe area\'): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // Zero crossing at the predecessor has taken place (! evaluation via result of previous number and not value !)
    // --> round up here (2.8 --> 3, but also 3.1 --> 3)
    if (_resultPreviousNumber <= 1) {
        // We simply assume that the current digit after the zero crossing of the predecessor
        // has passed through at least half (x.5)
        if (resultDecimalPlace >= 5) {
            result = resultIntergerPart + 1; // "Round": The current digit does not yet have a zero crossing, but the predecessor does..

            if (result >= 10) {
                result = 0;
            }
        }
        else {
            result = resultIntergerPart; // "Trunc": Act. digit and predecessor have zero crossing
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalDigitNumber (Digit previous number: zero crossing): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // remains only >= 9.x --> no zero crossing yet --> 2.8 --> 2,
    // and from 9.7(Digital_Transition_Area_Forward) 3.1 --> 2
    // everything >=x.4 can be considered as current number in transition. With 9.x predecessor the current
    // number can still be x.6 - x.7.
    // Preceding (else - branch) does not already happen from 9.
    if (((_valuePreviousNumber <= Digital_Transition_Area_Forward) && (_resultPreviousNumber == (int)(_valuePreviousNumber / 10.0))) ||
        resultDecimalPlace >= 4) {
        result = resultIntergerPart; // The current digit, like the previous digit, does not yet have a zero crossing.
    }
    else {
        // current digit precedes the smaller digit (9.x). So already >=x.0 while the previous digit has not yet
        // has no zero crossing. Therefore, it is reduced by 1.
        result = resultIntergerPart - 1;

        if (result < 0) {
            result = 9;
        }
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "evalDigitNumber (Digit previous number: no zero crossing yet): Result: " + std::to_string(result) +
                            ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                            ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                            ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));

    return result;
}


/* Evaluate analog to digit number transition */
int ClassFlowCNNGeneral::evalAnalogToDigitTransition(int _value, int _valuePreviousNumber, int _resultPreviousNumber,
                                                     int analogDigitSyncValue)
{
    int result = -1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    // Value within the digit inequalities
    if (resultDecimalPlace >= (10 - Digital_Uncertainty) // Band around the zero crossing -> Round, as number reaches inaccuracy zone
        ||
        (_resultPreviousNumber <= 4 && resultDecimalPlace >= 6)) // or number runs after (previous result <= 4, actucal decimal place >= 6)
    {
        if (resultDecimalPlace >= 5) { // "Round up"
            result = resultIntergerPart + 1;

            if (result >= 10) {
                result = 0;
            }

            // Correct back if no zero crossing detected
            // analogDigitSyncValue < _valuePreviousNumber < 0.2
            if (_resultPreviousNumber >= 6 && (_valuePreviousNumber > analogDigitSyncValue || _valuePreviousNumber <= 2)) {
                result = result - 1;
                if (result < 0) {
                    result = 9;
                }

                LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                    "evalAnalogToDigitTransition (Digit uncertainty, no zero crossing): Result: " + std::to_string(result) +
                                        ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                        ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            }
        }
        else {
            result = resultIntergerPart; // "Trunc -> Round down"
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalAnalogToDigitTransition (Digit uncertainty): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1) +
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber / 10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    }
    else {
        result = resultIntergerPart;
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "evalAnalogToDigitTransition: Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value / 10.0, 1));
    }

    return result;
}


bool ClassFlowCNNGeneral::cnnTypeAllowExtendedResolution() const
{
    return cnnType != CNNTYPE_DIGIT_CLASS11;
}


void ClassFlowCNNGeneral::drawROI(CImageBasis *image)
{
    if (!image->imageOkay()) {
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
                image->drawEllipse(roi->param->x + roi->param->dx / 2, roi->param->y + roi->param->dy / 2, roi->param->dx / 2,
                                   roi->param->dy / 2, color[0], color[1], color[2], 2);
                image->drawLine(roi->param->x + roi->param->dx / 2, roi->param->y, roi->param->x + roi->param->dx / 2,
                                roi->param->y + roi->param->dy, color[0], color[1], color[2], 2);
                image->drawLine(roi->param->x, roi->param->y + roi->param->dy / 2, roi->param->x + roi->param->dx,
                                roi->param->y + roi->param->dy / 2, color[0], color[1], color[2], 2);
            }
            else {
                image->drawRect(roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, color[0], color[1], color[2], 2);
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
