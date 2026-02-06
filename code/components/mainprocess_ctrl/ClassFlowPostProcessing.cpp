#include "ClassFlowPostProcessing.h"
#include "../../include/defines.h"

#include <sstream>
#include <iomanip>
#include <time.h>

#include "nvs_flash.h"
#include "nvs.h"

#include <esp_log.h>

#include "time_sntp.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "ClassMeterModel.h"


static const char *TAG = "POSTPROC";


ClassFlowPostProcessing::ClassFlowPostProcessing(ClassFlowTakeImage *_flowTakeImage, ClassFlowCNNGeneral *_flowDigit,
                                                 ClassFlowCNNGeneral *_flowAnalog)
{
    presetFlowStateHandler(true);

    fallbackValueLoaded = false;
    updateFallbackValue = false;

    flowTakeImage = _flowTakeImage;
    flowDigit = _flowDigit;
    flowAnalog = _flowAnalog;
}


bool ClassFlowPostProcessing::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionPostProcessing;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    bool fallbackValueActivated = false;

    for (auto &sequence : sequenceData) {
        // Link sequence related post processing config into sequenceData
        for (const auto &seqCfgData : cfgDataPtr->sequence) {
            if (sequence->sequenceId == seqCfgData.sequenceId) {
                sequence->paramPostProc = &seqCfgData;
                break;
            }
        }

        if (sequence->paramPostProc == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid sequence config");
            return false;
        }

        // Parameter plausibility check: Fallback value is mandatory to evaluate negative rates
        if (sequence->paramPostProc->maxRateCheckType > RATE_CHECK_OFF && sequence->paramPostProc->allowNegativeRate &&
            !sequence->paramPostProc->useFallbackValue) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                sequence->sequenceName + ": Activate parameter \'Use Fallback Value\' to use negative rate evaluation");
        }

        // Parameter plausibility check: Check if fallbackvalue usage is activated (at least in one sequence)
        if (!fallbackValueActivated && sequence->paramPostProc->useFallbackValue) {
            fallbackValueActivated = true;
        }

        // Set decimal place count depending on 'extended resolution' parameter
        setDecimalPlaceCount(*sequence);

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "Sequence: " + sequence->sequenceName + " | Digits: " + std::to_string(sequence->digitRoi.size()) +
                                ", Analogs: " + std::to_string(sequence->analogRoi.size()));

        // Prepare meter model
        if (sequence->paramPostProc->meterType == MeterType::Mechanical) {
            auto meter = std::make_unique<MeterModelMechanical>(sequence->digitRoi.size(), sequence->analogRoi.size(),
                                                                sequence->paramPostProc->wheelType);

            meter->setDecimalScaling(sequence->paramPostProc->decimalScaling);
            meter->setModelInfluence(sequence->paramPostProc->modelInfluence);

            meter->setWheelTransitionWidth(sequence->paramPostProc->wheelTransitionWidth);
            meter->setDialToWheelDetune(sequence->paramPostProc->dialToWheelDetune);
            meter->loadDigitAutoDetuneValues(*sequence);

            sequence->meterModel = std::move(meter); // Hand over pointer
        }
        else if (sequence->paramPostProc->meterType == MeterType::DigitalLcd) {
            auto meter = std::make_unique<MeterModelLcd>(sequence->digitRoi.size());
            meter->setDecimalScaling(sequence->paramPostProc->decimalScaling);
            meter->setModelInfluence(sequence->paramPostProc->modelInfluence);

            sequence->meterModel = std::move(meter); // Hand over pointer
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unknown meter type");
            return false;
        }

        if (sequence->sequenceLength > sequence->meterModel->maxSequenceLength) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Sequence length too long. Max. 15 digits are supported");
            return false;
        }
    }

    // Load fallback value only if valid system time is set
    // If not already loaded here, force loading before first usage in function doFlow
    if (fallbackValueActivated && getTimeIsSet()) {
        loadFallbackValue();
    }

    return true;
}


