#include "ClassFlowCNNGeneral.h"

#include <math.h>
#include <iomanip> 
#include <sys/types.h>
#include <sstream>      // std::stringstream

#include "ClassLogFile.h"
#include "esp_log.h"
#include "../../include/defines.h"

static const char* TAG = "CNN";

//#ifdef CONFIG_HEAP_TRACING_STANDALONE
#ifdef HEAP_TRACING_CLASS_FLOW_CNN_GENERAL_DO_ALING_AND_CUT
    #include <esp_heap_trace.h>
    #define NUM_RECORDS 300
    static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
#endif


ClassFlowCNNGeneral::ClassFlowCNNGeneral(ClassFlowAlignment *_flowalign, std::string _cnnname, t_CNNType _cnntype) : ClassFlowImage(NULL, TAG)
{
    PresetFlowStateHandler(true);
    tflite = new CTfLiteClass;
    cnnname = _cnnname;
    CNNType = _cnntype;
    cnnmodelfile = "";
    modelxsize = 32;
    modelysize = 32;
    modelchannel = 3;
    CNNGoodThreshold = 0.0;
    ListFlowControll = NULL;
    previousElement = NULL;   
    SaveAllFiles = false; 
    disabled = false;
    isLogImageSelect = false;
    flowpostalignment = _flowalign;
    imagesRetention = 5;
}


std::string ClassFlowCNNGeneral::getReadout(int _seqNo, bool _extendedResolution, int _valuePreviousNumber,
                                             int _resultPreviousNumber, int analogDigitalTransitionStart)
{ 
    if (GENERAL[_seqNo]->ROI.size() == 0) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout: No numbers in selected number sequence");
        return std::string("");
    }
    
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout: Number sequence: " + std::to_string(_seqNo) + 
                                                ", extendedResolution: " + std::to_string(_extendedResolution) + 
                                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) + 
                                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber) + 
                                                ", analogDigitalTransitionStart: " + to_stringWithPrecision(analogDigitalTransitionStart/10.0, 1));
 
    if (CNNType == Analogue || CNNType == Analogue100) { // Class-analog-model, ana-class100-model
        std::string result = "";
        int resultTemp = -1;
        int lastROI = GENERAL[_seqNo]->ROI.size() - 1;

        // Evaluate last number of number sequence (and potentially correct)
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(lastROI));
        resultTemp = EvalAnalogNumber(GENERAL[_seqNo]->ROI[lastROI]->CNNResult, -1);

        if (_extendedResolution) {
            int resultDecimalPlace = GENERAL[_seqNo]->ROI[lastROI]->CNNResult % 10; // Decimal place of number result
            result = std::to_string(resultTemp) + std::to_string(resultDecimalPlace);
        }
        else {
            result = std::to_string(resultTemp);
        }

        for (int i = lastROI - 1; i >= 0; --i) // Evaluate all remaining number of number sequence (and potentially correct)
        {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(i));
            resultTemp = EvalAnalogNumber(GENERAL[_seqNo]->ROI[i]->CNNResult, resultTemp); 
            result = std::to_string(resultTemp) + result;
        }

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout: Analog (ana-cont/ana-class100) Result: " + result);
        return result;
    }

    else if ((CNNType == DoubleHyprid10) || (CNNType == Digital100)) { // dig-cont-model, dig-class100-model
        std::string result = "";
        int resultTemp = -1;
        int lastROI = GENERAL[_seqNo]->ROI.size() - 1;

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(lastROI));
        resultTemp = GENERAL[_seqNo]->ROI[lastROI]->CNNResult;

        if (!GENERAL[_seqNo]->ROI[lastROI]->isRejected) { // valid result (e.g. model 'dig-cont*' --> bad fit)?
            if (_extendedResolution) { // NOTE: Ensure that this flag is only set if no analog previous number is available
                result = std::to_string(resultTemp);
                resultTemp = resultTemp / 10; // resultIntergerPart to hand over a previous result to next digit evaluation
            }
            else {
                if (_valuePreviousNumber >= 0) // If previous number available (analog value should be handed over)
                    resultTemp = EvalDigitNumber(GENERAL[_seqNo]->ROI[lastROI]->CNNResult, _valuePreviousNumber, 
                                                        _resultPreviousNumber, true, analogDigitalTransitionStart);
                else
                    resultTemp = EvalDigitNumber(GENERAL[_seqNo]->ROI[lastROI]->CNNResult, -1, -1); // No previous number
                
                result = std::to_string(resultTemp);
            }
        }
        else {
            result = "N";
            if (_extendedResolution)
                result = "NN";
            
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont/dig-class100): Rejected, substitude with N");
        }

        for (int i = lastROI - 1; i >= 0; --i) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(i));
            if (!GENERAL[_seqNo]->ROI[i]->isRejected) { // valid result (e.g. model 'dig-cont*' --> bad fit)?
                resultTemp = EvalDigitNumber(GENERAL[_seqNo]->ROI[i]->CNNResult, GENERAL[_seqNo]->ROI[i+1]->CNNResult, resultTemp);                
                result = std::to_string(resultTemp) + result;
            }
            else {
                resultTemp = -1;
                result = "N" + result;
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont/dig-class100): Rejected, substitude with N");
            }
        }

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout Digit (dig-cont/dig-class100): Result: " + result);
        return result;
    }

    else if (CNNType == Digital) {  // Class-11-model (1-0 + NaN)
        std::string result = "";

        for (int i = 0; i < GENERAL[_seqNo]->ROI.size(); ++i) {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(i));

            if (GENERAL[_seqNo]->ROI[i]->CNNResult == 10) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout: Digit (dig-class11): Result ambiguous, substitude with N");
                result = result + "N";
            }
            else {
                result = result + std::to_string(GENERAL[_seqNo]->ROI[i]->CNNResult);
            }
        }
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "getReadout: Digit (dig-class11) Result: " + result);
        return result;
    }

    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "getReadout: CNN Type unknown");
        return std::string("");
    }
}


