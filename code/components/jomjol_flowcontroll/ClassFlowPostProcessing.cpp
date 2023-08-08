#include "ClassFlowPostProcessing.h"
#include "Helper.h"
#include "ClassFlowTakeImage.h"
#include "ClassLogFile.h"

#include <iomanip>
#include <sstream>

#include <time.h>
#include "time_sntp.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_log.h"
#include "../../include/defines.h"

static const char* TAG = "POSTPROC";


//#define DEBUG_DETAIL_ON 


ClassFlowPostProcessing::ClassFlowPostProcessing(std::vector<ClassFlow*>* lfc, ClassFlowCNNGeneral *_analog, ClassFlowCNNGeneral *_digit)
{
    PresetFlowStateHandler(true);
    UseFallbackValue = true;
    UpdateFallbackValue = false;
    FallbackValueAgeStartup = 60;
    ErrorMessage = false;
    IgnoreLeadingNaN = false;
    ListFlowControll = lfc;
    flowTakeImage = NULL;
    flowAnalog = _analog;
    flowDigit = _digit;

    for (int i = 0; i < ListFlowControll->size(); ++i) {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowTakeImage") == 0) {
            flowTakeImage = (ClassFlowTakeImage*) (*ListFlowControll)[i];
        }
    }
}


void ClassFlowPostProcessing::handleDecimalExtendedResolution(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";

    for (int j = 0; j < NUMBERS.size(); ++j) {
        if (_digit == "default" || NUMBERS[j]->name == _digit) {
            if (toUpper(_value) == "TRUE")
                NUMBERS[j]->isExtendedResolution = true;
            else
                NUMBERS[j]->isExtendedResolution = false;

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleDecimalExtendedResolution: Name: %s, Pospunkt: %d, value: %d", _digit.c_str(), _pospunkt, NUMBERS[j]->isExtendedResolution);
            #endif
        }
    }
}


void ClassFlowPostProcessing::handleDecimalShift(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";

    for (int j = 0; j < NUMBERS.size(); ++j) {
        if (_digit == "default" || NUMBERS[j]->name == _digit) {
            NUMBERS[j]->decimalShift = stoi(_value);

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleDecimalShift: Name: %s, Pospunkt: %d, value: %d", _digit.c_str(), 
                            _pospunkt, NUMBERS[j]->decimalShift);
            #endif
        }
    }
}


void ClassFlowPostProcessing::handleAnalogDigitalTransitionStart(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";

    for (int j = 0; j < NUMBERS.size(); ++j) {    
        if (_digit == "default" || NUMBERS[j]->name == _digit) { 
            NUMBERS[j]->analogDigitalTransitionStart = (int) (stof(_value) * 10);

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleAnalogDigitalTransitionStart: Name: %s, Pospunkt: %d, value: %f", _digit.c_str(), 
                            _pospunkt, NUMBERS[j]->analogDigitalTransitionStart/10.0);
            #endif
        }
    }
}


void ClassFlowPostProcessing::handleAllowNegativeRate(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";

    for (int j = 0; j < NUMBERS.size(); ++j) {
        if (_digit == "default" || NUMBERS[j]->name == _digit) {
            if (toUpper(_value) == "TRUE") {
                NUMBERS[j]->allowNegativeRates = true;
            }
            else {
                NUMBERS[j]->allowNegativeRates = false;
                
                if (!UseFallbackValue) // Fallback Value is mandatory to evaluate negative rates
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Activate parameter \'Use Fallback Value\' to use negative rate evaluation"); 
            }

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleAllowNegativeRate: Name: %s, Pospunkt: %d, value: %d", _digit.c_str(), 
                            _pospunkt, NUMBERS[j]->allowNegativeRates);
            #endif
        }
    }
}


void ClassFlowPostProcessing::handleMaxRateType(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";

    for (int j = 0; j < NUMBERS.size(); ++j) {
        if (_digit == "default" || NUMBERS[j]->name == _digit) {
            if (toUpper(_value) == "RATEPERMIN") {
                NUMBERS[j]->rateType = rtRatePerMin;
                NUMBERS[j]->useMaxRateValue = true;
            }
            else if (toUpper(_value) == "RATEOFF") {
                NUMBERS[j]->rateType = rtRateOff;
                NUMBERS[j]->useMaxRateValue = false;
            }
            else {
                NUMBERS[j]->rateType = rtRatePerProcessing;
                NUMBERS[j]->useMaxRateValue = true;
            }

            if (NUMBERS[j]->useMaxRateValue && !UseFallbackValue) // Fallback Value is mandatory to evaluate rate limits
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Activate parameter \'Use Fallback Value\' to use rate limit evaluation"); 

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleMaxRateType: Name: %s, Pospunkt: %d, rateType: %d", _digit.c_str(), 
                            _pospunkt, NUMBERS[j]->rateType);
            #endif
        }
    }
}


