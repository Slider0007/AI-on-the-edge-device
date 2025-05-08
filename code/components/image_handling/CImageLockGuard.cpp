#include "CImageLockGuard.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ClassLogFile.h"

static const char *TAG = "IMG_LOCK";


CImageLockGuard::CImageLockGuard(const CImage &image) : imgPtr((void *)&image), isJpg(false), locked(false), alreadyLocked(false)
{
    CImage *img = static_cast<CImage *>(imgPtr);
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

    // Recursive lock detection
    if (img->lockCount > 0 && img->lockingTask == currentTask) {
        alreadyLocked = true;
#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Recursive lock detected in task %p", currentTask);
#endif // DEBUG_DETAIL_ON
    }
    else {
        locked = img->lock();
        if (!locked) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageLockGuard: Failed to acquire lock");
        }
    }
}


CImageLockGuard::CImageLockGuard(const CImageJpg &image) : imgPtr((void *)&image), isJpg(true), locked(false), alreadyLocked(false)
{
    CImageJpg *imgJpg = static_cast<CImageJpg *>(imgPtr);
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

    // Recursive lock detection
    if (imgJpg->lockCount > 0 && imgJpg->lockingTask == currentTask) {
        alreadyLocked = true;
#ifdef DEBUG_DETAIL_ON
        ESP_LOGI(TAG, "Recursive lock detected in task %p", currentTask);
#endif // DEBUG_DETAIL_ON
    }
    else {
        locked = imgJpg->lock();
        if (!locked) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CImageLockGuard: Failed to acquire lock");
        }
    }
}


CImageLockGuard::~CImageLockGuard()
{
    if (locked && !alreadyLocked) {
        if (isJpg) {
            CImageJpg *imgJpg = static_cast<CImageJpg *>(imgPtr);
            imgJpg->unlock();
        }
        else {
            CImage *img = static_cast<CImage *>(imgPtr);
            img->unlock();
        }

#ifdef DEBUG_DETAIL_ON
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        ESP_LOGI(TAG, "Lock released in task %p", currentTask);
#endif // DEBUG_DETAIL_ON
    }
}


bool CImageLockGuard::isLocked() const
{
    return locked || alreadyLocked;
}
