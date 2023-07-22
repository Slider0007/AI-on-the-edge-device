#pragma once

#ifndef CLASSFLOWDEFINETYPES_H
#define CLASSFLOWDEFINETYPES_H

#include <vector>
#include "CImageBasis.h"

enum t_CNNType {
    AutoDetect,
    Analogue,
    Analogue100,
    Digital,
    DigitalHyprid10,
    DoubleHyprid10,
    Digital100,
    None
 };


struct roi {
    int posx, posy, deltax, deltay;
    float result_float = -1.0;
    int result_klasse = -1;
    bool isReject, CCW;
    std::string name;
    CImageBasis *image, *image_org;
};


struct general {
    std::string name;
    std::vector<roi*> ROI;
};


enum t_RateType {
    RateCheckOff,
    AbsoluteChange,
    RateChange
 };


struct NumberPost {
    float MaxRateValue;
    bool useMaxRateValue;
    t_RateType RateType;
    bool ErrorMessage;
    bool PreValueOkay;
    bool AllowNegativeRates;
    bool checkDigitIncreaseConsistency;
    time_t lastvalue;
    std::string timeStamp;
    double FlowRateAct; // m3 / min
    double PreValue; // last value that was read out well
    double Value; // last value read out, incl. corrections
    std::string ReturnRateValue; // return value rate
    std::string ReturnChangeAbsolute; // return value rate
    std::string ReturnRawValue; // Raw value (with N & leading 0)    
    std::string ReturnValue; // corrected return value, if necessary with error message
    std::string ReturnPreValue; // corrected return value without error message
    std::string ErrorMessageText; // Error message for consistency check
    int AnzahlAnalog;
    int AnzahlDigital;
    int DecimalShift;
    int DecimalShiftInitial;
    int AnalogDigitalTransitionStart; // When is the digit > x.1, i.e. when does it start to tilt?
    int Nachkomma;

    std::string FieldV1; // Fieldname in InfluxDBv1  
    std::string MeasurementV1;   // Measurement in InfluxDBv1

    std::string FieldV2;         // Fieldname in InfluxDBv2  
    std::string MeasurementV2;   // Measurement in InfluxDBv2

    bool isExtendedResolution;

    general *digit_roi;
    general *analog_roi;

    std::string name;
};

#endif