void ClassFlowPostProcessing::handleMaxRateValue(std::string _decsep, std::string _value)
{
    std::string _digit;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";
    
    for (int j = 0; j < NUMBERS.size(); ++j) {
        if (_digit == "default" || NUMBERS[j]->name == _digit) {
            NUMBERS[j]->maxRateValue = stof(_value);

            #ifdef DEBUG_DETAIL_ON 
                ESP_LOGI(TAG, "handleMaxRateValue: Name: %s, Pospunkt: %d, value: %f", _digit.c_str(), 
                        _pospunkt, NUMBERS[j]->maxRateValue);
            #endif
        }
    }
}


bool ClassFlowPostProcessing::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;
    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;


    if (aktparamgraph.compare("[PostProcessing]") != 0)       // Paragraph does not fit PostProcessing
        return false;

    InitNUMBERS();

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph)) {
        splitted = ZerlegeZeile(aktparamgraph);
        std::string _param = GetParameterName(splitted[0]);

        if ((toUpper(_param) == "FALLBACKVALUEUSE") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                UseFallbackValue = true;
            else
                UseFallbackValue = false;
        }

        if ((toUpper(_param) == "FALLBACKVALUEAGESTARTUP") && (splitted.size() > 1)) {
            FallbackValueAgeStartup = std::stoi(splitted[1]);
        }

        if ((toUpper(_param) == "ERRORMESSAGE") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                ErrorMessage = true;
            else
                ErrorMessage = false;
        }

        if ((toUpper(_param) == "CHECKDIGITINCREASECONSISTENCY") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE") {
                for (int j = 0; j < NUMBERS.size(); ++j) {
                    NUMBERS[j]->checkDigitIncreaseConsistency = true;

                    if (flowDigit != NULL && flowDigit->getCNNType() != Digital)
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip \'Digit Increase Consistency\' check, only applicable for dig-class11 models");

                    if (!UseFallbackValue)
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Activate parameter \'Use Fallback Value\' to be able use \'Digit Increase Consistency\' check");
                }
            }
            else {
                for (int j = 0; j < NUMBERS.size(); ++j) {
                    NUMBERS[j]->checkDigitIncreaseConsistency = false;
                }
            }
        } 
        
        if ((toUpper(_param) == "ALLOWNEGATIVERATES") && (splitted.size() > 1)) {
            handleAllowNegativeRate(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "DECIMALSHIFT") && (splitted.size() > 1)) {
            handleDecimalShift(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "ANALOGDIGITALTRANSITIONSTART") && (splitted.size() > 1)) {
            handleAnalogDigitalTransitionStart(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "MAXRATETYPE") && (splitted.size() > 1)) {
            handleMaxRateType(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "MAXRATEVALUE") && (splitted.size() > 1)) {
            handleMaxRateValue(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "EXTENDEDRESOLUTION") && (splitted.size() > 1)) {
            handleDecimalExtendedResolution(splitted[0], splitted[1]);
        }

        if ((toUpper(_param) == "IGNORELEADINGNAN") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE") {
                if (flowDigit != NULL && flowDigit->getCNNType() != Digital) {
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip \'Ignore Leading NaNs\' check, only applicable for dig-class11 models");
                    IgnoreLeadingNaN = false;
                }
                else {
                    IgnoreLeadingNaN = true;
                }
            }
            else {
                IgnoreLeadingNaN = false;
            }
        }
    }

    /* Set decimal shift and number of decimal places in relation to extended resolution parameter */
    setDecimalShift();

    if (UseFallbackValue) {
        LoadFallbackValue();
    }

    return true;
}


void ClassFlowPostProcessing::InitNUMBERS()
{
    int anzDIGIT = 0;
    int anzANALOG = 0;
    std::vector<std::string> name_numbers;

    if (flowDigit)
    {
        anzDIGIT = flowDigit->getNumberGENERAL();
        flowDigit->UpdateNameNumbers(&name_numbers);
    }
    if (flowAnalog)
    {
        anzANALOG = flowAnalog->getNumberGENERAL();
        flowAnalog->UpdateNameNumbers(&name_numbers);
    }

    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGI(TAG, "Anzahl NUMBERS: %d - DIGITS: %d, ANALOG: %d", name_numbers.size(), anzDIGIT, anzANALOG);
    #endif

    for (int _num = 0; _num < name_numbers.size(); ++_num)
    {
        NumberPost *_number = new NumberPost;

        _number->name = name_numbers[_num];
        
        _number->digit_roi = NULL;
        if (flowDigit)
            _number->digit_roi = flowDigit->FindGENERAL(name_numbers[_num]);
        
        if (_number->digit_roi)
            _number->digitCount = _number->digit_roi->ROI.size();
        else
            _number->digitCount = 0;

        _number->analog_roi = NULL;
        if (flowAnalog)
            _number->analog_roi = flowAnalog->FindGENERAL(name_numbers[_num]);


        if (_number->analog_roi)
            _number->analogCount = _number->analog_roi->ROI.size();
        else
            _number->analogCount = 0;

        _number->decimalPlaceCount = _number->analogCount;

        _number->allowNegativeRates = false;
        _number->maxRateValue = 0.1;
        _number->rateType = rtRatePerMin;
        _number->useMaxRateValue = true;
        _number->checkDigitIncreaseConsistency = false;
        _number->decimalShift = 0;
        _number->isExtendedResolution = false;
        _number->analogDigitalTransitionStart = 92; // 9.2

        _number->isActualValueANumber = false;
        _number->isActualValueConfirmed = false;
        _number->isFallbackValueValid = false;

        _number->ratePerMin = 0;
        _number->ratePerProcessing = 0; 
        _number->fallbackValue = 0;
        _number->actualValue = 0;

        _number->sRatePerMin = "";
        _number->sRatePerProcessing = "";
        _number->sRawValue = "";
        _number->sFallbackValue = "";   
        _number->sActualValue = "";
        _number->sValueStatus = "";

        NUMBERS.push_back(_number);
    }

    for (int i = 0; i < NUMBERS.size(); ++i) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Number sequence: " + NUMBERS[i]->name + 
                                                ", Digits: " + std::to_string(NUMBERS[i]->digitCount) + 
                                                ", Analogs: " + std::to_string(NUMBERS[i]->analogCount));
    }
}


