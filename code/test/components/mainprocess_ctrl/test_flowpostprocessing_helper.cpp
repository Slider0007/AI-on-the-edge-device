#include "test_flowpostprocessing_helper.h"
#include "configClass.h"
#include "ClassFlow.h"

#include <esp_log.h>


static const char *UNITY_TAG_PPHELPER = "UNITYTEST_POSTPROCHELPER";


UnderTestPost *setUpClassFlowPostprocessing(CNNType digType, CNNType anaType)
{
    ClassFlowTakeImage *takeimage = new ClassFlowTakeImage();
    ClassFlowCNNGeneral *digit = new ClassFlowCNNGeneral(NULL, "Digit", digType);
    ClassFlowCNNGeneral *analog = new ClassFlowCNNGeneral(NULL, "Analog", anaType);

    // Init default config (including sequence related config)
    ConfigClass::getInstance()->clearCfgData();
    ConfigClass::getInstance()->readConfigFile(true);

    // Init sequence result data struct
    flowctrl.getSequenceData().clear();
    for (const auto &sequenceCfgData : ConfigClass::getInstance()->get()->sectionNumberSequences.sequence) {
        SequenceData *sequence = new SequenceData{};
        sequence->sequenceId = sequenceCfgData.sequenceId;
        sequence->sequenceName = sequenceCfgData.sequenceName;
        flowctrl.getSequenceData().push_back(sequence);
    }

    return new UnderTestPost(takeimage, analog, digit);
}


UnderTestPost *initDoFlow(std::vector<float> digits, std::vector<float> analogs, CNNType digType,
                            bool extendedResolution, int decimalShift, bool checkDigitIncreaseConsistency)
{
    UnderTestPost *_underTestPost = setUpClassFlowPostprocessing(digType, CNNTYPE_ANALOG_CLASS100);

    // Get sequenceData pointer
    _underTestPost->sequenceDataPtr = flowctrl.getSequenceData()[0];

    // Inject digit ROI
    ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.clear();
    ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.shrink_to_fit();
    _underTestPost->sequenceDataPtr->digitRoi.clear();
    _underTestPost->sequenceDataPtr->digitRoi.shrink_to_fit();

    if (digits.size() > 0) {
        // Fill ROI to global config due to name request
        for (int i = 0; i < digits.size(); i++) {
            RoiElement roiEl = RoiElement{};
            roiEl.roiName = "main_dig" + std::to_string(i+1);
            ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.push_back(roiEl);
        }

        // Set pointer to ROI config and inject CNN result
        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.size(); i++) {
            RoiData *roiEl = new RoiData{};
            roiEl->param = &ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi[i];

            if (digType != CNNTYPE_DIGIT_CLASS11) {
                roiEl->CNNResult = (int)(digits[i] * 10.0 + 0.1); // + 0.1 due to float to int rounding, will be truncated anyway
            }
            else {
                roiEl->CNNResult = (int)digits[i];
            }

            _underTestPost->sequenceDataPtr->digitRoi.push_back(roiEl);
        }
    }
    else {
        _underTestPost->flowDigit = NULL;
    }

    // Inject analog ROI
    ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.clear();
    ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.shrink_to_fit();
    _underTestPost->sequenceDataPtr->analogRoi.clear();
    _underTestPost->sequenceDataPtr->analogRoi.shrink_to_fit();

    if (analogs.size() > 0) {
        // Fill ROI to global config due to name request
        for (int i = 0; i < analogs.size(); i++) {
            RoiElement roiEl = RoiElement{};
            roiEl.roiName = "main_ana" + std::to_string(i+1);
            ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.push_back(roiEl);
        }

        // Set pointer to ROI config and inject CNN result
        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.size(); i++) {
            RoiData *roiDataEl = new RoiData{};
            roiDataEl->param = &ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi[i];

            roiDataEl->CNNResult = (int)(analogs[i] * 10.0 + 0.1); // + 0.1 due to float to int rounding, will be truncated anyway

            _underTestPost->sequenceDataPtr->analogRoi.push_back(roiDataEl);
        }
    }
    else {
        _underTestPost->flowAnalog = NULL;
    }

    // Modify sequence post processing config
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].maxRateCheckType = RATE_CHECK_OFF; // Avoid rate check errors
    _underTestPost->setFallbackValueLoaded(true); // Avoid loading fallbackvalue from NVS

    setDigitIncreaseConsistencyCheck(checkDigitIncreaseConsistency);
    setExtendedResolution(extendedResolution);
    setDecimalShift(decimalShift);

    // Load parameter needed for postprocessing flow
    _underTestPost->loadParameter();

    return _underTestPost;
}


std::string processDoFlow(UnderTestPost *_underTestPost)
{
    std::string time;
    TEST_ASSERT_TRUE(_underTestPost->doFlow(time));

    return _underTestPost->sequenceDataPtr->sActualValue;
}


std::string processDoFlow(std::vector<float> digits, std::vector<float> analogs, CNNType digType,
                            bool extendedResolution, int decimalShift, bool checkDigitIncreaseConsistency)
{
    UnderTestPost *_underTestPost = initDoFlow(digits, analogs, digType, extendedResolution, decimalShift, checkDigitIncreaseConsistency);

    std::string time;
    TEST_ASSERT_TRUE(_underTestPost->doFlow(time));

    std::string sActualValue = _underTestPost->sequenceDataPtr->sActualValue;
    delete _underTestPost;

    return sActualValue;
}


void setAllowNegative(bool _allowNegative)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].allowNegativeRate = _allowNegative;
}


void setDigitIncreaseConsistencyCheck(bool _checkDigitIncreaseConsistency)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].checkDigitIncreaseConsistency = _checkDigitIncreaseConsistency;
}


void setDecimalShift(int _decimalShift)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].decimalShift = _decimalShift;
}


void setExtendedResolution(bool _extendedResolution)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].extendedResolution = _extendedResolution;
}


void setAnalogDigitSyncValue(float _analogDigitSyncValue)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].analogDigitSyncValue = _analogDigitSyncValue;
}


void setFallbackValue(UnderTestPost *_underTestPost, double _fallbackValue)
{
    // Set fallbackValue usage
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].useFallbackValue = true;

    // Set fallbackValue
    _underTestPost->sequenceDataPtr->fallbackValue = _fallbackValue;
    _underTestPost->sequenceDataPtr->isFallbackValueValid = true;
}


int getDecimalPlaceCount(UnderTestPost *_underTestPost)
{
    return _underTestPost->sequenceDataPtr->decimalPlaceCount;
}
