#include "ClassMeterModel.h"

#include <cmath>

#include "ClassLogFile.h"

static const char *TAG = "METER_LCD";


MeterModelLcd::MeterModelLcd(size_t nDigits)
{
    m_nSequenceLength = nDigits;
    m_nDigits = nDigits;
    m_nAnalogDials = 0;
    m_nDecimalPlaces = 0;
}


MeterModel::Result MeterModelLcd::invokeModel(const SequenceData &sequenceData)
{
    Result result(m_nSequenceLength);

    if (m_nSequenceLength != sequenceData.digitRoi.size()) {
        result.status = ResultStatus::FailedConfigMismatch;
        return result;
    }

    return result;
}


float MeterModelLcd::calculateSigma(size_t idx, float confidence) const
{
    return 0.2;
}


void MeterModelLcd::forwardImpl(const double value, DigitVector &result) const
{
    ;
}