void ClassFlowPostProcessing::setDecimalShift()
{
    for (int j = 0; j < NUMBERS.size(); ++j)
    {
         // Only digit numbers
        if (NUMBERS[j]->digit_roi && !NUMBERS[j]->analog_roi) {
            if (NUMBERS[j]->isExtendedResolution && flowDigit->CNNTypeWithExtendedResolution())
                NUMBERS[j]->decimalShift -= 1;

            NUMBERS[j]->decimalPlaceCount = -(NUMBERS[j]->decimalShift);
        }
        // Only analog pointers
        else if (!NUMBERS[j]->digit_roi && NUMBERS[j]->analog_roi) {
            if (NUMBERS[j]->isExtendedResolution && flowAnalog->CNNTypeWithExtendedResolution())
                NUMBERS[j]->decimalShift -= 1;
            
            NUMBERS[j]->decimalPlaceCount = -NUMBERS[j]->decimalShift;
        }
        // Digit numbers & analog pointer available 
        else if (NUMBERS[j]->digit_roi && NUMBERS[j]->analog_roi) {
            NUMBERS[j]->decimalPlaceCount = NUMBERS[j]->analogCount - NUMBERS[j]->decimalShift;

            if (NUMBERS[j]->isExtendedResolution && flowAnalog->CNNTypeWithExtendedResolution())
                NUMBERS[j]->decimalPlaceCount += 1;
        }

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "setDecimalShift: Sequence %i, decimalPlace %i, DecShift %i", j, NUMBERS[j]->decimalPlaceCount, NUMBERS[j]->decimalShift);
        #endif
    }
}


