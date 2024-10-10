#include "CImageBasis.h"

#include <cstring>
#include <stdint.h>
#include <math.h>
#include <algorithm>

#include <esp_log.h>
#include "esp_heap_caps.h"
#include "esp_system.h"

#include "helper.h"
#include "psram.h"
#include "ClassLogFile.h"
#include "server_ota.h"


static const char *TAG = "IMG_BASIS";

bool jpgFileTooLarge = false;   // JPG creation verfication


CImageBasis::~CImageBasis()
{
    rgbImageLock();

    if (!externalImage) {
        //stbi_image_free(rgb_image);
        free_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ", " + std::to_string(memsize) + ")", rgb_image);
    }

    rgbImageRelease();
}


CImageBasis::CImageBasis(std::string _name)
{
    name = _name;
    externalImage = false;
    rgb_image = NULL;
    width = 0;
    height = 0;
    channels = 0;
    islocked = false;
}


CImageBasis::CImageBasis(std::string _name, CImageBasis *_copyfrom)
{
    name = _name;
    islocked = false;
    externalImage = false;
    channels = _copyfrom->channels;
    width = _copyfrom->width;
    height = _copyfrom->height;
    bpp = _copyfrom->bpp;

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_copyfrom - Start");
    #endif

    memsize = width * height * channels;

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-Copyfrom: Can't allocate enough memory: " + std::to_string(memsize));
        LogFile.writeHeapInfo("CImageBasis-Copyfrom");
        rgbImageRelease();
        return;
    }

    memCopy(_copyfrom->rgb_image, rgb_image, memsize);
    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_copyfrom - done");
    #endif
}


CImageBasis::CImageBasis(std::string _name, CImageBasis *_copyfrom, int add)
{
    name = _name;
    islocked = false;
    externalImage = false;
    channels = _copyfrom->channels;
    width = _copyfrom->width;
    height = _copyfrom->height;
    bpp = _copyfrom->bpp;

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_copyfrom - Start");
    #endif

    memsize = (width * height * channels) + add;

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-Copyfrom: Can't allocate enough memory: " + std::to_string(memsize));
        LogFile.writeHeapInfo("CImageBasis-Copyfrom");
        rgbImageRelease();
        return;
    }

    memCopy(_copyfrom->rgb_image, rgb_image, memsize);
    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_copyfrom - done");
    #endif
}


CImageBasis::CImageBasis(std::string _name, int _width, int _height, int _channels)
{
    name = _name;
    islocked = false;
    externalImage = false;
    channels = _channels;
    width = _width;
    height = _height;
    bpp = _channels;

    rgbImageLock();

     #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_width,height,ch - Start");
    #endif

    memsize = width * height * channels;

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-width,height,ch: Can't allocate enough memory: " + std::to_string(memsize));
        LogFile.writeHeapInfo("CImageBasis-width,height,ch");
        rgbImageRelease();
        return;
    }

    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_width,height,ch - done");
    #endif
}


CImageBasis::CImageBasis(std::string _name, std::string _image)
{
    name = _name;
    islocked = false;
    channels = 3;
    externalImage = false;
    filename = _image;

    if (getFileSize(_image.c_str()) == 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, _image + " is empty");
        return;
    }

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_image - Start");
    #endif

    rgb_image = stbi_load(_image.c_str(), &width, &height, &bpp, channels);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-image: Failed to load " + _image);
        LogFile.writeHeapInfo("CImageBasis-image");
        rgbImageRelease();
        return;
    }

    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        std::string zw = "CImageBasis after load " + _image;
        ESP_LOGD(TAG, "%s", zw.c_str());
        ESP_LOGD(TAG, "w %d, h %d, b %d, c %d", width, height, bpp, channels);
    #endif

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_image - done");
    #endif
}


CImageBasis::CImageBasis(std::string _name, std::string _image, bool _externalImage)
{
    name = _name;
    islocked = false;
    channels = 3;
    externalImage = _externalImage;
    filename = _image;

    if (getFileSize(_image.c_str()) == 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, _image + " is empty");
        return;
    }

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_image - Start");
    #endif

    rgb_image = stbi_load(_image.c_str(), &width, &height, &bpp, channels);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-image: Failed to load " + _image);
        LogFile.writeHeapInfo("CImageBasis-image");
        rgbImageRelease();
        return;
    }

    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        std::string zw = "CImageBasis after load " + _image;
        ESP_LOGD(TAG, "%s", zw.c_str());
        ESP_LOGD(TAG, "w %d, h %d, b %d, c %d", width, height, bpp, channels);
    #endif

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis_image - done");
    #endif
}


