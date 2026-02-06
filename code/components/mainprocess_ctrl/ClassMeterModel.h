#ifndef CLASSMETERMODEL_H
#define CLASSMETERMODEL_H

#include <memory>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "ClassFlowDefineTypes.h"


// Forward declaration
struct SequenceData;


/**
 * // The staticVector class template to be placed before using it as DigitVector
 *
 * @brief A minimal, fixed-size vector implementation to avoid dynamic memory reallocation
 *
 * @tparam T The type of elements stored
 * @tparam maxSize The maximum capacity of the vector
 */
template <typename T, std::size_t maxSize> class staticVector
{
  public:
    using valueType = T;
    using sizeType = std::size_t;
    using iterator = T *;
    using constIterator = const T *;

    /** @brief Default constructor. Initializes empty vector */
    staticVector() : size_(0) {}

    /**
     * @brief Constructor with size and initial value
     *
     * @param size Number of elements to initialize
     * @param initVal The value to assign to all elements
     */
    staticVector(size_t size, const T &initVal) : size_(size)
    {
        size_ = (size > maxSize) ? maxSize : size;
        for (size_t i = 0; i < size; ++i) {
            data_[i] = initVal;
        }
    }

    /** @brief Access element with bounds assertion */
    T &operator[](sizeType i)
    {
        assert(i < size_);
        return data_[i];
    }

    /** @brief Access element (const) with bounds assertion */
    const T &operator[](sizeType i) const
    {
        assert(i < size_);
        return data_[i];
    }

    /** @brief Returns pointer to the underlying array */
    T *data() { return data_; }

    /** @brief Returns const pointer to the underlying array */
    const T *data() const { return data_; }

    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    constIterator begin() const { return data_; }
    constIterator end() const { return data_ + size_; }

    /** @brief Returns the current number of elements */
    sizeType size() const { return size_; }

  private:
    T data_[maxSize]{};
    sizeType size_{0};
};


/**
 * @brief Abstract base meter model class
 */
class MeterModel
{
  public:
    static const size_t maxSequenceLength = 15; // Ensure precision
    using DigitVector = staticVector<float, maxSequenceLength>;

    enum class ResultStatus : int8_t {
        OK = 0,
        FailedConfigMismatch = -1, // ROI count doesn't match model
        BadVisualScore = -2,       // CNN and Model don't align
        TemporalError = -3,        // Physical impossibility (e.g. negative flow)
    };

    struct Result {
        double value{-1.0};                    // Value
        float score{-1000.0};                  // Score (Visual Only)
        float decisionScore{-1000.0};          // Score (Visual + Temporal)
        ResultStatus status{ResultStatus::OK}; // Status

        // Diagnostics
        DigitVector ocrValues;         // OCR values (CNN result: 0.0 - 9.9)
        DigitVector ocrConfidences;    // OCR confidences (CNN result: 0.0 - 1.0)
        DigitVector predictedValues;   // Modeled digits (single digits)
        DigitVector digitDistances;    // Digit distances
        DigitVector digitDetuneValues; // Digit detune values
        DigitVector digitStdDevSigma;  // Digit std deviation
        DigitVector digitLogScores;    // Digit log scores

        // Struct constructor
        Result(size_t len = 0)
            : ocrValues(len, 0.0), ocrConfidences(len, 1.0), predictedValues(len, 0.0), digitDistances(len, 0.0),
              digitDetuneValues(len, 0.0), digitStdDevSigma(len, 0.2), digitLogScores(len, 0.0)
        {}
    };

    virtual ~MeterModel() = default;

    /**
     * @brief Shifts the logical decimal point position
     *
     * @param nDecimalScaling Number of places to shift (positive moves decimal to the right, negative moves to left)
     *                      - Positive decimal places results in a decreased value by 10^n
     *                      - Negative decimal places results in a increased value by 10^n
     */
    void setDecimalScaling(const int nDecimalScaling) { m_nDecimalPlaces = (int)m_nAnalogDials - nDecimalScaling; }

