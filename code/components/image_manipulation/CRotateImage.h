#ifndef CROTATEIMAGE_H
#define CROTATEIMAGE_H

#include "CImageBasis.h"


class CRotateImage: public CImageBasis
{
    public:
        CImageBasis *imageTMP, *ImageOrg;
        bool doflip;
        CRotateImage(std::string name, std::string _image, bool _flip = false) :
                    CImageBasis(name, _image) {imageTMP = NULL; ImageOrg = NULL; doflip = _flip;};
        CRotateImage(std::string name, uint8_t* _rgb_image, int _channels, int _width, int _height, int _bpp, bool _flip = false) :
                    CImageBasis(name, _rgb_image, _channels, _width, _height, _bpp) {imageTMP = NULL;  ImageOrg = NULL; doflip = _flip;};
        CRotateImage(std::string name, CImageBasis *_org, CImageBasis *_temp, bool _flip = false);

        void rotateImage(float _angle);
        void rotateImageAntiAliasing(float _angle);

        void rotateImage(float _angle, int _centerx, int _centery);
        void rotateImageAntiAliasing(float _angle, int _centerx, int _centery);

        void translateImage(int _dx, int _dy);
        void mirrorImage();
};

#endif //CROTATEIMAGE_H