std::string ClassFlowPostProcessing::ShiftDecimal(std::string _value, int _decShift){

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


bool ClassFlowPostProcessing::doFlow(std::string zwtime)
{
    PresetFlowStateHandler();
    int resultPreviousNumberAnalog = -1;

    time_t _timeProcessed = flowTakeImage->getTimeImageTaken();
    if (_timeProcessed == 0)
        time(&_timeProcessed);

    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGI(TAG, "Quantity of number sequences: %d", NUMBERS.size());
    #endif

    /* Post-processing for all defined number sequences */
    for (int j = 0; j < NUMBERS.size(); ++j) {
        NUMBERS[j]->timeProcessed = _timeProcessed; // Update process time in seconds
        NUMBERS[j]->sTimeProcessed = ConvertTimeToString(NUMBERS[j]->timeProcessed, TIME_FORMAT_OUTPUT);
        NUMBERS[j]->isActualValueANumber = true;
        NUMBERS[j]->isActualValueConfirmed = true;

        /* Process analog numbers of sequence */
        if (NUMBERS[j]->analog_roi) {      
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "doFlow: Get analog numbers");
            NUMBERS[j]->sRawValue = flowAnalog->getReadout(j, NUMBERS[j]->isExtendedResolution);

            if (NUMBERS[j]->sRawValue.length() > 0) {
                if (NUMBERS[j]->sRawValue[0] >= 48 && NUMBERS[j]->sRawValue[0] <= 57) // Most significant analog value a number?
                    resultPreviousNumberAnalog = NUMBERS[j]->sRawValue[0] - 48;
            }
        }

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After analog->getReadout: RawValue %s", NUMBERS[j]->sRawValue.c_str());
        #endif

        /* Add decimal separator */
        if (NUMBERS[j]->digit_roi && NUMBERS[j]->analog_roi)
            NUMBERS[j]->sRawValue = "." + NUMBERS[j]->sRawValue;

        /* Process digit numbers of sequence */
        if (NUMBERS[j]->digit_roi) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "doFlow: Get digit numbers");
            if (NUMBERS[j]->analog_roi) // If analog numbers available
                NUMBERS[j]->sRawValue = flowDigit->getReadout(j, false, NUMBERS[j]->analog_roi->ROI[0]->CNNResult, resultPreviousNumberAnalog, 
                                                                    NUMBERS[j]->analogDigitalTransitionStart) + NUMBERS[j]->sRawValue;
            else
                NUMBERS[j]->sRawValue = flowDigit->getReadout(j, NUMBERS[j]->isExtendedResolution); // Extended resolution for digits only if no analog previous number
        }

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After digital->getReadout: RawValue %s", NUMBERS[j]->sRawValue.c_str());
        #endif

        /* Apply parametrized decimal shift */
        NUMBERS[j]->sRawValue = ShiftDecimal(NUMBERS[j]->sRawValue, NUMBERS[j]->decimalShift);

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After ShiftDecimal: RawValue %s", NUMBERS[j]->sRawValue.c_str());
        #endif

        /* Remove leading N */
        if (IgnoreLeadingNaN)               
            while ((NUMBERS[j]->sRawValue.length() > 1) && (NUMBERS[j]->sRawValue[0] == 'N'))
                NUMBERS[j]->sRawValue.erase(0, 1);

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After IgnoreLeadingNaN: RawValue %s", NUMBERS[j]->sRawValue.c_str());
        #endif

        /* Use fully processed "Raw Value" and transfer to "Value" for further processing */
        NUMBERS[j]->sActualValue = NUMBERS[j]->sRawValue;

        /* Substitute any N position with last valid number if available */
        if (findDelimiterPos(NUMBERS[j]->sActualValue, "N") != std::string::npos) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Substitude N positions for number sequence: " + NUMBERS[j]->name);
            if (UseFallbackValue && NUMBERS[j]->isFallbackValueValid) { // fallbackValue can be used to replace the N
                NUMBERS[j]->sActualValue = SubstitudeN(NUMBERS[j]->sActualValue, NUMBERS[j]->fallbackValue); 
            }
            else { // fallbackValue not valid to replace any N
                if (!UseFallbackValue)
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Activate parameter \'Use Fallback Value\' to be able to substitude N positions");

                NUMBERS[j]->ratePerMin = 0;
                NUMBERS[j]->ratePerProcessing = 0;
                NUMBERS[j]->sRatePerMin =  to_stringWithPrecision(NUMBERS[j]->ratePerMin, NUMBERS[j]->decimalPlaceCount+1);
                NUMBERS[j]->sRatePerProcessing = to_stringWithPrecision(NUMBERS[j]->ratePerProcessing, NUMBERS[j]->decimalPlaceCount);

                NUMBERS[j]->sValueStatus = std::string(VALUE_STATUS_001_NO_DATA_N_SUBST) + " | Raw: " + NUMBERS[j]->sRawValue;
                NUMBERS[j]->sActualValue = "";
                NUMBERS[j]->isActualValueANumber = false;
                NUMBERS[j]->isActualValueConfirmed = false;
      
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Sequence: " + NUMBERS[j]->name + ": Status: " + NUMBERS[j]->sValueStatus);

                WriteDataLog(j);
                continue; // Stop here, no valid number because there are still N.
            }
        }

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After SubstitudeN: ActualValue %s", NUMBERS[j]->sActualValue.c_str());
        #endif

        /* Delete leading zeros (unless there is only one 0 left) */
        while ((NUMBERS[j]->sActualValue.length() > 1) && (NUMBERS[j]->sActualValue[0] == '0'))
            NUMBERS[j]->sActualValue.erase(0, 1);
        
        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After removeLeadingZeros: ActualValue %s", NUMBERS[j]->sActualValue.c_str());
        #endif

        /* Convert actual value to double interpretation */
        NUMBERS[j]->actualValue = std::stod(NUMBERS[j]->sActualValue);

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "After converting to double: sActualValue: %s, actualValue: %f", NUMBERS[j]->sActualValue, NUMBERS[j]->actualValue);
        #endif

        if (UseFallbackValue) {
            /* Is fallbackValue OK (== not too old)*/
            if (NUMBERS[j]->isFallbackValueValid) {
                /* Update fallbackValue */
                NUMBERS[j]->sFallbackValue = to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount);

                /* Check digit plausibitily (only support and necessary for class-11 models (0-9 + NaN)) */
                if (NUMBERS[j]->checkDigitIncreaseConsistency) {
                    if (flowDigit) {
                        if (flowDigit->getCNNType() != Digital)
                            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip \'Digit Increase Consistency\' check, only applicable for dig-class11 models"); 
                        else {
                            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Check digit increase consistency for number sequence: " + NUMBERS[j]->name);
                            NUMBERS[j]->actualValue = checkDigitConsistency(NUMBERS[j]->actualValue, NUMBERS[j]->decimalShift, 
                                                                            NUMBERS[j]->analog_roi != NULL, NUMBERS[j]->fallbackValue);
                        }
                    }
                    else {
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip \'Digit Increase Consistency\' check, no digit numbers configured"); 
                    }
                }

                #ifdef DEBUG_DETAIL_ON 
                    ESP_LOGI(TAG, "After checkDigitIncreaseConsistency: actualValue %f", NUMBERS[j]->actualValue);
                #endif

                /* Update Rates */
                //Calculate delta time between this reading und last valid reading in seconds
                long timeDeltaToFallbackValue = abs((long)difftime(NUMBERS[j]->timeProcessed, NUMBERS[j]->timeFallbackValue)); // absolute delta in seconds

                if (timeDeltaToFallbackValue > 0) {
                    NUMBERS[j]->ratePerMin = (NUMBERS[j]->actualValue - NUMBERS[j]->fallbackValue) / (timeDeltaToFallbackValue / 60.0); // calculate rate / minute
                }
                else {
                    NUMBERS[j]->ratePerMin = 0;
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Rate per minute calculation not possible, time delta is zero");
                }
                
                NUMBERS[j]->ratePerProcessing = NUMBERS[j]->actualValue - NUMBERS[j]->fallbackValue;
                double RatePerSelection;  
                    if (NUMBERS[j]->rateType == rtRatePerMin)
                        RatePerSelection = NUMBERS[j]->ratePerMin;
                    else
                        RatePerSelection = NUMBERS[j]->ratePerProcessing; // If Rate check is off, use 'RatePerProcessing' for display only purpose (easier to interprete)

                /* Check for rate too high */
                if (NUMBERS[j]->useMaxRateValue) {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Check rate limit for number sequence: " + NUMBERS[j]->name);
                    if (abs(RatePerSelection) > abs((double)NUMBERS[j]->maxRateValue)) {
                        if (RatePerSelection < 0)
                            NUMBERS[j]->sValueStatus  = std::string(VALUE_STATUS_003_RATE_TOO_HIGH_NEG);
                        else
                            NUMBERS[j]->sValueStatus  = std::string(VALUE_STATUS_004_RATE_TOO_HIGH_POS);

                        NUMBERS[j]->sValueStatus += " | Value: " + to_stringWithPrecision(NUMBERS[j]->actualValue, NUMBERS[j]->decimalPlaceCount) + 
                                                    ", Fallback: " + NUMBERS[j]->sFallbackValue + 
                                                    ", Rate: " + to_stringWithPrecision(RatePerSelection, NUMBERS[j]->decimalPlaceCount);
                         
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Sequence: " + NUMBERS[j]->name + ": Status: " + NUMBERS[j]->sValueStatus);           
                        NUMBERS[j]->isActualValueConfirmed = false;
                    }
                }

                #ifdef DEBUG_DETAIL_ON 
                    ESP_LOGI(TAG, "After MaxRateCheck: actualValue %f", NUMBERS[j]->actualValue);
                #endif

                /* Check for negative rate */
                if (!NUMBERS[j]->allowNegativeRates && NUMBERS[j]->isActualValueConfirmed) {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Check negative rate for number sequence: " + NUMBERS[j]->name);
                    if (NUMBERS[j]->actualValue < NUMBERS[j]->fallbackValue) {
                        NUMBERS[j]->sValueStatus  = std::string(VALUE_STATUS_002_RATE_NEGATIVE);                      
                        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + NUMBERS[j]->name + 
                                                                ": Rate negative, use fallback | Value: " + std::to_string(NUMBERS[j]->actualValue) +
                                                                ", Fallback: " + std::to_string(NUMBERS[j]->fallbackValue) +
                                                                ", Rate: " + to_stringWithPrecision(RatePerSelection, NUMBERS[j]->decimalPlaceCount));
                        NUMBERS[j]->isActualValueConfirmed = false;
                    }
                }

                #ifdef DEBUG_DETAIL_ON 
                    ESP_LOGI(TAG, "After allowNegativeRates: actualValue %f", NUMBERS[j]->actualValue);
                #endif
            }
            else {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Fallback Value not valid");
            }

            /* Update string outputs */
            if (NUMBERS[j]->isActualValueANumber && NUMBERS[j]->isActualValueConfirmed) { // Value of actual reading is valid
                // Save value as fallback value
                NUMBERS[j]->fallbackValue = NUMBERS[j]->actualValue;
                NUMBERS[j]->sFallbackValue = to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount);     
                NUMBERS[j]->timeFallbackValue = NUMBERS[j]->timeProcessed;
                NUMBERS[j]->sTimeFallbackValue = ConvertTimeToString(NUMBERS[j]->timeFallbackValue, TIME_FORMAT_OUTPUT);
                NUMBERS[j]->isFallbackValueValid = true;
                UpdateFallbackValue = true;

                NUMBERS[j]->sValueStatus = std::string(VALUE_STATUS_000_VALID); 
            }
            else { // Value of actual reading is invalid, use fallback + rates = 0
                NUMBERS[j]->ratePerMin = 0;
                NUMBERS[j]->ratePerProcessing = 0;
                NUMBERS[j]->actualValue = NUMBERS[j]->fallbackValue;
            }
        }
        else { // FallbackValue usage disabled, no rate checking possible
            NUMBERS[j]->ratePerMin = 0;
            NUMBERS[j]->ratePerProcessing = 0;
            NUMBERS[j]->fallbackValue = 0;
            NUMBERS[j]->sFallbackValue = "";
            NUMBERS[j]->sValueStatus = std::string(VALUE_STATUS_000_VALID); 
        }

        /* Write output values */
        NUMBERS[j]->sRatePerMin =  to_stringWithPrecision(NUMBERS[j]->ratePerMin, NUMBERS[j]->decimalPlaceCount+1);
        NUMBERS[j]->sRatePerProcessing = to_stringWithPrecision(NUMBERS[j]->ratePerProcessing, NUMBERS[j]->decimalPlaceCount);
        NUMBERS[j]->sActualValue = to_stringWithPrecision(NUMBERS[j]->actualValue, NUMBERS[j]->decimalPlaceCount);

        /* Write log file entry */
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, NUMBERS[j]->name + ": Value: " + NUMBERS[j]->sActualValue + 
                                                                  ", Rate per min: " + NUMBERS[j]->sRatePerMin + 
                                                                  ", Status: " + NUMBERS[j]->sValueStatus);

        WriteDataLog(j);
    }

    SaveFallbackValue();
    return true;
}


