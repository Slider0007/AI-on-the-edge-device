#include <unity.h>
#include "ClassMeterModel.h"

// EPSILON for float comparisons
#define TEST_EPS 1e-5f

void test_pow10_lookup(void)
{
    TEST_ASSERT_EQUAL_FLOAT(1.0, MeterModelHelper::pow10(0));
    TEST_ASSERT_EQUAL_FLOAT(1000.0, MeterModelHelper::pow10(3));
    TEST_ASSERT_EQUAL_FLOAT(0.001, MeterModelHelper::pow10(-3));
    TEST_ASSERT_EQUAL_FLOAT(1e7, MeterModelHelper::pow10(7));
}

void test_modulo1_precision(void)
{
    // Standard positive
    TEST_ASSERT_FLOAT_WITHIN(TEST_EPS, 0.5f, MeterModelHelper::modulo1(1.5));

    // Python-like negative behavior (-0.1 mod 1.0 = 0.9)
    TEST_ASSERT_FLOAT_WITHIN(TEST_EPS, 0.9f, MeterModelHelper::modulo1(-0.1));

    // Epsilon "Snap to Zero" (The 1e-9 logic)
    // 0.9999999999 should be treated as 1.0 -> 0.0
    TEST_ASSERT_EQUAL_FLOAT(0.0f, MeterModelHelper::modulo1(0.9999999999));

    // Tiny positive should remain positive
    TEST_ASSERT_FLOAT_WITHIN(1e-8f, 1e-8f, MeterModelHelper::modulo1(1e-8));
}

void test_modulo10_large_values(void)
{
    // 1234567890.45 mod 10 should be 0.45
    double largeVal = 1234567890.45;
    TEST_ASSERT_FLOAT_WITHIN(1e-7, 0.45, MeterModelHelper::modulo10(largeVal));
}

void test_baseDigit_extraction(void)
{
    double meterValue = 123456.789;

    // Extract 10^3 (Thousands) -> 3
    TEST_ASSERT_EQUAL_FLOAT(3.0f, MeterModelHelper::baseDigit(meterValue, 3));

    // Extract 10^0 (Ones) -> 6
    TEST_ASSERT_EQUAL_FLOAT(6.0f, MeterModelHelper::baseDigit(meterValue, 0));

    // Extract 10^-3 (Last dial) -> 9
    TEST_ASSERT_EQUAL_FLOAT(9.0f, MeterModelHelper::baseDigit(meterValue, -3));

    // Rollover edge case: 9.9999999999 with epsilon should snap to 0.0 or stay 9.0
    // baseDigit uses floor(digit + 1e-9). If digit is 9.999999999, it becomes 10.0 -> 0.0
    TEST_ASSERT_EQUAL_FLOAT(0.0f, MeterModelHelper::baseDigit(9.9999999999, 0));
}

void test_decimalPhase(void)
{
    double meterValue = 123.456;
    // Phase of the 10^1 (Tens) is 0.3456 (because 23.456 / 10 is 2.3456)
    // Wait, decimalPhase = modulo1(val / 10^idx).
    // For 123.456 and idx 1: 123.456 / 10 = 12.3456. Modulo 1 = 0.3456
    TEST_ASSERT_FLOAT_WITHIN(TEST_EPS, 0.3456f, MeterModelHelper::decimalPhase(meterValue, 1));
}

void test_circDistance10(void)
{
    // Shortest path 1 to 3 is +2
    TEST_ASSERT_EQUAL_FLOAT(2.0f, MeterModelHelper::circDistance10(1.0f, 3.0f));

    // Shortest path 9 to 1 is +2 (9 -> 0 -> 1)
    TEST_ASSERT_EQUAL_FLOAT(2.0f, MeterModelHelper::circDistance10(9.0f, 1.0f));

    // Shortest path 1 to 9 is -2 (1 -> 0 -> 9)
    TEST_ASSERT_EQUAL_FLOAT(-2.0f, MeterModelHelper::circDistance10(1.0f, 9.0f));

    // Across the circle (5.0)
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, MeterModelHelper::circDistance10(0.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, MeterModelHelper::circDistance10(5.0f, 0.0f));
}

void test_gaussianMatchLogScore(void)
{
    // Perfect match (delta 0) = 0 log score (which is 1.0 probability)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, MeterModelHelper::gaussianMatchLogScore(0.0f, 0.2f));

    // 1 Sigma deviation: -0.5 * (1^2) = -0.5
    TEST_ASSERT_EQUAL_FLOAT(-0.5f, MeterModelHelper::gaussianMatchLogScore(0.2f, 0.2f));

    // 2 Sigma deviation: -0.5 * (2^2) = -2.0
    TEST_ASSERT_EQUAL_FLOAT(-2.0f, MeterModelHelper::gaussianMatchLogScore(0.4f, 0.2f));
}

void testPostProcessingModelHelper()
{
    RUN_TEST(test_pow10_lookup);
    RUN_TEST(test_modulo1_precision);
    RUN_TEST(test_modulo10_large_values);
    RUN_TEST(test_baseDigit_extraction);
    RUN_TEST(test_decimalPhase);
    RUN_TEST(test_circDistance10);
    RUN_TEST(test_gaussianMatchLogScore);
}