/* Evaluate analog number pointer */
int ClassFlowCNNGeneral::EvalAnalogNumber(int _value, int _resultPreviousNumber)
{
    int result = -1;

    if (_resultPreviousNumber <= -1)
    {
        result = _value / 10; // Return IntegerPart, remove decimal place (73 -> 7)
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogNumber (No previous number): Result: " + std::to_string(result) +
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
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogNumber (Ambiguous, use value + corretion): Result = " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
        else if (_resultPreviousNumber >= 10 - Analog_error)
        {
            result = valueMin / 10;
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogNumber (Ambiguous, use value - corretion): Result: " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            return result;
        }
    }
    
    result = _value / 10;
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogNumber (Unambiguous, use value): Result: " + std::to_string(result) +
                                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    return result;
}


/* Evaluate digit number */
int ClassFlowCNNGeneral::EvalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, 
                                              bool _isPreviousAnalog, int digitalAnalogTransitionStart)
{
    int result = - 1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    if (_resultPreviousNumber <= -1) { // no previous number -> no correction logic for transition needed, use value as is (integer part)
        result = resultIntergerPart;
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalDigitNumber (No previous number): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1));
        return result;
    }

    // previous number is analog (_valuePreviousNumber: 0-99), special transistion check needed 
    if (_isPreviousAnalog) {
        result = EvalAnalogToDigitTransition(_value, _valuePreviousNumber, _resultPreviousNumber, digitalAnalogTransitionStart);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalDigitNumber (Analog previous number): Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                    ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) + 
                                                    ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
        return result;
    }

    // Previous number is digital (_valuePreviousNumber: 0-99) No digit change, because predecessor is far enough away (+/- Digital_Transition_Area_Predecessor)
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

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalDigitNumber (Digit previous number: no zero crossing, \'safe area\'): Result: " + std::to_string(result) +
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
        
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalDigitNumber (Digit previous number: zero crossing): Result: " + std::to_string(result) +
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
    if (Digital_Transition_Area_Forward >= _valuePreviousNumber || resultDecimalPlace >= 4) {
        result =  resultIntergerPart; // The current digit, like the previous digit, does not yet have a zero crossing. 
    }
    else {
        // current digit precedes the smaller digit (9.x). So already >=x.0 while the previous digit has not yet
        // has no zero crossing. Therefore, it is reduced by 1.
        result =  resultIntergerPart - 1;

        if (result < 0)
            result = 9;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalDigitNumber (Digit previous number: no zero crossing yet): Result: " + std::to_string(result) +
                                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) + 
                                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    
    return result;
}


