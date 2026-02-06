#ifndef CTFLITECLASS_H
#define CTFLITECLASS_H

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "CImage.h"

class CTfLiteClass
{
  protected:
    tflite::MicroMutableOpResolver<10> microOpResolver;
    uint8_t *modelFile = nullptr;
    const tflite::Model *model;
    int kTensorArenaSize = 0;
    uint8_t *tensorArena = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *output = nullptr;

    int imHeight = 0;
    int imWidth = 0;
    int imChannel = 0;
    bool outputSoftmax = false;

    bool readFileToModel(const std::string &fileName);
    void loadOpResolver(void);
    bool checkModelOperators(const tflite::Model *model);
    bool outputIsSoftmax() const;
    float getProbabilityConfidenceScore(const float *data, int numOutputs, int index, bool outputSoftmaxed,
                                        bool returnConfidenceScore) const;

  public:
    CTfLiteClass();
    ~CTfLiteClass();
    void deleteInterpreter();

    bool makeAllocate();
    bool loadModel(const std::string &fileName);
    bool loadInputImage(CImage &image);
    bool invoke();

    bool parseInputDimension();
    int getInputDimension(int dim) const;

    int getOutputDimension() const;
    float getOutputValue(const int index, bool returnConfidenceScore = false) const;
    int getHighestScoringClass(int startIndex = -1, int endIndex = -1) const;
};

#endif // CTFLITECLASS_H
