#include "CImageJpg.h"

#include "esp_system.h"

#include "helper.h"
#include "psram.h"
#include "ClassLogFile.h"


static const char *TAG = "IMG_JPG";


CImageJpg::CImageJpg() : name("default"), imgDataSize(0), imgData(nullptr)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageJpg: Failed to create semaphore");
        return;
    }
}


CImageJpg::CImageJpg(std::string objName, int size, const uint8_t *data) : name(objName), imgDataSize(size)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageJpg: Failed to create semaphore");
        return;
    }

    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", imgDataSize,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Can't allocate enough memory: " + std::to_string(imgDataSize));
        LogFile.writeHeapInfo("CImageJpg");
        return;
    }

    if (data) {
        memcpy(imgData, data, imgDataSize);
    }
    else {
        memset(imgData, 0, imgDataSize);
    }
}


CImageJpg::CImageJpg(std::string objName, const std::string &filename) : name(std::move(objName)), imgDataSize(0), imgData(nullptr)
{
    imageMutex = xSemaphoreCreateRecursiveMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageJpg: Failed to create semaphore");
        return;
    }

    size_t fileSize = getFileSize(filename);
    if (fileSize <= 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File is empty or invalid: " + filename);
        return;
    }

    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to open file: " + filename);
        return;
    }

    freeImageData();
    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory for image data");
        LogFile.writeHeapInfo("CImageJpg-file");
        fclose(file);
        return;
    }

    size_t bytesRead = fread(imgData, 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to read complete file data");
        freeImageData();
        return;
    }

    imgDataSize = fileSize;
}


// Copy constructor
CImageJpg::CImageJpg(const CImageJpg &other)
    : imageMutex(xSemaphoreCreateRecursiveMutex()), name(other.name + "-copy"), imgDataSize(other.imgDataSize), imgData(nullptr)
{
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to create semaphore");
        return;
    }

    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to lock");
        return;
    }

    if (imgDataSize > 0) {
        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", imgDataSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to allocate memory: " + std::to_string(imgDataSize));
            LogFile.writeHeapInfo("CImageJpg-copy");
            return;
        }

        if (other.imgData) {
            memcpy(imgData, other.imgData, imgDataSize);
        }
        else {
            memset(imgData, 0, imgDataSize);
        }
    }
}


// Copy assignment
CImageJpg &CImageJpg::operator=(const CImageJpg &other)
{
    if (this == &other) {
        return *this;
    }

    const CImageJpg *first = (this < &other) ? this : &other;
    const CImageJpg *second = (this < &other) ? &other : this;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy-assign: Failed to lock");
        return *this;
    }

    // Allocate only if needed
    if (!imgData || imgDataSize < other.imgDataSize) {
        uint8_t *newData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->Copy-assign (" + other.name + ")", other.imgDataSize,
                                                        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!newData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy-assign: Allocation failed");
            LogFile.writeHeapInfo("CImageJpg-copy-assign");
            return *this;
        }

        freeImageData();
        imgData = newData;
    }

    name = other.name;
    imgDataSize = other.imgDataSize;

    if (other.imgData && imgData) {
        memcpy(imgData, other.imgData, other.imgDataSize);
    }
    else if (imgData) {
        memset(imgData, 0, imgDataSize);
    }

    return *this;
}


// Move constructor
CImageJpg::CImageJpg(CImageJpg &&other) noexcept : imageMutex(xSemaphoreCreateRecursiveMutex()), name(), imgDataSize(0), imgData(nullptr)
{
    if (!imageMutex) {
        return;
    }

    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Move: Failed to lock");
        return;
    }

    name = std::move(other.name);
    imgData = other.imgData;
    imgDataSize = other.imgDataSize;

    other.imgData = nullptr;
    other.imgDataSize = 0;
    other.name.clear();
}


// Move assignment
CImageJpg &CImageJpg::operator=(CImageJpg &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    const CImageJpg *first = (this < &other) ? this : &other;
    const CImageJpg *second = (this < &other) ? &other : this;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Move-assign: Failed to lock");
        return *this;
    }

    freeImageData();

    name = std::move(other.name);
    imgData = other.imgData;
    imgDataSize = other.imgDataSize;

    other.name.clear();
    other.imgData = nullptr;
    other.imgDataSize = 0;

    return *this;
}


esp_err_t CImageJpg::updateImageDataFromJpgBuffer(const uint8_t *newData, int newSize, bool updateContainerSize)
{
    if (newSize <= 0 || !newData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Invalid data or size");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Failed to lock");
        return ESP_ERR_TIMEOUT;
    }

    if (updateContainerSize) {
        freeImageData();

        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", newSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Failed to allocate memory for new data");
            LogFile.writeHeapInfo("updateImageDataFromJpgBuffer");
            return ESP_FAIL;
        }
    }
    else {
        if (imgDataSize < newSize) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Container buffer too small");
            return ESP_FAIL;
        }
    }

    memcpy(imgData, newData, newSize);
    imgDataSize = newSize;

    return ESP_OK;
}