void ClassFlowPostProcessing::WriteDataLog(int _index)
{
    if (!LogFile.GetDataLogToSD()) {
        return;
    }
    
    std::string analog = "";
    std::string digital = "";
    
    if (flowAnalog)
        analog = flowAnalog->getReadoutRawString(_index);
    
    if (flowDigit)
        digital = flowDigit->getReadoutRawString(_index);
    
    LogFile.WriteToData(NUMBERS[_index]->sTimeProcessed, NUMBERS[_index]->name, 
                        NUMBERS[_index]->sRawValue, NUMBERS[_index]->sActualValue, NUMBERS[_index]->sFallbackValue, 
                        NUMBERS[_index]->sRatePerMin, NUMBERS[_index]->sRatePerProcessing,
                        NUMBERS[_index]->sValueStatus.substr(0,3), 
                        digital, analog);
    
    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGI(TAG, "WriteDataLog: %s, %s, %s, %s, %s", NUMBERS[_index]->sRawValue.c_str(), NUMBERS[_index]->sActualValue.c_str(), 
                            NUMBERS[_index]->sValueStatus.substr(0,3).c_str(), digital.c_str(), analog.c_str());
    #endif
}


std::string ClassFlowPostProcessing::SubstitudeN(std::string input, double _fallbackValue)
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


