

#include "CImage.h"

#include "esp_heap_caps.h"
#include "esp_system.h"

#include "webserver.h"
#include "helper.h"
#include "psram.h"
#include "ClassLogFile.h"


static const char *TAG = "IMG";


CImage::CImage()
    : name("default"), width(0), height(0), channels(0), imgDataSize(0), imgData(nullptr), allocatedSize(0), externalMemory(false)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImage: Failed to create semaphore");
        return;
    }
}


CImage::CImage(std::string objName, int width, int height, int channels, bool stbLibMemoryMod, const uint8_t *data)
    : name(objName), width(width), height(height), channels(channels), imgDataSize(0), allocatedSize(0), externalMemory(false)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImage: Failed to create semaphore");
        return;
    }

    imgDataSize = width * height * channels;
    allocatedSize = imgDataSize;

    // Special case: Prepare for memory reuse for STB library which allocates one more byte than required by image size
    // Note: STB allocation strategy is defined by struct strSTBI (psram.h).
    //       The memory block which shall be used needs to configured in calling function right before usage of this function
    if (stbLibMemoryMod) {
        allocatedSize += 1;
    }

    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImage (" + name + ")", allocatedSize,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory: " + std::to_string(allocatedSize));
        LogFile.writeHeapInfo("CImage-width,height");
        return;
    }

    if (data) {
        memcpy(imgData, data, imgDataSize);
    }
    else {
        memset(imgData, IMAGE_COLOR_DEFAULT, imgDataSize);
    }
}


CImage::CImage(std::string objName, const std::string &filename, bool customStbLibMemAllocation, bool grayscale)
    : name(objName), width(0), height(0), channels(0), imgDataSize(0), allocatedSize(0), externalMemory(customStbLibMemAllocation)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImage: Failed to create semaphore");
        return;
    }

    if (getFileSize(filename) == 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImage: Source file has zero size:" + filename);
        return;
    }

    imgData = stbi_load(filename.c_str(), &width, &height, &channels, grayscale ? 1 : 0);
    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to load image: " + filename);
        LogFile.writeHeapInfo("CImage-file");
        return;
    }

    imgDataSize = width * height * channels;

    // Special case: Increase memory size by 1 byte (to follow STBI allocation strategy)
    allocatedSize = imgDataSize + 1;
}


// Copy constructor
CImage::CImage(const CImage &other)
    : imageMutex(xSemaphoreCreateRecursiveMutex()), name(other.name + "-copy"), width(other.width), height(other.height),
      channels(other.channels), imgDataSize(other.imgDataSize), imgData(nullptr), allocatedSize(other.imgDataSize), externalMemory(false)
{
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to create semaphore");
        allocatedSize = 0;
        return;
    }

    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to lock source");
        allocatedSize = 0;
        return;
    }

    if (allocatedSize > 0) {
        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->Copy (" + other.name + ")", allocatedSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to allocate memory: " + std::to_string(allocatedSize));
            LogFile.writeHeapInfo("CImage-copy");
            allocatedSize = 0;
            return;
        }

        if (other.imgData) {
            memcpy(imgData, other.imgData, imgDataSize);
        }
        else {
            memset(imgData, IMAGE_COLOR_DEFAULT, imgDataSize);
        }
    }
}


// Copy assignment
CImage &CImage::operator=(const CImage &other)
{
    if (this == &other) {
        return *this;
    }

    CImageLockGuard thisLock(*this);
    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy-assign: Failed to lock source");
        return *this;
    }

    // Allocate only if current buffer is too small
    if (allocatedSize < other.imgDataSize) {
        uint8_t *newImgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->Copy-assign (" + other.name + ")", other.imgDataSize,
                                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!newImgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy-assign: Allocation failed. Keeping original.");
            LogFile.writeHeapInfo("CImage-copy-assign");
            return *this;
        }

        freeImageData();
        imgData = newImgData;
        allocatedSize = other.imgDataSize;
    }

    name = other.name + "-copy";
    width = other.width;
    height = other.height;
    channels = other.channels;
    imgDataSize = other.imgDataSize;
    externalMemory = false;

    if (other.imgData && imgData) {
        memcpy(imgData, other.imgData, other.imgDataSize);
    }
    else if (imgData) {
        memset(imgData, IMAGE_COLOR_DEFAULT, imgDataSize);
    }

    return *this;
}


