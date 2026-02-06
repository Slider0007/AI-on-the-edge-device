#include "test_flowpostprocessing_helper.h"
#include "configClass.h"
#include "ClassFlow.h"

#include <esp_log.h>


// static const char *UNITY_TAG_PPHELPER = "UNITYTEST_POSTPROCHELPER"; // Unused


UnderTestPost *setUpClassFlowPostprocessing()
{
    ClassFlowTakeImage *takeimage = new ClassFlowTakeImage();
    ClassFlowCNNGeneral *digit = new ClassFlowCNNGeneral("Digit", CNNTYPE_DIGIT_CLASS100);
    ClassFlowCNNGeneral *analog = new ClassFlowCNNGeneral("Analog", CNNTYPE_ANALOG_CLASS100);

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


UnderTestPost *initDoFlow(std::vector<float> digits, std::vector<float> analogs, bool extendedResolution, int decimalScaling,
                          WheelType wheelType, float dialToWheelDetune, float wheelTransitionWidth)
{
    UnderTestPost *_underTestPost = setUpClassFlowPostprocessing();

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
            roiEl.roiName = "main_dig" + std::to_string(i + 1);
            ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.push_back(roiEl);
        }

        // Set pointer to ROI config and inject CNN result
        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi.size(); i++) {
            RoiData *roiDataEl = new RoiData{};
            roiDataEl->param = &ConfigClass::getInstance()->get()->sectionDigit.sequence[0].roi[i];
            roiDataEl->CNNResult = (int)(digits[i] * 10.0f + 0.1f); // + 0.1 due to float to int rounding, will be truncated anyway
            roiDataEl->CNNResultConfidence = 1.0f;                  // Ensure high confidence for deterministic tests

            _underTestPost->sequenceDataPtr->digitRoi.push_back(roiDataEl);
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
            roiEl.roiName = "main_ana" + std::to_string(i + 1);
            ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.push_back(roiEl);
        }

        // Set pointer to ROI config and inject CNN result
        for (int i = 0; i < ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi.size(); i++) {
            RoiData *roiDataEl = new RoiData{};
            roiDataEl->param = &ConfigClass::getInstance()->get()->sectionAnalog.sequence[0].roi[i];
            roiDataEl->CNNResult = (int)(analogs[i] * 10.0f + 0.1f); // + 0.1 due to float to int rounding, will be truncated anyway
            roiDataEl->CNNResultConfidence = 1.0f;                   // Ensure high confidence for deterministic tests

            _underTestPost->sequenceDataPtr->analogRoi.push_back(roiDataEl);
        }
    }
    else {
        _underTestPost->flowAnalog = NULL;
    }

    // Modify sequence post processing config
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].maxRateCheckType = RATE_CHECK_OFF; // Avoid rate check errors
    _underTestPost->setFallbackValueLoaded(true); // Avoid loading fallbackvalue from NVS

    // Overwrite config defaults with function parameter values
    setExtendedResolution(extendedResolution);
    setDecimalScaling(decimalScaling);
    setWheelType(wheelType);
    setWheelTransitionWidth(wheelTransitionWidth);
    setDialToWheelDetune(dialToWheelDetune);
    setModelInfluence(0.0f);

    // Load parameter from config
    _underTestPost->loadParameter();

    // Clear model autotune parameter
    _underTestPost->sequenceDataPtr->meterModel->resetDigitAutoDetuneValues(*(_underTestPost->sequenceDataPtr));

    return _underTestPost;
}


std::string processDoFlow(UnderTestPost *_underTestPost)
{
    std::string time;
    TEST_ASSERT_TRUE(_underTestPost->doFlow(time));

    return _underTestPost->sequenceDataPtr->sActualValue;
}


std::string processDoFlow(std::vector<float> digits, std::vector<float> analogs, bool extendedResolution, int decimalScaling,
                          WheelType wheelType, float dialToWheelDetune, float wheelTransitionWidth)
{
    UnderTestPost *_underTestPost = initDoFlow(digits, analogs, extendedResolution, decimalScaling, wheelType, dialToWheelDetune,
                                               wheelTransitionWidth);

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


void setDecimalScaling(int _decimalScaling)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].decimalScaling = _decimalScaling;
}


void setExtendedResolution(bool _extendedResolution)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].extendedResolution = _extendedResolution;
}


void setModelInfluence(float _modelInfluence)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].modelInfluence = _modelInfluence;
}


void setDialToWheelDetune(float _dialToWheelDetuneValue)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].dialToWheelDetune = _dialToWheelDetuneValue;
}


void setWheelType(WheelType _wheelType)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].wheelType = _wheelType;
}


void setWheelTransitionWidth(float _wheelTransitionWidth)
{
    ConfigClass::getInstance()->get()->sectionPostProcessing.sequence[0].wheelTransitionWidth = _wheelTransitionWidth;
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