    /**
     * @brief Sets the model logic validation strictness
     * Determines, how strictly the system enforces the physical constraints of the model
     *
     * @param logicVerificationStrictness Logic Validation Strictness [0.0: loose - 1.0: strict]
     */
    void setModelInfluence(const float modelInfluence) { m_modelInfluence = modelInfluence; }

    /** @brief Reset auto detune values to default
     *
     * @param sequenceData Pointer to sequence data
     */
    virtual void resetDigitAutoDetuneValues(const SequenceData &sequenceData) {}

    /** @brief Save auto detune values to NVS
     *
     * @param sequenceData Pointer to sequence data
     * @return esp_err_t Return value (ESP_OK = successful)
     */
    virtual esp_err_t saveDigitAutoDetuneValues(const SequenceData &sequenceData) const { return ESP_OK; }

    /** @brief Load auto detune values from NVS
     *
     * @param sequenceData Pointer to sequence data
     * @return esp_err_t Return value (ESP_OK = successful)
     */
    virtual esp_err_t loadDigitAutoDetuneValues(const SequenceData &sequenceData) { return ESP_OK; }

    /**
     * @brief Reconstructs the number sequence by searching for the most likely digits
     *
     * @param sequenceData Number sequence data containing CNN results
     * @return Result containing the calculated value and further metrics
     */
    virtual Result invokeModel(const SequenceData &sequenceData) = 0;

    /** @brief Get model type */
    virtual MeterType getType() const = 0;

  protected:
    size_t m_nSequenceLength{0}; // Number sequence length (sum of all digits)
    size_t m_nDigits{0};         // Number of rolling wheels or digits
    size_t m_nAnalogDials{0};    // Number of analog dials
    int m_nDecimalPlaces{0};     // Number of decimal places (fraction after the comma)
    float m_modelInfluence{0.5}; // Model influence [0.0: loose - 1.0: strict]

    /** @brief Maps a digit index to its position in the base-10 sequence */
    size_t baseIndex(size_t digitIdx) const { return (digitIdx >= m_nSequenceLength) ? 0 : (m_nSequenceLength - 1 - digitIdx); }

    /** @brief Internal implementation of the forward simulation */
    virtual void forwardImpl(const double value, DigitVector &result) const = 0;
};


/**
 * @brief Mathematical model for a mechanical driven meter
 *
 * * This class simulates the behavior of a meter containing a combination of
 * rolling odometer wheels (digits) and circular analog dials (hands). It accounts for
 * mechanical properties like geared synchronization, carry-over transitions, and
 * wheel misalignment (detune).
 */
class MeterModelMechanical : public MeterModel
{
  public:
    /**
     * @brief Motion profile for the least-significant rolling wheel digit
     */

    /**
     * @brief Constructor
     *
     * @param wheelType Wheel type (based on transition behaviour)
     * @param nDigits Number of rolling odometer wheels
     * @param nAnalogDials Number of circular analog dials
     */
    MeterModelMechanical(size_t nDigits, size_t nAnalogDials, WheelType wheelType = WheelType::LSWContinuous);

    /**
     * @brief Sets the width of the mechanical transition window for intermittent wheel motion
     *
     * For intermittent motion, this defines the "active" phase (e.g., 0.1 means
     * the wheel moves from 9 to 0 during the last 10% of the preceding dial's rotation).
     * For continuous motion, this value is ignored as the transition is constant (1.0).
     *
     * @param width Width in phase space [0, 1]. (e.g., 0.1 = 10% of a rotation)
     * @return Reference to self for method chaining
     */
    void setWheelTransitionWidth(const float width) { m_wheelTransitionWidth = width; }

    /**
     * @brief Sets the dial value where the dial-to-wheel carry-over ends
     */
    MeterModelMechanical &setDialToWheelDetune(float value);

    void resetDigitAutoDetuneValues(const SequenceData &sequenceData) override;
    esp_err_t saveDigitAutoDetuneValues(const SequenceData &sequenceData) const override;
    esp_err_t loadDigitAutoDetuneValues(const SequenceData &sequenceData) override;

    Result invokeModel(const SequenceData &sequence) override;
    MeterType getType() const override { return MeterType::Mechanical; }

