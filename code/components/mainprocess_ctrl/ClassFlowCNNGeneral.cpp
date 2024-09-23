#include "ClassFlowCNNGeneral.h"
#include "../../include/defines.h"

#include <math.h>
#include <iomanip>
#include <sys/types.h>
#include <sstream>      // std::stringstream

#include <esp_log.h>

#include "configClass.h"
#include "ClassLogFile.h"
#include "ClassControlCamera.h"


static const char* TAG = "CNN";


ClassFlowCNNGeneral::ClassFlowCNNGeneral(ClassFlowAlignment *_flowalignment, std::string _cnnname, CNNType _cnntype) : ClassLogImage(TAG)
{
    presetFlowStateHandler(true);
    flowalignment = _flowalignment;
    cnnname = _cnnname;
    cnnType = _cnntype;
    tflite = new CTfLiteClass;
    cnnmodelfile = "";
    modelxsize = 32;
    modelysize = 32;
    modelchannel = STBI_rgb;
    CNNGoodThreshold = 0.0;
    saveAllFiles = false;
}


bool roiPositionPlausibilityCheck(RoiData *roiEl)
{
    // ROI position plausibilty check - Check Flip Image Size
    int imgWidth = cameraCtrl.image_width;
    int imgHeight = cameraCtrl.image_height;

    if (ConfigClass::getInstance()->get()->sectionImageAlignment.flipImageSize) {
        imgWidth = cameraCtrl.image_height;
        imgHeight = cameraCtrl.image_width;
    }

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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unkown CNN class name");
        return false;
    }

    // Prepare internal sequence data struct (class instance related helper)
    for (const auto &sequence : sequenceData) {
        SequenceDataInternal *sequenceInternal = new SequenceDataInternal{};
        sequenceInternal->sequenceId = sequence->sequenceId;
        sequenceInternal->sequenceName = sequence->sequenceName;
        sequenceDataInternal.push_back(sequenceInternal);
    }

    if (cnnname == "Digit") {
        cnnmodelfile = "/sdcard/config/models/" + ConfigClass::getInstance()->get()->sectionDigit.model;
        CNNGoodThreshold = ConfigClass::getInstance()->get()->sectionDigit.cnnGoodThreshold;

        saveImagesEnabled = ConfigClass::getInstance()->get()->sectionDigit.debug.saveRoiImages;
        imagesLocation = "/sdcard" + ConfigClass::getInstance()->get()->sectionDigit.debug.roiImagesLocation;
        imagesRetention = ConfigClass::getInstance()->get()->sectionDigit.debug.roiImagesRetention;
        saveAllFiles = ConfigClass::getInstance()->get()->sectionDigit.debug.saveAllFiles;

        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionDigit.sequence.size(); i++) {
            for (int j = 0; j < ConfigClass::getInstance()->get()->sectionDigit.sequence[i].roi.size(); j++) {
                RoiData *roiEl = new RoiData{};
                roiEl->param = &ConfigClass::getInstance()->get()->sectionDigit.sequence[i].roi[j];

                if (!roiPositionPlausibilityCheck(roiEl))
                    return false;

                if (i < sequenceData.size()) {
                    sequenceData[i]->digitRoi.push_back(roiEl);
                    // Fill internal struct as well to make class internal processing independent of source (digit or analog)
                    sequenceDataInternal[i]->roiData.push_back(roiEl);
                }
            }
        }

    }
    else if (cnnname == "Analog") {
        cnnmodelfile = "/sdcard/config/models/" + ConfigClass::getInstance()->get()->sectionAnalog.model;

        saveImagesEnabled = ConfigClass::getInstance()->get()->sectionAnalog.debug.saveRoiImages;
        imagesLocation = "/sdcard" + ConfigClass::getInstance()->get()->sectionAnalog.debug.roiImagesLocation;
        imagesRetention = ConfigClass::getInstance()->get()->sectionAnalog.debug.roiImagesRetention;
        saveAllFiles = ConfigClass::getInstance()->get()->sectionDigit.debug.saveAllFiles;

        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionAnalog.sequence.size(); i++) {
            for (int j = 0; j < ConfigClass::getInstance()->get()->sectionAnalog.sequence[i].roi.size(); j++) {
                RoiData *roiEl = new RoiData{};
                roiEl->param = &ConfigClass::getInstance()->get()->sectionAnalog.sequence[i].roi[j];

                if (!roiPositionPlausibilityCheck(roiEl))
                    return false;

                if (i < sequenceData.size()) {
                    sequenceData[i]->analogRoi.push_back(roiEl);
                    // Fill internal struct as well to make class internal processing independent of source (digit or analog)
                    sequenceDataInternal[i]->roiData.push_back(roiEl);
                }
            }
        }
    }

    if (!resolveNetworkParameter())
        return false;

    for (int i = 0; auto &sequence : sequenceDataInternal) {
        for (int j = 0; auto &roi : sequence->roiData) {
            roi->imageRoiResized = new CImageBasis(roi->param->roiName, modelxsize, modelysize, modelchannel);
            roi->imageRoi = new CImageBasis(roi->param->roiName + "_org", roi->param->dx, roi->param->dy, STBI_rgb);

            // Set image pointer in global struct as well
            if (cnnname == "Digit") {
                sequenceData[i]->digitRoi[j]->imageRoiResized = roi->imageRoiResized;
                sequenceData[i]->digitRoi[j]->imageRoi = roi->imageRoi;
            }
            else if (cnnname == "Analog") {
                sequenceData[i]->analogRoi[j]->imageRoiResized = roi->imageRoiResized;
                sequenceData[i]->analogRoi[j]->imageRoi = roi->imageRoi;
            }
            j++;
        }
        i++;
    }

    return true;
}


