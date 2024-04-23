#ifndef CLASSFLOWALIGNMENT_H
#define CLASSFLOWALIGNMENT_H

#include <string>

#include "ClassFlow.h"
#include "Helper.h"
#include "CAlignAndCutImage.h"
#include "CFindTemplate.h"


class ClassFlowAlignment : public ClassFlow
{
    protected:
        CImageBasis *ImageTMP;
        CAlignAndCutImage *AlignAndCutImage;
        strRefInfo References[2];
        int anz_ref;
        float initalrotate;
        bool flipImageSize;
        bool use_antialiasing;
        bool SaveDebugInfo;
        bool SaveAllFiles;
        int AlignFAST_SADThreshold;

        void SetInitialParameter(void);
        bool LoadReferenceAlignmentValues(void);
        bool SaveReferenceAlignmentValues(void);
        void doPostProcessEventHandling();

    public:
        CImageBasis *ImageBasis;
        ImageData *AlgROI;

        ClassFlowAlignment(std::vector<ClassFlow*>* lfc);
        virtual ~ClassFlowAlignment();

        CAlignAndCutImage* GetAlignAndCutImage() {return AlignAndCutImage;};
        void DrawRef(CImageBasis *_zw);
        bool ReadParameter(FILE* pfile, std::string& aktparamgraph);
        bool doFlow(std::string time);
        bool getFlipImageSize();
        std::string getHTMLSingleStep(std::string host);
        std::string name() {return "ClassFlowAlignment";};
};


#endif //CLASSFLOWALIGNMENT_H
