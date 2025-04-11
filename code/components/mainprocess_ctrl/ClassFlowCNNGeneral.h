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

    ClassFlowAlignment *flowalignment;
    std::string cnnname;
    CNNType cnnType;

    CTfLiteClass *tflite;
    std::string cnnmodelfile;
    int modelxsize;
    int modelysize;
    int modelchannel;
    float CNNGoodThreshold;

    bool saveAllFiles;

    int evalAnalogNumber(int _value, int _resultPreviousNumber);
    int evalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, bool isPreviousAnalog = false,
                        int analogDigitSyncValue = 92);
    int evalAnalogToDigitTransition(int _value, int _valuePreviousNumber, int _resultPreviousNumber, int analogDigitSyncValue);

    bool resolveNetworkParameter();
    bool doExtractRoi(const std::string time);
    bool doInvokeCnn(const std::string time);

  public:
    ClassFlowCNNGeneral(ClassFlowAlignment *_flowalignment, const std::string _cnnname, const CNNType _cnntype = CNNTYPE_AUTODETECT);
    virtual ~ClassFlowCNNGeneral();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string getReadout(SequenceData *sequence, int _valuePreviousNumber = -1, int _resultPreviousNumber = -1);

    void drawROI(CImageBasis *image);

    CNNType getCNNType() const { return cnnType; };
    bool cnnTypeAllowExtendedResolution() const;

    std::string name() { return "ClassFlowCNNGeneral - " + cnnname; };
};

#endif // CLASSFLOWCNNGENERAL_H
