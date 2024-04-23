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


struct t_ROI {
    int posx, posy, deltax, deltay;
    int CNNResult = -10;     // normalized to 0-99 (exception for class11: 0-10: 0-9+NaN), default: negative number equal to "-1.0"
    bool isRejected, CCW;
    std::string name;
    CImageBasis *image, *image_org;
};


struct general {
    std::string name;
    std::vector<t_ROI*> ROI;
};


enum t_RateType {
    rtRateOff,
    rtRatePerMin,
    rtRatePerInterval
 };


struct NumberPost {
    bool useMaxRateValue;           // Rate too high check evaluation
    bool allowNegativeRates;        // Rate negative check evaluation
    bool checkDigitIncreaseConsistency; // Plausibility check for class11 models
    bool isExtendedResolution;      // Extended Resolution activated
    bool isFallbackValueValid;      // Fallback value is valid in terms of not being outdated
    bool isActualValueANumber;      // Actual value is valid number (further processing possible)
    bool isActualValueConfirmed;    // Actual value is without any deviation (fully processed by post-processing without deviation)

    int decimalShift;               // Shift the decimal point by defined value to archieve a value conversion (e.g. m3 -> liter)
    int analogDigitalTransitionStart; // When is the digit > x.1, i.e. when does it start to tilt?
    int decimalPlaceCount;          // No of decimal places

    int analogCount;                // Number of analog pointers
    int digitCount;                 // Number of digit numbers

    time_t timeProcessed;           // Time of actual source image was taken (== actual result time)
    time_t timeFallbackValue;       // Time of FallbackValue in seconds

    t_RateType rateType;            // Parameter: Select Rate Checking Procedure
    float maxRateValue;             // Parameter: Max allowed rate
    double ratePerMin;              // Rate per minute (e.g. m3/min)
    double ratePerInterval;         // Rate per interval, value delta between actual and fallback value,
                                    // e.g. dV/dT (actual value-fallbackvalue)/(actual processing timestamp-fallbackvalue processing timestamp)

    double fallbackValue;           // Fallback value, equal the last successful processed value (legacy name: prevalue)
    double actualValue;             // Actual result value

    std::string sTimeProcessed;     // Processed timestamp -> Time of last processing
    std::string sTimeFallbackValue; // FallbackValue timestamp -> Time of last valid value
    std::string sRatePerMin;        // Rate per minute, based on time between last valid and actual reading
    std::string sRatePerInterval;   // Rate per interval, value delta between actual value and last valid value (fallback value)
    std::string sRawValue;          // Raw value (possibly incl. N & leading 0)
    std::string sActualValue;       // Value of actual valid reading, incl. post-processing corrections
    std::string sFallbackValue;     // Fallback value, equal to last valid reading (legacy name: prevalue)
    std::string sValueStatus;       // Value status

    std::string FieldV1;            // Fieldname in InfluxDBv1
    std::string MeasurementV1;      // Measurement in InfluxDBv1

    std::string FieldV2;            // Fieldname in InfluxDBv2
    std::string MeasurementV2;      // Measurement in InfluxDBv2

    general *digit_roi;             // Pointer to digit ROI struct
    general *analog_roi;            // Pointer to analog ROI struct

    std::string name;               // Name of number sequence
};

#endif

