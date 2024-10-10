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
    std::vector<SequenceData *> sequenceData;
    SequenceData *sequence1 = new SequenceData;
    sequence1->sequenceName = "main";
    sequence1->sActualValue = "123.456";
    sequence1->sRatePerMin = "0.001";
    sequenceData.push_back(sequence1);

    const std::string metricNamePrefix = "ai_on_the_edge_device_";

    std::string expected1 = "# TYPE " + metricNamePrefix + "actual_value gauge\n# HELP " + metricNamePrefix +
                            "actual_value Actual value of meter\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            sequence1->sequenceName + "\"} " + sequence1->sActualValue + "\n" +
                            "# TYPE " + metricNamePrefix + "rate_per_minute gauge\n# HELP " + metricNamePrefix +
                            "rate_per_minute Rate per minute of meter\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            sequence1->sequenceName + "\"} " + sequence1->sRatePerMin + "\n";
    TEST_ASSERT_EQUAL_STRING(expected1.c_str(), createSequenceMetrics(metricNamePrefix, sequenceData).c_str());

    SequenceData *sequence2 = new SequenceData;
    sequence2->sequenceName = "main";
    sequence2->sActualValue = "1.0";
    sequence2->sRatePerMin = "0.0";
    sequenceData.push_back(sequence2);

    std::string expected2 = "# TYPE " + metricNamePrefix + "actual_value gauge\n# HELP " + metricNamePrefix +
                            "actual_value Actual value of meter\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            sequence1->sequenceName + "\"} " + sequence1->sActualValue + "\n" + metricNamePrefix + "actual_value{sequence=\"" +
                            sequence2->sequenceName + "\"} " + sequence2->sActualValue + "\n" +
                            "# TYPE " + metricNamePrefix + "rate_per_minute gauge\n# HELP " + metricNamePrefix +
                            "rate_per_minute Rate per minute of meter\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            sequence1->sequenceName + "\"} " + sequence1->sRatePerMin + "\n" + metricNamePrefix + "rate_per_minute{sequence=\"" +
                            sequence2->sequenceName + "\"} " + sequence2->sRatePerMin + "\n";
    TEST_ASSERT_EQUAL_STRING(expected2.c_str(), createSequenceMetrics(metricNamePrefix, sequenceData).c_str());
}


void test_openmetrics()
{
    test_createMetric();
    test_replaceString();
    test_createSequenceMetrics();
}
