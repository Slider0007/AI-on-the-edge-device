#include "CAlignAndCutImage.h"
#include "../../include/defines.h"

#include <math.h>
#include <algorithm>
#include <esp_log.h>

#include "psram.h"
#include "CRotateImage.h"
#include "ClassLogFile.h"
#include "helper.h"


static const char* TAG = "IMG_ALIGNCUT";


CAlignAndCutImage::CAlignAndCutImage(std::string _name, CImageBasis *_org, CImageBasis *_temp) : CImageBasis(_name)
{
    name = _name;
    rgb_image = _org->rgb_image;
    channels = _org->channels;
    width = _org->width;
    height = _org->height;
    bpp = _org->bpp;
    externalImage = true;

    islocked = false;

    imageTMP = _temp;
}


int IRAM_ATTR CAlignAndCutImage::alignImage(AlignmentMarker *_temp1, AlignmentMarker *_temp2)
{
    int dx1, dy1, dx2, dy2;
    int r0_x, r0_y, r1_x, r1_y;
    bool isSimilar1, isSimilar2;

    CFindTemplate* ft = new CFindTemplate("align", rgb_image, channels, width, height, bpp);

    r0_x = _temp1->targetX;
    r0_y = _temp1->targetY;
    //ESP_LOGD(TAG, "Before ft->findTemplate(_temp1); %s", _temp1->markerImageFilename.c_str());
    isSimilar1 = ft->findTemplate(_temp1, false);
    _temp1->width = ft->tpl_width;
    _temp1->height = ft->tpl_height;
    _temp1->errorMsg = "";

    r1_x = _temp2->targetX;
    r1_y = _temp2->targetY;
    //ESP_LOGD(TAG, "Before ft->findTemplate(_temp2); %s", _temp2->markerImageFilename.c_str());
    isSimilar2 = ft->findTemplate(_temp2, !isSimilar1); // disable FAST processing if first ALGO FAST result is -> no match
    _temp2->width = ft->tpl_width;
    _temp2->height = ft->tpl_height;
    _temp2->errorMsg = "";

    delete ft;

    dx1 = _temp1->targetX - _temp1->foundX;
    dy1 = _temp1->targetY - _temp1->foundY;

    dx2 = _temp2->targetX - _temp2->foundX;
    dy2 = _temp2->targetY - _temp2->foundY;

    r0_x += dx1;
    r0_y += dy1;

    r1_x += dx1;
    r1_y += dy1;

    float w_org, w_actual, angle_deviation;

    w_org = atan2(_temp2->foundY - _temp1->foundY, _temp2->foundX - _temp1->foundX);
    w_actual = atan2(r1_y - r0_y, r1_x - r0_x);

    angle_deviation = (w_actual - w_org) * 180 / M_PI;

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Align: dx1 %d, dy1 %d, dx2 %d, dy2 %d, angle dev %f", dx1, dy1, dx2, dy2, angle_deviation);
    #endif

    if (fabs(angle_deviation) > 45 || abs(dx1) >= _temp1->searchX || abs(dy1) >= _temp1->searchY  ||
                                      abs(dx2) >= _temp2->searchX || abs(dy2) >= _temp2->searchY)
    {
        _temp1->errorMsg = _temp2->errorMsg = "Angle dev: " + to_stringWithPrecision(angle_deviation, 1) +
                            ", Ref0dx: " + std::to_string(dx1)+ ", Ref0dy: " + std::to_string(dy1) +
                            ", Ref1dx: " + std::to_string(dx2)+ ", Ref1dy: " + std::to_string(dy2);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, _temp1->errorMsg);

        return -1; // ALIGNMENT FAILED
    }

    CRotateImage rt("Align", this, imageTMP);
    if ((dx1 > 0 && dx2 > 0 && dy1 > 0 && dy2 > 0) || (dx1 < 0 && dx2 < 0 && dy1 < 0 && dy2 < 0)) { // only move linaer because no rotative motion obviuos
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Align: Correction by linear dx + dy only");
        rt.translateImage(dx1, dy1);
    }
    else if ((dx1 > 0 && dx2 > 0) || (dx1 < 0 && dx2 < 0)) { // only rotate + move x direction
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Align: Correction by rotation + linear dx");
        rt.rotateImage(angle_deviation, width/2, height/2);
        rt.translateImage(dx1/2, 0); // correct only by half because some correction already happen with rotation
    }
    else if ((dy1 > 0 && dy2 > 0) || (dy1 < 0 && dy2 < 0)) { // only rotate + move y direction
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Align: Correction by rotation + linear dy");
        rt.rotateImage(angle_deviation, width/2, height/2);
        rt.translateImage(0, dy1/2); // correct only by half because some correction already happen with rotation
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Align: Correction by rotation only"); // only rotate because no obviuos linear motion detected
        rt.rotateImage(angle_deviation, width/2, height/2);
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Angle dev: " + to_stringWithPrecision(angle_deviation, 1) +
                                ", Ref0dx: " + std::to_string(dx1)+ ", Ref0dy: " + std::to_string(dy1) +
                                ", Ref1dx: " + std::to_string(dx2)+ ", Ref1dy: " + std::to_string(dy2));

    if (isSimilar1 && isSimilar2) {
        return 1; // ALGO FAST match
    }
    else {
        return 0; // ALGO STANDARD done
    }
}


