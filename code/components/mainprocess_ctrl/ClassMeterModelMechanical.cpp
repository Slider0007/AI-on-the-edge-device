#include "ClassMeterModel.h"

#include <cmath>

#include "nvs_flash.h"
#include "nvs.h"

#include "ClassLogFile.h"
#include "helper.h"

static const char *TAG = "METER_MECH";


MeterModelMechanical::MeterModelMechanical(size_t nDigits, size_t nAnalogDials, WheelType wheelType)
    : m_wheelType(wheelType), m_digitManualDetune(nDigits + nAnalogDials, 0.0f), m_digitAutoDetune(nDigits + nAnalogDials, 0.0f)
{
    m_nSequenceLength = nDigits + nAnalogDials;

    m_nDigits = nDigits;
    m_nAnalogDials = nAnalogDials;
    m_nDecimalPlaces = (int)nAnalogDials; // Assume dials as decimal places
}


MeterModelMechanical &MeterModelMechanical::setDialToWheelDetune(float detuneValue)
{
    size_t msdIdx = m_nDigits; // Default to first analog dial

    if (m_nAnalogDials == 0) {
        if (m_wheelType == WheelType::LSWContinuous || m_wheelType == WheelType::AllWheelsContinuous) {
            msdIdx = m_nDigits - 1; // The last digit is continuous rolling like an analog dial
        }
        else {
            return *this; // No continuous rolling transition -> no dial to wheel detune
        }
    }

    // Map input 0..10 to -5.0..5.0 range
    detuneValue = MeterModelHelper::modulo10(detuneValue);
    if (detuneValue > 5.0f) {
        detuneValue -= 10.0f;
    }

    if (msdIdx < m_digitManualDetune.size()) {
        m_digitManualDetune[msdIdx] = std::clamp(detuneValue, -5.0f, 5.0f);
    }

    return *this;
}


MeterModel::Result MeterModelMechanical::invokeModel(const SequenceData &sequenceData)
{
    Result result(m_nSequenceLength);

    if (m_nSequenceLength != sequenceData.digitRoi.size() + sequenceData.analogRoi.size()) {
        result.status = ResultStatus::FailedConfigMismatch;
        return result;
    }

    // Lambda helper to process ROIs and combine to one vector
    size_t idx = 0;
    auto processRoi = [&](const auto *roiPtr) {
        if (roiPtr && idx < m_nSequenceLength) {
            // Extract data
            result.ocrValues[idx] = std::clamp((float)roiPtr->CNNResult * 0.1f, 0.0f, 9.9f);
            result.ocrConfidences[idx] = (roiPtr->CNNResultConfidence > -1.0f) ? std::clamp(roiPtr->CNNResultConfidence, 0.0f, 1.0f) : 0.9f;

            // Precalculate std dev sigmas
            result.digitStdDevSigma[idx] = calculateSigma(idx, result.ocrConfidences[idx]);
            idx++;
        }
    };

    // Data extraction: Process digit ROIs
    for (const auto *roi : sequenceData.digitRoi) {
        processRoi(roi);
    }
    // Data extraction: Process analog ROIs
    for (const auto *roi : sequenceData.analogRoi) {
        processRoi(roi);
    }

    // Constants & Pre-allocated areas
    const int idxLsd = m_nSequenceLength - 1;
    const double detuneOffset = calculateDetuneOffset();
    DigitVector predictedBuffer(m_nSequenceLength, 0.0);

    // Greedy Search (MAP): Search from least-significant to most-significant position
    // Start with LSD as fixed anchor + perfect score
    double bestValue = std::round(MeterModelHelper::modulo10((double)result.ocrValues[idxLsd] - (double)getDigitDetune(idxLsd)) * 10.0) /
                       10.0;
    float bestScore = 0.0f;

    // Process all upward digits
    for (int i = idxLsd - 1; i >= 0; --i) {
        const double weight = MeterModelHelper::pow10((int)baseIndex(i));
        float bestCandidateScore = std::numeric_limits<float>::lowest();
        int bestDigit = 0;

        for (int digit = 0; digit < 10; ++digit) {
            const double candidate = bestValue + (digit * weight);

            forwardImpl(candidate, predictedBuffer);

            const float digitScore = MeterModelHelper::gaussianMatchLogScore(
                MeterModelHelper::circDistance10(predictedBuffer[i], result.ocrValues[i]), result.digitStdDevSigma[i]);

            const float candidateScore = bestScore + digitScore;

            if (candidateScore > bestCandidateScore) {
                bestCandidateScore = candidateScore;
                bestDigit = digit;
            }
        }
        bestValue += bestDigit * weight;
        bestScore = bestCandidateScore;
    }

    // Output generation
    // With extended resolution enabled: Use decimal place of LSD (least-siginificant dial)
    const double resolutionScaling = sequenceData.paramPostProc->extendedResolution ? 10.0 : 1.0;
    result.value = std::floor(((bestValue + detuneOffset) * resolutionScaling)) /
                   (MeterModelHelper::pow10(m_nDecimalPlaces) * resolutionScaling);
    result.decisionScore = result.score = bestScore;

    if (result.score < (float)m_nSequenceLength * -4.5f) {
        result.status = ResultStatus::BadVisualScore;
    }

    // Prepare result data
    forwardImpl(bestValue, result.predictedValues);

    for (size_t i = 0; i < result.digitDistances.size(); ++i) {
        result.digitDistances[i] = MeterModelHelper::circDistance10(result.predictedValues[i], result.ocrValues[i]);
        result.digitDetuneValues[i] = getDigitDetune(i);
        result.digitLogScores[i] = MeterModelHelper::gaussianMatchLogScore(result.digitDistances[i], result.digitStdDevSigma[i]);
    }

    // Auto-tune: Update digit detune based on actual results
    updateDigitAutoDetuneValues(result);

    // DIAGNOSTICS & LOGGING
    // Result summary
    char logBuf[320];
    const int precision = sequenceData.paramPostProc->extendedResolution ? m_nDecimalPlaces + 1 : m_nDecimalPlaces;

    snprintf(logBuf, sizeof(logBuf), "Model Result: %.*f | Score: %.2f | DecScore: %.2f | TotalDetune: %.*f", precision, result.value,
             result.score, result.decisionScore, m_nDecimalPlaces, detuneOffset / MeterModelHelper::pow10(m_nDecimalPlaces));

    LogFile.writeToFile(ESP_LOG_INFO, TAG, logBuf);

    // Digit details
    for (size_t i = 0; i < m_nSequenceLength; ++i) {
        const char *status = (result.digitStdDevSigma[i] >= 1.3f)          ? "BLIND"
                             : (result.digitStdDevSigma[i] >= 1.0f)        ? "WEAK"
                             : (result.digitLogScores[i] < -3.0f)          ? "FIGHT"
                             : (std::abs(result.digitDistances[i]) > 0.8f) ? "JUMP"
                                                                           : "OK";

        snprintf(logBuf, sizeof(logBuf),
                 "Idx: %d | %c | OCRConf: %d%% | OCR: %.2f | Model: %.2f | Distance: %.2f | Detune: %.2f | Sigma: %.2f | LogScore: %.2f | "
                 "Status: %s",
                 (int)(i + 1), (i < m_nDigits ? 'W' : 'D'), (int)(result.ocrConfidences[i] * 100.0f), result.ocrValues[i],
                 result.predictedValues[i], result.digitDistances[i], getDigitDetune(i), result.digitStdDevSigma[i],
                 result.digitLogScores[i], status);

        LogFile.writeToFile(ESP_LOG_INFO, TAG, logBuf);
    }

    return result;
}