/* Evaluate analog to digit number transition */
int ClassFlowCNNGeneral::EvalAnalogToDigitTransition(int _value, int _valuePreviousNumber,  int _resultPreviousNumber, int analogDigitalTransitionStart)
{
    int result = - 1;
    int resultIntergerPart = _value / 10;
    int resultDecimalPlace = _value % 10;

    // Value within the digital inequalities 
    if (resultDecimalPlace >= (10 - Digital_Uncertainty)    // Band around the zero crossing -> Round, as number reaches inaccuracy zone
        || (_resultPreviousNumber <= 4 && resultDecimalPlace >= 6)) // or number runs after (previous result <= 4, actucal decimal place >= 6)
    {   
        if (resultDecimalPlace >= 5) { // "Round up"
            result = resultIntergerPart + 1;

            if (result >= 10)
                result = 0;

            // Correct back if no zero crossing detected
            if (_resultPreviousNumber >= 6 && (_valuePreviousNumber > analogDigitalTransitionStart || _valuePreviousNumber <= 2)) // analogDigitalTransitionStart < _valuePreviousNumber < 0.2
            {
                result = result - 1;
                if (result < 0)
                    result = 9;
                
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogToDigitTransition (Digital uncertainty, no zero crossing): Result: " + std::to_string(result) +
                                                        ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                                        ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                                        ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
            }
        }
        else {
            result = resultIntergerPart; // "Trunc -> Round down"
        }

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogToDigitTransition (Digital uncertainty): Result: " + std::to_string(result) +
                                ", Value: " + to_stringWithPrecision(_value/10.0, 1) + 
                                ", valuePreviousNumber: " + to_stringWithPrecision(_valuePreviousNumber/10.0, 1) +
                                ", resultPreviousNumber: " + std::to_string(_resultPreviousNumber));
    } 
    else {
        result = resultIntergerPart;
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "EvalAnalogToDigitTransition: Result: " + std::to_string(result) +
                                                    ", Value: " + to_stringWithPrecision(_value/10.0, 1));
    }

    return result;
}


bool ClassFlowCNNGeneral::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    bool bRetVal = true;
    std::vector<std::string> splitted;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;


    if ((toUpper(aktparamgraph) != "[ANALOG]") && (toUpper(aktparamgraph) != ";[ANALOG]") 
        && (toUpper(aktparamgraph) != "[DIGIT]") && (toUpper(aktparamgraph) != ";[DIGIT]")
        && (toUpper(aktparamgraph) != "[DIGITS]") && (toUpper(aktparamgraph) != ";[DIGITS]")
        )       // Paragraph passt nicht
        return false;

    if (aktparamgraph[0] == ';') {
        disabled = true;
        while (getNextLine(pfile, &aktparamgraph) && !isNewParagraph(aktparamgraph));
        ESP_LOGD(TAG, "[Analog/Digit] is disabled");
        return true;
    }


    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph)) {
        splitted = ZerlegeZeile(aktparamgraph);

        if ((toUpper(splitted[0]) == "MODEL") && (splitted.size() > 1)) {
            this->cnnmodelfile = splitted[1];
        }

        if ((toUpper(splitted[0]) == "CNNGOODTHRESHOLD") && (splitted.size() > 1)) {
            CNNGoodThreshold = std::stof(splitted[1]);
        }
        
        if ((toUpper(splitted[0]) == "ROIIMAGESLOCATION") && (splitted.size() > 1)) {
            this->imagesLocation = "/sdcard" + splitted[1];
            this->isLogImage = true;
        }

        if ((toUpper(splitted[0]) == "ROIIMAGESRETENTION") && (splitted.size() > 1)) {
            this->imagesRetention = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "LOGIMAGESELECT") && (splitted.size() > 1)) {
            LogImageSelect = splitted[1];
            isLogImageSelect = true;            
        }

        if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                SaveAllFiles = true;
            else
                SaveAllFiles = false;
        }
        
        if (splitted.size() >= 5) {
            general* _analog = GetGENERAL(splitted[0], true);
            t_ROI* newROI = _analog->ROI[_analog->ROI.size()-1];
            newROI->posx = std::stoi(splitted[1]);
            newROI->posy = std::stoi(splitted[2]);
            newROI->deltax = std::stoi(splitted[3]);
            newROI->deltay = std::stoi(splitted[4]);
            newROI->CCW = false;
            if (splitted.size() >= 6) {
                newROI->CCW = toUpper(splitted[5]) == "TRUE";
            }
            newROI->CNNResult = -1;
            newROI->image = NULL;
            newROI->image_org = NULL;
        }
    }

    if (!getNetworkParameter())
        bRetVal = false;

    for (int _ana = 0; _ana < GENERAL.size(); ++_ana)
        for (int i = 0; i < GENERAL[_ana]->ROI.size(); ++i) {
            GENERAL[_ana]->ROI[i]->image = new CImageBasis("ROI " + GENERAL[_ana]->ROI[i]->name, 
                    modelxsize, modelysize, modelchannel);
            GENERAL[_ana]->ROI[i]->image_org = new CImageBasis("ROI " + GENERAL[_ana]->ROI[i]->name + "_org",
                    GENERAL[_ana]->ROI[i]->deltax, GENERAL[_ana]->ROI[i]->deltay, 3);
        }

    return bRetVal;
}


