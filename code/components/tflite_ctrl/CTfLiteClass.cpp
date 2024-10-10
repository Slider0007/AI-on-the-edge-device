#include "CTfLiteClass.h"
#include "../../include/defines.h"

#include "ClassLogFile.h"
#include "helper.h"
#include "psram.h"


static const char *TAG = "TFLITE";

// #define DEBUG_DETAIL_ON


float CTfLiteClass::GetOutputValue(int nr)
{
    TfLiteTensor* output2 = interpreter->output(0);

    if (output2 == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "GetOutputValue failed");
        return -1000;
    }

    int numeroutput = output2->dims->data[1];
    if ((nr+1) > numeroutput)
        return -1000;
    else
        return output2->data.f[nr];
}


int CTfLiteClass::GetClassFromImageBasis(CImageBasis *rs)
{
    if (!LoadInputImageBasis(rs))
        return -1000;

    Invoke();

    return GetOutClassification();
}


int CTfLiteClass::GetOutClassification(int _von, int _bis)
{
    TfLiteTensor* output2 = interpreter->output(0);

    if (output2 == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "GetOutClassification failed");
        return -1;
    }

    float zw_max;
    float zw;
    int zw_class;

    int numeroutput = output2->dims->data[1];
    //ESP_LOGD(TAG, "number output neurons: %d", numeroutput);

    if (_bis == -1)
        _bis = numeroutput -1;

    if (_von == -1)
        _von = 0;

    if (_bis >= numeroutput)
    {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "GetOutClassification: NUMBER OF OUTPUT NEURONS does not match required classification");
        return -1;
    }

    zw_max = output2->data.f[_von];
    zw_class = _von;
    for (int i = _von + 1; i <= _bis; ++i)
    {
        zw = output2->data.f[i];
        if (zw > zw_max)
        {
            zw_max = zw;
            zw_class = i;
        }
    }
    return (zw_class - _von);
}


bool CTfLiteClass::GetInputDimension(bool silent = false)
{
  TfLiteTensor* input2 = interpreter->input(0);

    if (input2 == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "GetInputDimension failed");
        return false;
    }

    int numdim = input2->dims->size;
    if (!silent)  ESP_LOGD(TAG, "NumDimension: %d", numdim);

    int sizeofdim;
    for (int j = 0; j < numdim; ++j)
    {
      sizeofdim = input2->dims->data[j];
      if (!silent) ESP_LOGD(TAG, "SizeOfDimension %d: %d", j, sizeofdim);
      if (j == 1) im_height = sizeofdim;
      if (j == 2) im_width = sizeofdim;
      if (j == 3) im_channel = sizeofdim;
    }

    return true;
}


int CTfLiteClass::ReadInputDimenstion(int _dim)
{
    if (_dim == 0)
        return im_width;
    if (_dim == 1)
        return im_height;
    if (_dim == 2)
        return im_channel;

    return -1;
}


int CTfLiteClass::GetAnzOutPut(bool silent)
{
  TfLiteTensor* output2 = interpreter->output(0);

    if (output2 == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "GetAnzOutPut failed");
        return -1;
    }

    int numdim = output2->dims->size;
    if (!silent) ESP_LOGD(TAG, "NumDimension: %d", numdim);

    int sizeofdim;
    for (int j = 0; j < numdim; ++j)
    {
        sizeofdim = output2->dims->data[j];
        if (!silent) ESP_LOGD(TAG, "SizeOfDimension %d: %d", j, sizeofdim);
    }

    float fo;
    // Process the inference results.
    int numeroutput = output2->dims->data[1];
    for (int i = 0; i < numeroutput; ++i)
    {
        fo = output2->data.f[i];
        if (!silent) ESP_LOGD(TAG, "Result %d: %f", i, fo);
    }
    return numeroutput;
}


void CTfLiteClass::Invoke()
{
    if (interpreter != nullptr)
      interpreter->Invoke();
}


