#include <unity.h>
#include "openmetrics.cpp"


void test_createMetric()
{
    // simple happy path
    const char *expected = "# TYPE metric_name gauge\n# HELP metric_name short description\nmetric_name 123.456\n";
    std::string result = createMetric("metric_name", "gauge", "short description", "123.456");
    TEST_ASSERT_EQUAL_STRING(expected, result.c_str());
}


/**
 * test the replaceString function as it's a dependency to sanitize sequence names
 */
void test_replaceString()
{
    std::string sample = "hello\\world\\";
    replaceAll(sample, "\\", "");
    TEST_ASSERT_EQUAL_STRING("helloworld", sample.c_str());

    sample = "hello\"world\"";
    replaceAll(sample, "\"", "");
    TEST_ASSERT_EQUAL_STRING("helloworld", sample.c_str());

    sample = "hello\nworld\n";
    replaceAll(sample, "\n", "");
    TEST_ASSERT_EQUAL_STRING("helloworld", sample.c_str());

    sample = "\\\\\\\\\\\\\\\\\\hello\\world\\\\\\\\\\\\\\\\\\\\";
    replaceAll(sample, "\\", "");
    TEST_ASSERT_EQUAL_STRING("helloworld", sample.c_str());
}


void test_createSequenceMetrics()
{
    std::vector<NumberPost *> sequences;
    NumberPost *number_1 = new NumberPost;
    number_1->name = "main";
    number_1->sActualValue = "123.456";
    number_1->sRatePerMin = "0.001";
    sequences.push_back(number_1);

    const std::string metricNamePrefix = "ai_on_the_edge_device_";

    std::string expected1 = "# TYPE " + metricNamePrefix + "actual_value gauge\n# HELP " + metricNamePrefix +
                            "actual_value Actual value of meter\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            number_1->name + "\"} " + number_1->sActualValue + "\n" +
                            "# TYPE " + metricNamePrefix + "rate_per_minute gauge\n# HELP " + metricNamePrefix +
                            "rate_per_minute Rate per minute of meter\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            number_1->name + "\"} " + number_1->sRatePerMin + "\n";
    TEST_ASSERT_EQUAL_STRING(expected1.c_str(), createSequenceMetrics(metricNamePrefix, sequences).c_str());

    NumberPost *number_2 = new NumberPost;
    number_2->name = "secondary";
    number_2->sActualValue = "1.0";
    number_2->sRatePerMin = "0.0";
    sequences.push_back(number_2);

    std::string expected2 = "# TYPE " + metricNamePrefix + "actual_value gauge\n# HELP " + metricNamePrefix +
                            "actual_value Actual value of meter\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            number_1->name + "\"} " + number_1->sActualValue + "\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            number_2->name + "\"} " + number_2->sActualValue + "\n" +
                            "# TYPE " + metricNamePrefix + "rate_per_minute gauge\n# HELP " + metricNamePrefix +
                            "rate_per_minute Rate per minute of meter\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            number_1->name + "\"} " + number_1->sRatePerMin + "\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            number_2->name + "\"} " + number_2->sRatePerMin + "\n";
    TEST_ASSERT_EQUAL_STRING(expected2.c_str(), createSequenceMetrics(metricNamePrefix, sequences).c_str());
}


void test_openmetrics()
{
    test_createMetric();
    test_replaceString();
    test_createSequenceMetrics();
}