float ClassFlowPostProcessing::checkDigitConsistency(double input, int _decilamshift, bool _isanalog, double _fallbackValue){
    int aktdigit, olddigit;
    int aktdigit_before, olddigit_before;
    int pot, pot_max;
    float zw;
    bool no_nulldurchgang = false;

    pot = _decilamshift;
    if (!_isanalog)             // if there are no analogue values, the last one cannot be evaluated
    {
        pot++;
    }
    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGD(TAG, "checkDigitConsistency: pot=%d, decimalShift=%d", pot, _decilamshift);
    #endif
    pot_max = ((int) log10(input)) + 1;
    while (pot <= pot_max)
    {
        zw = input / pow(10, pot-1);
        aktdigit_before = ((int) zw) % 10;
        zw = _fallbackValue / pow(10, pot-1);
        olddigit_before = ((int) zw) % 10;

        zw = input / pow(10, pot);
        aktdigit = ((int) zw) % 10;
        zw = _fallbackValue / pow(10, pot);
        olddigit = ((int) zw) % 10;

        no_nulldurchgang = (olddigit_before <= aktdigit_before);

        if (no_nulldurchgang)
        {
            if (aktdigit != olddigit) 
            {
                input = input + ((float) (olddigit - aktdigit)) * pow(10, pot);     // New Digit is replaced by old Digit;
            }
        }
        else
        {
            if (aktdigit == olddigit)                   // despite zero crossing, digit was not incremented --> add 1
            {
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


std::string ClassFlowPostProcessing::GetFallbackValue(std::string _number)
{
    std::string result;
    int index = -1;

    if (_number == "")
        _number = "default"; 

    for (int i = 0; i < NUMBERS.size(); ++i)
        if (NUMBERS[i]->name == _number)
            index = i;

    if (index == -1)
        return std::string("");

    result = to_stringWithPrecision(NUMBERS[index]->fallbackValue, NUMBERS[index]->decimalPlaceCount);

    return result;
}


bool ClassFlowPostProcessing::SetFallbackValue(double _newvalue, std::string _numbersname)
{
    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGI(TAG, "SetFallbackValue: %f, %s", _newvalue, _numbersname.c_str());
    #endif

    for (int j = 0; j < NUMBERS.size(); ++j) {
        //ESP_LOGD(TAG, "Number %d, %s", j, NUMBERS[j]->name.c_str());
        if (NUMBERS[j]->name == _numbersname) {
            if (_newvalue >= 0) {  // if new value posivive, use provided value to preset fallbackValue
                NUMBERS[j]->fallbackValue = _newvalue;
            }
            else {          // if new value negative, use last raw value to preset fallbackValue
                char* p;
                double ReturnRawValueAsDouble = strtod(NUMBERS[j]->sRawValue.c_str(), &p);
                if (ReturnRawValueAsDouble == 0) {
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "SetFallbackValue: RawValue not a valid value for further processing: "
                                                            + NUMBERS[j]->sRawValue);
                    return false;
                }
                NUMBERS[j]->fallbackValue = ReturnRawValueAsDouble;
            }

            time(&(NUMBERS[j]->timeFallbackValue)); // timezone already set at boot
            NUMBERS[j]->sTimeFallbackValue = ConvertTimeToString(NUMBERS[j]->timeFallbackValue, TIME_FORMAT_OUTPUT);
            NUMBERS[j]->sFallbackValue = to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount + 1);
            NUMBERS[j]->isFallbackValueValid = true;

            //ESP_LOGD(TAG, "Found %d! - set to %.8f", j,  NUMBERS[j]->fallbackValue);
            
            UpdateFallbackValue = true;   // Only update fallbackValue file if a new value is set
            SaveFallbackValue();

            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "SetFallbackValue: FallbackValue for " + NUMBERS[j]->name + " set to " + 
                                                     std::to_string(NUMBERS[j]->fallbackValue));
            return true;
        }
    }
    
    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "SetFallbackValue: Numbersname not found or not valid");
    return false;   // No new value was set (e.g. wrong numbersname, no numbers at all)
}


