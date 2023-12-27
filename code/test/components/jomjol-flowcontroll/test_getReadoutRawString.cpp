#include "test_flow_postrocess_helper.h"


void test_getReadoutRawString() {

    // no ROIs setted up
    UnderTestPost* _undertestPost = setUpClassFlowPostprocessing(Digital100, Analogue100);

    std::string result = _undertestPost->flowAnalog->getReadoutRawString(0);
    TEST_ASSERT_EQUAL_STRING("", result.c_str());

    // setted value
    general* gen_analog = _undertestPost->flowAnalog->GetGENERAL("default", true);
    gen_analog->ROI.clear();
    t_ROI* anaROI = new t_ROI();
    std::string name = "ana_1";
    anaROI->name = name;
    anaROI->CNNResult = 55; // 5.5
    gen_analog->ROI.push_back(anaROI);

    result = _undertestPost->flowAnalog->getReadoutRawString(0);
    TEST_ASSERT_EQUAL_STRING(",5.5", result.c_str());
}