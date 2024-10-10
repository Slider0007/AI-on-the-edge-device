#include "test_flowpostprocessing_helper.h"


/**
 * @brief Testfall für Überprüfung allowNegatives
 *
 */
void testNegative()
{
    // Ohne decimalShift
    std::vector<float> digits = { 1.2, 6.7};
    std::vector<float> analogs = { 9.5, 8.4};
    double fallbackValue_extended = 16.985;
    double fallbackValue = 16.98;

    const char* expected = "16.98";

    // extendResolution=false
    // da kein negativ, sollte kein Error auftreten
    UnderTestPost* underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100);
    setAllowNegative(false);
    setFallbackValue(underTestPost, fallbackValue);
    std::string result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("000 Valid", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
    delete underTestPost;

    // extendResolution=true
    // da negativ im Rahmen (letzte Stelle -0.2 > ergebnis), kein Error
    // Aber der setFallbackValue wird gesetzt
    underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100, true);
    setAllowNegative(false);
    setFallbackValue(underTestPost, fallbackValue_extended);
    result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("E91 Rate negative", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(to_stringWithPrecision(fallbackValue_extended,
                                    getDecimalPlaceCount(underTestPost)).c_str(), result.c_str()); // Use Fallback value
    delete underTestPost;

    // extendResolution=true
    // Toleranz überschritten, Error wird gesetzt, kein ReturnValue
    fallbackValue_extended = 16.988; // zu groß
    underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100, true);
    setAllowNegative(false);
    setFallbackValue(underTestPost, fallbackValue_extended);
    result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("E91 Rate negative", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(to_stringWithPrecision(fallbackValue_extended,
                                    getDecimalPlaceCount(underTestPost)).c_str(), result.c_str()); // Use Fallback value
    delete underTestPost;

    // extendResolution=false
    // value < fallbackValue
    fallbackValue = 16.99; // zu groß
    underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100);
    setAllowNegative(false);
    setFallbackValue(underTestPost, fallbackValue_extended);
    result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("E91 Rate negative", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(to_stringWithPrecision(fallbackValue,
                                    getDecimalPlaceCount(underTestPost)).c_str(), result.c_str()); // Use Fallback value
    delete underTestPost;


    // extendResolution=false
    // value < fallbackValue
    // Aber Prüfung abgeschaltet => kein Fehler
    fallbackValue = 16.99; // zu groß
    underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100);
    setAllowNegative(true);
    setFallbackValue(underTestPost, fallbackValue_extended);
    result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("000 Valid", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
    delete underTestPost;
}


/**
 * @brief Fehlerberichte aus Issues
 *
 */
void testNegative_Issues()
{
    // Ohne decimalShift
    std::vector<float> digits = {2.0, 2.0, 0.0, 1.0, 7.2, 9.0, 8.0};
    std::vector<float> analogs = { };
    double fallbackValue_extended = 22018.080;
    double fallbackValue = 22018.08;

    const char* expected = "22017.98";

    // https://github.com/jomjol/AI-on-the-edge-device/issues/2145#issuecomment-1461899094
    // extendResolution=false
    // value < fallbackValue
    // Prüfung eingeschaltet => Fehler
    fallbackValue = 22018.08; // zu groß
    UnderTestPost* underTestPost = initDoFlow(digits, analogs, CNNTYPE_DIGIT_CLASS100, false, -2);
    setAllowNegative(false);
    setFallbackValue(underTestPost, fallbackValue);
    std::string result = processDoFlow(underTestPost);
    TEST_ASSERT_EQUAL_STRING("E91 Rate negative", underTestPost->sequenceDataPtr->sValueStatus.c_str());
    TEST_ASSERT_EQUAL_STRING(to_stringWithPrecision(fallbackValue,
                                    getDecimalPlaceCount(underTestPost)).c_str(), result.c_str()); // Use Fallback value
    delete underTestPost;
}