void IRAM_ATTR CAlignAndCutImage::cutAndSaveImage(std::string _template1, int x1, int y1, int dx, int dy)
{
    int x2, y2;

    x2 = x1 + dx;
    y2 = y1 + dy;
    x2 = std::min(x2, width - 1);
    y2 = std::min(y2, height - 1);

    dx = x2 - x1;
    dy = y2 - y1;

    int memsize = dx * dy * channels;
    uint8_t* odata = (unsigned char*) malloc_psram_heap(std::string(TAG) + "->odata", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    stbi_uc* p_target;
    stbi_uc* p_source;

    rgbImageLock();

    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            p_target = odata + (channels * ((y - y1) * dx + (x - x1)));
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_target[_channels] = p_source[_channels];
        }
    }

    #ifdef STBI_ONLY_JPEG
        stbi_write_jpg(_template1.c_str(), dx, dy, channels, odata, 100);
    #else
        stbi_write_bmp(_template1.c_str(), dx, dy, channels, odata);
    #endif

    rgbImageRelease();

    free_psram_heap(std::string(TAG) + "->odata", odata);
}


void IRAM_ATTR CAlignAndCutImage::cutAndSaveImage(int x1, int y1, int dx, int dy, CImageBasis *_target)
{
    int x2, y2;

    x2 = x1 + dx;
    y2 = y1 + dy;
    x2 = std::min(x2, width - 1);
    y2 = std::min(y2, height - 1);

    dx = x2 - x1;
    dy = y2 - y1;

    if ((_target->height != dy) || (_target->width != dx) || (_target->channels != channels)) {
        ESP_LOGD(TAG, "CAlignAndCutImage::CutAndSave - Image size does not match");
        return;
    }

    uint8_t* odata = _target->rgbImageLock();
    rgbImageLock();

    stbi_uc* p_target;
    stbi_uc* p_source;

    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            p_target = odata + (channels * ((y - y1) * dx + (x - x1)));
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_target[_channels] = p_source[_channels];
        }
    }

    rgbImageRelease();
    _target->rgbImageRelease();
}


CImageBasis* IRAM_ATTR CAlignAndCutImage::cutAndSaveImage(int x1, int y1, int dx, int dy)
{
    int x2, y2;

    x2 = x1 + dx;
    y2 = y1 + dy;
    x2 = std::min(x2, width - 1);
    y2 = std::min(y2, height - 1);

    dx = x2 - x1;
    dy = y2 - y1;

    int memsize = dx * dy * channels;
    uint8_t* odata = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->odata", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    stbi_uc* p_target;
    stbi_uc* p_source;

    rgbImageLock();

    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            p_target = odata + (channels * ((y - y1) * dx + (x - x1)));
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_target[_channels] = p_source[_channels];
        }
    }

    CImageBasis* rs = new CImageBasis("CutAndSave", odata, channels, dx, dy, bpp);
    rgbImageRelease();
    rs->setIndepended();
    return rs;
}


CAlignAndCutImage::~CAlignAndCutImage()
{
    //Nothing to do
}