bool ClassFlowPostProcessing::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);

    time_t _timeProcessed = flowTakeImage->getTimeImageTaken();
    if (_timeProcessed == 0) {
        time(&_timeProcessed);
    }

    // Post-processing for all defined number sequences
    for (auto &sequence : sequenceData) {
        if (!cfgDataPtr || !sequence->paramPostProc) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
            return false;
        }

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Processing number sequence: " + sequence->sequenceName);

        // Initialize sequence metadata
        sequence->timeProcessed = _timeProcessed;
        sequence->sTimeProcessed = convertTimeToString(sequence->timeProcessed, TIME_FORMAT_OUTPUT);
        sequence->isActualValueConfirmed = true;

        // Check for empty ROI data
        if (sequence->digitRoi.empty() && sequence->analogRoi.empty()) {
            sequence->sRawValue = "";
            sequence->actualValue = 0.0;
            sequence->sActualValue = "";
            sequence->isActualValueConfirmed = false;
            sequence->sValueStatus = VALUE_STATUS_W01_EMPTY_DATA;
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Sequence: " + sequence->sequenceName + " | Status: " + sequence->sValueStatus);
            continue;
        }

        // Perform physical meter model evaluation
        auto modelResult = sequence->meterModel->invokeModel(*sequence);
        sequence->actualValue = modelResult.value;
        sequence->sRawValue = to_stringWithPrecision(modelResult.value, sequence->decimalPlaceCount);

        if (modelResult.status == MeterModel::ResultStatus::FailedConfigMismatch) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Sequence: " + sequence->sequenceName + " | Model execution failed: Config mismatch");
            return false;
        }
        else if (modelResult.status == MeterModel::ResultStatus::BadVisualScore ||
                 modelResult.status == MeterModel::ResultStatus::TemporalError) {
            std::string msg = (modelResult.status == MeterModel::ResultStatus::BadVisualScore) ? "Bad visual score"
                                                                                               : "Physical impossibility";
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequence->sequenceName + " | Model result: " + msg);
        }

        // Fallback and rate logic
        if (sequence->paramPostProc->useFallbackValue) {
            loadFallbackValue();

            if (sequence->isFallbackValueValid) {
                sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount);

                // Calculate rates
                long timeDelta = abs((long)difftime(sequence->timeProcessed, sequence->timeFallbackValue));
                sequence->ratePerInterval = sequence->actualValue - sequence->fallbackValue;
                sequence->ratePerMin = (timeDelta > 0) ? (sequence->ratePerInterval / (timeDelta / 60.0)) : 0;

                if (timeDelta == 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Rate calculation skipped: zero time delta");
                }

                double rateToCheck = (sequence->paramPostProc->maxRateCheckType == RATE_PER_MIN) ? sequence->ratePerMin
                                                                                                 : sequence->ratePerInterval;

                // Validation: Max Rate Check
                if (sequence->paramPostProc->maxRateCheckType > RATE_CHECK_OFF) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequence->sequenceName + " | Checking for max. rate deviation");
                    if (abs(rateToCheck) > abs((double)sequence->paramPostProc->maxRate)) {
                        sequence->isActualValueConfirmed = false;
                        if (rateToCheck < 0) { // Diagnostic: update fallback time for negative drift
                            sequence->timeFallbackValue = sequence->timeProcessed;
                            sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);
                        }

                        sequence->sValueStatus = (rateToCheck < 0) ? VALUE_STATUS_003_RATE_TOO_HIGH_NEG
                                                                   : VALUE_STATUS_004_RATE_TOO_HIGH_POS;
                        sequence->sValueStatus += " | Rate: " + to_stringWithPrecision(rateToCheck, sequence->decimalPlaceCount) +
                                                  ", Discarded: " + sequence->sRawValue + ", Using Fallback: " +
                                                  to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount + 1);

                        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                            "Sequence: " + sequence->sequenceName + " | Status: " + sequence->sValueStatus);

                        setFlowStateHandlerEvent(1); // Set warning event code for post cycle error handler 'doPostProcessEventHandling'
                    }
                }

                // Validation: Negative Rate Check
                if (!sequence->paramPostProc->allowNegativeRate && sequence->isActualValueConfirmed) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequence->sequenceName + " | Checking for negative rate");

                    if (sequence->actualValue < sequence->fallbackValue) {
                        sequence->isActualValueConfirmed = false;
                        sequence->timeFallbackValue = sequence->timeProcessed;
                        sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);

                        sequence->sValueStatus = VALUE_STATUS_002_RATE_NEGATIVE;
                        sequence->sValueStatus += " | Rate: " + to_stringWithPrecision(rateToCheck, sequence->decimalPlaceCount) +
                                                  ", Discarded: " + sequence->sRawValue + ", Using Fallback: " +
                                                  to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount + 1);

                        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                            "Sequence: " + sequence->sequenceName + " | Status: " + sequence->sValueStatus);
                    }
                }
            }
            else {
                sequence->ratePerMin = sequence->ratePerInterval = 0;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Fallback value outdated/undeterminable");
            }

            // Finalize values based on confirmation status
            if (sequence->isActualValueConfirmed) {
                sequence->fallbackValue = sequence->actualValue;
                sequence->sFallbackValue = sequence->sRawValue;
                sequence->timeFallbackValue = sequence->timeProcessed;
                sequence->sTimeFallbackValue = sequence->sTimeProcessed;
                sequence->isFallbackValueValid = true;
                sequence->sValueStatus = VALUE_STATUS_000_VALID;
                updateFallbackValue = true;

                // Only for meter which uses digit autotune parameter (e.g. mechanical meter)
                sequence->meterModel->saveDigitAutoDetuneValues(*sequence);
            }
            else {
                sequence->ratePerMin = sequence->ratePerInterval = 0;
                sequence->actualValue = sequence->fallbackValue;
            }
        }
        else {
            // Fallback disabled
            sequence->ratePerMin = sequence->ratePerInterval = sequence->fallbackValue = 0;
            sequence->sFallbackValue = "Disabled";
            sequence->sValueStatus = VALUE_STATUS_000_VALID;

            // Only for meter which uses digit autotune parameter (e.g. mechanical meter)
            sequence->meterModel->saveDigitAutoDetuneValues(*sequence);
        }

        // Save result
        sequence->sRatePerMin = to_stringWithPrecision(sequence->ratePerMin, sequence->decimalPlaceCount + 1);
        sequence->sRatePerInterval = to_stringWithPrecision(sequence->ratePerInterval, sequence->decimalPlaceCount);
        sequence->sActualValue = to_stringWithPrecision(sequence->actualValue, sequence->decimalPlaceCount);

        writeDataLog(*sequence, modelResult);

        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Sequence: " + sequence->sequenceName + " | Value: " + sequence->sActualValue +
                                " | Rate per min: " + sequence->sRatePerMin + " | Status: " + sequence->sValueStatus);
    }

    saveFallbackValue();

    return getFlowState()->isSuccessful;
}