  private:
    WheelType m_wheelType{WheelType::LSWContinuous}; // Meter type (Default: Least-significant wheel with continuous transition)
    float m_wheelTransitionWidth{0.12};              // Intermittent wheel: Rollover Transition Width
    DigitVector m_digitManualDetune;                 // User-defined detune, digit-specific [-5.0 - 5.0]
    DigitVector m_digitAutoDetune;                   // Process relelated detune (EMA of digit distance), digit-specific [-5.0 - 5.0]

    void forwardImpl(const double value, DigitVector &result) const;

    /** @brief Calculates a dynamic sigma (standard deviation) based on AI confidence */
    float calculateSigma(const size_t idx, const float confidence) const;

    /**
     * @brief Computes detune offset value based on detune parameter
     */
    double calculateDetuneOffset() const;

    /**
     * @brief Model a circular dial (0-9 / 360°) / Wheel with continuous transition (0-9 / odometer-style)
     *
     * For each digit of parameter``value``, this function imagines a circular dial
     * labeled 0–9 with a single pointer or a wheel labeled 0-9.
     * The pointer / the wheel moves *continuously* as the underlying value changes:
     *
     * - The integer decimal digit sets the base position on the dial / wheel
     * - The phase within the current decade (0..1) moves the pointer / wheel between
     *   the current digit and the next
     *
     * @param value   The true numeric value to be represented. Typically a
     *                non-negative value; only its decimal digits their phases are used.
     *
     * @param idx  The index of the digit to be computed starting from
     *                the most-significant digit first
     *
     * @return        The position on each 0–9 dial / wheel, wrapped modulo 10.
     */
    float continuousModel(const double value, const size_t idx) const;

    /**
     * @brief Model a wheel with intermittent transition (0-9 / odometer-style)
     *
     * This models how each digit of parameter ``value`` would appear on an
     * rolling wheel (odometer-style), where wheel transition smoothly to
     * the next value over a finite phase window.
     *
     * Each digit has its own phase offset (``m_phaseDetuneVec``) to account for
     * misalignment or detuning between digit wheels. The transition width
     * is controlled by ``dt``: smaller values give a sharper, more
     * instantaneous change; larger values give a longer rolling transition.
     *
     * @param value   The true numeric value to be represented. Typically a
     *                non-negative value; only its decimal digits are used.
     *
     * @param idx  The index of the digit to be computed starting from
     *                the most-significant digit first
     *
     * @param dt      Width of the transition region in phase space, in [0, 1).
     *                A value of 0.1 means the wheel takes 10% of a full cycle
     *                to roll from its current value to the next.
     *
     * @return        The modeled digit value. This is generally a non-integer in the
     *                transition region and are taken modulo 10, so they are
     *                suitable for rendering as analog/rolling digits.
     */
    float intermittentModel(const double value, const size_t idx, float dt) const;

    /**
     * @brief Transfer function for wheels with intermittent transition
     *
     * @param phase The rotation phase of the driving wheel [0, 1]
     * @param dt The width of the transition zone
     * @return The resulting offset for the driven wheel
     */
    static float intermittentTransitionFunction(const float phase, float dt);

    /**
     * @brief Adaptively updates the digit-specific logic detune using an Exponential Moving Average (EMA)
     *
     * This method performs "online learning" to identify digits that consistently deviate from
     * the logical meter model. By observing the distance between the raw OCR output and the
     * verified result, it builds a statistical "reputation" for each digit.
     *
     * @param result The final verified result after applying meter logic
     */
    void updateDigitAutoDetuneValues(const Result &result);

    /**
     * @brief Retrieves the total digit-specific logic detune
     *
     * Combines manual technician-set penalties with the adaptive auto-detune penalty
     *
     * @param idx The index of the digit in the sequence (0 = MSD)
     * @return Total detune value clamped between [-5.0, 5.0]
     */
    float getDigitDetune(const size_t idx) const { return std::clamp(m_digitManualDetune[idx] + m_digitAutoDetune[idx], -5.0f, 5.0f); }
};


class MeterModelLcd : public MeterModel
{
  public:
    MeterModelLcd(size_t nDigits);