general* ClassFlowCNNGeneral::FindGENERAL(std::string _numbername)
{
    for (int i = 0; i < GENERAL.size(); ++i)
        if (GENERAL[i]->name == _numbername)
            return GENERAL[i];
    return NULL;
}


general* ClassFlowCNNGeneral::GetGENERAL(std::string _name, bool _create = true)
{
    std::string numbername, roiname;
    int _pospunkt = _name.find_first_of(".");

    if (_pospunkt > -1) {
        numbername = _name.substr(0, _pospunkt);
        roiname = _name.substr(_pospunkt+1, _name.length() - _pospunkt - 1);
    }
    else {
        numbername = "default";
        roiname = _name;
    }

    general *_ret = NULL;

    for (int i = 0; i < GENERAL.size(); ++i)
        if (GENERAL[i]->name == numbername)
            _ret = GENERAL[i];

    if (!_create)         // not found and should not be created
        return _ret;

    if (_ret == NULL) {
        _ret = new general;
        _ret->name = numbername;
        GENERAL.push_back(_ret);
    }

    t_ROI* newROI = new t_ROI;
    newROI->name = roiname;

    _ret->ROI.push_back(newROI);

    ESP_LOGD(TAG, "GetGENERAL - GENERAL %s - roi %s - CCW: %d", numbername.c_str(), roiname.c_str(), newROI->CCW);

    return _ret;
}


std::string ClassFlowCNNGeneral::getHTMLSingleStep(std::string host)
{
    std::string result, zw;
    std::vector<HTMLInfo*> htmlinfo;

    result = "<p>Found ROIs: </p> <p><img src=\"" + host + "/img_tmp/alg_roi.jpg\"></p>\n";
    result = result + "Analog Pointers: <p> ";

    htmlinfo = GetHTMLInfo();
    for (int i = 0; i < htmlinfo.size(); ++i)
    {
        zw = to_stringWithPrecision(htmlinfo[i]->val, 1);
        result = result + "<img src=\"" + host + "/img_tmp/" +  htmlinfo[i]->filename + "\"> " + zw;
        
        delete htmlinfo[i];
    }
    htmlinfo.clear();         

    return result;
}


bool ClassFlowCNNGeneral::doFlow(std::string time)
{
    #ifdef HEAP_TRACING_CLASS_FLOW_CNN_GENERAL_DO_ALING_AND_CUT
        //register a buffer to record the memory trace
        ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );
        // start tracing
        ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
    #endif

    PresetFlowStateHandler();

    if (disabled)
        return true;

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "start AlignandCut");

    if (!doAlignAndCut(time))
        return false;

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "start NeuralNetwork");

    if (!doNeuralNetwork(time))
        return false;

    RemoveOldLogs();

    #ifdef HEAP_TRACING_CLASS_FLOW_CNN_GENERAL_DO_ALING_AND_CUT
        ESP_ERROR_CHECK( heap_trace_stop() );
        heap_trace_dump(); 
    #endif   

    return true;
}


bool ClassFlowCNNGeneral::doAlignAndCut(std::string time)
{
    if (disabled)
        return true;

    CAlignAndCutImage *caic = flowpostalignment->GetAlignAndCutImage();

    if (caic == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "doAlignAndCut: Aligned image not available");
        return false;
    }

    for (int _ana = 0; _ana < GENERAL.size(); ++_ana)
        for (int i = 0; i < GENERAL[_ana]->ROI.size(); ++i) {
            ESP_LOGD(TAG, "General %d - Align&Cut", i);
            
            caic->CutAndSave(GENERAL[_ana]->ROI[i]->posx, GENERAL[_ana]->ROI[i]->posy, GENERAL[_ana]->ROI[i]->deltax, 
                                GENERAL[_ana]->ROI[i]->deltay, GENERAL[_ana]->ROI[i]->image_org);
            if (SaveAllFiles) {
                if (GENERAL[_ana]->name == "default")
                    GENERAL[_ana]->ROI[i]->image_org->SaveToFile(FormatFileName("/sdcard/img_tmp/" + 
                                                        GENERAL[_ana]->ROI[i]->name + "_org.jpg"));
                else
                    GENERAL[_ana]->ROI[i]->image_org->SaveToFile(FormatFileName("/sdcard/img_tmp/" + GENERAL[_ana]->name + "_" + 
                                                        GENERAL[_ana]->ROI[i]->name + "_org.jpg"));
            } 

            GENERAL[_ana]->ROI[i]->image_org->Resize(modelxsize, modelysize, GENERAL[_ana]->ROI[i]->image);
            if (SaveAllFiles) {
                if (GENERAL[_ana]->name == "default")
                    GENERAL[_ana]->ROI[i]->image->SaveToFile(FormatFileName("/sdcard/img_tmp/" + 
                                                        GENERAL[_ana]->ROI[i]->name + ".jpg"));
                else
                    GENERAL[_ana]->ROI[i]->image->SaveToFile(FormatFileName("/sdcard/img_tmp/" + 
                                                        GENERAL[_ana]->name + "_" + GENERAL[_ana]->ROI[i]->name + ".jpg"));
            } 
        }

    return true;
} 