// Move constructor
CImage::CImage(CImage &&other) noexcept
    : imageMutex(other.imageMutex), name(std::move(other.name)), width(other.width), height(other.height), channels(other.channels),
      imgDataSize(other.imgDataSize), imgData(other.imgData), allocatedSize(other.allocatedSize), externalMemory(other.externalMemory)
{
    other.imageMutex = nullptr;
    other.imgData = nullptr;
    other.width = 0;
    other.height = 0;
    other.channels = 0;
    other.imgDataSize = 0;
    other.allocatedSize = 0;
    other.externalMemory = false;
}


// Move assignment
CImage &CImage::operator=(CImage &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    SemaphoreHandle_t mutexToDelete = nullptr;

    // Scope - limited locking to safely clear existing data
    {
        CImageLockGuard thisLock(*this);
        CImageLockGuard otherLock(other);
        if (!thisLock.isLocked() || !otherLock.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Move-assign: Failed to lock source");
            return *this;
        }

        if (!externalMemory) {
            freeImageData();
        }

        mutexToDelete = imageMutex;
        imageMutex = other.imageMutex;

        name = std::move(other.name);
        width = other.width;
        height = other.height;
        channels = other.channels;
        imgDataSize = other.imgDataSize;
        imgData = other.imgData;
        allocatedSize = other.allocatedSize;
        externalMemory = other.externalMemory;

        other.imageMutex = nullptr;
        other.imgData = nullptr;
        other.width = 0;
        other.height = 0;
        other.channels = 0;
        other.imgDataSize = 0;
        other.allocatedSize = 0;
    }

    if (mutexToDelete) {
        vSemaphoreDelete(mutexToDelete);
    }

    return *this;
}


esp_err_t CImage::loadJpgFromFile(const std::string &filename, bool overwriteSource, bool grayscale)
{
    if (filename.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: Filename is empty");
        return ESP_FAIL;
    }

    if (overwriteSource && !isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: No allocated memory");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Special case: Prepare for memory reuse for STB library
    if (overwriteSource) {
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.name = name;
        STBIObjectPSRAM.PreallocatedMemory = imgData;
        STBIObjectPSRAM.PreallocatedMemorySize = allocatedSize;
    }
    else {
        STBIObjectPSRAM.usePreallocated = false;
        freeImageData();
    }

    int newWidth, newHeight, newChannels;
    imgData = stbi_load(filename.c_str(), &newWidth, &newHeight, &newChannels, grayscale ? 1 : 0);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: Failed to load image: " + filename);
        return ESP_FAIL;
    }

    int newImgDataSize = newWidth * newHeight * newChannels;
    imgDataSize = newImgDataSize;
    width = newWidth;
    height = newHeight;
    channels = newChannels;

    // Special case: Increase memory size by 1 byte (to follow STBI allocation strategy)
    newImgDataSize++;

    if (overwriteSource) {
        if (newImgDataSize > allocatedSize) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: Buffer overflow");
            return ESP_FAIL;
        }
    }
    else {
        allocatedSize = newImgDataSize;
    }

    return ESP_OK;
}


esp_err_t CImage::loadJpgFromMemory(void *buffer, int size, bool overwriteSource, bool grayscale)
{
    if (size <= 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Invalid buffer size");
        return ESP_FAIL;
    }

    if (overwriteSource && !isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: No allocated memory");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Special case: Prepare for memory reuse for STB library
    if (overwriteSource) {
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.name = name;
        STBIObjectPSRAM.PreallocatedMemory = imgData;
        STBIObjectPSRAM.PreallocatedMemorySize = allocatedSize;
    }
    else {
        STBIObjectPSRAM.usePreallocated = false;
        freeImageData();
    }

    int newWidth, newHeight, newChannels;
    imgData = stbi_load_from_memory((stbi_uc *)buffer, size, &newWidth, &newHeight, &newChannels, grayscale ? 1 : 0);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Image loading failed");
        LogFile.writeHeapInfo("loadJpgFromMemory");
        return ESP_FAIL;
    }

    int newImgDataSize = newWidth * newHeight * newChannels;
    imgDataSize = newImgDataSize;
    width = newWidth;
    height = newHeight;
    channels = newChannels;

    // Special case: Increase memory size by 1 byte (to follow STBI allocation strategy)
    newImgDataSize++;

    if (overwriteSource) {
        if (newImgDataSize > allocatedSize) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromFile: Buffer overflow");
            return ESP_FAIL;
        }
    }
    else {
        allocatedSize = newImgDataSize;
    }

    return ESP_OK;
}


