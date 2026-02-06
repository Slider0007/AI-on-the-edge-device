#include "CTfLiteClass.h"
#include "../../include/defines.h"

#include <flatbuffers/verifier.h>

#include "ClassLogFile.h"
#include "helper.h"
#include "psram.h"


static const char *TAG = "TFLITE";


CTfLiteClass::CTfLiteClass()
{
    modelFile = nullptr;
    model = nullptr;
    tensorArena = nullptr;
    kTensorArenaSize = 800 * 1024;
    interpreter = nullptr;
    output = nullptr;
    imHeight = 0;
    imWidth = 0;
    imChannel = 0;
    outputSoftmax = false;
}


void CTfLiteClass::loadOpResolver(void)
{
    // Add only needed OP resolver to save memory (flash memory + RAM)
    // NOTE: Whenever used model gets extended by new ops, they need to be added here
    microOpResolver.AddConv2D();
    microOpResolver.AddMaxPool2D();
    microOpResolver.AddMul();
    microOpResolver.AddAdd();
    microOpResolver.AddLeakyRelu();
    microOpResolver.AddQuantize();
    microOpResolver.AddDequantize();
    microOpResolver.AddReshape();
    microOpResolver.AddFullyConnected();
    microOpResolver.AddSoftmax();
}


bool CTfLiteClass::checkModelOperators(const tflite::Model *model)
{
    bool allOpsOk = true;
    auto *subgraph = model->subgraphs()->Get(0);

    for (unsigned int i = 0; i < subgraph->operators()->size(); i++) {
        auto *op = subgraph->operators()->Get(i);
        auto *opCode = model->operator_codes()->Get(op->opcode_index());

        if (opCode->builtin_code() == tflite::BuiltinOperator_CUSTOM) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Missing custom operator: " + std::string(opCode->custom_code()->c_str()));
            allOpsOk = false;
        }
        else {
            auto builtin = static_cast<tflite::BuiltinOperator>(opCode->builtin_code());
            const TFLMRegistration *registration = microOpResolver.FindOp(builtin);
            if (!registration) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "Missing operator: " + std::string(tflite::EnumNameBuiltinOperator(builtin)) + " (v" +
                                        std::to_string(opCode->version()) + ")");
                allOpsOk = false;
            }
        }
    }

    return allOpsOk;
}


bool CTfLiteClass::outputIsSoftmax() const
{
    if (!model) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "outputIsSoftmax: Model not loaded");
        return false;
    }

    auto *subgraph = model->subgraphs()->Get(0);
    int numOperators = subgraph->operators()->size();

    // 1. Iterate backward through the operators
    for (int i = numOperators - 1; i >= 0; --i) {
        auto *op = subgraph->operators()->Get(i);
        auto *opcode = model->operator_codes()->Get(op->opcode_index());
        auto builtin_code = opcode->builtin_code();

        // 2. Skip conversion/utility operations
        if (builtin_code == tflite::BuiltinOperator_DEQUANTIZE || builtin_code == tflite::BuiltinOperator_QUANTIZE) {
            // Found a conversion layer, skip it and check the previous one
            continue;
        }

        // 3. Check the first *functional* layer found
        if (builtin_code == tflite::BuiltinOperator_SOFTMAX) {
            return true; // Found Softmax as the last functional op
        }

        // 4. If we find any *other* functional operation (e.g., FullyConnected, Add)
        // before finding Softmax, then the output is logits.
        // We can stop here and conclude it's not Softmax.
        return false;
    }

    // Fallback if the subgraph is empty (highly unlikely)
    return false;
}


bool CTfLiteClass::makeAllocate()
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Allocating tensors");
    tensorArena = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->tensor_arena", kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!tensorArena) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "makeAllocate: Failed to allocate tensor arena memory");
        LogFile.writeHeapInfo("makeAllocate-Tensor arena: malloc failed");
        return false;
    }

    interpreter = new tflite::MicroInterpreter(model, microOpResolver, tensorArena, kTensorArenaSize);

    if (!interpreter) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "makeAllocate: Failed to create interpreter");
        LogFile.writeHeapInfo("makeAllocate-new tflite::MicroInterpreter failed");
        return false;
    }

    TfLiteStatus allocateStatus = interpreter->AllocateTensors();
    if (allocateStatus != kTfLiteOk) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "makeAllocate: Failed to allocate tensors");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Tensors successfully allocated");
    return true;
}


bool CTfLiteClass::readFileToModel(const std::string &fileName)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "readFileToModel: Read model file: " + fileName);

    size_t size = getFileSize(fileName);
    if (size <= 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readFileToModel: File not existing or zero size: " + fileName);
        return false;
    }

    modelFile = (unsigned char *)malloc_psram_heap(std::string(TAG) + "->modelFile", size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!modelFile) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readFileToModel: Can't allocate enough memory: " + std::to_string(size));
        LogFile.writeHeapInfo("readFileToModel: Allocation failed");
        return false;
    }

    FILE *file = fopen(fileName.c_str(), "rb");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readFileToModel: Failed to open model file: " + fileName);
        return false;
    }

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    setvbuf(file, nullptr, _IOFBF, 512);

    if (fread(modelFile, 1, size, file) != size) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readFileToModel: Reading error: Size differs");
        free_psram_heap(std::string(TAG) + "->modelFile", modelFile);
        fclose(file);
        return false;
    }
    fclose(file);

    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t *>(modelFile), size);
    if (!tflite::VerifyModelBuffer(verifier)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readFileToModel: Model file is corrupt or invalid");
        return false;
    }

    return true;
}