void ClassFlowPostProcessing::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    for (int i = 0; i < getFlowState()->EventCode.size(); i++) {
        // If saving debug infos enabled and "rate to high" event
        if (cfgDataPtr->debug.saveDebugInfo && getFlowState()->EventCode[i] == 1) {
            time_t actualtime;
            time(&actualtime);

            // Define path, e.g. /sdcard/log/debug/20230814/20230814-125528/ClassFlowPostProcessing
            std::string destination = std::string(LOG_DEBUG_ROOT_FOLDER) + "/" +
                                      getFlowState()->ExecutionTime.DEFAULT_TIME_FORMAT_DATE_EXTR + "/" + getFlowState()->ExecutionTime +
                                      "/" + getFlowState()->ClassName;

            if (!makeDir(destination)) {
                return;
            }

            for (const auto &sequence : sequenceData) {
                std::string resultFileName = "/" + sequence->sequenceName + "_rate_too_high.txt";

                // Save result in file
                FILE *fpResult = fopen((destination + resultFileName).c_str(), "w");
                fwrite(sequence->sValueStatus.c_str(), (sequence->sValueStatus).length(), 1, fpResult);
                fclose(fpResult);

                // Save digit ROIs
                if (!sequence->digitRoi.empty()) {
                    for (const auto &roi : sequence->digitRoi) {
                        roi->imageRoi->saveJpgToFile(destination + "/" + to_stringWithPrecision(roi->CNNResult / 10.0, 1) + "_" +
                                                     roi->param->roiName + ".jpg");
                    }
                }

                // Save analog ROIs
                if (!sequence->analogRoi.empty()) {
                    for (const auto &roi : sequence->analogRoi) {
                        roi->imageRoi->saveJpgToFile(destination + "/" + to_stringWithPrecision(roi->CNNResult / 10.0, 1) + "_" +
                                                     roi->param->roiName + ".jpg");
                    }
                }
            }

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Rate too high, debug infos saved: " + destination);
        }
    }
}