void ClassFlowCNNGeneral::DrawROI(CImageBasis *_zw)
{
    if (_zw->ImageOkay()) 
    { 
        if (CNNType == Analogue || CNNType == Analogue100) {
            int r = 0;
            int g = 255;
            int b = 0;

            for (int _ana = 0; _ana < GENERAL.size(); ++_ana)
                for (int i = 0; i < GENERAL[_ana]->ROI.size(); ++i) {
                    _zw->drawRect(GENERAL[_ana]->ROI[i]->posx, GENERAL[_ana]->ROI[i]->posy, GENERAL[_ana]->ROI[i]->deltax, 
                                    GENERAL[_ana]->ROI[i]->deltay, r, g, b, 1);
                    
                    _zw->drawEllipse((int) (GENERAL[_ana]->ROI[i]->posx + GENERAL[_ana]->ROI[i]->deltax/2), 
                                        (int)  (GENERAL[_ana]->ROI[i]->posy + GENERAL[_ana]->ROI[i]->deltay/2), 
                                        (int) (GENERAL[_ana]->ROI[i]->deltax/2), (int) (GENERAL[_ana]->ROI[i]->deltay/2), r, g, b, 2);

                    _zw->drawLine((int) (GENERAL[_ana]->ROI[i]->posx + GENERAL[_ana]->ROI[i]->deltax/2), 
                                        (int) GENERAL[_ana]->ROI[i]->posy, (int) (GENERAL[_ana]->ROI[i]->posx + GENERAL[_ana]->ROI[i]->deltax/2), 
                                        (int) (GENERAL[_ana]->ROI[i]->posy + GENERAL[_ana]->ROI[i]->deltay), r, g, b, 2);

                    _zw->drawLine((int) GENERAL[_ana]->ROI[i]->posx, (int) (GENERAL[_ana]->ROI[i]->posy + GENERAL[_ana]->ROI[i]->deltay/2), 
                                        (int) GENERAL[_ana]->ROI[i]->posx + GENERAL[_ana]->ROI[i]->deltax, (int) (GENERAL[_ana]->ROI[i]->posy + 
                                        GENERAL[_ana]->ROI[i]->deltay/2), r, g, b, 2);
                }
        }
        else {
            for (int _dig = 0; _dig < GENERAL.size(); ++_dig)
                for (int i = 0; i < GENERAL[_dig]->ROI.size(); ++i)
                    _zw->drawRect(GENERAL[_dig]->ROI[i]->posx, GENERAL[_dig]->ROI[i]->posy, GENERAL[_dig]->ROI[i]->deltax, 
                                        GENERAL[_dig]->ROI[i]->deltay, 0, 0, (255 - _dig*100), 2);
        }
    }
} 


