#ifndef TEST_FLOWPOSTPROCESSING_HELPER_H
#define TEST_FLOWPOSTPROCESSING_HELPER_H

#include "MainFlowControl.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlowCNNGeneral.h"
#include "ClassFlowPostProcessing.h"
#include "helper.h"


class UnderTestPost : public ClassFlowPostProcessing
{
  public:
    UnderTestPost(ClassFlowTakeImage *takeimage, ClassFlowCNNGeneral *analog, ClassFlowCNNGeneral *digit)
        : ClassFlowPostProcessing::ClassFlowPostProcessing(takeimage, digit, analog)
    {}

    SequenceData *sequenceDataPtr = NULL;

    using ClassFlowPostProcessing::flowAnalog;
    using ClassFlowPostProcessing::flowDigit;
};


/**
 * @brief Set the Up Class Flow Postprocessing object
 *
 * @return UnderTestPost* Testobject (created but not configured)
 */
UnderTestPost *setUpClassFlowPostprocessing();


/**
 * @brief creates a testobject (including setup). AnalogType is Class100, because all analog types do the same.
 *
 * @param digits Digit results
 * @param analog Analog results
 * @param extendedResolution Sets ExtendedResolution (default = false)
 * @param decimalScaling Set decimalScaling (default = 0)
 * @return UnderTestPost* Testobject
 */
UnderTestPost *initDoFlow(std::vector<float> digits, std::vector<float> analogs, bool extendedResolution = false, int decimalScaling = 0,
                          WheelType wheelType = WheelType::AllWheelsIntermittent, float dialToWheelDetuneValue = 0.0,
                          float wheelTransitionWidth = 0.15);


/**
 * @brief creates a testobject an run do flow (including setup). AnalogType is Class100, because all analog types do the same.
 *
 * @param digits Digits results
 * @param analog Analog results
 * @param extendedResolution sets property extendedResolution (default = false)
 * @param decimalScaling set property decimalScaling ( default = 0)
 * @return Actual Value
 */
std::string processDoFlow(std::vector<float> digits, std::vector<float> analogs, bool extendedResolution = false, int decimalScaling = 0,
                          WheelType wheelType = WheelType::AllWheelsIntermittent, float dialToWheelDetuneValue = 0.0,
                          float wheelTransitionWidth = 0.15);


/**
 * @brief Process doFlow
 *
 * @param _underTestPost Test object
 * @return Actual Value
 */
std::string processDoFlow(UnderTestPost *_underTestPost);


/**
 * @brief Allow negative rate
 *
 * @param _allowNegative true/false
 */
void setAllowNegative(bool _allowNegative);


/**
 * @brief Set Decimal Shift
 *
 * @param _decimalScaling decimal shift (default = 0)
 */
void setDecimalScaling(int _decimalScaling);


/**
 * @brief Set the Extended Resolution
 *
 * @param _extendedResolution true/false
 */
void setExtendedResolution(bool _extendedResolution);


/**
 * @brief Set model influence factor
 *
 * @param _modelInfluenceFactor Model influence factor
 */
void setModelInfluence(float _modelInfluence);


/**
 * @brief Set the dial to wheel detune
 *
 * @param _dialToWheelDetune Dial-To-Wheel-Detune (default = 0.0)
 */
void setDialToWheelDetune(float _dialToWheelDetune);


/**
 * @brief Set wheel type
 *
 * @param _wheelType Wheel type
 */
void setWheelType(WheelType _wheelType);


/**
 * @brief Set wheel transition width
 *
 * @param _factor Wheel transition width
 */
void setWheelTransitionWidth(float _wheelTransitionWidth);


/**
 * @brief Set Fallback Value
 *
 * @param _underTestPost Test object
 * @param _fallbackValue Fallback value
 */
void setFallbackValue(UnderTestPost *_underTestPost, double _fallbackValue);


/**
 * @brief Get the count of decimal places
 *
 * @param _underTestPost Test object
 * @return Number of decimal places
 *
 */
int getDecimalPlaceCount(UnderTestPost *_underTestPost);

#endif // TEST_FLOWPOSTPROCESSING_HELPER_H
