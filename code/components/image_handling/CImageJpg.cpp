#include "CImageJpg.h"

#include <fstream>

#include "esp_system.h"

#include "helper.h"
#include "psram.h"
#include "ClassLogFile.h"


static const char *TAG = "IMG_JPG";


CImageJpg::CImageJpg() : name("default"), imgDataSize(0), imgData(nullptr)
{
    imageMutex = xSemaphoreCreateMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageJpg: Failed to create semaphore");
        return;
    }
}


CImageJpg::CImageJpg(std::string objName, int size, const uint8_t *data) : name(objName), imgDataSize(size)
{
    imageMutex = xSemaphoreCreateMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageJpg: Failed to create semaphore");
        return;
    }

    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", imgDataSize,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Can't allocate enough memory: " + std::to_string(imgDataSize));
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
    imageMutex = xSemaphoreCreateMutex();
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


CImageJpg::CImageJpg(const CImageJpg &other) : name(other.name + "-copy"), imgDataSize(other.imgDataSize)
{
    imageMutex = xSemaphoreCreateMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to create semaphore");
        return;
    }

    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Could not acquire lock");
        return;
    }

    if (imgDataSize > 0) {
        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", imgDataSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy: Failed to allocate memory: " + std::to_string(imgDataSize));
            return;
        }

        if (other.imgData) {
            memcpy(imgData, other.imgData, imgDataSize);
        }
        else {
            imgData = nullptr;
        }
    }
    else {
        imgData = nullptr;
    }
}


CImageJpg &CImageJpg::operator=(const CImageJpg &other)
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

    if (imageMutex) {
        vSemaphoreDelete(imageMutex);
    }
    imageMutex = xSemaphoreCreateMutex();
    if (!imageMutex) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Copy-assign: Failed to create semaphore");
        return *this;
    }

    name = other.name + "-copy-assign";
    imgDataSize = other.imgDataSize;

    freeImageData();
    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", imgDataSize,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory: " + std::to_string(imgDataSize));
        return *this;
    }

    if (other.imgData) {
        memcpy(imgData, other.imgData, imgDataSize);
    }
    else {
        memset(imgData, 0, imgDataSize);
    }

    return *this;
}


CImageJpg::CImageJpg(CImageJpg &&other) noexcept
    : imageMutex(other.imageMutex), name(std::move(other.name)), imgDataSize(other.imgDataSize), imgData(other.imgData)
{
    CImageLockGuard otherLock(other);
    if (!otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Move: Failed to lock source");
        return;
    }

    other.imageMutex = nullptr;
    other.imgDataSize = 0;
    other.imgData = nullptr;
}


// Move assignment operator
CImageJpg &CImageJpg::operator=(CImageJpg &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    CImageLockGuard thisLock(*this);
    CImageLockGuard otherLock(other);
    if (!thisLock.isLocked() || !otherLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Move-Assign: Failed to lock images");
        return *this;
    }

    freeImageData();

    if (imageMutex) {
        vSemaphoreDelete(imageMutex);
    }

    imageMutex = other.imageMutex;
    other.imageMutex = nullptr;

    name = std::move(other.name);
    imgDataSize = other.imgDataSize;
    imgData = other.imgData;

    other.imgDataSize = 0;
    other.imgData = nullptr;

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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    if (updateContainerSize) {
        freeImageData();

        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", newSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgBuffer: Failed to allocate memory for new data");
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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    if (updateContainerSize) {
        freeImageData();

        imgDataSize = fileSize;
        imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", fileSize,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!imgData) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "updateImageDataFromJpgFile: Failed to allocate memory for image data");
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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    freeImageData();
    imgData = (uint8_t *)malloc_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ")", size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!imgData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadJpgFromMemory: Failed to allocate memory: " + std::to_string(size));
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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveJpgToFile: Could not acquire lock");
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
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "sendJpgToHttp: Could not acquire lock");
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
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send image");
        }

        bytesSent += currentChunkSize;
    }

    // End the response
    if (httpd_resp_send_chunk(req, NULL, 0) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send image");
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
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

    if (lockingTask == currentTask) {
        lockCount++;
#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Recursive lock acquired by task %p, count: %d", currentTask, lockCount);
#endif // DEBUG_DETAIL_ON
        return true;
    }

#ifdef DEBUG_DETAIL_ON
    ESP_LOGI(TAG, "Task %p attempting to lock image...", currentTask);
#endif // DEBUG_DETAIL_ON

    if (imageMutex && xSemaphoreTake(imageMutex, pdMS_TO_TICKS(10000))) {
        lockingTask = currentTask;
        lockCount = 1;
#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Task %p successfully locked image", currentTask);
#endif // DEBUG_DETAIL_ON
        return true;
    }

    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "lock: Timeout - Failed to acquire mutex");
    return false;
}

void CImageJpg::unlock() const
{
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

    if (lockingTask == currentTask) {
        lockCount--;

        if (lockCount == 0) {
            lockingTask = NULL;
            if (imageMutex) {
                xSemaphoreGive(imageMutex);
            }
#ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "Task %p unlocked image", currentTask);
#endif // DEBUG_DETAIL_ON
        }
        else {
#ifdef DEBUG_DETAIL_ON
            ESP_LOGI(TAG, "Task %p decreased lock count to %d", currentTask, lockCount);
#endif // DEBUG_DETAIL_ON
        }
    }
    else {
#ifdef DEBUG_DETAIL_ON
        ESP_LOGE(TAG, "Task %p tried to unlock an image it does not own", currentTask);
#endif // DEBUG_DETAIL_ON
    }
}


void CImageJpg::freeImageData()
{
    if (imgData) {
        free_psram_heap(std::string(TAG) + "->CImageJpg (" + name + ", " + std::to_string(imgDataSize) + ")", imgData);
        imgData = nullptr;
    }
}


CImageJpg::~CImageJpg()
{
    freeImageData();

    if (imageMutex) {
        vSemaphoreDelete(imageMutex);
    }
}