bool ClassFlowPostProcessing::LoadFallbackValue(void)
{
    esp_err_t err = ESP_OK;

    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READONLY, &fallbackvalue_nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: No valid NVS handle - error code: " + std::to_string(err));
        return false;
    }
    else if (err != ESP_OK && (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NVS_INVALID_HANDLE)) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "LoadFallbackValue: No NVS data avilable for namespace 'fallbackvalue'");
        return false;
    }

    int16_t numbers_size = 0;
    err = nvs_get_i16(fallbackvalue_nvshandle, "numbers_size", &numbers_size);   // Use numbers size to ensure that only already saved data will be loaded
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_i16 numbers_size - error code: " + std::to_string(err));
        return false;
    }

    for (int i = 0; i < numbers_size; ++i) {
        // Name: Read from NVS
        size_t required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str name size - error code: " + std::to_string(err));
            return false;
        }

        char cName[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("name" + std::to_string(i)).c_str(), cName, &required_size);
            if (err != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str name - error code: " + std::to_string(err));
                return false;
            }
        }

        // Timestamp: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str timestamp size - error code: " + std::to_string(err));
            return false;
        }

        char cTime[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("time" + std::to_string(i)).c_str(), cTime, &required_size);
            if (err != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str timestamp - error code: " + std::to_string(err));
                return false;
            }
        }

        // Value: Read from NVS
        required_size = 0;
        err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), NULL, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str fallbackvalue size - error code: " + std::to_string(err));
            return false;
        }

        char cValue[required_size+1];
        if (required_size > 0) {
            err = nvs_get_str(fallbackvalue_nvshandle, ("value" + std::to_string(i)).c_str(), cValue, &required_size);
            if (err != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadFallbackValue: nvs_get_str fallbackvalue - error code: " + std::to_string(err));
                return false;
            }
        }

        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "LoadFallbackValue: Sequence: %s, Time: %s, Value: %s", cName, cTime, cValue);
        #endif

        for (int j = 0; j < NUMBERS.size(); ++j)
        {           
            if ((NUMBERS[j]->name).compare(std::string(cName)) == 0)
            {

                NUMBERS[j]->fallbackValue = stod(std::string(cValue));
                NUMBERS[j]->sFallbackValue = to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount + 1);      // To be on the safe side, 1 digit more, as Exgtended Resolution may be on (will only be set during the first run).

                time_t tStart;
                int yy, month, dd, hh, mm, ss;
                struct tm whenStart;

                sscanf(cTime, FALLBACKVALUE_TIME_FORMAT_INPUT, &yy, &month, &dd, &hh, &mm, &ss);
                whenStart.tm_year = yy - 1900;
                whenStart.tm_mon = month - 1;
                whenStart.tm_mday = dd;
                whenStart.tm_hour = hh;
                whenStart.tm_min = mm;
                whenStart.tm_sec = ss;
                whenStart.tm_isdst = -1;

                NUMBERS[j]->timeFallbackValue = mktime(&whenStart);

                time(&tStart);
                int AgeInMinutes = (int)(difftime(tStart, NUMBERS[j]->timeFallbackValue) / 60.0); // delta in minutes
                if (AgeInMinutes > FallbackValueAgeStartup) {
                    NUMBERS[j]->isFallbackValueValid = false;
                    NUMBERS[j]->fallbackValue = 0;
                    NUMBERS[j]->sFallbackValue = "";
                }
                else {
                    NUMBERS[j]->isFallbackValueValid = true;
                }
            }
        }
    }
    nvs_close(fallbackvalue_nvshandle);
    
    return true;
}