bool CTfLiteClass::loadModel(const std::string &fileName)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Loading model");

    if (!readFileToModel(fileName)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadModel: Failed to read model file");
        return false;
    }

    model = tflite::GetModel(modelFile);

    if (!model) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadModel: Failed to parse model file");
        return false;
    }

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "loadModel: Model is incompatible (Schema version: " + std::to_string(model->version()) +
                                ") | Supported schema version: " + std::to_string(TFLITE_SCHEMA_VERSION));
        return false;
    }

    // Preload operation resolver for tensor interpreter execution (make sure that this only gets called once)
    loadOpResolver();

    // Check model operators support
    if (!checkModelOperators(model)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadModel: Model is incompatible. Unsupported operators");
        return false;
    }

    // Check output config (softmax, logits)
    outputSoftmax = outputIsSoftmax();

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        (std::string) "Model successfully loaded | Output: " + (outputSoftmax ? "Probability (Softmax)" : "Raw (Logits)"));
    return true;
}


bool CTfLiteClass::loadInputImage(CImage &image)
{
    if (!image.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadInputImage: No image data");
        return false;
    }

    float *inputDataPtr = (interpreter->input(0))->data.f;

    if (!inputDataPtr) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadInputImage: No input data");
        return false;
    }

    uint8_t *imgData = image.getImgData();
    for (int i = 0; i < image.getImgDataSize(); ++i) {
        inputDataPtr[i] = (float)imgData[i];
    }

    return true;
}


bool CTfLiteClass::invoke()
{
    if (!interpreter) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "invoke: No interpreter loaded");
        return false;
    }

    TfLiteStatus status = interpreter->Invoke();
    if (status != kTfLiteOk) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to invoke | Error: " + std::to_string(status));
        return false;
    }

    return true;
}


bool CTfLiteClass::parseInputDimension()
{
    TfLiteTensor *inputTensor = interpreter->input(0);

    if (!inputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getInputDimension: Invalid inputTensor");
        return false;
    }

#ifdef DEBUG_DETAIL_ON
    ESP_LOGD(TAG, "Num Dimension: %d", inputTensor->dims->size);
#endif // DEBUG_DETAIL_ON

    for (int j = 0; j < inputTensor->dims->size; ++j) {
        switch (j) {
            case 1:
                imHeight = inputTensor->dims->data[j];
                break;
            case 2:
                imWidth = inputTensor->dims->data[j];
                break;
            case 3:
                imChannel = inputTensor->dims->data[j];
                break;
            default:
                break;
        }

#ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "Input Dimension %d: %d", j, inputTensor->dims->data[j]);
#endif // DEBUG_DETAIL_ON
    }

    return true;
}


int CTfLiteClass::getInputDimension(int dim) const
{
    switch (dim) {
        case 0:
            return imWidth;
        case 1:
            return imHeight;
        case 2:
            return imChannel;
        default:
            return -1;
    }
}


int CTfLiteClass::getOutputDimension() const
{
    if (!interpreter) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputDimension: No interpreter loaded");
        return -1;
    }

    TfLiteTensor *outputTensor = interpreter->output(0);

    if (!outputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputDimension: Invalid outputTensor");
        return -1;
    }

#ifdef DEBUG_DETAIL_ON
    int numDim = outputTensor->dims->size;
    ESP_LOGD(TAG, "Num Dimension: %d", numDim);

    for (int j = 0; j < numDim; ++j) {
        int dimSize = outputTensor->dims->data[j];

        ESP_LOGD(TAG, "Output Dimension %d: %d", j, dimSize);
    }

    int numOutput = outputTensor->dims->data[1];
    for (int i = 0; i < numOutput; ++i) {
        float val = outputTensor->data.f[i];

        ESP_LOGD(TAG, "Result %d: %f", i, val);
    }
#endif // DEBUG_DETAIL_ON

    return outputTensor->dims->data[1];
}


float CTfLiteClass::getOutputValue(const int index, bool returnConfidenceScore) const
{
    if (!interpreter) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputValue: No interpreter loaded");
        return -1.0f;
    }

    TfLiteTensor *outputTensor = interpreter->output(0);
    if (!outputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputValue: Invalid outputTensor");
        return -1.0f;
    }

    const int numOutputs = outputTensor->dims->data[1];
    if (index < 0 || index >= numOutputs) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputValue: Index out of range");
        return -1.0f;
    }

    // If raw logits requested (no softmax, no confidence), return directly
    if (!outputSoftmax && !returnConfidenceScore) {
        return outputTensor->data.f[index];
    }

    // Otherwise return probability or confidence score
    return getProbabilityConfidenceScore(outputTensor->data.f, numOutputs, index, outputSoftmax, returnConfidenceScore);
}


