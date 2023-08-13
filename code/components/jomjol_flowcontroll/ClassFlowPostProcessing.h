#pragma once

#ifndef CLASSFFLOWPOSTPROCESSING_H
#define CLASSFFLOWPOSTPROCESSING_H

#include "ClassFlow.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlowCNNGeneral.h"
#include "ClassFlowDefineTypes.h"

#include <string>

class ClassFlowPostProcessing : public ClassFlow
{
protected:
    std::vector<NumberPost*> NUMBERS;
    bool UseFallbackValue;
    bool UpdateFallbackValue;
    int FallbackValueAgeStartup; 
    bool IgnoreLeadingNaN;

    ClassFlowCNNGeneral* flowAnalog;
    ClassFlowCNNGeneral* flowDigit;    
    ClassFlowTakeImage *flowTakeImage;

    bool LoadFallbackValue(void);
    bool SaveFallbackValue(void);

    void setDecimalShift();
    std::string ShiftDecimal(std::string in, int _decShift);
    std::string SubstitudeN(std::string, double _fallbackValue);
    float checkDigitConsistency(double _value, int _decimalshift, bool _isanalog, double _fallbackValue);

    void InitNUMBERS();

    void handleDecimalShift(std::string _decsep, std::string _value);
    void handleMaxRateValue(std::string _decsep, std::string _value);
    void handleDecimalExtendedResolution(std::string _decsep, std::string _value); 
    void handleMaxRateType(std::string _decsep, std::string _value);
    void handleAnalogDigitalTransitionStart(std::string _decsep, std::string _value);
    void handleAllowNegativeRate(std::string _decsep, std::string _value);
    
    void WriteDataLog(int _index);

public:
    ClassFlowPostProcessing(std::vector<ClassFlow*>* lfc, ClassFlowCNNGeneral *_analog, ClassFlowCNNGeneral *_digit);
    virtual ~ClassFlowPostProcessing();
    bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
    bool doFlow(std::string time);

    std::vector<NumberPost*>* GetNumbers() {return &NUMBERS;};
    std::string getNumbersName();
    std::string getReadout(int _number);
    std::string getReadoutParam(bool _rawValue, bool _noerror, int _number = 0);
    std::string getReadoutError(int _number = 0);
    std::string getReadoutRate(int _number = 0);
    std::string getReadoutTimeStamp(int _number = 0);
    std::string getJsonFromNumber(int i, std::string _lineend);
    std::string GetJSON(std::string _lineend = "\n");
    std::string GetFallbackValue(std::string _number = "");
    bool SetFallbackValue(double zw, std::string _numbers);
    bool getUseFallbackValue(void);
    void setUseFallbackValue(bool _value);

    std::string name() {return "ClassFlowPostProcessing";};
};

#endif //CLASSFFLOWPOSTPROCESSING_H