float MeterModelMechanical::calculateSigma(const size_t idx, const float confidence) const
{
    // t: 0.0 (MSD) -> 1.0 (LSD)
    const float t = (m_nSequenceLength > 1) ? float(idx) / (m_nSequenceLength - 1) : 1.0f;

    // 1. Base Uncertainty
    const float baseSigma = 0.15f + (0.1f * t);

    // 2. OCR Uncertainty
    const float ocrUncertainty = 1.0f - std::clamp(confidence, 0.0f, 1.0f);

    // 3. Positional bias
    const float positionalBoost = 1.0f + (0.3f * t);

    // 4. Dynamic Scaling (user-adjustable)
    // Scale based on how much the OCR 'fight' the mechanical transition
    const float multiplier = 1.0f + (2.0f * m_modelInfluence) + (ocrUncertainty * 5.0f * m_modelInfluence);

    return std::clamp(baseSigma * positionalBoost * multiplier, 0.1f, 2.0f);
}


void MeterModelMechanical::forwardImpl(const double value, DigitVector &result) const
{
    // All wheels with continuous transition
    int lastIdxContinuous = 0; // 0 = MSW (most-significant wheel)

    // Hybrid: Least-siginificant wheel (LSW): Continuous transition / Higher wheels: Intermittent transition
    if (m_wheelType == WheelType::LSWContinuous) {
        lastIdxContinuous = std::max(0, (int)m_nDigits - 1);
    }
    // All wheels with intermittent transition
    else if (m_wheelType == WheelType::AllWheelsIntermittent) {
        lastIdxContinuous = (int)m_nDigits;
    }

    // Analog dials & wheels with continuous transition
    for (int idx = (int)m_nSequenceLength - 1; idx >= lastIdxContinuous; --idx) {
        result[idx] = continuousModel(value, idx);
    }

    // Wheels with intermittent transition
    float currentDt = m_wheelTransitionWidth;
    for (int idx = lastIdxContinuous - 1; idx >= 0; --idx) {
        result[idx] = intermittentModel(value, idx, currentDt);
        currentDt = std::max(currentDt * 0.1f, 0.001f); // Reduce the width for each wheel by 1/10 + limit for stability
    }
}


float MeterModelMechanical::continuousModel(const double value, const size_t idx) const
{
    const size_t bIdx = baseIndex(idx);

    // Extract i-th digit from value
    const float base = MeterModelHelper::baseDigit(value, bIdx);
    const float phase = MeterModelHelper::decimalPhase(value, bIdx);

    return MeterModelHelper::modulo10(base + phase + getDigitDetune(idx));
}


