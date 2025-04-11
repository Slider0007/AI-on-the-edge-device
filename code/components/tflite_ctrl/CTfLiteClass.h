#ifndef CTFLITECLASS_H
#define CTFLITECLASS_H

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "CImageBasis.h"

class CTfLiteClass
{
  protected:
    tflite::MicroMutableOpResolver<10> microOpResolver;
    uint8_t *modelFile;
    const tflite::Model *model;
    int kTensorArenaSize;
    uint8_t *tensorArena;
    tflite::MicroInterpreter *interpreter;
    TfLiteTensor *output;

    int imHeight, imWidth, imChannel;

    bool readFileToModel(std::string fileName);
    void loadOpResolver(void);

  public:
    CTfLiteClass();
    ~CTfLiteClass();
    void deleteInterpreter();

    bool makeAllocate();
    bool loadModel(std::string fileName);
    bool loadInputImage(CImageBasis *image);
    bool invoke();

    bool parseInputDimension();
    int getInputDimension(int dim);

    int getOutputDimension();
    int getOutClassification(int from = -1, int to = -1);
    float getOutputValue(int index);
};

#endif // CTFLITECLASS_H