CImageBasis::CImageBasis(std::string _name, uint8_t* _rgb_image, int _channels, int _width, int _height, int _bpp)
{
    name = _name;
    islocked = false;
    rgb_image = _rgb_image;
    channels = _channels;
    width = _width;
    height = _height;
    bpp = _bpp;
    externalImage = true;
}


uint8_t * CImageBasis::rgbImageLock(int _waitmaxsec)
{
    if (islocked) {
        #ifdef DEBUG_DETAIL_ON
                ESP_LOGD(TAG, "Image is locked: sleep for: %ds", _waitmaxsec);
        #endif
        TickType_t xDelay;
        xDelay = 1000 / portTICK_PERIOD_MS;
        for (int i = 0; i <= _waitmaxsec; ++i) {
            vTaskDelay( xDelay );
            if (!islocked) {
                break;
            }
        }
    }

    if (islocked) {
        return NULL;
    }

    return rgb_image;
}


void CImageBasis::rgbImageRelease()
{
    islocked = false;
}


bool CImageBasis::imageOkay()
{
    return rgb_image != NULL;
}


uint8_t * CImageBasis::getRgbImage()
{
    return rgb_image;
}


bool CImageBasis::createEmptyImage(int _width, int _height, int _channels)
{
    bpp = _channels;
    width = _width;
    height = _height;
    channels = _channels;

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("createEmptyImage");
    #endif

    memsize = width * height * channels;

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "createEmptyImage: Can't allocate enough memory: " + std::to_string(memsize));
        LogFile.writeHeapInfo("createEmptyImage");
        rgbImageRelease();
        return false;
    }

    stbi_uc* p_source;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_source[_channels] = (uint8_t) 0;
        }
    }

    rgbImageRelease();

    return true;
}


bool CImageBasis::createEmptyImage(int _width, int _height, int _channels, int add)
{
    bpp = _channels;
    width = _width;
    height = _height;
    channels = _channels;

    rgbImageLock();

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("createEmptyImage");
    #endif

    memsize = (width * height * channels) + add;

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "createEmptyImage: Can't allocate enough memory: " + std::to_string(memsize));
        LogFile.writeHeapInfo("createEmptyImage");
        rgbImageRelease();
        return false;
    }

    stbi_uc* p_source;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_source[_channels] = (uint8_t) 0;
        }
    }

    rgbImageRelease();

    return true;
}


void CImageBasis::emptyImage()
{
    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("emptyImage");
    #endif

    stbi_uc* p_source;

    rgbImageLock();

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_source[_channels] = (uint8_t) 0;
        }
    }

    rgbImageRelease();
}


bool CImageBasis::loadFromMemory(stbi_uc *_buffer, int len)
{
    rgbImageLock();

    if (rgb_image != NULL ) {
        //stbi_image_free(rgb_image);
        free_psram_heap(std::string(TAG) + "->rgb_image (loadFromMemory)", rgb_image);
    }

    rgb_image = stbi_load_from_memory(_buffer, len, &width, &height, &channels, 3);
    bpp = channels;
    ESP_LOGD(TAG, "Image loaded from memory: %d, %d, %d", width, height, channels);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFromMemory: Image loading failed");
        LogFile.writeHeapInfo("loadFromMemory");
        return false;
    }

    rgbImageRelease();
    return true;
}


bool CImageBasis::loadFromMemoryPreallocated(stbi_uc *_buffer, int len)
{
    rgbImageLock();

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "No preallocation found");
        return false;
    }

    rgb_image = stbi_load_from_memory(_buffer, len, &width, &height, &channels, 3);
    bpp = channels;
    ESP_LOGD(TAG, "Image loaded from memory: %d, %d, %d", width, height, channels);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadFromMemoryPreallocated: Image loading failed");
        LogFile.writeHeapInfo("loadFromMemoryPreallocated");
        return false;
    }

    rgbImageRelease();
    return true;
}