float CTfLiteClass::getProbabilityConfidenceScore(const float *data, int numOutputs, int index, bool outputSoftmaxed,
                                                  bool returnConfidenceScore) const
{
    if (!data || index < 0 || index >= numOutputs || numOutputs < 1) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getConfidenceScore: Invalid data or index");
        return -1.0f;
    }

    float maxLogit = 0.0f;
    float sumExp = 1.0f; // default for already softmaxed

    if (!outputSoftmaxed) {
        // Find max for numerical stability
        maxLogit = data[0];
        for (int i = 1; i < numOutputs; ++i) {
            maxLogit = fmaxf(maxLogit, data[i]);
        }

        // Compute sum of exponentials once
        sumExp = 0.0f;
        for (int i = 0; i < numOutputs; ++i) {
            sumExp += expf(data[i] - maxLogit);
        }
    }

    auto softmax = [&](int i) -> float { return outputSoftmaxed ? data[i] : expf(data[i] - maxLogit) / sumExp; };

    // If just probability is requested
    if (!returnConfidenceScore) {
        float probability = std::clamp(softmax(index), 0.0f, 1.0f);
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Probability: " + to_stringWithPrecision(probability, 2));
        return probability;
    }

    // Compute main cluster probability (Gaussian weighted)
    constexpr float sigma = 2.0f;
    float probabilityMainCluster = 0.0f;
    for (int i = 0; i < numOutputs; ++i) {
        int dist = circularDistance(i, index, numOutputs);
        float weight = expf(-(dist * dist) / (2.0f * sigma * sigma));
        probabilityMainCluster += softmax(i) * weight;
    }
    probabilityMainCluster = std::clamp(probabilityMainCluster, 0.0f, 1.0f);

    // Find next-best cluster outside �2
    float nextBestValue = -INFINITY;
    int nextBestIndex = -1;
    for (int i = 0; i < numOutputs; ++i) {
        if (circularDistance(i, index, numOutputs) > 2) {
            if (data[i] > nextBestValue) {
                nextBestValue = data[i];
                nextBestIndex = i;
            }
        }
    }

    constexpr float kMarginScale = 0.2f;
    float probabilityMargin = 1.0f;
    float confidenceScore = 0.0f;

    if (nextBestIndex >= 0) {
        float probabilityNextCluster = 0.0f;
        for (int i = 0; i < numOutputs; ++i) {
            if (circularDistance(i, nextBestIndex, numOutputs) <= 2) {
                probabilityNextCluster += softmax(i);
            }
        }
        probabilityNextCluster = std::clamp(probabilityNextCluster, 0.0f, 1.0f);

        probabilityMargin = fmaxf(probabilityMainCluster - probabilityNextCluster, 0.0f);
        confidenceScore = probabilityMainCluster * fminf(probabilityMargin / kMarginScale, 1.0f);
    }
    else {
        confidenceScore = probabilityMainCluster;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "ProbMain: " + to_stringWithPrecision(probabilityMainCluster, 2) + ", ProbMargin: " +
                            to_stringWithPrecision(probabilityMargin, 2) + ", Confidence: " + to_stringWithPrecision(confidenceScore, 2));

    return std::clamp(confidenceScore, 0.0f, 1.0f);
}


int CTfLiteClass::getHighestScoringClass(int startIndex, int endIndex) const
{
    if (!interpreter) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getMaxOutputIndex: No interpreter loaded");
        return -1;
    }

    TfLiteTensor *outputTensor = interpreter->output(0);
    if (!outputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getMaxOutputIndex: Invalid outputTensor");
        return -1;
    }

    int numOutputs = outputTensor->dims->data[1];

    startIndex = std::max(0, startIndex);
    endIndex = (endIndex < 0) ? numOutputs - 1 : std::min(endIndex, numOutputs - 1);

    if (startIndex > endIndex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getMaxOutputIndex: Invalid range (startIndex > endIndex)");
        return -1;
    }

    const float *outputs = outputTensor->data.f;

    int bestIndex = startIndex;
    float bestScore = outputs[startIndex];

    for (int i = startIndex + 1; i <= endIndex; ++i) {
        if (outputs[i] > bestScore) {
            bestScore = outputs[i];
            bestIndex = i;
        }
    }

    return bestIndex; // absolute index
}


void CTfLiteClass::deleteInterpreter()
{
    if (interpreter && tensorArena) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TFLite arena - Used bytes: " + std::to_string(interpreter->arena_used_bytes()));
        free_psram_heap(std::string(TAG) + "->tensorArena", tensorArena);
        tensorArena = nullptr;
    }

    if (interpreter) {
        delete interpreter;
        interpreter = nullptr;
    }
}


CTfLiteClass::~CTfLiteClass()
{
    if (tensorArena) {
        free_psram_heap(std::string(TAG) + "->tensorArena", tensorArena);
        tensorArena = nullptr;
    }

    if (interpreter) {
        delete interpreter;
        interpreter = nullptr;
    }

    free_psram_heap(std::string(TAG) + "->modelFile", modelFile);
    modelFile = nullptr;
}