bool ClassFlowPostProcessing::SaveFallbackValue()
{ 
    if (!UpdateFallbackValue)         // fallbackValue unchanged
        return false;
    
    esp_err_t err = ESP_OK;    
    nvs_handle_t fallbackvalue_nvshandle;

    err = nvs_open("fallbackvalue", NVS_READWRITE, &fallbackvalue_nvshandle);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_set_i16(fallbackvalue_nvshandle, "numbers_size", (int16_t)NUMBERS.size());    // Save numbers size to ensure that only already saved data will be loaded
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: nvs_set_i16 numbers_size - error code: " + std::to_string(err));
        return false;
    }

    for (int j = 0; j < NUMBERS.size(); ++j)
    {           
        #ifdef DEBUG_DETAIL_ON 
            ESP_LOGI(TAG, "SaveFallbackValue: Sequence: %s, Time: %s, Value: %s", (NUMBERS[j]->name).c_str(), (NUMBERS[j]->sTimeFallbackValue).c_str(), 
                        (to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount)).c_str());
        #endif
        
        err = nvs_set_str(fallbackvalue_nvshandle, ("name" + std::to_string(j)).c_str(), (NUMBERS[j]->name).c_str());
        if (err != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: nvs_set_str name - error code: " + std::to_string(err));
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("time" + std::to_string(j)).c_str(), (NUMBERS[j]->sTimeFallbackValue).c_str());
        if (err != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: nvs_set_str timestamp - error code: " + std::to_string(err));
            return false;
        }
        err = nvs_set_str(fallbackvalue_nvshandle, ("value" + std::to_string(j)).c_str(), 
                            (to_stringWithPrecision(NUMBERS[j]->fallbackValue, NUMBERS[j]->decimalPlaceCount)).c_str());
        if (err != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: nvs_set_str fallbackvalue - error code: " + std::to_string(err));
            return false;
        }
    }

    err = nvs_commit(fallbackvalue_nvshandle);
    nvs_close(fallbackvalue_nvshandle);

    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveFallbackValue: nvs_commit - error code: " + std::to_string(err));
        return false;
    }

    return true;
}


std::string ClassFlowPostProcessing::getReadout(int _number)
{
    return NUMBERS[_number]->sActualValue;
}


std::string ClassFlowPostProcessing::getReadoutParam(bool _rawValue, bool _noerror, int _number)
{
    if (_rawValue)
        return NUMBERS[_number]->sRawValue;
    
    if (_noerror)
        return NUMBERS[_number]->sActualValue;
    
    return NUMBERS[_number]->sActualValue;
}


std::string ClassFlowPostProcessing::getReadoutRate(int _number)
{
    return std::to_string(NUMBERS[_number]->ratePerMin);
}


std::string ClassFlowPostProcessing::getReadoutTimeStamp(int _number)
{
   return NUMBERS[_number]->sTimeFallbackValue;
}


std::string ClassFlowPostProcessing::getReadoutError(int _number) 
{
    return NUMBERS[_number]->sValueStatus;
}



bool ClassFlowPostProcessing::getUseFallbackValue(void)
{
    return UseFallbackValue;
}


void ClassFlowPostProcessing::setUseFallbackValue(bool _value)
{
    UseFallbackValue = _value;
}


std::string ClassFlowPostProcessing::getNumbersName()
{
    std::string ret="";

    for (int i = 0; i < NUMBERS.size(); ++i)
    {
        ret += NUMBERS[i]->name;
        if (i < NUMBERS.size()-1)
            ret = ret + "\t";
    }

    #ifdef DEBUG_DETAIL_ON 
        ESP_LOGI(TAG, "Result ClassFlowPostProcessing::getNumbersName: %s", ret.c_str());
    #endif

    return ret;
}


std::string ClassFlowPostProcessing::GetJSON(std::string _lineend)
{
    std::string json="{" + _lineend;

    for (int i = 0; i < NUMBERS.size(); ++i)
    {
        json += "\"" + NUMBERS[i]->name + "\":"  + _lineend;

        json += getJsonFromNumber(i, _lineend) + _lineend;

        if ((i+1) < NUMBERS.size())
            json += "," + _lineend;
    }
    json += "}";

    return json;
}


std::string ClassFlowPostProcessing::getJsonFromNumber(int i, std::string _lineend) {
	std::string json = "";

	json += "  {" + _lineend;
	json += "    \"actual_value\": \"" + NUMBERS[i]->sActualValue + "\"," + _lineend;
	json += "    \"fallback_value\": \"" + NUMBERS[i]->sFallbackValue + "\"," + _lineend;
	json += "    \"raw_value\": \"" + NUMBERS[i]->sRawValue + "\"," + _lineend;
	json += "    \"value_status\": \"" + NUMBERS[i]->sValueStatus + "\"," + _lineend;
	json += "    \"rate_per_min\": \"" + NUMBERS[i]->sRatePerMin + "\"," + _lineend;
    json += "    \"rate_per_processing\": \"" + NUMBERS[i]->sRatePerProcessing + "\"," + _lineend;
	json += "    \"timestamp_processed\": \"" + NUMBERS[i]->sTimeProcessed + "\"" + _lineend;

	json += "  }" + _lineend;

	return json;
}


ClassFlowPostProcessing::~ClassFlowPostProcessing()
{
    // nothing to do
}
