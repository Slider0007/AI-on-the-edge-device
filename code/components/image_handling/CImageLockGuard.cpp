#include "CImageLockGuard.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ClassLogFile.h"

static const char *TAG = "IMG_LOCK";


CImageLockGuard::CImageLockGuard(const CImage &image) : imgPtr((void *)&image), isJpg(false), locked(false)
{
    const CImage *img = static_cast<const CImage *>(imgPtr);
    locked = img->lock();

    if (!locked) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to acquire lock");
    }
}


CImageLockGuard::CImageLockGuard(const CImageJpg &image) : imgPtr((void *)&image), isJpg(true), locked(false)
{
    const CImageJpg *imgJpg = static_cast<const CImageJpg *>(imgPtr);
    locked = imgJpg->lock();

    if (!locked) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to acquire lock");
    }
}


CImageLockGuard::~CImageLockGuard()
{
    if (locked) {
        if (isJpg) {
            static_cast<CImageJpg *>(imgPtr)->unlock();
        }
        else {
            static_cast<CImage *>(imgPtr)->unlock();
        }
    }
}


bool CImageLockGuard::isLocked() const
{
    return locked;
}