bool CImageBasis::loadFromFilePreallocated(std::string _name, std::string _image)
{
    name = _name;
    islocked = false;
    channels = 3;
    externalImage = false;
    filename = _image;

    if (getFileSize(_image.c_str()) == 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, filename + " is empty");
        return false;
    }

    rgbImageLock();

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "No preallocation found");
    }

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis-loadFromFilePreallocated - Start");
    #endif

    rgb_image = stbi_load(filename.c_str(), &width, &height, &bpp, channels);

    if (rgb_image == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageBasis-loadFromFilePreallocated: Failed to load " + filename);
        LogFile.writeHeapInfo("CImageBasis-loadFromFilePreallocated");
        rgbImageRelease();
        return false;
    }

    rgbImageRelease();

    #ifdef DEBUG_DETAIL_ON
        std::string zw = "CImageBasis loadFromFilePreallocated after load " + _image;
        ESP_LOGI(TAG, "%s", zw.c_str());
        ESP_LOGI(TAG, "w %d, h %d, b %d, c %d", width, height, bpp, channels);
    #endif

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("CImageBasis-loadFromFilePreallocated - done");
    #endif

    return true;
}


bool CImageBasis::copyFromMemory(uint8_t* _source, int _size)
{
    int gr = height * width * channels;
    if (gr != _size) { // Size does not fit
        ESP_LOGE(TAG, "Cannot copy image from memory - sizes do not match: should be %d, but is %d", _size, gr);
        return false;
    }

    rgbImageLock();
    memCopy(_source, rgb_image, _size);
    rgbImageRelease();

    return true;
}


void CImageBasis::memCopy(uint8_t* _source, uint8_t* _target, int _size)
{
#ifdef _ESP32_PSRAM
    for (int i = 0; i < _size; ++i)
        *(_target + i) = *(_source + i);
#else
    memcpy(_target, _source, _size);
#endif
}


void CImageBasis::createNegativeImage(void)
{
    rgbImageLock();

    for (int i = 0; i < width * height * channels; i += channels) {
        for (int c = 0; c < channels; c++) {
            rgb_image[i+c] = 255 - rgb_image[i+c];
        }
    }

    rgbImageRelease();
}


void CImageBasis::setContrast(float _contrast)  //input range [-100..100]
{
    stbi_uc* p_source;

    float contrast = (_contrast/100) + 1;  //convert to decimal & shift range: [0..2]
    float intercept = 128 * (1 - contrast);

    rgbImageLock();

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            p_source = rgb_image + (channels * (y * width + x));
            for (int _channels = 0; _channels < channels; ++_channels)
                p_source[_channels] = (uint8_t) std::min(255, std::max(0, (int) (p_source[_channels] * contrast + intercept)));
        }
    }

    rgbImageRelease();
}


void CImageBasis::resizeImage(int _new_dx, int _new_dy)
{
    memsize = _new_dx * _new_dy * channels;
    uint8_t* odata = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->odata", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    rgbImageLock();

    stbir_resize_uint8(rgb_image, width, height, 0, odata, _new_dx, _new_dy, 0, channels);

    rgb_image = (unsigned char*)malloc_psram_heap(std::string(TAG) + "->CImageBasis resizeImage (" + name + ")", memsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    memCopy(odata, rgb_image, memsize);
    width = _new_dx;
    height = _new_dy;

    free_psram_heap(std::string(TAG) + "->odata", odata);

    rgbImageRelease();
}


void CImageBasis::resizeImage(int _new_dx, int _new_dy, CImageBasis *_target)
{
    if ((_target->height != _new_dy) || (_target->width != _new_dx) || (_target->channels != channels)) {
        ESP_LOGE(TAG, "resizeImage - Target image size does not fit");
        return;
    }

    rgbImageLock();

    uint8_t* odata = _target->rgb_image;
    stbir_resize_uint8(rgb_image, width, height, 0, odata, _new_dx, _new_dy, 0, channels);

    rgbImageRelease();
}


void CImageBasis::saveToFile(std::string _imageout)
{
    std::string typ = getFileType(_imageout);

    rgbImageLock();

    if ((typ == "jpg") || (typ == "JPG")) {       // CAUTION PROBLEMATIC IN ESP32
        stbi_write_jpg(_imageout.c_str(), width, height, channels, rgb_image, 0);
    }

    #ifndef STBI_ONLY_JPEG
    if ((typ == "bmp") || (typ == "BMP")) {
        stbi_write_bmp(_imageout.c_str(), width, height, channels, rgb_image);
    }
    #endif
    rgbImageRelease();
}


void writeJPGHelper(void *context, void *data, int size)
{
    // ESP_LOGD(TAG, "Size all: %d, size %d", ((ImageData*)context)->size, size);
    ImageData* _zw = (ImageData*) context;
    uint8_t *voidstart = _zw->data;
    uint8_t *datastart = (uint8_t*) data;

    if ((_zw->size < MAX_JPG_SIZE)) {   // Abort copy to prevent buffer overflow
        voidstart += _zw->size;

        for (int i = 0; i < size; ++i) {
            *(voidstart + i) = *(datastart + i);
        }

        _zw->size += size;
    }
    else {
        jpgFileTooLarge = true;
    }
}


ImageData* CImageBasis::writeToMemoryAsJPG(const int quality)
{
    ImageData* ii = new ImageData;

    rgbImageLock();
    stbi_write_jpg_to_func(writeJPGHelper, ii, width, height, channels, rgb_image, quality);
    rgbImageRelease();

    if (jpgFileTooLarge) {
        jpgFileTooLarge = false;
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "writeToMemoryAsJPG: Creation aborted! JPG size > preallocated buffer: " + std::to_string(MAX_JPG_SIZE));
    }
    return ii;
}