bool ClassFlowCNNGeneral::doFlow(std::string time)
{
    presetFlowStateHandler(false, time);

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Process ROI extraction");
    if (!doAlignAndCut(time))
        return false;

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Process neural network");
    if (!doNeuralNetwork(time))
        return false;

    removeOldLogs();

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


void ClassFlowCNNGeneral::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed

}


bool ClassFlowCNNGeneral::doAlignAndCut(std::string time)
{
    CAlignAndCutImage *caic = flowalignment->getAlignAndCutImage();

    if (caic == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "doAlignAndCut: Aligned image not available");
        return false;
    }

    for (auto &sequence : sequenceDataInternal) {
        for (auto &roi : sequence->roiData) {
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
    if (!tflite->LoadModel(formatFileName(cnnmodelfile))) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load model: " + cnnmodelfile);
        LogFile.writeHeapInfo("getNetworkParameter-LoadModel");
        return false;
    }

    if (!tflite->MakeAllocate()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLITE: Allocation of tensors failed");
        LogFile.writeHeapInfo("getNetworkParameter-MakeAllocate");
        return false;
    }

    if (cnnType == CNNTYPE_AUTODETECT) {
        if (tflite->GetInputDimension(false)) {
            modelxsize = tflite->ReadInputDimenstion(0);
            modelysize = tflite->ReadInputDimenstion(1);
            modelchannel = tflite->ReadInputDimenstion(2);
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load input dimensions");
            return false;
        }
        int _anzoutputdimensions = tflite->GetAnzOutPut();
        switch (_anzoutputdimensions)
        {
            case -1:
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load output dimensions");
                return false;
            case 2:
                cnnType = CNNTYPE_ANALOG_CONT;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Analog - Cont");
                break;
            case 10:
                cnnType = CNNTYPE_DIGIT_DOUBLE_HYBRID10;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Digit - DoubleHyprid10");
                break;
            case 11:
                cnnType = CNNTYPE_DIGIT_CLASS11;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Digit - Class11");
                break;
             case 100:
                if (modelxsize == 32 && modelysize == 32) {
                    cnnType = CNNTYPE_ANALOG_CLASS100;
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Analog - Class100");
                } else {
                    cnnType = CNNTYPE_DIGIT_CLASS100;
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Digit - Class100");
                }
                break;
            default:
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CNN-Type does not fit the firmware (output_dimension=" + std::to_string(_anzoutputdimensions) + ")");
                return false;
        }
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Network parameter loaded: " + cnnmodelfile);

    tflite->CTfLiteClassDeleteInterpreter();

    return true;
}


bool ClassFlowCNNGeneral::doNeuralNetwork(std::string time)
{
    std::string logPath = createLogFolder(time);

    if (!tflite->MakeAllocate()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Allocation of TFLITE tensors failed");
        LogFile.writeHeapInfo("doNeuralNetwork-MakeAllocate");
        return false;
    }

    for (auto &sequence : sequenceDataInternal) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Processing number sequence: " + sequence->sequenceName);
        for (auto &roi : sequence->roiData) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + roi->param->roiName);

            switch (cnnType) {
                case CNNTYPE_DIGIT_CLASS11: // for models dig-class11*
                {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Digit (dig-class11)");

                    roi->CNNResult = tflite->GetClassFromImageBasis(roi->imageRoiResized); // 0-9 + 10 => NaN

                    if (roi->CNNResult == 10) {
                        roi->sCNNResult = "N";
                    }
                    else {
                        roi->sCNNResult = std::to_string(roi->CNNResult);
                    }

                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Result: " + roi->sCNNResult);

                    if (saveImagesEnabled) {
                        logImage(logPath, roi->param->roiName, CNNTYPE_DIGIT_CLASS11, roi->CNNResult, time, roi->imageRoi);
                    }
                } break;

                case CNNTYPE_DIGIT_DOUBLE_HYBRID10: // for models dig-cont*
                {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Digit (dig-cont)");

                    int LogImageResult;
                    int _num, _numplus, _numminus;
                    float _val, _valplus, _valminus, _fit;

                    if (tflite->LoadInputImageBasis(roi->imageRoiResized)) {
                        tflite->Invoke();
                        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                        return false;
                    }

                    _num = tflite->GetOutClassification(0, 9);
                    _numplus = (_num + 1) % 10;
                    _numminus = (_num - 1 + 10) % 10;

                    _val = tflite->GetOutputValue(_num);
                    _valplus = tflite->GetOutputValue(_numplus);
                    _valminus = tflite->GetOutputValue(_numminus);

                    float result = _num;

                    if (_valplus > _valminus) {
                        result = result + _valplus / (_valplus + _val);
                        _fit = _val + _valplus;
                    }
                    else {
                        result = result - _valminus / (_val + _valminus);
                        _fit = _val + _valminus;
                    }

                    if (result >= 10)
                        result = result - 10;
                    if (result < 0)
                        result = result + 10;

                    roi->CNNResult = result * 10.0; //  result normalized to 0-99

                    if (roi->CNNResult < 0)
                        roi->CNNResult = 0;
                    else if (roi->CNNResult >= 100)
                        roi->CNNResult = 99;

                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);

                    /*std::string zw = "num (p, m): " + std::to_string(_num) + " (" + std::to_string(_numplus) + " , " + std::to_string(_numminus) +
                                    "), val (p, m): " + std::to_string(_val) + " (" + std::to_string(_valplus) + " , " + std::to_string(_valminus) +
                                    "), result: " + roi->sCNNResult + ", fit: " + std::to_string(_fit);
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, zw);
                    */

                    if (_fit < CNNGoodThreshold) {
                        roi->isRejected = true;
                        LogImageResult = -1 * roi->CNNResult;     // In case fit is not sufficient, the result should still be saved with "-x.y".
                        std::string zw = "Result rejected - bad fit (Fit: " + std::to_string(_fit) + ", Threshold: " + std::to_string(CNNGoodThreshold) + ")";
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, zw);
                    }
                    else {
                        roi->isRejected = false;
                        LogImageResult = roi->CNNResult;
                    }

                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Result: " + roi->sCNNResult);

                    if (saveImagesEnabled) {
                        logImage(logPath, roi->param->roiName, CNNTYPE_DIGIT_DOUBLE_HYBRID10, LogImageResult, time, roi->imageRoi);
                    }
                } break;

                case CNNTYPE_ANALOG_CLASS100:
                case CNNTYPE_DIGIT_CLASS100:  // for models dig-class100*
                {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Analog / Digit (ana-class100 / dig-class100)");

                    if (tflite->LoadInputImageBasis(roi->imageRoiResized)) {
                        tflite->Invoke();
                        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                        return false;
                    }

                    int _num = tflite->GetOutClassification();

                    if(roi->param->ccw) {
                        if (_num == 0)
                            roi->CNNResult = 0;
                        else
                            roi->CNNResult = 100 -_num;

                    }
                    else {
                        roi->CNNResult = _num;
                    }

                    if (roi->CNNResult < 0)
                        roi->CNNResult = 0;
                    else if (roi->CNNResult >= 100)
                        roi->CNNResult = 99;

                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);

                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Result: " + roi->sCNNResult);

                    if (saveImagesEnabled) {
                        logImage(logPath, roi->param->roiName, CNNTYPE_DIGIT_CLASS100, roi->CNNResult, time, roi->imageRoi);
                    }
                } break;

                case CNNTYPE_ANALOG_CONT: // for models ana-cont*
                {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Type: Analog (ana-cont)");

                    if (tflite->LoadInputImageBasis(roi->imageRoiResized)) {
                        tflite->Invoke();
                        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                        return false;
                    }

                    float result = fmod(atan2(tflite->GetOutputValue(0), tflite->GetOutputValue(1)) / (M_PI * 2) + 2, 1);

                    if(roi->param->ccw) {
                        if (result == 0.0)
                            roi->CNNResult = 0;
                        else
                            roi->CNNResult = 100 - (result * 100);  // result normalized to 0-99
                    }
                    else {
                        roi->CNNResult = result * 100; // result normalized to 0-99
                    }

                    if (roi->CNNResult < 0)
                        roi->CNNResult = 0;
                    else if (roi->CNNResult >= 100)
                        roi->CNNResult = 99;

                    roi->sCNNResult = to_stringWithPrecision(roi->CNNResult / 10.0, 1);

                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Result: " + roi->sCNNResult);

                    if (saveImagesEnabled)
                        logImage(logPath, roi->param->roiName, CNNTYPE_ANALOG_CONT, roi->CNNResult, time, roi->imageRoi);
                } break;

                default:
                break;
            }
        }
    }

    tflite->CTfLiteClassDeleteInterpreter();

    return true;
}


