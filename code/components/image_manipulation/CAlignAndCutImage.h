#ifndef CALIGNANDCUTIMAGE_H
#define CALIGNANDCUTIMAGE_H

#include "ClassFlowDefineTypes.h"
#include "CImageBasis.h"
#include "CFindTemplate.h"

class CAlignAndCutImage : public CImageBasis
{
    public:
        CImageBasis *imageTMP;

        CAlignAndCutImage(std::string name, std::string _image) : CImageBasis(name, _image) {imageTMP = NULL;};
        CAlignAndCutImage(std::string name, std::string _image, bool _externalImage) :
                            CImageBasis(name, _image, _externalImage) {imageTMP = NULL;};
        CAlignAndCutImage(std::string name, uint8_t* _rgb_image, int _channels, int _width, int _height, int _bpp) :
                            CImageBasis(name, _rgb_image, _channels, _width, _height, _bpp) {imageTMP = NULL;};
        CAlignAndCutImage(std::string name, CImageBasis *_org, CImageBasis *_temp);
        ~CAlignAndCutImage();

        int alignImage(AlignmentMarker *_temp1, AlignmentMarker *_temp2);
        void cutAndSaveImage(std::string _template1, int x1, int y1, int dx, int dy);
        CImageBasis* cutAndSaveImage(int x1, int y1, int dx, int dy);
        void cutAndSaveImage(int x1, int y1, int dx, int dy, CImageBasis *_target);
};

#endif //CALIGNANDCUTIMAGE_H