float MeterModelMechanical::intermittentModel(const double value, const size_t idx, float dt) const
{
    const size_t bIdx = baseIndex(idx);

    // Extract i-th digit from value
    const float base = MeterModelHelper::baseDigit(value, bIdx);
    const float phase = MeterModelHelper::decimalPhase(value, bIdx);

    const float localShift = intermittentTransitionFunction(phase + getDigitDetune(idx), dt);

    return MeterModelHelper::modulo10(base + localShift);
}


float MeterModelMechanical::intermittentTransitionFunction(const float phase, float dt)
{
    dt = std::clamp(dt, 0.001f, 1.0f);

    const float floorOffset = std::floor(phase);
    const float fractionalPhase = phase - floorOffset;

    const float transitionStart = 1.0f - dt;

    // Early exit: Not in transition zone
    if (fractionalPhase <= transitionStart) {
        return floorOffset;
    }

    // Smoothstep S-Curve mapping: 3x^2 - 2x^3
    const float x = std::clamp((fractionalPhase - transitionStart) / dt, 0.0f, 1.0f);
    const float snapProgress = x * x * (3.0f - 2.0f * x);

    return floorOffset + snapProgress;

    // Fallback: Linear mapping
    // return floorOffset + (fractionalPhase - transitionStart) / dt;
}


double MeterModelMechanical::calculateDetuneOffset() const
{
    double totalDetuneOffset = 0.0;
    for (size_t idx = 0; idx < m_nSequenceLength; ++idx) {
        totalDetuneOffset += MeterModelHelper::pow10((int)baseIndex(idx)) * (double)m_digitManualDetune[idx];
    }
    return totalDetuneOffset;
}


void MeterModelMechanical::updateDigitAutoDetuneValues(const Result &result)
{
    constexpr float alpha = 0.005f; // Small alpha, to avoid overreaction

    for (size_t i = 0; i < m_nSequenceLength; ++i) {
        // Only update if OCR is very confident and the global result makes sense
        if (result.ocrConfidences[i] > 0.95f && result.status == ResultStatus::OK && result.digitDistances[i] < 0.3f) {
            // Update the rolling average
            // digitDistances in Digits (e.g., 0.5 digits ahead)
            m_digitAutoDetune[i] = (result.digitDistances[i] * alpha) + (m_digitAutoDetune[i] * (1.0f - alpha));

            // Clamp to -0.05 to 0.05 allows the system to auto-correct by up to 0.25 digit (9 degree).
            m_digitAutoDetune[i] = std::clamp(m_digitAutoDetune[i], -0.25f, 0.25f);
        }
    }
}


void MeterModelMechanical::resetDigitAutoDetuneValues(const SequenceData &sequenceData)
{
    std::fill(m_digitAutoDetune.begin(), m_digitAutoDetune.end(), 0.0f);

    nvs_handle_t nvsHandle;
    if (nvs_open("model_mech", NVS_READWRITE, &nvsHandle) == ESP_OK) {
        std::string nvsKey = "dt_seqid" + std::to_string(sequenceData.sequenceId);
        nvs_erase_key(nvsHandle, nvsKey.c_str());
        nvs_commit(nvsHandle);
        nvs_close(nvsHandle);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequenceData.sequenceName + " | DigitAutoDetune values reset to default");
    }
}


esp_err_t MeterModelMechanical::saveDigitAutoDetuneValues(const SequenceData &sequenceData) const
{
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open("model_mech", NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
        return err;
    }

    size_t requiredSize = m_digitAutoDetune.size() * sizeof(float);
    std::string nvsKey = "dt_seqid" + std::to_string(sequenceData.sequenceId);
    err = nvs_set_blob(nvsHandle, nvsKey.c_str(), m_digitAutoDetune.data(), requiredSize);

    if (err == ESP_OK) {
        err = nvs_commit(nvsHandle);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Sequence: " + sequenceData.sequenceName + " | DigitAutoDetune values saved");
    }

    nvs_close(nvsHandle);
    return err;
}


esp_err_t MeterModelMechanical::loadDigitAutoDetuneValues(const SequenceData &sequenceData)
{
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open("model_mech", NVS_READONLY, &nvsHandle);
    if (err != ESP_OK) {
        return err;
    }

    size_t requiredSize = 0;
    // Check if key exists and get size
    std::string nvsKey = "dt_seqid" + std::to_string(sequenceData.sequenceId);
    err = nvs_get_blob(nvsHandle, nvsKey.c_str(), NULL, &requiredSize);

    if (err == ESP_OK && requiredSize == (m_digitAutoDetune.size() * sizeof(float))) {
        err = nvs_get_blob(nvsHandle, nvsKey.c_str(), m_digitAutoDetune.data(), &requiredSize);
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Sequence: " + sequenceData.sequenceName + " | DigitAutoDetune values loaded");
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "Sequence: " + sequenceData.sequenceName + " | DigitAutoDetune NVS storage empty. Using defaults");
        err = ESP_ERR_NVS_NOT_FOUND;
    }

    nvs_close(nvsHandle);
    return err;
}
