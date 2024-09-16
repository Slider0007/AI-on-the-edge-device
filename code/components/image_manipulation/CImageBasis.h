#ifndef CIMAGEBASIS_H
#define CIMAGEBASIS_H

#include "../../include/defines.h"

#include <string>

#include <esp_http_server.h>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"


struct ImageData {
    uint8_t data[MAX_JPG_SIZE];
    size_t size = 0;
};


class CImageBasis
{
  protected:
    bool externalImage;
    std::string filename;
    std::string name; // Just used for diagnostics
    int memsize = 0;
    bool islocked;

    void memCopy(uint8_t *_source, uint8_t *_target, int _size);
    bool isInImage(int x, int y);

  public:
    uint8_t *rgb_image = NULL;
    int channels;
    int width, height, bpp;

    CImageBasis(std::string name);
    CImageBasis(std::string name, std::string _image);
    CImageBasis(std::string name, std::string _image, bool _externalImage);
    CImageBasis(std::string name, uint8_t *_rgb_image, int _channels, int _width, int _height, int _bpp);
    CImageBasis(std::string name, int _width, int _height, int _channels);
    CImageBasis(std::string name, CImageBasis *_copyfrom);
    CImageBasis(std::string _name, CImageBasis *_copyfrom, int add);
    virtual ~CImageBasis();

    uint8_t *rgbImageLock(int _waitmaxsec = 60);
    void rgbImageRelease();

    void setIndepended() { externalImage = false; };
    bool imageOkay();

    uint8_t *getRgbImage();
    int getWidth() { return this->width; };
    int getHeight() { return this->height; };
    int getChannels() { return this->channels; };
    int getMemsize() { return this->memsize; };

    bool createEmptyImage(int _width, int _height, int _channels);
    bool createEmptyImage(int _width, int _height, int _channels, int add);
    void emptyImage();
    bool loadFromMemory(stbi_uc *_buffer, int len);
    bool loadFromMemoryPreallocated(stbi_uc *_buffer, int len);
    bool loadFromFilePreallocated(std::string _name, std::string _image);
    bool copyFromMemory(uint8_t *_source, int _size);

    void createNegativeImage(void);
    void setContrast(float _contrast);

    void resizeImage(int _new_dx, int _new_dy);
    void resizeImage(int _new_dx, int _new_dy, CImageBasis *_target);

    void saveToFile(std::string _imageout);

    ImageData *writeToMemoryAsJPG(const int quality = 90);
    void writeToMemoryAsJPG(ImageData *ii, const int quality = 90);

    esp_err_t sendJPGtoHTTP(httpd_req_t *req, const int quality = 90);

    uint8_t getPixelColor(int x, int y, int ch);
    void setPixelColor(int x, int y, int r, int g, int b);

    void drawRect(int x, int y, int dx, int dy, int r = 255, int g = 255, int b = 255, int thickness = 1);
    void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int thickness = 1);
    void drawCircle(int x1, int y1, int rad, int r, int g, int b, int thickness = 1);
    void drawEllipse(int x1, int y1, int radx, int rady, int r, int g, int b, int thickness = 1);
};


#endif // CIMAGEBASIS_H