esp_err_t CImage::saveJpgToFile(const std::string &filename, const int quality)
{
    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: No image data to save");
        return ESP_FAIL;
    }

    std::string fileType = toLower(getFileType(filename));
    if (fileType == "jpg" || fileType == "jpeg") {
        CImageLockGuard lockGuard(*this);
        if (!lockGuard.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }

        if (!stbi_write_jpg(filename.c_str(), width, height, channels, imgData, quality)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Failed to write file");
            return ESP_FAIL;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: File type not supported (only jpg, jpeg)");
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}


esp_err_t CImage::saveJpgToBuffer(uint8_t *jpgBuffer, const int size, const int quality)
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToBuffer: No valid image data");
        return ESP_FAIL;
    }

    if (!jpgBuffer || size <= 0 || (size > 0 && size < imgDataSize)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToBuffer: Buffer invalid or too small");
        return ESP_ERR_NO_MEM;
    }

    struct JpgWriteContext {
        uint8_t *buffer;
        int actBufferSize;
        bool bufferOverflow;
    } jpgWriteCtx = {jpgBuffer, 0, false};

    auto writeJPGHelper = [](void *context, void *data, int dataSize) {
        JpgWriteContext *ctx = (JpgWriteContext *)context;

        if (!ctx || !ctx->buffer || (ctx->actBufferSize + dataSize > IMAGE_JPG_MAX_SIZE)) {
            ctx->bufferOverflow = true;
            return;
        }

        memcpy(ctx->buffer + ctx->actBufferSize, data, dataSize);
        ctx->actBufferSize += dataSize;
    };


    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToBuffer: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    if (!stbi_write_jpg_to_func(writeJPGHelper, &jpgWriteCtx, width, height, channels, imgData, quality)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToBuffer: JPG encoding failed");

        return ESP_FAIL;
    }

    if (jpgWriteCtx.bufferOverflow) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToBuffer: Invalid buffer or buffer overflow");
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t CImage::saveJpgToContainer(CImageJpg *jpgContainer, const int quality)
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: No valid image data");
        return ESP_FAIL;
    }

    if (!jpgContainer || !jpgContainer->isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: Invalid JPG container");
        return ESP_FAIL;
    }

    CImageLockGuard jpgLock(*jpgContainer);
    if (!jpgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: Could not acquire lock (jpg)");
        return ESP_ERR_TIMEOUT;
    }

    CImageLockGuard imgLock(*this);
    if (!imgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    struct JpgWriteContext {
        uint8_t *buffer;
        int actBufferSize;
        bool bufferOverflow;
    } jpgWriteCtx = {jpgContainer->getImgData(), 0, false};

    auto writeJPGHelper = [](void *context, void *data, int dataSize) {
        JpgWriteContext *ctx = (JpgWriteContext *)context;

        if (!ctx || !ctx->buffer || (ctx->actBufferSize + dataSize) > IMAGE_JPG_MAX_SIZE) {
            ctx->bufferOverflow = true;
            return;
        }

        memcpy(ctx->buffer + ctx->actBufferSize, data, dataSize);
        ctx->actBufferSize += dataSize;
    };

    if (!stbi_write_jpg_to_func(writeJPGHelper, &jpgWriteCtx, width, height, channels, imgData, quality)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: JPG encoding failed");
        return ESP_FAIL;
    }

    if (jpgWriteCtx.bufferOverflow) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: Buffer overflow, target buffer insufficient");
        return ESP_FAIL;
    }

    jpgContainer->setImgDataSize(jpgWriteCtx.actBufferSize);

    return ESP_OK;
}