void ClassFlowPostProcessing::setDecimalPlaceCount(SequenceData &sequence)
{
    // Calculate effective decimal places
    // - If digits exist, decimals start after the digits (base = analog count)
    // - If ONLY analogs exist, treat them as integers (base = 0)
    const int base = (sequence.digitRoi.empty()) ? 0 : sequence.analogRoi.size();

    // Adapt scaling based on 'Extended Resolution' (add one more decimal place)
    const int decimalScaling = sequence.paramPostProc->extendedResolution ? sequence.paramPostProc->decimalScaling - 1
                                                                          : sequence.paramPostProc->decimalScaling;

    sequence.decimalPlaceCount = base - decimalScaling;
}


void ClassFlowPostProcessing::writeDataLog(const SequenceData &sequence, const MeterModel::Result &modelResult)
{
    if (!LogFile.getDataLogToSDStatus()) {
        return;
    }

    // Plausibility check: Skip data log if timestamp of image is older than a day
    // (e.g. due to time jump while processing) to avoid data logging issue with wrong time in a row
    time_t tNow;
    time(&tNow);
    if (sequence.timeProcessed < (tNow - 86400000)) { // Image time is older than now - 1 day
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Skip data log due to ambiguous timestamp (older than 1 day)");
        return;
    }

    LogFile.writeToData(sequence, modelResult);
}


std::string ClassFlowPostProcessing::getFallbackValue(std::string sequenceName)
{
    for (const auto &sequence : sequenceData) {
        if (sequence->sequenceName == sequenceName) {
            return to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount);
        }
    }

    return std::string("");
}


bool ClassFlowPostProcessing::setFallbackValue(double value, std::string sequenceName)
{
    for (const auto &sequence : sequenceData) {
        if (sequence->sequenceName == sequenceName) {
            if (value >= 0) { // if new value positive, use provided value to preset fallbackValue
                sequence->fallbackValue = value;
            }
            else { // if new value negative, use last raw value to preset fallbackValue
                char *p;
                double ReturnRawValueAsDouble = strtod(sequence->sRawValue.c_str(), &p);
                if (ReturnRawValueAsDouble == 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                        "setFallbackValue: Raw value not a valid value for further processing: " + sequence->sRawValue);
                    return false;
                }
                sequence->fallbackValue = ReturnRawValueAsDouble;
            }

            time(&(sequence->timeFallbackValue)); // Timezone is already set at boot
            sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);
            sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount + 1);
            sequence->isFallbackValueValid = true;
            updateFallbackValue = true;
            saveFallbackValue();
            LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Set fallback value to: " + sequence->sFallbackValue);

            // Meter which uses autotune parameter (mechanical meter)
            sequence->meterModel->resetDigitAutoDetuneValues(*sequence);

            return true;
        }
    }

    LogFile.writeToFile(ESP_LOG_WARN, TAG, "setFallbackValue: Failed to set fallback value | Error: No sequence found");
    return false;
}


