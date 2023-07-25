#pragma once

#ifndef CLASSFLOWCNNGENERAL_H
#define CLASSFLOWCNNGENERAL_H

#include"ClassFlowDefineTypes.h"
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
    string LogImageSelect;
    bool isLogImageSelect;
    bool SaveAllFiles;   

    int EvalAnalogNumber(int _value, int _resultPreviousNumber);
    int EvalDigitNumber(int _value, int _valuePreviousNumber, int _resultPreviousNumber, bool isPreviousAnalog = false, int analogDigitalTransitionStart=92);
    int EvalAnalogToDigitTransition(int _value, int _valuePreviousNumber,  int _resultPreviousNumber, int analogDigitalTransitionStart);

    bool doNeuralNetwork(std::string time); 
    bool doAlignAndCut(string time);

    bool getNetworkParameter();

public:
    ClassFlowCNNGeneral(ClassFlowAlignment *_flowalign, std::string _cnnname, t_CNNType _cnntype = AutoDetect);
    virtual ~ClassFlowCNNGeneral();

    bool ReadParameter(FILE* pfile, string& aktparamgraph);
    bool doFlow(string time);

    string getHTMLSingleStep(string host);
    std::string getReadout(int _seqNo = 0, bool _extendedResolution = false, int _valuePreviousNumber = -1, int _resultPreviousNumber = -1, int analogDigitalTransitionStart = 92);

    std::string getReadoutRawString(int _seqNo);  

    void DrawROI(CImageBasis *_zw); 

   	std::vector<HTMLInfo*> GetHTMLInfo();   

    int getNumberGENERAL();
    general* GetGENERAL(int _analog);
    general* GetGENERAL(string _name, bool _create);
    general* FindGENERAL(string _name_number);    
    string getNameGENERAL(int _analog);    

    bool isExtendedResolution(int _number = 0);

    void UpdateNameNumbers(std::vector<std::string> *_name_numbers);

    t_CNNType getCNNType(){return CNNType;};

    string name(){return "ClassFlowCNNGeneral - " +  cnnname;}; 
};

#endif
