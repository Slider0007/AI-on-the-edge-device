#pragma once

#ifndef CLASSFLOWALIGNMENT_H
#define CLASSFLOWALIGNMENT_H

#include "ClassFlow.h"
#include "Helper.h"
#include "CAlignAndCutImage.h"
#include "CFindTemplate.h"

#include <string>

using namespace std;

class ClassFlowAlignment : public ClassFlow
{
protected:
    CImageBasis *ImageTMP;
    CAlignAndCutImage *AlignAndCutImage;
    strRefInfo References[2];
    int anz_ref;
    float initalrotate;
    bool initialmirror;
    bool initialflip;
    bool use_antialiasing;
    bool SaveAllFiles;
    int AlignFAST_SADThreshold;

    void SetInitialParameter(void);
    bool LoadReferenceAlignmentValues(void);
    bool SaveReferenceAlignmentValues(void);

public:
    CImageBasis *ImageBasis;
    ImageData *AlgROI;
    
    ClassFlowAlignment(std::vector<ClassFlow*>* lfc);
    virtual ~ClassFlowAlignment();

    CAlignAndCutImage* GetAlignAndCutImage() {return AlignAndCutImage;};
    void DrawRef(CImageBasis *_zw);
    bool ReadParameter(FILE* pfile, string& aktparamgraph);
    bool doFlow(string time);
    string getHTMLSingleStep(string host);
    string name() {return "ClassFlowAlignment";};
};


#endif //CLASSFLOWALIGNMENT_H
