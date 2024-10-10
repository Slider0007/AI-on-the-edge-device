#ifndef CFINDTEMPLATE_H
#define CFINDTEMPLATE_H

#include "CImageBasis.h"
#include "ClassFlowDefineTypes.h"


class CFindTemplate : public CImageBasis
{
    private:
        uint8_t* rgb_template;
        bool calcSimularities(struct AlignmentMarker *_ref);

    public:
        int tpl_width, tpl_height, tpl_bpp;

        CFindTemplate(std::string name, uint8_t* _rgb_image, int _channels, int _width, int _height, int _bpp) :
                    CImageBasis(name, _rgb_image, _channels, _width, _height, _bpp) {rgb_template = NULL;};

        bool findTemplate(struct AlignmentMarker *_ref, bool _noFAST);
};

#endif //CFINDTEMPLATE_H