bool ClassFlowCNNGeneral::getNetworkParameter()
{
    if (disabled)
        return true;

    std::string zwcnn = "/sdcard" + cnnmodelfile;
    zwcnn = FormatFileName(zwcnn);
    ESP_LOGD(TAG, "%s", zwcnn.c_str());
    if (!tflite->LoadModel(zwcnn)) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load model: " + cnnmodelfile);
        LogFile.WriteHeapInfo("getNetworkParameter-LoadModel");
        return false;
    } 

    if (!tflite->MakeAllocate()) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TFLITE: Allocation of tensors failed");
        LogFile.WriteHeapInfo("getNetworkParameter-MakeAllocate");
        return false;
    }

    if (CNNType == AutoDetect) {
        if (tflite->GetInputDimension(false)) {
            modelxsize = tflite->ReadInputDimenstion(0);
            modelysize = tflite->ReadInputDimenstion(1);
            modelchannel = tflite->ReadInputDimenstion(2);
        }
        else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load input dimensions");
            return false;
        }
        int _anzoutputdimensions = tflite->GetAnzOutPut();
        switch (_anzoutputdimensions)
        {
            case -1:
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "TFLITE: Failed to load output dimensions");
                return false;
            case 2:
                CNNType = Analogue;
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Analogue");
                break;
            case 10:
                CNNType = DoubleHyprid10;
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: DoubleHyprid10");
                break;
            case 11:
                CNNType = Digital;
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Digital");
                break;
             case 100:
                if (modelxsize == 32 && modelysize == 32) {
                    CNNType = Analogue100;
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Analogue100");
                } else {
                    CNNType = Digital100;
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CNN-Type: Digital100");
                }
                break;
            default:
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CNN-Type does not fit the firmware (output_dimension=" + std::to_string(_anzoutputdimensions) + ")");
                return false;
        }
    }

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Network parameter loaded: " + cnnmodelfile);

    tflite->CTfLiteClassDeleteInterpreter();
    
    return true;
}