bool CTfLiteClass::LoadInputImageBasis(CImageBasis *rs)
{
    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("LoadInputImageBasis - Start");
    #endif

    if (rs == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadInputImageBasis: No image data");
        return false;
    }

    unsigned int w = rs->width;
    unsigned int h = rs->height;
    unsigned char red, green, blue;
//    ESP_LOGD(TAG, "Image: %s size: %d x %d\n", _fn.c_str(), w, h);

    input_i = 0;
    float* input_data_ptr = (interpreter->input(0))->data.f;

    if (input_data_ptr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadInputImageBasis: No input data");
        return false;
    }

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            {
                red = rs->getPixelColor(x, y, 0);
                green = rs->getPixelColor(x, y, 1);
                blue = rs->getPixelColor(x, y, 2);
                *(input_data_ptr) = (float) red;
                input_data_ptr++;
                *(input_data_ptr) = (float) green;
                input_data_ptr++;
                *(input_data_ptr) = (float) blue;
                input_data_ptr++;
            }

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("LoadInputImageBasis - done");
    #endif

    return true;
}


void CTfLiteClass::LoadOpResolver(void)
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


bool CTfLiteClass::MakeAllocate()
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Allocating tensors");
    tensor_arena = (uint8_t*)malloc_psram_heap(std::string(TAG) + "->tensor_arena", kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (tensor_arena == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Tensor arena: malloc failed");
        LogFile.writeHeapInfo("MakeAllocate-Tensor arena: malloc failed");
        return false;
    }

    interpreter = new tflite::MicroInterpreter(model, microOpResolver, tensor_arena, kTensorArenaSize);

    if (interpreter == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "new tflite::MicroInterpreter failed");
        LogFile.writeHeapInfo("MakeAllocate-new tflite::MicroInterpreter failed");
        return false;
    }

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Allocate tensors failed");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Tensors successfully allocated");
    return true;
}


void CTfLiteClass::GetInputTensorSize()
{
#ifdef DEBUG_DETAIL_ON
    float *zw = input;
    int test = sizeof(zw);
    ESP_LOGD(TAG, "Input Tensor Dimension: %d", test);
#endif
}


bool CTfLiteClass::ReadFileToModel(std::string _fn)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Read TFLITE model file: " + _fn);

    #ifdef DEBUG_DETAIL_ON
            LogFile.writeHeapInfo("ReadFileToModel: start");
    #endif

    size_t size = getFileSize(_fn);
    if (size <= 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File not existing or zero size: " + _fn);
        return false;
    }

    modelfile = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->modelfile", size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

	  if(modelfile == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ReadFileToModel: Can't allocate enough memory: " + std::to_string(size));
        LogFile.writeHeapInfo("ReadFileToModel: Allocation failed");
        return false;
    }

    FILE* f = fopen(_fn.c_str(), "rb");     // previously only "r

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(f, NULL, _IOFBF, 512);

    if (fread(modelfile, 1, size, f) != size) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ReadFileToModel: Reading error: Size differs");
        free_psram_heap(std::string(TAG) + "->modelfile", modelfile);
        fclose(f);
        return false;
    }
    fclose(f);

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("ReadFileToModel: done");
    #endif

    return true;
}


bool CTfLiteClass::LoadModel(std::string _fn)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Loading TFLITE model");

    if (!ReadFileToModel(_fn)) {
      LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadModel: TFLITE model file reading failed");
      return false;
    }

    model = tflite::GetModel(modelfile);

    if(model == nullptr) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadModel: GetModel failed");
        return false;
    }

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "LoadModel: Model provided is schema version " + std::to_string(model->version()) +
                                                " not equal to supported version " + std::to_string(TFLITE_SCHEMA_VERSION));
        return false;
    }

    LoadOpResolver(); // Preload operation resolver for tensor interpreter execution (make sure that this only gets called once)

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TFLITE model successfully loaded");
    return true;
}


CTfLiteClass::CTfLiteClass()
{
    model = nullptr;
    modelfile = NULL;
    interpreter = nullptr;
    input = nullptr;
    output = nullptr;
    tensor_arena = nullptr;
    kTensorArenaSize = 800 * 1024; // according to testfile: 108000 - so far 600;; 2021-09-11: 200 * 1024
}


void CTfLiteClass::CTfLiteClassDeleteInterpreter()
{
    if (tensor_arena != nullptr) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "TFLITE arena - Used bytes: " + std::to_string(interpreter->arena_used_bytes()));
        free_psram_heap(std::string(TAG) + "->tensor_arena", tensor_arena);
        tensor_arena = nullptr;
    }

    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }
}


CTfLiteClass::~CTfLiteClass()
{
    if (tensor_arena != nullptr) {
        free_psram_heap(std::string(TAG) + "->tensor_arena", tensor_arena);
    }

    if (interpreter != nullptr) {
        delete interpreter;
    }

    free_psram_heap(std::string(TAG) + "->modelfile", modelfile);
}
