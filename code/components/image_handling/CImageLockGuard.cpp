#include "CImageLockGuard.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ClassLogFile.h"

// static const char *TAG = "IMG_LOCK"; // Unused


CImageLockGuard::CImageLockGuard(const CImage &image) : imgPtr((void *)&image), isJpg(false), locked(false)
{
    CImage *img = static_cast<CImage *>(imgPtr);

    locked = img->lock();
}


CImageLockGuard::CImageLockGuard(const CImageJpg &image) : imgPtr((void *)&image), isJpg(true), locked(false)
{
    CImageJpg *imgJpg = static_cast<CImageJpg *>(imgPtr);

    locked = imgJpg->lock();
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