void CImageBasis::writeToMemoryAsJPG(ImageData* i, const int quality)
{
    ImageData* ii = new ImageData;

    rgbImageLock();
    stbi_write_jpg_to_func(writeJPGHelper, ii, width, height, channels, rgb_image, quality);
    rgbImageRelease();

    if (jpgFileTooLarge) {
        jpgFileTooLarge = false;
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "writeToMemoryAsJPG: Creation aborted! JPG size > preallocated buffer: " + std::to_string(MAX_JPG_SIZE));
    }
    memCopy((uint8_t*) ii, (uint8_t*) i, sizeof(ImageData));
    delete ii;
}


struct SendJPGHTTP
{
    httpd_req_t *req;
    esp_err_t res;
    char buf[HTTP_BUFFER_SENT];
    int size = 0;
};


inline void writeJPGToHttpHelper(void *context, void *data, int size)
{
    SendJPGHTTP* _send = (SendJPGHTTP*) context;
    if ((_send->size + size) >= HTTP_BUFFER_SENT) { // data no longer fits in buffer
        if (httpd_resp_send_chunk(_send->req, _send->buf, _send->size) != ESP_OK) {
            ESP_LOGE(TAG, "File sending failed");
            _send->res = ESP_FAIL;
        }
        _send->size = 0;
    }
    std::memcpy((void*) (&(_send->buf[0]) + _send->size), data, size);
    _send->size+= size;
}


esp_err_t CImageBasis::sendJPGtoHTTP(httpd_req_t *_req, const int quality)
{
    SendJPGHTTP ii;
    ii.req = _req;
    ii.res = ESP_OK;
    ii.size = 0;

    rgbImageLock();
    stbi_write_jpg_to_func(writeJPGToHttpHelper, &ii, width, height, channels, rgb_image, quality);

    if (ii.size > 0) {
        if (httpd_resp_send_chunk(_req, (char*) ii.buf, ii.size) != ESP_OK) { // still send the rest
            ESP_LOGE(TAG, "File sending failed");
            ii.res = ESP_FAIL;
        }
    }

    rgbImageRelease();

    return ii.res;
}


uint8_t CImageBasis::getPixelColor(int x, int y, int ch)
{
    stbi_uc* p_source;
    p_source = rgb_image + (channels * (y * width + x));
    return p_source[ch];
}


void CImageBasis::setPixelColor(int x, int y, int r, int g, int b)
{
    stbi_uc* p_source;

    rgbImageLock();
    p_source = rgb_image + (channels * (y * width + x));
    p_source[0] = r;
    if ( channels > 2) {
        p_source[1] = g;
        p_source[2] = b;
    }

    rgbImageRelease();
}


bool CImageBasis::isInImage(int x, int y)
{
    if ((x < 0) || (x > width - 1)) {
        return false;
    }

    if ((y < 0) || (y > height- 1)) {
        return false;
    }

    return true;
}