esp_err_t CImageJpg::updateImageDataFromJpgFile(const std::string &filename, bool updateContainerSize)
{
    size_t fileSize = getFileSize(filename);
    if (fileSize <= 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: File is empty or invalid: " + filename);
        return ESP_FAIL;
    }

    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Failed to open file: " + filename);
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        fclose(file);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Failed to lock");
        return ESP_ERR_TIMEOUT;
    }

    if (updateContainerSize) {
        freeImageData();

        imgDataSize = fileSize;
        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", fileSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Failed to allocate memory for image data");
            LogFile.writeHeapInfo("updateImageDataFromJpgFile");
            fclose(file);
            return ESP_FAIL;
        }
    }
    else {
        if (imgDataSize < fileSize) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Container buffer too small");
            fclose(file);
            return ESP_FAIL;
        }
        imgDataSize = fileSize;
    }

    size_t bytesRead = fread(imgData, 1, fileSize, file);
    if (bytesRead != fileSize) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Failed to read the entire file: " + filename);
        fclose(file);
        return ESP_FAIL;
    }

    fclose(file);

    return ESP_OK;
}


esp_err_t CImageJpg::loadJpgFromMemory(const void *data, int size)
{
    if (size <= 0 || !data) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Invalid data or size");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Failed to lock");
        return ESP_ERR_TIMEOUT;
    }

    freeImageData();
    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Failed to allocate memory: " + std::to_string(size));
        LogFile.writeHeapInfo("loadJpgFromMemory");
        return ESP_FAIL;
    }

    memcpy(imgData, data, size);
    imgDataSize = size;

    return ESP_OK;
}


esp_err_t CImageJpg::saveJpgToFile(const std::string &filename)
{
    if (!imgData || imgDataSize <= 0) { // Ensure valid JPEG buffer
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: No or invalid data");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Failed to lock");
        return ESP_ERR_TIMEOUT;
    }

    FILE *file = fopen(filename.c_str(), "wb");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Failed to open file");
        return ESP_FAIL;
    }

    size_t written = fwrite(imgData, 1, imgDataSize, file);
    fclose(file);

    if (written != (size_t)imgDataSize) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Failed to write file");
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t CImageJpg::sendJpgToHttp(httpd_req_t *req)
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendJpgToHttp: No valid image data");
        return ESP_FAIL;
    }

    CImageLockGuard lockGuard(*this);
    if (!lockGuard.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendJpgToHttp: Failed to lock");
        return ESP_ERR_TIMEOUT;
    }

    httpd_resp_set_hdr(req, "Cache-Control", "max-age=0");
    httpd_resp_set_type(req, "image/jpeg");

    size_t chunkSize = WEBSERVER_SCRATCH_BUFSIZE; // Use same chunksize than for other web processes
    size_t bytesSent = 0;

    while (bytesSent < imgDataSize) {
        size_t bytesRemaining = imgDataSize - bytesSent;
        size_t currentChunkSize = (bytesRemaining < chunkSize) ? bytesRemaining : chunkSize;

        // Send a chunk of the image data
        if (httpd_resp_send_chunk(req, (const char *)(imgData + bytesSent), currentChunkSize) != ESP_OK) {
            return ESP_FAIL;
        }

        bytesSent += currentChunkSize;
    }

    // End the response
    if (httpd_resp_send_chunk(req, NULL, 0) != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}


bool CImageJpg::isValid() const
{
    return (imgData != nullptr && imgDataSize > 0);
}


uint8_t *CImageJpg::getImgData()
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getImgData (const): No image data");
        return nullptr;
    }

    return imgData;
}


const uint8_t *CImageJpg::getImgData() const
{
    if (!isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getImgData (const): No image data");
        return nullptr;
    }

    return imgData;
}


bool CImageJpg::lock() const
{
    if (imageMutex && xSemaphoreTakeRecursive(imageMutex, pdMS_TO_TICKS(30000)) == pdTRUE) {
        return true;
    }

#ifdef DEBUG_DETAIL_ON
    TaskHandle_t holder = xSemaphoreGetMutexHolder(imageMutex);
    if (holder != NULL) {
        char *taskName = pcTaskGetName(holder);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Lock Timeout: Held by task: " + std::string(taskName));
    }
#endif

    return false;
}

void CImageJpg::unlock() const
{
    if (imageMutex) {
        xSemaphoreGiveRecursive(imageMutex);
    }
}


void CImageJpg::freeImageData()
{
    if (imgData) {
        free_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ", " + std::to_string(imgDataSize) + ")", imgData);
        imgData = nullptr;
        imgDataSize = 0;
    }
}


CImageJpg::~CImageJpg()
{
    freeImageData();

    if (imageMutex) {
        vSemaphoreDelete(imageMutex);
    }
}