esp_err_t CImage::sendJpgToHttp(httpd_req_t *req, const int quality)
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendJpgToHttp: No valid image data");
        return ESP_FAIL;
    }

    struct SendJpgHttp {
        httpd_req_t *req;
        esp_err_t retVal;
        char *buffer;
        size_t actBufferSize;
    } sendJpgCtx = {req, ESP_OK, (char *)((struct HttpServerData *)req->user_ctx)->scratch, 0};

    auto sendJPGToHttpHelper = [](void *context, void *data, int dataSize) {
        auto *sendJpgHttp = (SendJpgHttp *)context;
        if ((sendJpgHttp->actBufferSize + dataSize) >= WEBSERVER_SCRATCH_BUFSIZE) { // Buffer full, send chunk
            if (httpd_resp_send_chunk(sendJpgHttp->req, (const char *)sendJpgHttp->buffer, sendJpgHttp->actBufferSize) != ESP_OK) {
                sendJpgHttp->retVal = ESP_FAIL;
                return;
            }
            sendJpgHttp->actBufferSize = 0;
        }
        memcpy(sendJpgHttp->buffer + sendJpgHttp->actBufferSize, data, dataSize);
        sendJpgHttp->actBufferSize += dataSize;
    };

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToContainer: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    httpd_resp_set_hdr(req, "Cache-Control", "max-age=0");
    httpd_resp_set_type(req, "image/jpeg");

    if (!stbi_write_jpg_to_func(sendJPGToHttpHelper, &sendJpgCtx, width, height, channels, imgData, quality)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendJPGtoHTTP: Failed to encode and send JPG");
        return ESP_FAIL;
    }

    if (sendJpgCtx.actBufferSize > 0) {
        if (httpd_resp_send_chunk(req, (const char *)sendJpgCtx.buffer, sendJpgCtx.actBufferSize) != ESP_OK) { // still send the rest
            return ESP_FAIL;
        }
    }

    if (sendJpgCtx.retVal == ESP_OK) {
        httpd_resp_send_chunk(req, nullptr, 0);
    }

    return sendJpgCtx.retVal;
}


bool CImage::lock() const
{
    if (imageMutex && xSemaphoreTakeRecursive(imageMutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
        return true;
    }
    return false;
}


void CImage::unlock() const
{
    if (imageMutex) {
        xSemaphoreGiveRecursive(imageMutex);
    }
}


bool CImage::isValid() const
{
    return (imgData != nullptr && allocatedSize > 0 && imgDataSize > 0 && width > 0 && height > 0 && channels > 0);
}


uint8_t *CImage::getImgData()
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getImgData: No image data");
        return nullptr;
    }

    return imgData;
}


const uint8_t *CImage::getImgData() const
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getImgData (const): No image data");
        return nullptr;
    }

    return imgData;
}


bool CImage::getIsInbound(int x, int y)
{
    if ((x < 0) || (x > width - 1)) {
        return false;
    }

    if ((y < 0) || (y > height - 1)) {
        return false;
    }

    return true;
}


uint8_t CImage::getPixelColor(int x, int y, int ch)
{
    return imgData[((y * width + x) * channels) + ch];
}


void CImage::setPixelColor(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (!getIsInbound(x, y)) {
        return;
    }

    uint8_t *p_source = imgData + ((y * width + x) * channels);

    p_source[0] = r;
    if (channels > 2) {
        p_source[1] = g;
        p_source[2] = b;
    }
}


void CImage::freeImageData()
{
    if (imgData) {
        free_psram_heap(std::string(TAG) + "->CImage (" + name + ", " + std::to_string(imgDataSize) + ")", imgData);
        imgData = nullptr;
    }
}


CImage::~CImage()
{
    if (!externalMemory) {
        freeImageData();
    }

    if (imageMutex) {
        vSemaphoreDelete(imageMutex);
    }
}
