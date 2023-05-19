#pragma once

#ifndef CFINDTEMPLATE_H
#define CFINDTEMPLATE_H

#include "CImageBasis.h"

struct strRefInfo {
    std::string image_file; 
    int alignment_algo = 0;             // 0 = "Default" (nur R-Kanal), 1 = "HighAccuracy" (RGB-Kanal), 2 = "Fast" (1.x RGB, dann isSimilar)
    int target_x = 0;
    int target_y = 0;
    int width = 0;
    int height = 0;
    int found_x = 0;
    int found_y = 0;
    int search_x = 0;
    int search_y = 0;
    int fastalg_x = 0;
    int fastalg_y = 0;
    int fastalg_SADThreshold = 0;
};


class CFindTemplate : public CImageBasis
{
    public:
        int tpl_width, tpl_height, tpl_bpp;    
        CFindTemplate(std::string name, uint8_t* _rgb_image, int _channels, int _width, int _height, int _bpp) : CImageBasis(name, _rgb_image, _channels, _width, _height, _bpp) {};

        bool FindTemplate(strRefInfo *_ref, bool _noFAST);
        bool CalculateSimularities(uint8_t* _rgb_tmpl, strRefInfo *_ref);
};

#endif //CFINDTEMPLATE_H