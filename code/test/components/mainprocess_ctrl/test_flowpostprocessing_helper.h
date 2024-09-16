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
                        : ClassFlowPostProcessing::ClassFlowPostProcessing(takeimage, digit, analog) {}

        SequenceData *sequenceDataPtr = NULL;

        using ClassFlowPostProcessing::flowDigit;
        using ClassFlowPostProcessing::flowAnalog;
};


/**
 * @brief Set the Up Class Flow Postprocessing object
 *
 * @param digType Model type for digits
 * @param anaType Model type for analogs
 * @return UnderTestPost* Testobject (created but not configured)
 */
UnderTestPost* setUpClassFlowPostprocessing(CNNType digType, CNNType anaType);


/**
 * @brief creates a testobject (including setup). AnalogType is Class100, because all analog types do the same.
 *
 * @param digits Digit results
 * @param analog Analog results
 * @param digType Digit model type (default CNNTYPE_DIGIT_CLASS100)
 * @param extendedResolution Sets ExtendedResolution (default = false)
 * @param decimalShift Set decimalShift (default = 0)
 * @param checkConsistency Sets CheckConsistency check (default = false) (Only for class11 models)
 * @return UnderTestPost* Testobject
 */
UnderTestPost* initDoFlow(std::vector<float> digits, std::vector<float> analogs, CNNType digType = CNNTYPE_DIGIT_CLASS100,
                            bool extendedResolution=false, int decimalShift=0, bool checkConsistency=false);



/**
 * @brief creates a testobject an run do flow (including setup). AnalogType is Class100, because all analog types do the same.
 *
 * @param digits Digits results
 * @param analog Analog results
 * @param digType Digit model type (default = CNNTYPE_DIGIT_CLASS100)
 * @param extendedResolution sets property extendedResolution (default = false)
 * @param decimalShift set property decimalShift ( default = 0)
 * @param checkConsistency Sets CheckConsistency check (default = false) (Only for class11 models)
 * @return Actual Value
 */
std::string processDoFlow(std::vector<float> digits, std::vector<float> analogs, CNNType digType = CNNTYPE_DIGIT_CLASS100,
                            bool extendedResolution=false, int decimalShift=0, bool checkConsistency=false);



/**
 * @brief Process doFlow
 *
 * @param _underTestPost Test object
 * @return Actual Value
 */
std::string processDoFlow(UnderTestPost* _underTestPost);


/**
 * @brief Allow negative rate
 *
 * @param _allowNegative true/false
 */
void setAllowNegative(bool _allowNegative);


/**
 * @brief Set Digit Increase Consistency Check
 *
 * @param _checkDigitIncreaseConsistency true/false
 */
void setDigitIncreaseConsistencyCheck(bool _checkDigitIncreaseConsistency);


/**
 * @brief Set Decimal Shift
 *
 * @param _decimalShift decimal shift (default = 0)
 */
void setDecimalShift(int _decimalShift);


/**
 * @brief Set the Extended Resolution
 *
 * @param _extendedResolution true/false
 */
void setExtendedResolution(bool _extendedResolution);


/**
 * @brief Set the Analog Digit Sync Value
 *
 * @param _analogdigitSyncValue Analog Digit Sync value (default = 9.2)
 */
void setAnalogDigitSyncValue(float _analogdigitSyncValue);


/**
 * @brief Set Fallback Value
 *
 * @param _underTestPost Test object
 * @param _fallbackValue Fallback value
 */
void setFallbackValue(UnderTestPost* _underTestPost, double _fallbackValue);


/**
 * @brief Get the count of decimal places
 *
 * @param _underTestPost Test object
 * @return Number of decimal places
 *
 */
int getDecimalPlaceCount(UnderTestPost* _underTestPost);

#endif // TEST_FLOWPOSTPROCESSING_HELPER_H