std::string ClassFlowCNNGeneral::getReadout(SequenceData *sequence, int valuePreviousNumber, int resultPreviousNumber)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout: Number sequence: " + sequence->sequenceName +
                        ", extendedResolution: " + std::to_string(sequence->paramPostProc->extendedResolution) +
                        ", valuePreviousNumber: " + to_stringWithPrecision(valuePreviousNumber/10.0, 1) +
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
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Digit Number (No previous number, Extended Resolution): Result: " + result +
                                                        ", Value: " + to_stringWithPrecision((resultTemp/10.0), 1));
                resultTemp = resultTemp / 10; // resultIntergerPart to hand over a previous result to next digit evaluation
            }
            else {
                if (valuePreviousNumber >= 0) // If previous number available (analog value should be handed over)
                    resultTemp = evalDigitNumber(sequence->digitRoi[lastROI]->CNNResult, valuePreviousNumber,
                                                        resultPreviousNumber, true, int(sequence->paramPostProc->analogDigitSyncValue * 10.0));
                else
                    resultTemp = evalDigitNumber(sequence->digitRoi[lastROI]->CNNResult, -1, -1); // No previous number

                result = std::to_string(resultTemp);
            }
        }
        else {
            result = "N";
            if (sequence->paramPostProc->extendedResolution)
                result = "NN";

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont): Rejected, substitude with N");
        }

        // Evaluate all remaining ROI of number sequence (and potentially correct)
        for (int i = lastROI - 1; i >= 0; i--) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "ROI: " + sequence->digitRoi[i]->param->roiName);
            if (!sequence->digitRoi[i]->isRejected) { // valid result (e.g. model 'dig-cont*' --> bad fit)?
                resultTemp = evalDigitNumber(sequence->digitRoi[i]->CNNResult, sequence->digitRoi[i+1]->CNNResult, resultTemp);
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
    else if (cnnType == CNNTYPE_DIGIT_CLASS11) {  // Class-11-model (1-0 + NaN)
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

    if (_resultPreviousNumber <= -1)
    {
        result = _value / 10; // Return IntegerPart, remove decimal place (73 -> 7)
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogNumber (No previous number): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1));
        return result;
    }

    int valueMax = _value + Analog_error;
    if (valueMax >= 100) // e.g. 10.2 -> 0.2 (value = 02)
        valueMax = valueMax - 100;

    int valueMin = _value - Analog_error;
    if (valueMin < 0) // e.g. -0.3 -> 9.7 (value = 97)
        valueMin = 100 + valueMin;

    if (((valueMin / 10 - valueMax / 10)) != 0)
    {
        if (_resultPreviousNumber <= Analog_error)
        {
            result = valueMax / 10;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogNumber (Ambiguous, use value + corretion): Result: " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
        else if (_resultPreviousNumber >= 10 - Analog_error)
        {
            result = valueMin / 10;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogNumber (Ambiguous, use value - corretion): Result: " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
    }

    result = _value / 10;
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogNumber (Unambiguous, use value): Result: " + std::to_string(result) +
                                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    return result;
}


/* Evaluate digit number */
int ClassFlowCNNGeneral::evalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber,
                                              bool _isPreviousAnalog, int digitalAnalogTransitionStart)
{
    int result = - 1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    if (_resultPreviousNumber <= -1) { // no previous number -> no correction logic for transition needed, use value as is (integer part)
        result = resultIntergerPart;
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalDigitNumber (No previous number): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1));
        return result;
    }

    // previous number is analog (_valuePreviousNumber: 0-99), special transistion check needed
    if (_isPreviousAnalog) {
        result = evalAnalogToDigitTransition(_value, _valuePreviousNumber, _resultPreviousNumber, digitalAnalogTransitionStart);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalDigitNumber (Analog previous number): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                    ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // Previous number is digit (_valuePreviousNumber: 0-99) No digit change, because predecessor is far enough away (+/- Digital_Transition_Area_Predecessor)
    if ((_valuePreviousNumber >= Digital_Transition_Area_Predecessor) && (_valuePreviousNumber <= (100 - Digital_Transition_Area_Predecessor))) {
        // Band around the digit --> Round, as digit reaches inaccuracy in the frame
        if ((resultDecimalPlace <= DigitalBand) || (resultDecimalPlace >= (10-DigitalBand))) {
            if (resultDecimalPlace >= 5) {
                result = resultIntergerPart + 1; // "Round"

                if (result >= 10)
                    result = 0;
            }
            else {
                result = resultIntergerPart; // "Trunc"
            }
        }
        else {
            result = resultIntergerPart; // "Trunc"
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalDigitNumber (Digit previous number: no zero crossing, \'safe area\'): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                    ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // Zero crossing at the predecessor has taken place (! evaluation via result of previous number and not value !)
    // --> round up here (2.8 --> 3, but also 3.1 --> 3)
    if (_resultPreviousNumber <= 1) {
        // We simply assume that the current digit after the zero crossing of the predecessor
        // has passed through at least half (x.5)
        if (resultDecimalPlace >= 5) {
            result =  resultIntergerPart + 1; // "Round": The current digit does not yet have a zero crossing, but the predecessor does..

            if (result >= 10)
                result = 0;
        }
        else {
            result =  resultIntergerPart; // "Trunc": Act. digit and predecessor have zero crossing
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalDigitNumber (Digit previous number: zero crossing): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                    ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // remains only >= 9.x --> no zero crossing yet --> 2.8 --> 2,
    // and from 9.7(Digital_Transition_Area_Forward) 3.1 --> 2
    // everything >=x.4 can be considered as current number in transition. With 9.x predecessor the current
    // number can still be x.6 - x.7.
    // Preceding (else - branch) does not already happen from 9.
    if (((_valuePreviousNumber <= Digital_Transition_Area_Forward) && (_resultPreviousNumber == (int)(_valuePreviousNumber/10.0))) ||
        resultDecimalPlace >= 4)
    {
        result =  resultIntergerPart; // The current digit, like the previous digit, does not yet have a zero crossing.
    }
    else {
        // current digit precedes the smaller digit (9.x). So already >=x.0 while the previous digit has not yet
        // has no zero crossing. Therefore, it is reduced by 1.
        result =  resultIntergerPart - 1;

        if (result < 0)
            result = 9;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalDigitNumber (Digit previous number: no zero crossing yet): Result: " + std::to_string(result) +
                                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));

    return result;
}


/* Evaluate analog to digit number transition */
int ClassFlowCNNGeneral::evalAnalogToDigitTransition(int _value, int _valuePreviousNumber,  int _resultPreviousNumber, int analogDigitSyncValue)
{
    int result = - 1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    // Value within the digit inequalities
    if (resultDecimalPlace >= (10 - Digital_Uncertainty)    // Band around the zero crossing -> Round, as number reaches inaccuracy zone
        || (_resultPreviousNumber <= 4 && resultDecimalPlace >= 6)) // or number runs after (previous result <= 4, actucal decimal place >= 6)
    {
        if (resultDecimalPlace >= 5) { // "Round up"
            result = resultIntergerPart + 1;

            if (result >= 10)
                result = 0;

            // Correct back if no zero crossing detected
            // analogDigitSyncValue < _valuePreviousNumber < 0.2
            if (_resultPreviousNumber >= 6 && (_valuePreviousNumber > analogDigitSyncValue || _valuePreviousNumber <= 2))
            {
                result = result - 1;
                if (result < 0)
                    result = 9;

                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogToDigitTransition (Digit uncertainty, no zero crossing): Result: " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                                        ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            }
        }
        else {
            result = resultIntergerPart; // "Trunc -> Round down"
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogToDigitTransition (Digit uncertainty): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) +
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    }
    else {
        result = resultIntergerPart;
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "evalAnalogToDigitTransition: Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1));
    }

    return result;
}


bool ClassFlowCNNGeneral::cnnTypeAllowExtendedResolution()
{
    if (cnnType == CNNTYPE_DIGIT_CLASS11)
        return false;

    return true;
}


void ClassFlowCNNGeneral::drawROI(CImageBasis *image)
{
    if (!image->imageOkay())
        return;

    for (int i = 0; const auto &sequence : sequenceDataInternal) {
        std::array<int, 3> color = {0, 255, 0};
        if (i == 0)
            color = {0, 255, 0}; // Green
        else if (i == 1)
            color = {0, 0, 255}; // Blue
        else if (i == 2)
            color = {0, 255, 255}; // Cyan
        else if (i == 3)
            color = {255, 0, 255}; // Pink
        else if (i == 4)
            color = {255, 255, 0}; // Yellow

        for (const auto &roi : sequence->roiData) {
            if (cnnType == CNNTYPE_ANALOG_CONT || cnnType == CNNTYPE_ANALOG_CLASS100) {
                // image->drawRect(roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, color[0], color[1], color[2], 1);
                image->drawEllipse((int) (roi->param->x + roi->param->dx/2), (int) (roi->param->y + roi->param->dy/2),
                                    (int) (roi->param->dx/2), (int) (roi->param->dy/2), color[0], color[1], color[2], 2);
                image->drawLine((int) (roi->param->x + roi->param->dx/2), (int) roi->param->y, (int) (roi->param->x + roi->param->dx/2),
                                (int) (roi->param->y + roi->param->dy), color[0], color[1], color[2], 2);
                image->drawLine((int) roi->param->x, (int) (roi->param->y + roi->param->dy/2), (int) roi->param->x + roi->param->dx,
                                (int) (roi->param->y + roi->param->dy/2), color[0], color[1], color[2], 2);
            }
            else {
                image->drawRect(roi->param->x, roi->param->y, roi->param->dx, roi->param->dy, color[0], color[1], color[2], 2);
            }
        }
        i++;
    }
}


ClassFlowCNNGeneral::~ClassFlowCNNGeneral()
{
    delete tflite;
    tflite = NULL;

    for (auto &sequence : sequenceDataInternal) {
        for (auto &roi : sequence->roiData) {
            delete roi->imageRoiResized;
            roi->imageRoiResized = NULL;
            delete roi->imageRoi;
            roi->imageRoi = NULL;
        }
        sequence->roiData.clear();
        std::vector<RoiData *>().swap(sequence->roiData); // Ensure that memory gets freed (instead using shrink_to_fit())
    }

    sequenceDataInternal.clear();
    std::vector<SequenceDataInternal *>().swap(sequenceDataInternal); // Ensure that memory gets freed (instead using shrink_to_fit())
}
