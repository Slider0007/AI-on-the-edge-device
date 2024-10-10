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


static const char* TAG = "POSTPROC";


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

        // Plausibility checks
        // Fallback Value is mandatory to evaluate negative rates
        if (sequence->paramPostProc->maxRateCheckType > RATE_CHECK_OFF &&
                sequence->paramPostProc->allowNegativeRate && !sequence->paramPostProc->useFallbackValue) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, sequence->sequenceName +
                                    ": Activate parameter \'Use Fallback Value\' to use negative rate evaluation");
        }

        // Only valid for Class11 models
        if (sequence->paramPostProc->checkDigitIncreaseConsistency) {
            if (flowDigit != NULL && flowDigit->getCNNType() != CNNTYPE_DIGIT_CLASS11) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, sequence->sequenceName +
                                    ": Skip \'Digit Increase Consistency\' check, only applicable for dig-class11 models");
            }

            if (!sequence->paramPostProc->useFallbackValue) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, sequence->sequenceName +
                                    ": Activate parameter \'Use Fallback Value\' to be able use \'Digit Increase Consistency\' check");
            }
        }

        if (sequence->paramPostProc->ignoreLeadingNaN) {
            if (flowDigit != NULL && flowDigit->getCNNType() != CNNTYPE_DIGIT_CLASS11) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip \'Ignore Leading NaN\' check, only applicable for dig-class11 models");
            }
        }

        // Check if fallbackvalue usage is activated (at least in one sequence)
        if (!fallbackValueActivated && sequence->paramPostProc->useFallbackValue)
            fallbackValueActivated = true;

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Number sequence: " + sequence->sequenceName +
                                                ", Digits: " + std::to_string(sequence->digitRoi.size()) +
                                                ", Analogs: " + std::to_string(sequence->analogRoi.size()));
    }

    setDecimalShift();  // Set decimal shift and number of decimal places in relation to extended resolution parameter

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
    int resultPreviousNumberAnalog = -1;

    time_t _timeProcessed = flowTakeImage->getTimeImageTaken();
    if (_timeProcessed == 0)
        time(&_timeProcessed);

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Quantity of number sequences: %d", sequenceData.size());
    #endif

    /* Post-processing for all defined number sequences */
    for (auto &sequence : sequenceData) {
        if (cfgDataPtr == NULL || sequence->paramPostProc == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
            return false;
        }

        sequence->timeProcessed = _timeProcessed; // Update process time in seconds
        sequence->sTimeProcessed = convertTimeToString(sequence->timeProcessed, TIME_FORMAT_OUTPUT);
        sequence->isActualValueANumber = true;
        sequence->isActualValueConfirmed = true;
        std::string sRawValue = ""; // Helper to avoid on the fly modifactions of sequence->sRawValue
        std::string sActualValue = ""; // Helper to avoid on the fly modifactions of sequence->sActualValue

        /* Process analog numbers of sequence */
        if (!sequence->analogRoi.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "doFlow: Get analog ROI results");
            sRawValue = flowAnalog->getReadout(sequence);

            if (sRawValue.length() > 0) {
                if (sRawValue[0] >= 48 && sRawValue[0] <= 57) // Most significant analog value a number?
                    resultPreviousNumberAnalog = sRawValue[0] - 48;
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After analog->getReadout: RawValue %s", sRawValue.c_str());
        #endif

        /* Add decimal separator */
        if (!sequence->digitRoi.empty() && !sequence->analogRoi.empty()) {
            sRawValue = "." + sRawValue;
        }

        /* Process digit numbers of sequence */
        if (!sequence->digitRoi.empty()) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "doFlow: Get digit ROI results");
            if (!sequence->analogRoi.empty()) {// If analog numbers available
                sRawValue = flowDigit->getReadout(sequence, sequence->analogRoi[0]->CNNResult,
                                                resultPreviousNumberAnalog) + sRawValue;
            }
            else { // Extended resolution for digits only if no analog previous number
                sRawValue = flowDigit->getReadout(sequence);
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After digit->getReadout: RawValue %s", sRawValue.c_str());
        #endif

        /* Check for any data */
        if (sRawValue.empty()) {
            sequence->sRawValue = "";
            sequence->actualValue = 0.0;
            sequence->sActualValue = "";
            sequence->isActualValueANumber = false;
            sequence->isActualValueConfirmed = false;
            sequence->sValueStatus = std::string(VALUE_STATUS_W01_EMPTY_DATA);
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Sequence: " + sequence->sequenceName + ": Status: " + sequence->sValueStatus);
            continue; // Stop here, no data
        }

        /* Apply parametrized decimal shift */
        sRawValue = shiftDecimal(sRawValue, sequence->correctedDecimalShift);

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After shiftDecimal: RawValue %s", sRawValue.c_str());
        #endif

        /* Remove leading N */
        if (sequence->paramPostProc->ignoreLeadingNaN)
            while ((sRawValue.length() > 1) && (sRawValue[0] == 'N'))
                sRawValue.erase(0, 1);

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After IgnoreLeadingNaN: RawValue %s", sRawValue.c_str());
        #endif

        /* Use fully processed "Raw Value" and transfer to "Value" for further processing */

        sActualValue = sequence->sRawValue = sRawValue;

        /* Substitute any N position with last valid number if available */
        if (findDelimiterPos(sActualValue, "N") != std::string::npos) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Substitude N positions for number sequence: " + sequence->sequenceName);
            if (sequence->paramPostProc->useFallbackValue && sequence->isFallbackValueValid) { // fallbackValue can be used to replace the N
                sActualValue = substitudeN(sActualValue, sequence->fallbackValue);
            }
            else { // fallbackValue not valid to replace any N
                if (!sequence->paramPostProc->useFallbackValue)
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, sequence->sequenceName +
                                            ": Activate parameter \'Use Fallback Value\' to be able to substitude N positions");

                sequence->ratePerMin = 0;
                sequence->ratePerInterval = 0;
                sequence->sRatePerMin =  to_stringWithPrecision(sequence->ratePerMin, sequence->decimalPlaceCount+1);
                sequence->sRatePerInterval = to_stringWithPrecision(sequence->ratePerInterval, sequence->decimalPlaceCount);

                sequence->sValueStatus = std::string(VALUE_STATUS_001_DATA_N_SUBST) + " | Raw: " + sequence->sRawValue;
                sequence->sActualValue = "";
                sequence->isActualValueANumber = false;
                sequence->isActualValueConfirmed = false;

                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Sequence: " + sequence->sequenceName + ": Status: " + sequence->sValueStatus);

                writeDataLog(sequence->sequenceName);
                continue; // Stop here, no valid number because there are still N.
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After substitudeN: ActualValue %s", sActualValue.c_str());
        #endif

        /* Delete leading zeros (unless there is only one 0 left) */
        while ((sActualValue.length() > 1) && (sActualValue[0] == '0'))
            sActualValue.erase(0, 1);

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After removeLeadingZeros: ActualValue %s", sActualValue.c_str());
        #endif

        /* Convert actual value to double interpretation */
        if (!sActualValue.empty()) {
            sequence->actualValue = std::stod(sActualValue);
        }
        else {
            sequence->actualValue = 0.0;
            sequence->sActualValue = "";
            sequence->isActualValueANumber = false;
            sequence->isActualValueConfirmed = false;
            sequence->sValueStatus = std::string(VALUE_STATUS_W01_EMPTY_DATA);
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Sequence: " + sequence->sequenceName + ": Status: " + sequence->sValueStatus);
            continue; // Stop here, invalid number
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "After converting to double: sActualValue: %s, actualValue: %f", sActualValue.c_str(), sequence->actualValue);
        #endif

        if (sequence->paramPostProc->useFallbackValue) {
            /* Load fallback value if not yet loaded during init due to missing valid system time */
            loadFallbackValue();

            /* Is fallbackValue valid (== not outdated) */
            if (sequence->isFallbackValueValid) {
                /* Update fallbackValue */
                sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount);

                /* Check digit plausibitily (only support and necessary for class-11 models (0-9 + NaN)) */
                if (sequence->paramPostProc->checkDigitIncreaseConsistency) {
                    if (flowDigit) {
                        if (flowDigit->getCNNType() != CNNTYPE_DIGIT_CLASS11)
                            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip \'Digit Increase Consistency\' check, only applicable for dig-class11 models");
                        else {
                            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Check digit increase consistency for number sequence: " + sequence->sequenceName);
                            sequence->actualValue = checkDigitConsistency(sequence->actualValue, sequence->correctedDecimalShift,
                                                                            !sequence->analogRoi.empty(), sequence->fallbackValue);
                        }
                    }
                    else {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip \'Digit Increase Consistency\' check, no digit numbers configured");
                    }
                }

                #ifdef DEBUG_DETAIL_ON
                    ESP_LOGI(TAG, "After checkDigitIncreaseConsistency: actualValue %f", sequence->actualValue);
                #endif

                /* Update Rates */
                //Calculate delta time between this reading und last valid reading in seconds
                long timeDeltaToFallbackValue = abs((long)difftime(sequence->timeProcessed, sequence->timeFallbackValue)); // absolute delta in seconds

                if (timeDeltaToFallbackValue > 0) {
                    sequence->ratePerMin = (sequence->actualValue - sequence->fallbackValue) / (timeDeltaToFallbackValue / 60.0); // calculate rate / minute
                }
                else {
                    sequence->ratePerMin = 0;
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Rate calculation skipped, time delta between now and fallback value timestamp is zero");
                }

                sequence->ratePerInterval = sequence->actualValue - sequence->fallbackValue;

                double RatePerSelection;
                if (sequence->paramPostProc->maxRateCheckType == RATE_PER_MIN) {
                    RatePerSelection = sequence->ratePerMin;
                }
                else {
                    RatePerSelection = sequence->ratePerInterval; // If Rate check is off, use 'ratePerInterval' for display only purpose (easier to interprete)
                }

                /* Check for rate too high */
                if (sequence->paramPostProc->maxRateCheckType > RATE_CHECK_OFF) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Max. rate check for number sequence: " + sequence->sequenceName);
                    if (abs(RatePerSelection) > abs((double)sequence->paramPostProc->maxRate)) {
                        if (RatePerSelection < 0) {
                            sequence->sValueStatus  = std::string(VALUE_STATUS_003_RATE_TOO_HIGH_NEG);

                            /* Update timestamp of fallback value to be prepared to identify next negative movement larger than max. rate threshold (diagnostic purpose) */
                            sequence->timeFallbackValue = sequence->timeProcessed;
                            sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);
                        }
                        else {
                            sequence->sValueStatus  = std::string(VALUE_STATUS_004_RATE_TOO_HIGH_POS);
                        }

                        sequence->sValueStatus += " | Value: " + to_stringWithPrecision(sequence->actualValue, sequence->decimalPlaceCount) +
                                                    ", Fallback: " + sequence->sFallbackValue +
                                                    ", Rate: " + to_stringWithPrecision(RatePerSelection, sequence->decimalPlaceCount);

                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Sequence: " + sequence->sequenceName + ", Status: " + sequence->sValueStatus);
                        sequence->isActualValueConfirmed = false;
                        setFlowStateHandlerEvent(1); // Set warning event code for post cycle error handler 'doPostProcessEventHandling' (only warning level)
                    }
                }

                #ifdef DEBUG_DETAIL_ON
                    ESP_LOGI(TAG, "After MaxRateCheck: actualValue %f", sequence->actualValue);
                #endif

                /* Check for negative rate */
                if (!sequence->paramPostProc->allowNegativeRate && sequence->isActualValueConfirmed) {
                    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Negative rate check for number sequence: " + sequence->sequenceName);
                    if (sequence->actualValue < sequence->fallbackValue) {
                        sequence->sValueStatus  = std::string(VALUE_STATUS_002_RATE_NEGATIVE);

                        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequence->sequenceName + ", Status: " + sequence->sValueStatus +
                                                                " | Value: " + std::to_string(sequence->actualValue) +
                                                                ", Fallback: " + std::to_string(sequence->fallbackValue) +
                                                                ", Rate: " + to_stringWithPrecision(RatePerSelection, sequence->decimalPlaceCount));
                        sequence->isActualValueConfirmed = false;

                        /* Update timestamp of fallback value to be prepared to identify every negative movement larger than max. rate threshold (diagnostic purpose) */
                        sequence->timeFallbackValue = sequence->timeProcessed;
                        sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);
                    }
                }

                #ifdef DEBUG_DETAIL_ON
                    ESP_LOGI(TAG, "After allowNegativeRates: actualValue %f", sequence->actualValue);
                #endif
            }
            else { // Fallback value is outdated or age indeterminable (could be the case after a reboot) -> force rates to zero
                sequence->ratePerMin = 0;
                sequence->ratePerInterval = 0;
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Fallback value outdated or age indeterminable");
            }

            /* Update fallback value + set status */
            if (sequence->isActualValueANumber && sequence->isActualValueConfirmed) { // Value of actual reading is valid
                // Save value as fallback value
                sequence->fallbackValue = sequence->actualValue;
                sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount);
                sequence->timeFallbackValue = sequence->timeProcessed;
                sequence->sTimeFallbackValue = convertTimeToString(sequence->timeFallbackValue, TIME_FORMAT_OUTPUT);
                sequence->isFallbackValueValid = true;
                updateFallbackValue = true;

                sequence->sValueStatus = std::string(VALUE_STATUS_000_VALID);
            }
            else { // Value of actual reading is invalid, use fallback value and froce rates to zero
                sequence->ratePerMin = 0;
                sequence->ratePerInterval = 0;
                sequence->actualValue = sequence->fallbackValue;
            }
        }
        else { // FallbackValue usage disabled, no rate checking possible
            sequence->ratePerMin = 0;
            sequence->ratePerInterval = 0;
            sequence->fallbackValue = 0;
            sequence->sFallbackValue = "Disabled";
            sequence->sValueStatus = std::string(VALUE_STATUS_000_VALID);
        }

        /* Write output values */
        sequence->sRatePerMin =  to_stringWithPrecision(sequence->ratePerMin, sequence->decimalPlaceCount+1);
        sequence->sRatePerInterval = to_stringWithPrecision(sequence->ratePerInterval, sequence->decimalPlaceCount);
        sequence->sActualValue = to_stringWithPrecision(sequence->actualValue, sequence->decimalPlaceCount);

        /* Write log file entry */
        LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Value: " + sequence->sActualValue +
                                                                  ", Rate per min: " + sequence->sRatePerMin +
                                                                  ", Status: " + sequence->sValueStatus);

        writeDataLog(sequence->sequenceName);
    }

    saveFallbackValue();

    if (!FlowState.isSuccessful) // Return false if any error detected
        return false;

    return true;
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
            std::string destination = std::string(LOG_DEBUG_ROOT_FOLDER) + "/" + getFlowState()->ExecutionTime.DEFAULT_TIME_FORMAT_DATE_EXTR + "/" +
                                        getFlowState()->ExecutionTime + "/" + getFlowState()->ClassName;

            if (!makeDir(destination))
                return;

            for (const auto &sequence : sequenceData) {
                std::string resultFileName = "/" + sequence->sequenceName + "_rate_too_high.txt";

                // Save result in file
                FILE* fpResult = fopen((destination + resultFileName).c_str(), "w");
                fwrite(sequence->sValueStatus.c_str(), (sequence->sValueStatus).length(), 1, fpResult);
                fclose(fpResult);

                // Save digit ROIs
                if (!sequence->digitRoi.empty())
                    for (const auto &roi : sequence->digitRoi)
                        roi->imageRoi->saveToFile(destination + "/" + to_stringWithPrecision(roi->CNNResult/10.0, 1) +
                                                    "_" + roi->param->roiName + ".jpg");

                // Save analog ROIs
                if (!sequence->analogRoi.empty())
                    for (const auto &roi : sequence->analogRoi)
                        roi->imageRoi->saveToFile(destination + "/" + to_stringWithPrecision(roi->CNNResult/10.0, 1) +
                                                    "_" + roi->param->roiName + ".jpg");
            }

            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Rate too high, debug infos saved: " + destination);
        }
    }
}


