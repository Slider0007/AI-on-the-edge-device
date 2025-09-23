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

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Model successfully loaded");
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


int CTfLiteClass::getOutClassification(int from, int to) const
{
    TfLiteTensor *outputTensor = interpreter->output(0);

    if (!outputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutClassification: Invalid outputTensor");
        return -1;
    }

    int numOutputs = outputTensor->dims->data[1];

    if (from < 0) {
        from = 0;
    }

    if (to < 0) {
        to = numOutputs - 1;
    }

    if (to >= numOutputs || from > to) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutClassification: Invalid range (from > to or to >= numOutputs)");
        return -1;
    }

    const float *outputData = outputTensor->data.f;
    int maxIndex = from;
    float maxValue = outputData[from];

    for (int i = from + 1; i <= to; ++i) {
        if (outputData[i] > maxValue) {
            maxValue = outputData[i];
            maxIndex = i;
        }
    }

    return maxIndex - from;
}


float CTfLiteClass::getOutputValue(int index) const
{
    TfLiteTensor *outputTensor = interpreter->output(0);

    if (!outputTensor) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputValue: Invalid outputTensor");
        return -1.0f;
    }

    int numOutputs = outputTensor->dims->data[1];

    if (index < 0 || index >= numOutputs) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getOutputValue: Index out of range");
        return -1.0f;
    }

    return outputTensor->data.f[index];
}


void CTfLiteClass::deleteInterpreter()
{
    if (tensorArena) {
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
