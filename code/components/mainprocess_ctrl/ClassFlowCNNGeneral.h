#ifndef CLASSFLOWCNNGENERAL_H
#define CLASSFLOWCNNGENERAL_H

#include "ClassFlowDefineTypes.h"
#include "ClassLogImage.h"
#include "ClassFlowAlignment.h"
#include "CTfLiteClass.h"
#include "ClassLogImage.h"


// Helper struct to multiuse class without adapting variable names
// @TODO: Not ideal, could this be done more effcient to avoid this helper?
struct SequenceDataInternal {
    int sequenceId;                 // Sequence ID
    std::string sequenceName;       // Sequence Name
    std::vector<RoiData *> roiData; // ROI Data
};

class ClassFlowCNNGeneral : public ClassLogImage
{
  protected:
    std::vector<SequenceDataInternal *> sequenceDataInternal;

    ClassFlowAlignment *flowalignment;
    std::string cnnname;
    CNNType cnnType;

    CTfLiteClass *tflite;
    std::string cnnmodelfile;
    float CNNGoodThreshold;
    int modelxsize;
    int modelysize;
    int modelchannel;

    bool saveAllFiles;

    int evalAnalogNumber(int _value, int _resultPreviousNumber);
    int evalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, bool isPreviousAnalog = false, int analogDigitSyncValue = 92);
    int evalAnalogToDigitTransition(int _value, int _valuePreviousNumber, int _resultPreviousNumber, int analogDigitSyncValue);

    bool resolveNetworkParameter();
    bool doAlignAndCut(std::string time);
    bool doNeuralNetwork(std::string time);

  public:
    ClassFlowCNNGeneral(ClassFlowAlignment *_flowalignment, std::string _cnnname, CNNType _cnntype = CNNTYPE_AUTODETECT);
    virtual ~ClassFlowCNNGeneral();

    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string getReadout(SequenceData *sequence, int _valuePreviousNumber = -1, int _resultPreviousNumber = -1);

    void drawROI(CImageBasis *image);

    CNNType getCNNType() { return cnnType; };
    bool cnnTypeAllowExtendedResolution();

    std::string name() { return "ClassFlowCNNGeneral - " + cnnname; };
};

#endif