void ClassFlowPostProcessing::setDecimalShift()
{
    for (auto &sequence : sequenceData) {
        sequence->correctedDecimalShift = sequence->paramPostProc->decimalShift; // Create updated decimalshift depending on config

         // Only digit numbers
        if (!sequence->digitRoi.empty() && sequence->analogRoi.empty()) {
            if (sequence->paramPostProc->extendedResolution && flowDigit->cnnTypeAllowExtendedResolution()) {
                sequence->correctedDecimalShift -= 1;
            }

            sequence->decimalPlaceCount = -1 * (sequence->correctedDecimalShift);
        }
        // Only analog pointers
        else if (sequence->digitRoi.empty() && !sequence->analogRoi.empty()) {
            if (sequence->paramPostProc->extendedResolution && flowAnalog->cnnTypeAllowExtendedResolution()) {
                sequence->correctedDecimalShift -= 1;
            }

            sequence->decimalPlaceCount = -1 * (sequence->correctedDecimalShift);
        }
        // Digit numbers & analog pointer available
        else if (!sequence->digitRoi.empty() && !sequence->analogRoi.empty()) {
            sequence->decimalPlaceCount = sequence->analogRoi.size() - sequence->correctedDecimalShift;

            if (sequence->paramPostProc->extendedResolution && flowAnalog->cnnTypeAllowExtendedResolution()) {
                sequence->decimalPlaceCount += 1;
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "setDecimalShift: Sequence %s, decimalPlace %i, DecShift %i", sequence->sequenceName.c_str(),
                        sequence->decimalPlaceCount, sequence->correctedDecimalShift);
        #endif
    }
}


std::string ClassFlowPostProcessing::shiftDecimal(std::string _value, int _decShift){

    if (_decShift == 0){
        return _value;
    }

    int _pos_dec_org, _pos_dec_new;

    _pos_dec_org = findDelimiterPos(_value, ".");
    if (_pos_dec_org == std::string::npos) {
        _pos_dec_org = _value.length();
    }
    else {
        _value = _value.erase(_pos_dec_org, 1);
    }

    _pos_dec_new = _pos_dec_org + _decShift;

    if (_pos_dec_new <= 0) {        // comma is before the first digit
        for (int i = 0; i > _pos_dec_new; --i){
            _value = _value.insert(0, "0");
        }
        _value = "0." + _value;
        return _value;
    }

    if (_pos_dec_new > _value.length()){    // Comma should be after string (123 --> 1230)
        for (int i = _value.length(); i < _pos_dec_new; ++i){
            _value = _value.insert(_value.length(), "0");
        }
        return _value;
    }

    std::string zw;
    zw = _value.substr(0, _pos_dec_new);
    zw = zw + ".";
    zw = zw + _value.substr(_pos_dec_new, _value.length() - _pos_dec_new);

    return zw;
}


std::string ClassFlowPostProcessing::substitudeN(std::string input, double _fallbackValue)
{
    int posN, posPunkt;
    int pot, ziffer;
    float zw;

    posN = findDelimiterPos(input, "N");
    posPunkt = findDelimiterPos(input, ".");
    if (posPunkt == std::string::npos){
        posPunkt = input.length();
    }

    while (posN != std::string::npos)
    {
        if (posN < posPunkt) {
            pot = posPunkt - posN - 1;
        }
        else {
            pot = posPunkt - posN;
        }

        zw =_fallbackValue / pow(10, pot);
        ziffer = ((int) zw) % 10;
        input[posN] = ziffer + 48;

        posN = findDelimiterPos(input, "N");
    }

    return input;
}


float ClassFlowPostProcessing::checkDigitConsistency(double input, int _decimalshift, bool _isanalog, double _fallbackValue){
    int aktdigit, olddigit;
    int aktdigit_before, olddigit_before;
    int pot, pot_max;
    float zw;
    bool no_nulldurchgang = false;

    pot = _decimalshift;
    if (!_isanalog) { // if there are no analogue values, the last one cannot be evaluated
        pot++;
    }

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "checkDigitConsistency: pot=%d, decimalShift=%d", pot, _decimalshift);
    #endif

    pot_max = ((int) log10(input)) + 1;
    while (pot <= pot_max) {
        zw = input / pow(10, pot-1);
        aktdigit_before = ((int) zw) % 10;
        zw = _fallbackValue / pow(10, pot-1);
        olddigit_before = ((int) zw) % 10;

        zw = input / pow(10, pot);
        aktdigit = ((int) zw) % 10;
        zw = _fallbackValue / pow(10, pot);
        olddigit = ((int) zw) % 10;

        no_nulldurchgang = (olddigit_before <= aktdigit_before);

        if (no_nulldurchgang) {
            if (aktdigit != olddigit) {
                input = input + ((float) (olddigit - aktdigit)) * pow(10, pot); // New Digit is replaced by old Digit;
            }
        }
        else {
            if (aktdigit == olddigit) { // despite zero crossing, digit was not incremented --> add 1
                input = input + ((float) (1)) * pow(10, pot);   // add 1 at the point
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGD(TAG, "checkDigitConsistency: input=%f", input);
        #endif

        pot++;
    }

    return input;
}


void ClassFlowPostProcessing::writeDataLog(std::string sequenceName)
{
    if (!LogFile.getDataLogToSDStatus()) {
        return;
    }

    std::string analog = "";
    std::string digit = "";

    for (const auto &sequence : sequenceData) {
        if (sequence->sequenceName == sequenceName) {
            for (const auto &roi : sequence->digitRoi) {
                digit += "," + roi->sCNNResult;
            }

            for (const auto &roi : sequence->analogRoi) {
                analog += "," + roi->sCNNResult;
            }

            // Plausibilty check: Skip data log if timestamp of image is older than a day
            // (e.g. due to time jump while processing) to avoid data logging issue with wrong time in a row
            time_t tNow;
            time(&tNow);
            if (sequence->timeProcessed < (tNow - 86400000)) { // Image time is older than now - 1 day
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Skip data log due to ambigious timestamp (older than 1 day)");
                return;
            }

            LogFile.writeToData(sequence->sTimeProcessed, sequence->sequenceName, sequence->sRawValue,
                                sequence->sActualValue, sequence->sFallbackValue, sequence->sRatePerMin,
                                sequence->sRatePerInterval, sequence->sValueStatus.substr(0,3), digit, analog);

            #ifdef DEBUG_DETAIL_ON
                ESP_LOGI(TAG, "writeDataLog: %s, %s, %s, %s, %s", sequence->sRawValue.c_str(), sequence->sActualValue.c_str(),
                                    sequence->sValueStatus.substr(0,3).c_str(), digit.c_str(), analog.c_str());
            #endif

            break;
        }
    }
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
    #ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "setFallbackValue: %f, %s", value, sequenceName.c_str());
    #endif

    for (const auto &sequence : sequenceData) {
        if (sequence->sequenceName == sequenceName) {
            if (value >= 0) {  // if new value posivive, use provided value to preset fallbackValue
                sequence->fallbackValue = value;
            }
            else {          // if new value negative, use last raw value to preset fallbackValue
                char* p;
                double ReturnRawValueAsDouble = strtod(sequence->sRawValue.c_str(), &p);
                if (ReturnRawValueAsDouble == 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "setFallbackValue: RawValue not a valid value for further processing: "
                                                            + sequence->sRawValue);
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

            LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Set FallbackValue to: " + sequence->sFallbackValue);

            return true;
        }
    }

    LogFile.writeToFile(ESP_LOG_WARN, TAG, "setFallbackValue: Numbersname not found or not valid");
    return false;   // No new value was set (e.g. wrong numbersname, no numbers at all)
}


bool ClassFlowPostProcessing::loadFallbackValue(void)
{
    if (fallbackValueLoaded)         // fallbackValue already loaded
        return false;

    esp_err_t err = ESP_OK;
    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READONLY, &fallbackvalue_nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_open | error: " + intToHexString(err));
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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_i16 sequence_size - error: " + intToHexString(err));
        nvs_close(fallbackvalue_nvshandle);
        return false;
    }

    for (int i = 0; i < sequence_size; ++i) {
        // Name: Read from NVS
        size_t required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str name size - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cName[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), cName, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str name - error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

        // Timestamp: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str timestamp size - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cTime[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), cTime, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str timestamp - error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

        // Value: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str fallbackvalue size - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }

        char cValue[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), cValue, &required_size);
            if (err != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFallbackValue: nvs_get_str fallbackvalue - error: " + intToHexString(err));
                nvs_close(fallbackvalue_nvshandle);
                return false;
            }
        }

        #ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "loadFallbackValue: Sequence: %s, Time: %s, Value: %s", cName, cTime, cValue);
        #endif

        for (const auto &sequence : sequenceData) {
            if (std::string(cName) == sequence->sequenceName) {
                if (!sequence->paramPostProc->useFallbackValue) // Skip, because FallbackValue is disabled
                    continue;

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

                time(&tStart);
                int AgeInMinutes = (int)(difftime(tStart, sequence->timeFallbackValue) / 60.0); // delta in minutes
                // Fallback value outdated
                if (AgeInMinutes > sequence->paramPostProc->fallbackValueAgeStartup) {
                    sequence->isFallbackValueValid = false;
                    sequence->fallbackValue = 0;
                    sequence->sFallbackValue = "Outdated";
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Fallback value outdated | Time: " + std::string(cTime));
                }
                // Start time is older than fallback value timestamp -> age indeterminable
                else if (AgeInMinutes < 0) {
                    sequence->isFallbackValueValid = false;
                    sequence->fallbackValue = 0;
                    sequence->sFallbackValue = "Not Determinable";
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Fallback value age not determinable | Time: " + std::string(cTime));
                }
                // Fallback value valid
                else {
                    sequence->isFallbackValueValid = true;
                    char *pEnd = NULL;
                    sequence->fallbackValue = strtod(cValue, &pEnd);
                    sequence->sFallbackValue = to_stringWithPrecision(sequence->fallbackValue, sequence->decimalPlaceCount + 1); // Keep one digit more
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, sequence->sequenceName + ": Fallback value valid | Time: " + std::string(cTime));
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
    if (!updateFallbackValue)         // fallbackValue unchanged
        return false;

    esp_err_t err = ESP_OK;
    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READWRITE, &fallbackvalue_nvshandle);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: No valid NVS handle - error: " + intToHexString(err));
        return false;
    }

    // Save number sequence size to ensure that only already saved data will be loaded
    err = nvs_set_i16(fallbackvalue_nvshandle, "sequence_size", (int16_t)sequenceData.size());
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_i16 sequence_size - error: " + intToHexString(err));
        nvs_close(fallbackvalue_nvshandle);
        return false;
    }

    for (int i = 0; i < sequenceData.size(); i++) {
        if (!sequenceData[i]->paramPostProc->useFallbackValue) // Skip, because FallbackValue is disabled
            continue;

        #ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "saveFallbackValue: Sequence: %s, Time: %s, Value: %s", sequenceData[i]->sequenceName.c_str(),
                    (sequenceData[i]->sTimeFallbackValue).c_str(), (to_stringWithPrecision(sequenceData[i]->fallbackValue,
                    sequenceData[i]->decimalPlaceCount)).c_str());
        #endif

        err = nvs_set_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), sequenceData[i]->sequenceName.c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str name - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), sequenceData[i]->sTimeFallbackValue.c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str timestamp - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(),
                            to_stringWithPrecision(sequenceData[i]->fallbackValue, sequenceData[i]->decimalPlaceCount).c_str());
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_set_str fallbackvalue - error: " + intToHexString(err));
            nvs_close(fallbackvalue_nvshandle);
            return false;
        }
    }

    err = nvs_commit(fallbackvalue_nvshandle);
    nvs_close(fallbackvalue_nvshandle);

    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveFallbackValue: nvs_commit - error: " + intToHexString(err));
        return false;
    }

    return true;
}


ClassFlowPostProcessing::~ClassFlowPostProcessing()
{
    // nothing to do
}