    Result invokeModel(const SequenceData &sequence) override;
    MeterType getType() const override { return MeterType::DigitalLcd; }

  private:
    void forwardImpl(double value, DigitVector &result) const override;

    /** @brief Calculates a dynamic sigma (standard deviation) based on AI confidence */
    float calculateSigma(const size_t idx, const float confidence) const;
};


namespace MeterModelHelper
{
static constexpr const double pow10Table[] = {
    1.0,               // 10^0
    10.0,              // 10^1
    100.0,             // 10^2
    1000.0,            // 10^3
    10000.0,           // 10^4
    100000.0,          // 10^5
    1000000.0,         // 10^6
    10000000.0,        // 10^7
    100000000.0,       // 10^8
    1000000000.0,      // 10^9
    10000000000.0,     // 10^10
    100000000000.0,    // 10^11
    1000000000000.0,   // 10^12
    10000000000000.0,  // 10^13
    100000000000000.0, // 10^14
    1000000000000000.0 // 10^15
};
static constexpr const size_t pow10TableSize = std::size(pow10Table);


/** @brief A pre-computed pow10 function */
static inline double pow10(const int n)
{
    if (n >= 0 && n < (int)pow10TableSize) {
        return pow10Table[n]; // Return 10^+n
    }

    if (n < 0 && -n < (int)pow10TableSize) {
        return 1.0 / pow10Table[-n]; // Return 10^-n
    }

    // Only use for larger exponents than in lookup table available
    return std::pow(10.0, n);
}


/** @brief A python-like modulo [0, y] */
static inline double modulo(const double x, const double y)
{
    // Python's % operator: result = x - y * floor(x/y)
    return x - y * std::floor((x / y) + 1e-9);
}


/** @brief A python-like modulo for phase [0, 1] */
static inline float modulo1(const double x)
{
    // Returns value in range [0, 1]
    const double phase = x - std::floor(x + 1e-4);

    // Hard boundary safety
    if (phase <= 0.0 || phase >= 1.0) {
        return 0.0f;
    }

    return (float)phase;
}


/** @brief A python-like modulo [0, 10] */
static inline double modulo10(const double x)
{
    // Returns value in range [0, 10]
    return x - 10.0 * std::floor((x * 0.1) + 1e-4);
}


/** @brief A python-like modulo for digits [0, 10] */
static inline float modulo10(const float x)
{
    // Returns value in range [0, 10]
    return x - 10.0f * std::floor((x * 0.1f) + 1e-4f);
}


/** @brief Extracts the integer digit [0-9] at a specific position */
static inline float baseDigit(const double value, const int idx)
{
    const double digit = modulo10(value / MeterModelHelper::pow10(idx));
    const float result = (float)std::floor(digit + 1e-4);

    // Physical rollover safety
    return (result >= 10.0f) ? 0.0f : result;
}


/** @brief Calculates the fractional phase [0, 1] of a specific decade */
static inline float decimalPhase(const double value, const int idx)
{
    // Extract fractional part [0.0, 1.0)
    return modulo1(value / MeterModelHelper::pow10(idx));
}


/**
 * @brief Compute the circular distance between values on a 0–10 circle
 *
 * This returns the shortest distance between ``predictedVal`` and ``ocrVal``
 * on a circular scale that wraps at 10. The distance is always in [-5, 5].
 *
 * @return The circular distance(s) between ``predictedVal`` and ``ocrVal``
 */
static inline float circDistance10(const float predictedVal, const float ocrVal)
{
    const float diff = ocrVal - predictedVal; // Direction matters: Reality - Model
    return diff - 10.0f * std::floor((diff + 5.0f) * 0.1f);
}


/**
 * @brief Log of a gaussian shaped compatibility score for a deviation
 *
 * @param deltaX Difference between model prediction and AI measurement
 * @param sigma The standard deviation (expected error)
 * @return The log match score
 */
static inline float gaussianMatchLogScore(const float deltaX, const float sigma)
{
    const float z = deltaX / sigma;
    return -0.5f * z * z;
}
} // namespace MeterModelHelper

#endif // CLASSMETERMODEL_H
