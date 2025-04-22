#ifndef CLASSFLOWCNNGENERAL_H
#define CLASSFLOWCNNGENERAL_H

#include "ClassFlowDefineTypes.h"
#include "ClassLogImage.h"
#include "ClassFlowAlignment.h"
#include "CTfLiteClass.h"
#include "ClassLogImage.h"
#include "configClass.h"


class ClassFlowCNNGeneral : public ClassLogImage
{
  protected:
    const CfgData::SectionDigit *sectionDigitPtr = nullptr;
    const CfgData::SectionAnalog *sectionAnalogPtr = nullptr;

    std::string cnnName;
    CNNType cnnType;

    CTfLiteClass *tflite = nullptr;
    std::string cnnModelFile;
    int modelWidth;
    int modelHeight;
    int modelChannel;
    float cnnGoodThreshold;

    bool saveAllFiles;

    int evalAnalogNumber(int _value, int _resultPreviousNumber) const;
    int evalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, bool isPreviousAnalog = false,
                        int analogDigitSyncValue = 92) const;
    int evalAnalogToDigitTransition(int _value, int _valuePreviousNumber, int _resultPreviousNumber, int analogDigitSyncValue) const;

    bool resolveNetworkParameter();
    bool doExtractRoi(const std::string time);
    bool doInvokeCnn(const std::string time);

  public:
    ClassFlowCNNGeneral(std::string _cnnName, CNNType _cnnType = CNNTYPE_AUTODETECT);
    virtual ~ClassFlowCNNGeneral();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string getReadout(SequenceData *sequence, int _valuePreviousNumber = -1, int _resultPreviousNumber = -1) const;

    void drawROI(CImage &image);

    CNNType getCNNType() const { return cnnType; };
    bool cnnTypeAllowExtendedResolution() const;

    std::string name() { return "ClassFlowCNNGeneral - " + cnnName; };
};

#endif // CLASSFLOWCNNGENERAL_H
