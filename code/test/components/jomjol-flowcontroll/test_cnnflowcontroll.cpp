#include <ClassFlowCNNGeneral.h>

class UnderTestCNN : public ClassFlowCNNGeneral {
    public:
    using ClassFlowCNNGeneral::EvalAnalogNumber;
    using ClassFlowCNNGeneral::EvalDigitNumber;
    using ClassFlowCNNGeneral::ClassFlowCNNGeneral;
    
};

// Helper to enter value as float (1.0 -> 10, 4.5 -> 45)
#define FLOAT_AS_INT(x) (int)(x*10)


/**
 * @brief test if all combinations of digit 
 * evaluation are running correctly
 */
void test_EvalAnalogNumber() 
{
    UnderTestCNN undertest = UnderTestCNN(nullptr, "analog", Digital100);

    // the 5.2 is already above 5.0 and the previous digit too (3)
    printf("Test 5.2, 3\n");
    int result = undertest.EvalAnalogNumber(FLOAT_AS_INT(5.2), 3);
    TEST_ASSERT_EQUAL(5, result);

    // the 5.2 is already above 5.0 and the previous digit not (9)
    // so the current digit shoult be reduced (4.9)
    printf("Test 5.2, 9\n");
    TEST_ASSERT_EQUAL(4, undertest.EvalAnalogNumber(FLOAT_AS_INT(5.2), 9));

    printf("Test 4.4, 9\n");
    // the 4.4 (digital100) is not above 5  and the previous digit (analog) too (9.3)
    TEST_ASSERT_EQUAL(4, undertest.EvalAnalogNumber(FLOAT_AS_INT(4.4), 9));

    printf("Test 4.5, 9\n");
    // the 4.5 (digital100) is not above 5  and the previous digit (analog) too (9.6)
    TEST_ASSERT_EQUAL(4, undertest.EvalAnalogNumber(FLOAT_AS_INT(4.5), 9));    

}

/**
 * @brief test if all combinations of digit 
 * evaluation are running correctly
 * 
 * -> Desciption for call undertest.EvalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, 
 * bool isPreviousAnalog, float digitalAnalogTransitionStart)
 * @param _value: is the current ROI as int value with one decimal digit (e.g. 10 -> 1.0)
 * @param _valuePreviousNumber: is the last (lower) ROI as int with one decimal digit (e.g. 10 -> 1.0)
 * @param _resultPreviousNumber: is the evaluated number. Sometimes a much lower value can change higer values
 *                          example: 9.8, 9.9, 0.1
 *                          0.1 => 0 (resultPreviousNumber)
 *                          The 0 makes a 9.9 to 0 (resultPreviousNumber)
 *                          The 0 makes a 9.8 to 0 
 * @param isPreviousAnalog: false/true if the last ROI is an analog or digit ROI (default=false == digit)
 *                              runs in special handling because analog is much less precise
 * @param analogDigitalTransitionStart  start of the transitionlogic begins on valuePreviousNumber (default=9.2)
 */
void test_EvalDigitNumber() {
    UnderTestCNN undertest = UnderTestCNN(nullptr, "digit", Digital100);

    // the 5.2 and no previous should trunc to 5
    printf("EvalDigitNumber(5.2, 0, -1)\n");
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.2), 0, -1));

    // the 5.3 and no previous should trunc to 5
    printf("EvalDigitNumber(5.3, 0, -1)\n");
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.3), 0, -1));

    printf("EvalDigitNumber(5.7, 0, -1)\n");
    // the 5.7 and no previous should trunc to 5
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.7), 0, -1, false, FLOAT_AS_INT(9.2)));

    // the 5.8 and no previous should round to 6 (already in transition)
    printf("EvalDigitNumber(5.8, 8.0, 8, false, 8.0)\n");
    TEST_ASSERT_EQUAL(6, undertest.EvalDigitNumber(FLOAT_AS_INT(5.8), FLOAT_AS_INT(8.0), 8, false, FLOAT_AS_INT(8.0)));

    // the 5.7 with previous and the previous between 0.3-0.7 should round up to 6
    TEST_ASSERT_EQUAL(6, undertest.EvalDigitNumber(FLOAT_AS_INT(5.7), FLOAT_AS_INT(0.4), 0));

    // the 5.3 with previous and the previous between 0.3-0.7 should trunc to 5
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.3), FLOAT_AS_INT(0.7), 0));

    // the 5.3 with previous and the previous <=0.7 should trunc to 5
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.3), FLOAT_AS_INT(0.1), 0));

    // the 5.2 with previous and the previous >9.7 (#define Digital_Transition_Area_Forward) should reduce to 4
    TEST_ASSERT_EQUAL(4, undertest.EvalDigitNumber(FLOAT_AS_INT(5.2), FLOAT_AS_INT(9.8), 9, false, FLOAT_AS_INT(9.0)));

    // the 5.7 with previous and the previous >9.7 (#define Digital_Transition_Area_Forward) should trunc to 5 (reason: decimal place >=4)
    TEST_ASSERT_EQUAL(5, undertest.EvalDigitNumber(FLOAT_AS_INT(5.7), FLOAT_AS_INT(9.8), 9));

    // the 4.5 (digital100) is not above 5 and the previous digit (analog) not over zero (9.7) -> #define Digital_Transition_Area_Forward
    TEST_ASSERT_EQUAL(4, undertest.EvalDigitNumber(FLOAT_AS_INT(4.5), FLOAT_AS_INT(9.8), 9));    

    // the 4.5 (digital100) is not above 5 and the previous digit (analog) over zero (0.7) -> #define Digital_Transition_Area_Predecessor
    TEST_ASSERT_EQUAL(4, undertest.EvalDigitNumber(FLOAT_AS_INT(4.5), FLOAT_AS_INT(0.7), 0));   

    // the 4.5 (digital100) is not above 5 and the previous digit (analog) not over zero (9.5)
    TEST_ASSERT_EQUAL(4, undertest.EvalDigitNumber(FLOAT_AS_INT(4.5), FLOAT_AS_INT(9.5), 9));    

    // 59.96889 - Pre: 58.94888
    // 8.6 : 9.8 : 6.7
    // the 8.6 (digital100) is not above 8 and the previous digit (analog) not over zero (9.8)
    TEST_ASSERT_EQUAL(8, undertest.EvalDigitNumber(FLOAT_AS_INT(8.6), FLOAT_AS_INT(9.8), 9));    

    // pre = 9.9 (0.0 raw)
    // zahl = 1.8
    TEST_ASSERT_EQUAL(2, undertest.EvalDigitNumber(FLOAT_AS_INT(1.8), FLOAT_AS_INT(9.0), 9));    
 
    // if a digit have an early transition and the pointer is < 9.0 
    // prev (pointer) = 6.2, but on digital readout = 6 (result is int parameter)
    // zahl = 4.6
    TEST_ASSERT_EQUAL(4, undertest.EvalDigitNumber(FLOAT_AS_INT(4.6), FLOAT_AS_INT(6.2), 6)); 
}