bool ClassFlowPostProcessing::loadFallbackValue(void)
{
    if (fallbackValueLoaded) { // fallbackValue already loaded
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READONLY, &fallbackvalue_nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_open | Error: " + intToHexString(err));
        return false;
    }
    else if (err != ESP_OK && (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NVS_INVALID_HANDLE)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "loadFallbackValue: nvs_open | No data in NVS namespace 'fallbackvalue'");
        return false;
    }

    // Use sequence size to ensure that only already saved data will be loaded
    int16_t sequence_size = 0;
    err = nvs_get_i16(fallbackvalue_nvshandle, "sequence_size", &sequence_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_i16 sequence_size | Error: " + intToHexString(err));
        nvs_close(fallbackvalue_nvshandle);
        return false;
    }

    for (int i = 0; i < sequence_size; ++i) {
        // Name: Read from NVS
        size_t required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str name size | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cName[required_size + 1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), cName, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str name | Error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

        // Timestamp: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str timestamp size | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cTime[required_size + 1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), cTime, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str timestamp | Error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

        // Value: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str fallbackvalue size | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cValue[required_size + 1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), cValue, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str fallbackvalue | Error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "loadFallbackValue: Sequence: %s, Time: %s, Value: %s", cName, cTime, cValue);
#endif // DEBUG_DETAIL_ON

        for (const auto &sequence : sequenceData) {
            if (std::string(cName) == sequence->sequenceName) {
                if (!sequence->paramPostProc->useFallbackValue) { // Skip, because FallbackValue is disabled
                    continue;
                }

                time_t tStart;
                int yy, month, dd, hh, mm, ss;
                struct tm tmFallbackValue;

                sscanf(cTime, FALLBACKVALUE_TIME_FORMAT_INPUT, &yy, &month, &dd, &hh, &mm, &ss);
                tmFallbackValue.tm_year = yy - 1900;
                tmFallbackValue.tm_mon = month - 1;
                tmFallbackValue.tm_mday = dd;
                tmFallbackValue.tm_hour = hh;
                tmFallbackValue.tm_min = mm;
                tmFallbackValue.tm_sec = ss;
                tmFallbackValue.tm_isdst = -1;

                sequence->timeFallbackValue = mktime(&tmFallbackValue);
                sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);

                time(&tStart);
                int AgeInMinutes = (int)(difftime(tStart, sequence->timeFallbackValue) / 60.0); // delta in minutes
                // Fallback value outdated
                if (AgeInMinutes > sequence->paramPostProc->fallbackValueAgeStartup) {
                    sequence->isFallbackValueValid = false;
                    sequence->fallbackValue = 0;
                    sequence->sFallbackValue = "Outdated";
                    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                                        sequence->sequenceName + ": Fallback value outdated | Timestamp: " + sequence->sTimeFallbackValue);
                }
                // Start time is older than fallback value timestamp -> age not determinable
                else if (AgeInMinutes < 0) {
                    sequence->isFallbackValueValid = false;
                    sequence->fallbackValue = 0;
                    sequence->sFallbackValue = "Not Determinable";
                    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                                        sequence->sequenceName +
                                            ": Fallback value age not determinable | Timestamp: " + sequence->sTimeFallbackValue);
                }
                // Fallback value valid
                else {
                    sequence->isFallbackValueValid = true;
                    char *pEnd = NULL;
                    sequence->fallbackValue = strtod(cValue, &pEnd);
                    sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue,
                                                                      sequence->decimalPlaceCount + 1); // Keep one digit more
                    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                                        sequence->sequenceName + ": Fallback value valid | Timestamp: " + sequence->sTimeFallbackValue);
                }
                break;
            }
        }
    }
    nvs_close(fallbackvalue_nvshandle);

    fallbackValueLoaded = true;
    return true;
}


bool ClassFlowPostProcessing::saveFallbackValue()
{
    if (!updateFallbackValue) { // fallbackValue unchanged
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READWRITE, &fallbackvalue_nvshandle);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: No valid NVS handle | Error: " + intToHexString(err));
        return false;
    }

    // Save number sequence size to ensure that only already saved data will be loaded
    err = nvs_set_i16(fallbackvalue_nvshandle, "sequence_size", (int16_t)sequenceData.size());
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_i16 sequence_size | Error: " + intToHexString(err));
        nvs_close(fallbackvalue_nvshandle);
        return false;
    }

    for (int i = 0; i < sequenceData.size(); i++) {
        if (!sequenceData[i]->paramPostProc->useFallbackValue) { // Skip, because FallbackValue is disabled
            continue;
        }

#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "saveFallbackValue: Sequence: %s, Time: %s, Value: %s", sequenceData[i]->sequenceName.c_str(),
                 (sequenceData[i]->sTimeFallbackValue).c_str(),
                 (to_stringWithPrecision(sequenceData[i]->fallbackValue, sequenceData[i]->decimalPlaceCount)).c_str());
#endif // DEBUG_DETAIL_ON

        err = nvs_set_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), sequenceData[i]->sequenceName.c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str name | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), sequenceData[i]->sTimeFallbackValue.c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str timestamp | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(),
                          to_stringWithPrecision(sequenceData[i]->fallbackValue, sequenceData[i]->decimalPlaceCount).c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str fallbackvalue | Error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
    }

    err = nvs_commit(fallbackvalue_nvshandle);
    nvs_close(fallbackvalue_nvshandle);

    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_commit | Error: " + intToHexString(err));
        return false;
    }

    return true;
}


ClassFlowPostProcessing::~ClassFlowPostProcessing()
{
    // nothing to do
}
