#include "test_flowpostprocessing_helper.h"


/* @brief Unity tests for post-processing */
void flowPP_case01(void)
{
    std::string result = processDoFlow({1.2, 6.7}, {9.5, 8.4});
    TEST_ASSERT_EQUAL_STRING("16.98", result.c_str());
}

void flowPP_case02(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issue-1344032217
    std::string result = processDoFlow({3.0, 7.0, 6.0, 5.0, 2.5, 9.6}, {6.4}, false, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("376529.6", result.c_str());
}

void flowPP_case03(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1220365920
    std::string result = processDoFlow({1.1, 6.0, 7.0, 7.0, 3.0, 4.6}, {6.2}, false, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("167734.6", result.c_str());
}


void flowPP_case04(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/919
    std::string result = processDoFlow({5.0, 8.6}, {9.8, 6.7, 8.9, 8.6, 9.8});
    TEST_ASSERT_EQUAL_STRING("58.96889", result.c_str());
}

void flowPP_case05(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1222672175
    std::string result = processDoFlow({2.9, 7.0, 6.8, 9.9, 8.0, 3.9}, {9.7});
    TEST_ASSERT_EQUAL_STRING("377083.9", result.c_str());
}


void flowPP_case06(void)
{
    std::string result = processDoFlow({1.1, 9.0, 4.0}, {6.1, 2.6, 6.2, 9.7}, false, 0, WheelType::AllWheelsIntermittent, 1.0f);
    TEST_ASSERT_EQUAL_STRING("194.6259", result.c_str());

    result = processDoFlow({1.1, 9.0, 4.0}, {8.1, 2.6, 6.2, 9.7}, false, 0, WheelType::AllWheelsIntermittent, 1.0f);
    TEST_ASSERT_EQUAL_STRING("194.8259", result.c_str());

    result = processDoFlow({1.1, 9.0, 4.0}, {9.1, 2.6, 6.2, 9.7}, false, 0, WheelType::AllWheelsIntermittent, 1.0f);
    TEST_ASSERT_EQUAL_STRING("194.9259", result.c_str());
}


void flowPP_case07(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/discussions/950
    std::string result = processDoFlow({1.1, 9.0, 9.0}, {7.5, 4.8, 8.2});
    TEST_ASSERT_EQUAL_STRING("199.748", result.c_str());
}

void flowPP_case08(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/948
    std::string result = processDoFlow({1.1, 9.0, 9.0, 6.0, 6.0, 1.0, 8.0, 8.0}, {}, false, -4);
    TEST_ASSERT_EQUAL_STRING("1996.6188", result.c_str());
}


void flowPP_case09(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/942#issuecomment-1226966346
    std::string result = processDoFlow({0.0, 2.9, 3.0, 2.9, 3.0, 8.9}, {}, false, -1, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("3333.8", result.c_str());

    result = processDoFlow({0.0, 2.9, 3.0, 2.9, 3.5, 9.5}, {}, false, -1, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("3333.9", result.c_str());

    result = processDoFlow({9.9, 2.8, 2.9, 2.9, 3.7, 9.7}, {}, false, -1, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("3333.9", result.c_str());

    result = processDoFlow({9.9, 2.8, 2.9, 2.9, 3.7, 0.1}, {}, false, -1, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("3334.0", result.c_str());
}

void flowPP_case10(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/942
    std::string result = processDoFlow({0.0, 9.9, 6.8, 9.9, 3.7, 0.8, 6.9, 8.7}, {}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("704.178", result.c_str());
}

void flowPP_case11(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/942#issuecomment-1228343319
    std::string result = processDoFlow({9.9, 6.8, 1.1, 4.7, 2.7, 6.0, 9.0, 2.8}, {}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("7153.692", result.c_str());
}

void flowPP_case12(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/942#issuecomment-1228343319
    std::string result = processDoFlow({1.0, 9.0, 4.3}, {8.9, 0.7, 8.9, 9.4});
    TEST_ASSERT_EQUAL_STRING("194.9089", result.c_str());
}

void flowPP_case13(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1229552041
    std::string result = processDoFlow({2.9, 7.0, 7.0, 9.1, 8.1, 8.5}, {4.1}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("377.9884", result.c_str());
}

void flowPP_case14(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1233149877
    std::string result = processDoFlow({0.0, 0.0, 7.0, 8.9}, {0.1, 0.1, 0.1, 9.6});
    TEST_ASSERT_EQUAL_STRING("78.9999", result.c_str());
}

void flowPP_case15(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1236119370
    std::string result = processDoFlow({3.1, 9.1, 5.7}, {8.8, 6.1, 3.0, 2.0}, false, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("395.8632", result.c_str());
}

void flowPP_case16(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/discussions/950#discussion-4338615
    std::string result = processDoFlow({1.1, 9.0, 9.0}, {7.5, 4.8, 8.3});
    TEST_ASSERT_EQUAL_STRING("199.748", result.c_str());
}

void flowPP_case17(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/921#issuecomment-1242730397
    std::string result = processDoFlow({3.0, 2.0, 2.0, 8.0, 9.0, 4.0, 1.7, 9.8}, {}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("32289.419", result.c_str());

    // with extended resolution
    result = processDoFlow({3.0, 2.0, 2.0, 8.0, 9.0, 4.0, 1.7, 9.8}, {}, true, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("32289.4198", result.c_str());
}

void flowPP_case18(void)
{
    std::string result = processDoFlow({0.0, 0.0, 7.9, 3.8}, {0.0, 9.4, 4.1, 0.1});
    TEST_ASSERT_EQUAL_STRING("83.9940", result.c_str());

    // with extended resolution
    result = processDoFlow({0.0, 0.0, 7.9, 3.8}, {0.0, 9.4, 4.1, 0.1}, true);
    TEST_ASSERT_EQUAL_STRING("83.99401", result.c_str());
}

void flowPP_case19(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/994#issue-1368570945
    std::string result = processDoFlow({0.0, 0.0, 1.0, 2.0, 2.8, 1.9, 2.8, 5.6}, {}, false, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("123235", result.c_str());

    // with extended resolution
    result = processDoFlow({0.0, 0.0, 1.0, 2.0, 2.8, 1.9, 2.8, 5.6}, {}, true, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("123235.6", result.c_str());
}

void flowPP_case20(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/discussions/950#discussioncomment-3661982
    std::string result = processDoFlow({3.0, 2.0, 4.1, 9.0, 4.0, 6.3, 9.2}, {}, false, -3);
    TEST_ASSERT_EQUAL_STRING("3249.469", result.c_str());

    // with extended resolution
    result = processDoFlow({3.0, 2.0, 4.1, 9.0, 4.0, 6.3, 9.2}, {}, true, -3);
    TEST_ASSERT_EQUAL_STRING("3249.4692", result.c_str());
}

void flowPP_case21(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1020#issue-1375648891
    std::string result = processDoFlow({0.0, 2.0, 6.3, 9.4}, {9.0, 2.3, 2.7, 7.5});
    TEST_ASSERT_EQUAL_STRING("269.9227", result.c_str());

    // with extended resolution
    result = processDoFlow({0.0, 2.0, 6.3, 9.4}, {9.0, 2.3, 2.7, 7.5}, true);
    TEST_ASSERT_EQUAL_STRING("269.92275", result.c_str());
}

void flowPP_case22(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1028#issuecomment-1250239481
    std::string result = processDoFlow({1.1, 6.0, 9.1, 3.0, 5.3, 9.4}, {3.5}, false, -3);
    TEST_ASSERT_EQUAL_STRING("169.3593", result.c_str());

    // with extended resolution
    result = processDoFlow({1.1, 6.0, 9.1, 3.0, 5.3, 9.4}, {3.5}, true, -3);
    TEST_ASSERT_EQUAL_STRING("169.35935", result.c_str());
}

void flowPP_case23(void)
{
    std::string result = processDoFlow({9.8, 9.8, 1.9, 0.9, 0.9, 9.9, 2.9, 4.8}, {5.5}, false, -3);
    TEST_ASSERT_EQUAL_STRING("211.0355", result.c_str());

    // with extended resolution
    result = processDoFlow({9.8, 9.8, 1.9, 0.9, 0.9, 9.9, 2.9, 4.8}, {5.5}, true, -3);
    TEST_ASSERT_EQUAL_STRING("211.03555", result.c_str());
}

void flowPP_case24(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issuecomment-1277425333
    std::string result = processDoFlow({2.2, 4.5, 5.9}, {9.4, 3.8, 8.6}, false, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("245.938", result.c_str());

    // with extended resolution
    result = processDoFlow({2.2, 4.5, 5.9}, {9.4, 3.8, 8.6}, true, 0, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("245.9386", result.c_str());
}

void flowPP_case25(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issuecomment-1265523710
    std::string result = processDoFlow({2.0, 4.0, 6.8}, {2.2, 0.1, 4.3});
    TEST_ASSERT_EQUAL_STRING("247.204", result.c_str());

    // with extended resolution
    result = processDoFlow({2.0, 4.0, 6.8}, {2.2, 0.1, 4.3}, true);
    TEST_ASSERT_EQUAL_STRING("247.2043", result.c_str());
}

void flowPP_case26(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issue-1391153343
    std::string result = processDoFlow({1.0, 4.0, 2.2}, {9.2, 2.5, 6.8, 9.0});
    TEST_ASSERT_EQUAL_STRING("142.9269", result.c_str());

    // with extended resolution
    result = processDoFlow({1.0, 4.0, 2.2}, {9.2, 2.5, 6.8, 9.0}, true);
    TEST_ASSERT_EQUAL_STRING("142.92690", result.c_str());
}

void flowPP_case27(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issuecomment-1262626388
    std::string result = processDoFlow({1.2, 6.8, 0.0, 0.0, 5.0, 2.8}, {8.7}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("170.0528", result.c_str());

    // with extended resolution
    result = processDoFlow({1.2, 6.8, 0.0, 0.0, 5.0, 2.8}, {8.7}, true, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("170.05287", result.c_str());
}

void flowPP_case28(void)
{
    std::string result = processDoFlow({0.0, 0.0, 9.0, 1.0}, {9.0, 8.0, 1.8, 7.4});
    TEST_ASSERT_EQUAL_STRING("91.8817", result.c_str());

    // with extended resolution
    result = processDoFlow({0.0, 0.0, 9.0, 1.0}, {9.0, 8.0, 1.8, 7.4}, true);
    TEST_ASSERT_EQUAL_STRING("91.88174", result.c_str());


    // with extended resolution
    result = processDoFlow({0.0, 0.0, 9.0, 1.9}, {3.6, 8.2, 3.2, 2.0}, true);
    TEST_ASSERT_EQUAL_STRING("92.38320", result.c_str());
}

void flowPP_case29(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1143#issue-1400807695
    // wrong result: 7472.749

    std::string result = processDoFlow({7.0, 4.0, 7.0, 2.0, 7.0, 5.4, 9.4}, {}, false, -3);
    TEST_ASSERT_EQUAL_STRING("7472.759", result.c_str());

    // with extended resolution
    result = processDoFlow({7.0, 4.0, 7.0, 2.0, 7.0, 5.4, 9.4}, {}, true, -3);
    TEST_ASSERT_EQUAL_STRING("7472.7594", result.c_str());
}

void flowPP_case30(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1143#issuecomment-1274434805
    std::string result = processDoFlow({4.9, 6.9, 6.8}, {8.6, 6.2, 5.0, 9.0});
    TEST_ASSERT_EQUAL_STRING("577.8649", result.c_str());

    // with extended resolution
    result = processDoFlow({4.9, 6.9, 6.8}, {8.6, 6.2, 5.0, 9.0}, true);
    TEST_ASSERT_EQUAL_STRING("577.86490", result.c_str());
}

void flowPP_case31(void)
{
    std::string result = processDoFlow({2.0, 1.0, 1.0, 0.0, 3.0, 4.8}, {8.0}, false, -3);
    TEST_ASSERT_EQUAL_STRING("211.0358", result.c_str());

    // with extended resolution
    result = processDoFlow({2.0, 1.0, 1.0, 0.0, 3.0, 4.8}, {8.0}, true, -3);
    TEST_ASSERT_EQUAL_STRING("211.03580", result.c_str());
}

void flowPP_case32(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1143#issuecomment-1281231468
    std::string result = processDoFlow({1.0, 1.9, 6.0}, {9.3, 2.3, 3.1});
    TEST_ASSERT_EQUAL_STRING("126.923", result.c_str());

    // with extended resolution
    result = processDoFlow({1.0, 1.9, 6.0}, {9.3, 2.3, 3.1}, true);
    TEST_ASSERT_EQUAL_STRING("126.9231", result.c_str());
}

void flowPP_case33(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issuecomment-1282168030
    std::string result = processDoFlow({3.0, 8.1, 5.9, 0.0, 5.0, 6.7}, {7.2}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("386.0567", result.c_str());

    // with extended resolution
    result = processDoFlow({3.0, 8.1, 5.9, 0.0, 5.0, 6.7}, {7.2}, true, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("386.05672", result.c_str());
}

void flowPP_case34(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1110#issuecomment-1282168030
    std::string result = processDoFlow({1.2, 7.0, 1.2, 2.0, 4.0, 1.8}, {7.8}, false, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("171.2417", result.c_str());

    // with extended resolution
    result = processDoFlow({1.2, 7.0, 1.2, 2.0, 4.0, 1.8}, {7.8}, true, -3, WheelType::LSWContinuous);
    TEST_ASSERT_EQUAL_STRING("171.24178", result.c_str());
}


void flowPP_case35(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1503#issuecomment-1343335855
    std::string result = processDoFlow({0.0, 0.0, 6.9, 1.0, 6.6}, {9.9, 1.8, 6.6, 5.8});
    TEST_ASSERT_EQUAL_STRING("717.0165", result.c_str());

    // with extended resolution
    result = processDoFlow({0.0, 0.0, 6.9, 1.0, 6.6}, {9.9, 1.8, 6.6, 5.8}, true);
    TEST_ASSERT_EQUAL_STRING("717.01658", result.c_str());
}


void flowPP_case36(void)
{
    // Real case 2023-08-19 -> Transistion issue from 99.99 to 100.00
    std::string result = processDoFlow({1.0, 0.0, 9.8}, {9.9, 9.3}, true);
    TEST_ASSERT_EQUAL_STRING("99.993", result.c_str());

    result = processDoFlow({1.0, 9.9, 9.8}, {9.9, 9.3}, true);
    TEST_ASSERT_EQUAL_STRING("99.993", result.c_str());

    result = processDoFlow({0.9, 0.0, 9.8}, {9.9, 9.4}, true);
    TEST_ASSERT_EQUAL_STRING("99.994", result.c_str());

    result = processDoFlow({1.0, 0.0, 9.9}, {9.9, 0.1}, true);
    TEST_ASSERT_EQUAL_STRING("100.001", result.c_str());
}


/**
 * A very confusing case of late transition
 *
 * This is a real world case of MY own water meter, where the last digit starts transitioning,
 * when the first dial is at around 3.0
 *
 * This tests shows increasing true meter values. Hence, also the expected values need to
 * increase from test to test, even though, some tests seem to be counterintuitive (case 1 and 2).
 */
void flowPP_case37(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/1503#issuecomment-1343335855
    std::string result = processDoFlow({0.0, 1.0, 1.0}, {0.2, 2.2, 1.0, 0.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("12.0210", result.c_str());

    result = processDoFlow({0.0, 1.0, 1.0}, {3.3, 2.2, 1.0, 0.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("12.3210", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.0}, {4.3, 2.2, 1.0, 0.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("12.4210", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.0}, {9.8, 8.7, 7.0, 0.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("12.9870", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.0}, {0.1, 0.3, 3.1, 0.1}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("13.0030", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.8}, {3.5, 5.2, 1.1, 0.1}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("13.3510", result.c_str());

    result = processDoFlow({0.0, 1.0, 3.0}, {4.1, 2.2, 1.1, 0.1}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("13.4210", result.c_str());
}


/**
 * Early transition test suite
 *
 * The meter transitions early: the analog→digital transition end is 8.0.
 */
void flowPP_case38(void)
{
    std::string result = processDoFlow({0.0, 1.0, 2.0}, {6.7, 7.8, 8.9, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 8.0f);
    TEST_ASSERT_EQUAL_STRING("12.6789", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.4}, {7.2, 2.3, 3.4, 4.0}, false, 0, WheelType::AllWheelsIntermittent, 8.0f);
    TEST_ASSERT_EQUAL_STRING("12.7234", result.c_str());

    result = processDoFlow({0.0, 1.0, 2.7}, {7.7, 7.8, 8.9, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 8.0f);
    TEST_ASSERT_EQUAL_STRING("12.7789", result.c_str());

    result = processDoFlow({0.0, 1.0, 3.0}, {8.1, 1.2, 2.3, 3.0}, false, 0, WheelType::AllWheelsIntermittent, 8.0f);
    TEST_ASSERT_EQUAL_STRING("12.8123", result.c_str());

    result = processDoFlow({0.0, 1.0, 3.0}, {1.2, 2.3, 3.4, 4.0}, false, 0, WheelType::AllWheelsIntermittent, 8.0f);
    TEST_ASSERT_EQUAL_STRING("13.1234", result.c_str());
}


void flowPP_case39(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/2857
    // reported by gec75
    std::string result = processDoFlow({2.0, 5.0, 1.9}, {0.8, 8.8, 9.9, 0.1}, false, 3);
    TEST_ASSERT_EQUAL_STRING("252090.0", result.c_str());
}


void flowPP_case40(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/2857
    // reported by Kornelius777
    std::string result = processDoFlow({0.0, 1.0, 0.0, 1.0, 7.0}, {8.2, 0.9, 9.9, 9.8});
    TEST_ASSERT_EQUAL_STRING("1017.8099", result.c_str());

    // with hanging digit
    result = processDoFlow({0.0, 1.0, 0.0, 1.0, 6.9}, {8.2, 0.9, 9.9, 9.8});
    TEST_ASSERT_EQUAL_STRING("1017.8099", result.c_str());
}


void flowPP_case41(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/2857
    // reported by marcniedersachsen
    std::string result = processDoFlow({0.0, 7.0, 7.0, 7.9}, {1.4, 4.7, 8.0, 0.5});
    TEST_ASSERT_EQUAL_STRING("778.1480", result.c_str());
}


void flowPP_case42(void)
{
    // https://github.com/jomjol/AI-on-the-edge-device/issues/2857
    // reported by ohkaja
    std::string result = processDoFlow({0.0, 1.0, 0.0, 4.9, 2.0}, {6.7, 6.7, 6.9, 9.1});
    TEST_ASSERT_EQUAL_STRING("1052.6669", result.c_str());
}


void flowPP_case43(void)
{
    // reported by rainman110
    std::string result = processDoFlow({4.0, 1.0, 1.8}, {3.6, 9.9, 8.1, 3.5}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("412.3983", result.c_str());

    // check before transition
    // The digit should have went from 8(7.9) -> 9. It will do after pasing 4 on the first analog
    result = processDoFlow({4.0, 1.0, 7.9}, {2.5, 5.8, 7.7, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("419.2579", result.c_str());

    result = processDoFlow({4.0, 1.0, 8.0}, {2.5, 5.8, 7.7, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("419.2579", result.c_str());

    // check after transition
    result = processDoFlow({4.0, 1.0, 9.0}, {5.5, 5.8, 7.7, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("419.5579", result.c_str());

    result = processDoFlow({4.0, 1.0, 8.9}, {5.5, 5.8, 7.7, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("419.5579", result.c_str());

    result = processDoFlow({4.0, 1.0, 9.1}, {5.5, 5.8, 7.7, 9.0}, false, 0, WheelType::AllWheelsIntermittent, 4.0f);
    TEST_ASSERT_EQUAL_STRING("419.5579", result.c_str());
}

void testPostProcessing()
{
    RUN_TEST(flowPP_case01);
    RUN_TEST(flowPP_case02);
    RUN_TEST(flowPP_case03);
    RUN_TEST(flowPP_case04);
    RUN_TEST(flowPP_case05);
    RUN_TEST(flowPP_case06);
    RUN_TEST(flowPP_case07);
    RUN_TEST(flowPP_case08);
    RUN_TEST(flowPP_case09);
    RUN_TEST(flowPP_case10);
    RUN_TEST(flowPP_case11);
    RUN_TEST(flowPP_case12);
    RUN_TEST(flowPP_case13);
    RUN_TEST(flowPP_case14);
    RUN_TEST(flowPP_case15);
    RUN_TEST(flowPP_case16);
    RUN_TEST(flowPP_case17);
    RUN_TEST(flowPP_case18);
    RUN_TEST(flowPP_case19);
    RUN_TEST(flowPP_case20);
    RUN_TEST(flowPP_case21);
    RUN_TEST(flowPP_case22);
    RUN_TEST(flowPP_case23);
    RUN_TEST(flowPP_case24);
    RUN_TEST(flowPP_case25);
    RUN_TEST(flowPP_case26);
    RUN_TEST(flowPP_case27);
    RUN_TEST(flowPP_case28);
    RUN_TEST(flowPP_case29);
    RUN_TEST(flowPP_case30);
    RUN_TEST(flowPP_case31);
    RUN_TEST(flowPP_case32);
    RUN_TEST(flowPP_case33);
    RUN_TEST(flowPP_case34);
    RUN_TEST(flowPP_case35);
    RUN_TEST(flowPP_case36);
    RUN_TEST(flowPP_case37);
    RUN_TEST(flowPP_case38);
    RUN_TEST(flowPP_case39);
    RUN_TEST(flowPP_case40);
    RUN_TEST(flowPP_case41);
    RUN_TEST(flowPP_case42);
    RUN_TEST(flowPP_case43);
}
