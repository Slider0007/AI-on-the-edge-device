#ifndef CLASSFFLOWPOSTPROCESSING_H
#define CLASSFFLOWPOSTPROCESSING_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowDefineTypes.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlowCNNGeneral.h"


class ClassFlowPostProcessing : public ClassFlow
{
  protected:
    const CfgData::SectionPostProcessing *cfgDataPtr = NULL;
    ClassFlowTakeImage *flowTakeImage;
    ClassFlowCNNGeneral *flowDigit;
    ClassFlowCNNGeneral *flowAnalog;

    bool fallbackValueLoaded;
    bool updateFallbackValue;
    bool loadFallbackValue(void);
    bool saveFallbackValue(void);

    void setDecimalShift();
    std::string shiftDecimal(std::string in, int _decShift);
    std::string substitudeN(std::string, double _fallbackValue);
    float checkDigitConsistency(double _value, int _decimalshift,
                                bool _isanalog, double _fallbackValue);

    void writeDataLog(std::string sequenceName);

  public:
    ClassFlowPostProcessing(ClassFlowTakeImage *_flowTakeImage, ClassFlowCNNGeneral *_flowDigit,
                                ClassFlowCNNGeneral *_flowAnalog);
    virtual ~ClassFlowPostProcessing();
    bool loadParameter();
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string getFallbackValue(std::string _sequenceName);
    bool setFallbackValue(double value, std::string _sequenceName);

    std::string name() { return "ClassFlowPostProcessing"; };

    // Only used for unity testing
    void setFallbackValueLoaded(bool value) { fallbackValueLoaded = value; };
};

#endif // CLASSFFLOWPOSTPROCESSING_H