void CImageBasis::drawRect(int x, int y, int dx, int dy, int r, int g, int b, int thickness)
{
    int zwx1, zwx2, zwy1, zwy2;
    int _x, _y, _thick;

    zwx1 = x - thickness + 1;
    zwx2 = x + dx + thickness - 1;
    zwy1 = y;
    zwy2 = y;

    rgbImageLock();

    for (_thick = 0; _thick < thickness; _thick++) {
        for (_x = zwx1; _x <= zwx2; ++_x) {
            for (_y = zwy1; _y <= zwy2; _y++) {
                if (isInImage(_x, _y)) {
                    setPixelColor(_x, _y - _thick, r, g, b);
                }
            }
        }
    }

    zwx1 = x - thickness + 1;
    zwx2 = x + dx + thickness - 1;
    zwy1 = y + dy;
    zwy2 = y + dy;
    for (_thick = 0; _thick < thickness; _thick++)
        for (_x = zwx1; _x <= zwx2; ++_x)
            for (_y = zwy1; _y <= zwy2; _y++)
                if (isInImage(_x, _y))
                    setPixelColor(_x, _y + _thick, r, g, b);

    zwx1 = x;
    zwx2 = x;
    zwy1 = y;
    zwy2 = y + dy;
    for (_thick = 0; _thick < thickness; _thick++) {
        for (_x = zwx1; _x <= zwx2; ++_x) {
            for (_y = zwy1; _y <= zwy2; _y++) {
                if (isInImage(_x, _y)) {
                    setPixelColor(_x - _thick, _y, r, g, b);
                }
            }
        }
    }

    zwx1 = x + dx;
    zwx2 = x + dx;
    zwy1 = y;
    zwy2 = y + dy;
    for (_thick = 0; _thick < thickness; _thick++) {
        for (_x = zwx1; _x <= zwx2; ++_x) {
            for (_y = zwy1; _y <= zwy2; _y++) {
                if (isInImage(_x, _y)) {
                    setPixelColor(_x + _thick, _y, r, g, b);
                }
            }
        }
    }

    rgbImageRelease();
}


void CImageBasis::drawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int thickness)
{
    int _x, _y, _thick;
    int _zwy1, _zwy2;
    thickness = (thickness-1) / 2;

    rgbImageLock();

    for (_thick = 0; _thick <= thickness; ++_thick) {
        for (_x = x1 - _thick; _x <= x2 + _thick; ++_x) {
            if (x2 == x1) {
                _zwy1 = y1;
                _zwy2 = y2;
            }
            else {
                _zwy1 = (y2 - y1) * (float)(_x - x1) / (float)(x2 - x1) + y1;
                _zwy2 = (y2 - y1) * (float)(_x + 1 - x1) / (float)(x2 - x1) + y1;
            }

            for (_y = _zwy1 - _thick; _y <= _zwy2 + _thick; _y++) {
                if (isInImage(_x, _y)) {
                    setPixelColor(_x, _y, r, g, b);
                }
            }
        }
    }

    rgbImageRelease();
}


void CImageBasis::drawEllipse(int x1, int y1, int radx, int rady, int r, int g, int b, int thickness)
{
    float deltarad, aktrad;
    int _thick, _x, _y;
    int rad = radx;

    if (rady > radx)
        rad = rady;

    deltarad = 1 / (4 * M_PI * (rad + thickness - 1));

    rgbImageLock();

    for (aktrad = 0; aktrad <= (2 * M_PI); aktrad += deltarad) {
        for (_thick = 0; _thick < thickness; ++_thick) {
            _x = sin(aktrad) * (radx + _thick) + x1;
            _y = cos(aktrad) * (rady + _thick) + y1;
            if (isInImage(_x, _y)) {
                setPixelColor(_x, _y, r, g, b);
            }
        }
    }

    rgbImageRelease();
}


void CImageBasis::drawCircle(int x1, int y1, int rad, int r, int g, int b, int thickness)
{
    float deltarad, aktrad;
    int _thick, _x, _y;

    deltarad = 1 / (4 * M_PI * (rad + thickness - 1));

    rgbImageLock();

    for (aktrad = 0; aktrad <= (2 * M_PI); aktrad += deltarad) {
        for (_thick = 0; _thick < thickness; ++_thick) {
            _x = sin(aktrad) * (rad + _thick) + x1;
            _y = cos(aktrad) * (rad + _thick) + y1;
            if (isInImage(_x, _y)) {
                setPixelColor(_x, _y, r, g, b);
            }
        }
    }

    rgbImageRelease();
}