bool ClassFlowCNNGeneral::doNeuralNetwork(std::string time)
{
    if (disabled)
        return true;

    std::string logPath = CreateLogFolder(time);


    if (!tflite->MakeAllocate()) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Allocation of TFLITE tensors failed");
        LogFile.WriteHeapInfo("doNeuralNetwork-MakeAllocate");
        return false;
    }

    for (int n = 0; n < GENERAL.size(); ++n) // For each NUMBER SEQUENCE
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Processing number sequence: " + GENERAL[n]->name);
        for (int roi = 0; roi < GENERAL[n]->ROI.size(); ++roi) // For each ROI
        {
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "ROI #" + std::to_string(roi));

            switch (CNNType) {
                case Analogue: // for models ana-cont*
                {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Type: Analogue (ana-cont)");

                    if (tflite->LoadInputImageBasis(GENERAL[n]->ROI[roi]->image)) {
                        tflite->Invoke();
                        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                        return false;
                    }

                    float result = fmod(atan2(tflite->GetOutputValue(0), tflite->GetOutputValue(1)) / (M_PI * 2) + 2, 1);
                            
                    if(GENERAL[n]->ROI[roi]->CCW) {
                        if (result == 0.0)
                            GENERAL[n]->ROI[roi]->CNNResult = 0;
                        else
                            GENERAL[n]->ROI[roi]->CNNResult = 100 - (result * 100);  // result normalized to 0-99
                    }
                    else {
                        GENERAL[n]->ROI[roi]->CNNResult = result * 100; // result normalized to 0-99
                    }

                    if (GENERAL[n]->ROI[roi]->CNNResult < 0)
                        GENERAL[n]->ROI[roi]->CNNResult = 0;
                    else if (GENERAL[n]->ROI[roi]->CNNResult >= 100)
                        GENERAL[n]->ROI[roi]->CNNResult = 99;

                    GENERAL[n]->ROI[roi]->isRejected = false;
                                                
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Result: " + to_stringWithPrecision(GENERAL[n]->ROI[roi]->CNNResult / 10.0, 1));
                    
                    if (isLogImage)
                        LogImage(logPath, GENERAL[n]->ROI[roi]->name, Analogue, GENERAL[n]->ROI[roi]->CNNResult, time, GENERAL[n]->ROI[roi]->image_org);
                } break;

                case Digital: // for models dig-class11*
                {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Type: Digital (dig-class11)");

                    GENERAL[n]->ROI[roi]->CNNResult = tflite->GetClassFromImageBasis(GENERAL[n]->ROI[roi]->image); // 0-9 + 10 => NaN             
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Result: " + std::to_string(GENERAL[n]->ROI[roi]->CNNResult));

                    if (isLogImage) {
                        std::string imagename = GENERAL[n]->name +  "_" + GENERAL[n]->ROI[roi]->name;

                        if (isLogImageSelect) {
                            if (LogImageSelect.find(GENERAL[n]->ROI[roi]->name) != std::string::npos)
                                LogImage(logPath, imagename, Digital, GENERAL[n]->ROI[roi]->CNNResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                        else {
                            LogImage(logPath, imagename, Digital, GENERAL[n]->ROI[roi]->CNNResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                    }
                } break;

                case DoubleHyprid10: // for models dig-cont*
                {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Type: DoubleHyprid10 (dig-cont)");
                    
                    int LogImageResult;
                    int _num, _numplus, _numminus;
                    float _val, _valplus, _valminus, _fit;

                    if (tflite->LoadInputImageBasis(GENERAL[n]->ROI[roi]->image)) {
                        tflite->Invoke();
                        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
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

                    GENERAL[n]->ROI[roi]->CNNResult = result * 10.0; //  result normalized to 0-99

                    if (GENERAL[n]->ROI[roi]->CNNResult < 0)
                        GENERAL[n]->ROI[roi]->CNNResult = 0;
                    else if (GENERAL[n]->ROI[roi]->CNNResult >= 100)
                        GENERAL[n]->ROI[roi]->CNNResult = 99;

                    /*
                    std::string zw = "num (p, m): " + std::to_string(_num) + " (" + std::to_string(_numplus) + " , " + std::to_string(_numminus) + 
                                     "), val (p, m): " + std::to_string(_val) + " (" + std::to_string(_valplus) + " , " + std::to_string(_valminus) +
                                     "), result: " + to_stringWithPrecision(GENERAL[n]->ROI[roi]->CNNResult/10.0, 1) + ", fit: " + std::to_string(_fit);
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, zw);
                    */

                    if (_fit < CNNGoodThreshold) {
                        GENERAL[n]->ROI[roi]->isRejected = true;
                        LogImageResult = -1 * GENERAL[n]->ROI[roi]->CNNResult;     // In case fit is not sufficient, the result should still be saved with "-x.y".
                        std::string zw = "Result rejected - bad fit (Fit: " + std::to_string(_fit) + ", Threshold: " + std::to_string(CNNGoodThreshold) + ")";
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, zw);
                    }
                    else {
                        GENERAL[n]->ROI[roi]->isRejected = false;
                        LogImageResult = GENERAL[n]->ROI[roi]->CNNResult;
                    }
                    
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Result: " + to_stringWithPrecision(GENERAL[n]->ROI[roi]->CNNResult / 10.0, 1));

                    if (isLogImage) {
                        std::string imagename = GENERAL[n]->name +  "_" + GENERAL[n]->ROI[roi]->name;

                        if (isLogImageSelect) {
                            if (LogImageSelect.find(GENERAL[n]->ROI[roi]->name) != std::string::npos)
                                LogImage(logPath, imagename, DoubleHyprid10, LogImageResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                        else {
                            LogImage(logPath, imagename, DoubleHyprid10, LogImageResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                    }
                } break;
                
                case Digital100:  // for models dig-class100*
                case Analogue100: // for models ana-class100*
                {
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Type: Analogue100/Digital100 (ana/dig-class100)");
                   
                    if (tflite->LoadInputImageBasis(GENERAL[n]->ROI[roi]->image)) {
                        tflite->Invoke();
                        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Invoke done");
                    }
                    else {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Invoke aborted");
                        return false;
                    }

                    int _num = tflite->GetOutClassification();
                    
                    if(GENERAL[n]->ROI[roi]->CCW) {
                        if (_num == 0)
                            GENERAL[n]->ROI[roi]->CNNResult = 0;
                        else
                            GENERAL[n]->ROI[roi]->CNNResult = 100 -_num;

                    }                       
                    else {
                        GENERAL[n]->ROI[roi]->CNNResult = _num; 
                    }

                    if (GENERAL[n]->ROI[roi]->CNNResult < 0)
                        GENERAL[n]->ROI[roi]->CNNResult = 0;
                    else if (GENERAL[n]->ROI[roi]->CNNResult >= 100)
                        GENERAL[n]->ROI[roi]->CNNResult = 99;

                    GENERAL[n]->ROI[roi]->isRejected = false;   

                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Result: " + to_stringWithPrecision(GENERAL[n]->ROI[roi]->CNNResult / 10.0, 1));

                    if (isLogImage) {
                        std::string imagename = GENERAL[n]->name +  "_" + GENERAL[n]->ROI[roi]->name;

                        if (isLogImageSelect) {
                            if (LogImageSelect.find(GENERAL[n]->ROI[roi]->name) != std::string::npos)
                                LogImage(logPath, imagename, Digital100, GENERAL[n]->ROI[roi]->CNNResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                        else {
                            LogImage(logPath, imagename, Digital100, GENERAL[n]->ROI[roi]->CNNResult, time, GENERAL[n]->ROI[roi]->image_org);
                        }
                    }
                } break;
                        
                default:
                break;
            }
        }
    }

    tflite->CTfLiteClassDeleteInterpreter();
    
    return true;
}


bool ClassFlowCNNGeneral::CNNTypeWithExtendedResolution()
{
    if (CNNType == Digital)
        return false;

    return true;
}


std::vector<HTMLInfo*> ClassFlowCNNGeneral::GetHTMLInfo()
{
    std::vector<HTMLInfo*> result;

    for (int _ana = 0; _ana < GENERAL.size(); ++_ana)
        for (int i = 0; i < GENERAL[_ana]->ROI.size(); ++i) {
            ESP_LOGD(TAG, "Image: %d", (int) GENERAL[_ana]->ROI[i]->image);
            if (SaveAllFiles) {
                if (GENERAL[_ana]->ROI[i]->image) {
                    if (GENERAL[_ana]->name == "default")
                        GENERAL[_ana]->ROI[i]->image->SaveToFile(FormatFileName("/sdcard/img_tmp/" + GENERAL[_ana]->ROI[i]->name + ".jpg"));
                    else
                        GENERAL[_ana]->ROI[i]->image->SaveToFile(FormatFileName("/sdcard/img_tmp/" + GENERAL[_ana]->name + "_" + GENERAL[_ana]->ROI[i]->name + ".jpg"));
                }
            }

            HTMLInfo *zw = new HTMLInfo;
            
            zw->name = GENERAL[_ana]->name;
            zw->position = i;

            if (GENERAL[_ana]->name == "default") {
                zw->filename = GENERAL[_ana]->ROI[i]->name + ".jpg";
                zw->filename_org = GENERAL[_ana]->ROI[i]->name + "_org.jpg";
            }
            else {
                zw->filename = GENERAL[_ana]->name + "_" + GENERAL[_ana]->ROI[i]->name + ".jpg";
                zw->filename_org = GENERAL[_ana]->name + "_" + GENERAL[_ana]->ROI[i]->name + "_org.jpg";
            }

            if (CNNType == Digital)
                zw->val = GENERAL[_ana]->ROI[i]->CNNResult;
            else
                zw->val = GENERAL[_ana]->ROI[i]->CNNResult / 10.0;
            
            zw->image = GENERAL[_ana]->ROI[i]->image;
            zw->image_org = GENERAL[_ana]->ROI[i]->image_org;

            result.push_back(zw);
        }

    return result;
}


int ClassFlowCNNGeneral::getNumberGENERAL()
{
    return GENERAL.size();
}


std::string ClassFlowCNNGeneral::getNameGENERAL(int _seqNo)
{
    if (_seqNo < GENERAL.size())
        return GENERAL[_seqNo]->name;

    return "GENERAL DOES NOT EXIST";
}


general* ClassFlowCNNGeneral::GetGENERAL(int _seqNo)
{
    if (_seqNo < GENERAL.size())
        return GENERAL[_seqNo];

    return NULL;
}


void ClassFlowCNNGeneral::UpdateNameNumbers(std::vector<std::string> *_name_numbers)
{
    for (int _dig = 0; _dig < GENERAL.size(); _dig++) {
        std::string _name = GENERAL[_dig]->name;
        bool found = false;
        for (int i = 0; i < (*_name_numbers).size(); ++i) {
            if ((*_name_numbers)[i] == _name)
                found = true;
        }

        if (!found)
            (*_name_numbers).push_back(_name);
    }
}


std::string ClassFlowCNNGeneral::getReadoutRawString(int _seqNo) 
{
    std::string rt = "";

    if (_seqNo >= GENERAL.size() || GENERAL[_seqNo]==NULL || GENERAL[_seqNo]->ROI.size() == 0)
        return rt;
 
    for (int i = 0; i < GENERAL[_seqNo]->ROI.size(); ++i) {
        if (CNNType == Analogue || CNNType == Analogue100 || CNNType == DoubleHyprid10 || CNNType == Digital100) {
            rt = rt + "," + to_stringWithPrecision(GENERAL[_seqNo]->ROI[i]->CNNResult / 10.0, 1);
        }
        else if (CNNType == Digital) {
            if (GENERAL[_seqNo]->ROI[i]->CNNResult == 10)
                rt = rt + ",N";
            else
                rt = rt + "," + std::to_string(GENERAL[_seqNo]->ROI[i]->CNNResult);
        }
    }

    return rt;
}


ClassFlowCNNGeneral::~ClassFlowCNNGeneral()
{
    delete tflite;
    tflite = NULL;
    
    for (int _ana = 0; _ana < GENERAL.size(); ++_ana)
        for (int i = 0; i < GENERAL[_ana]->ROI.size(); ++i) {
            delete GENERAL[_ana]->ROI[i]->image;
            delete GENERAL[_ana]->ROI[i]->image_org;
        }
}
