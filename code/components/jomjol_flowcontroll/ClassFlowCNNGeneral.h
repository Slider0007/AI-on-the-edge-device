#pragma once

#ifndef CLASSFLOWCNNGENERAL_H
#define CLASSFLOWCNNGENERAL_H

#include "ClassFlowDefineTypes.h"
#include "ClassFlowImage.h"
#include "ClassFlowAlignment.h"
#include "CTfLiteClass.h"
#include "ClassFlowImage.h"


class ClassFlowCNNGeneral : public ClassFlowImage
{
protected:
    t_CNNType CNNType;
    std::vector<general*> GENERAL;
    float CNNGoodThreshold;

    CTfLiteClass *tflite;
    ClassFlowAlignment* flowpostalignment;
    std::string cnnname;
    std::string cnnmodelfile;
    int modelxsize, modelysize, modelchannel;
    std::string LogImageSelect;
    bool isLogImageSelect;
    bool SaveAllFiles;   

    int EvalAnalogNumber(int _value, int _resultPreviousNumber);
    int EvalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, 
                            bool isPreviousAnalog = false, int analogDigitalTransitionStart=92);
    int EvalAnalogToDigitTransition(int _value, int _valuePreviousNumber,  int _resultPreviousNumber, int analogDigitalTransitionStart);

    bool doNeuralNetwork(std::string time); 
    bool doAlignAndCut(std::string time);

    bool getNetworkParameter();

public:
    ClassFlowCNNGeneral(ClassFlowAlignment *_flowalign, std::string _cnnname, t_CNNType _cnntype = AutoDetect);
    virtual ~ClassFlowCNNGeneral();

    bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
    bool doFlow(std::string time);
    void doPostProcessEventHandling();

    std::string getHTMLSingleStep(std::string host);
    std::string getReadout(int _seqNo = 0, bool _extendedResolution = false, int _valuePreviousNumber = -1, 
                                int _resultPreviousNumber = -1, int analogDigitalTransitionStart = 92);

    std::string getReadoutRawString(int _seqNo);  

    void DrawROI(CImageBasis *_zw); 

   	std::vector<HTMLInfo*> GetHTMLInfo();   

    int getNumberGENERAL();
    general* GetGENERAL(int _seqNo);
    general* GetGENERAL(std::string _name, bool _create);
    general* FindGENERAL(std::string _numbername);    
    std::string getNameGENERAL(int _seqNo);    

    void UpdateNameNumbers(std::vector<std::string> *_name_numbers);

    t_CNNType getCNNType() {return CNNType;};
    bool CNNTypeWithExtendedResolution();

    std::string name() {return "ClassFlowCNNGeneral - " +  cnnname;}; 
};

#endif
