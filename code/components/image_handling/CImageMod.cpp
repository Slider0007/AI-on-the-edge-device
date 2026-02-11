#include "CImageMod.h"

#include <math.h>

#include "esp_log.h"

#include "psram.h"
#include "ClassLogFile.h"


static const char *TAG = "IMG_MOD";


esp_err_t IRAM_ATTR CImageMod::rotate(CImage &img, float angle, CImage &imgHelper, bool overwriteSource)
{
    return rotate(img, angle, img.getWidth() / 2, img.getHeight() / 2, imgHelper, overwriteSource);
}


esp_err_t IRAM_ATTR CImageMod::rotate(CImage &img, float angle, int centerX, int centerY, CImage &imgHelper, bool overwriteSource)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "rotate: Invalid source image");
        return ESP_FAIL;
    }

    if (!imgHelper.isValid() || imgHelper.getImgDataSize() < img.getImgDataSize()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "rotate: Invalid helper image or insufficient buffer size");
        return ESP_FAIL;
    }

    const CImage *first = (&img < &imgHelper) ? &img : &imgHelper;
    const CImage *second = (&img < &imgHelper) ? &imgHelper : &img;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "rotate: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Get image properties
    const int width = img.getWidth();
    const int height = img.getHeight();
    const int channels = img.getChannels();
    uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = imgHelper.getImgData();

    // Precompute helpers
    const float rad = angle * M_PI / 180.0f;
    const float cosA = cosf(rad);
    const float sinA = sinf(rad);
    const float tx = (1 - cosA) * centerX - sinA * centerY;
    const float ty = sinA * centerX + (1 - cosA) * centerY;

    const int widthInBytes = width * channels;

#if ROTATE_MODE == MODE_BILINEAR
    int x0, y0, x1, y1;
    float a, b, val;
#endif // ROTATE_MODE

    for (int y = 0; y < height; ++y) {
        uint8_t *pTarget = imgDataTarget + y * widthInBytes;

        for (int x = 0; x < width; ++x) {
#if ROTATE_MODE == MODE_BILINEAR
            const float srcX = cosA * x + sinA * y + tx;
            const float srcY = -sinA * x + cosA * y + ty;

            x0 = (int)srcX;
            y0 = (int)srcY;
            x1 = x0 + 1;
            y1 = y0 + 1;

            // Check if within bounds
            if (x0 >= 0 && x1 < width && y0 >= 0 && y1 < height) {
                a = srcX - (float)x0; // Fractional part for x
                b = srcY - (float)y0; // Fractional part for y

                // Access the four neighboring pixels
                const uint8_t *p00 = imgData + ((y0)*width + (x0)) * channels;
                const uint8_t *p01 = imgData + ((y0)*width + (x1)) * channels;
                const uint8_t *p10 = imgData + ((y1)*width + (x0)) * channels;
                const uint8_t *p11 = imgData + ((y1)*width + (x1)) * channels;

                for (int c = 0; c < channels; ++c) {
                    val = (1 - a) * (1 - b) * p00[c] + a * (1 - b) * p01[c] + (1 - a) * b * p10[c] + a * b * p11[c];
                    pTarget[c] = (uint8_t)val;
                }
            }
            else {
                memset(pTarget, IMAGE_COLOR_OUT_OF_BOUND, channels);
            }

#elif ROTATE_MODE == MODE_NEAREST // Nearest-neighbor interpolation
            const int nearestX = (int)(cosA * x + sinA * y + tx + 0.5f);
            const int nearestY = (int)(-sinA * x + cosA * y + ty + 0.5f);

            if ((unsigned)nearestX < (unsigned)width && (unsigned)nearestY < (unsigned)height) {
                const uint8_t *pSource = imgData + (nearestY * width + nearestX) * channels;
                memcpy(pTarget, pSource, channels);
            }
            else {
                memset(pTarget, IMAGE_COLOR_OUT_OF_BOUND, channels);
            }

#endif // ROTATE_MODE

            pTarget += channels; // Move to the next pixel
        }
    }

    if (overwriteSource) {
        memcpy(imgData, imgDataTarget, img.getImgDataSize());
    }

    return ESP_OK;
}


esp_err_t IRAM_ATTR CImageMod::translate(CImage &img, int dx, int dy, CImage &imgHelper, bool overwriteSource)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "translate: Invalid source image");
        return ESP_FAIL;
    }

    if (!imgHelper.isValid() || imgHelper.getImgDataSize() < img.getImgDataSize()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "translate: Invalid helper image or insufficient buffer size");
        return ESP_FAIL;
    }

    const CImage *first = (&img < &imgHelper) ? &img : &imgHelper;
    const CImage *second = (&img < &imgHelper) ? &imgHelper : &img;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "translate: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Get image properties
    const int width = img.getWidth();
    const int height = img.getHeight();
    const int channels = img.getChannels();
    uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = imgHelper.getImgData();

    const int widthInBytes = width * channels;

#if TRANSLATE_MODE == MODE_BILINEAR
    int x0, y0, x1, y1;
    float a, b, val;
#endif // TRANSLATE_MODE

    for (int y = 0; y < height; ++y) {
        uint8_t *pTarget = imgDataTarget + y * widthInBytes;

        for (int x = 0; x < width; ++x) {
            const float srcX = x - dx;
            const float srcY = y - dy;

#if TRANSLATE_MODE == MODE_BILINEAR
            x0 = (int)srcX;
            y0 = (int)srcY;
            x1 = x0 + 1;
            y1 = y0 + 1;

            if (x0 >= 0 && x1 < width && y0 >= 0 && y1 < height) {
                a = srcX - (float)x0; // Fractional part for x
                b = srcY - (float)y0; // Fractional part for y

                // Access the four neighboring pixels
                const uint8_t *p00 = imgData + ((y0)*width + (x0)) * channels;
                const uint8_t *p01 = imgData + ((y0)*width + (x1)) * channels;
                const uint8_t *p10 = imgData + ((y1)*width + (x0)) * channels;
                const uint8_t *p11 = imgData + ((y1)*width + (x1)) * channels;

                for (int c = 0; c < channels; ++c) {
                    val = (1 - a) * (1 - b) * p00[c] + a * (1 - b) * p01[c] + (1 - a) * b * p10[c] + a * b * p11[c];
                    pTarget[c] = (uint8_t)val;
                }
            }
            else {
                memset(pTarget, IMAGE_COLOR_OUT_OF_BOUND, channels);
            }

#elif TRANSLATE_MODE == MODE_NEAREST // Nearest-Neighbor Interpolation
            const int nearestX = (int)(srcX + 0.5f);
            const int nearestY = (int)(srcY + 0.5f);

            if ((unsigned)nearestX < (unsigned)width && (unsigned)nearestY < (unsigned)height) {
                const uint8_t *pSource = imgData + (nearestY * width + nearestX) * channels;
                memcpy(pTarget, pSource, channels);
            }
            else {
                memset(pTarget, IMAGE_COLOR_OUT_OF_BOUND, channels);
            }

#endif // TRANSLATE_MODE

            pTarget += channels; // Move to the next pixel
        }
    }

    if (overwriteSource) {
        memcpy(imgData, imgDataTarget, img.getImgDataSize());
    }

    return ESP_OK;
}


esp_err_t CImageMod::crop(CImage &img, int x, int y, int newWidth, int newHeight, CImage &imgTarget)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "crop: Invalid source image");
        return ESP_FAIL;
    }

    if (!imgTarget.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "crop: Invalid target image");
        return ESP_FAIL;
    }

    const CImage *first = (&img < &imgTarget) ? &img : &imgTarget;
    const CImage *second = (&img < &imgTarget) ? &imgTarget : &img;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "crop: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Get image properties
    const int channels = img.getChannels();
    const uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = imgTarget.getImgData();

    for (int i = 0; i < newHeight; ++i) {
        const uint8_t *pOriginalRowStart = imgData + ((y + i) * img.getWidth() + x) * channels;
        uint8_t *pCroppedRowStart = imgDataTarget + i * newWidth * channels;

        memcpy(pCroppedRowStart, pOriginalRowStart, newWidth * channels * sizeof(uint8_t));
    }

    return ESP_OK;
}


esp_err_t CImageMod::resize(CImage &img, int newWidth, int newHeight, CImage &imgTarget)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "resize: Invalid source image");
        return ESP_FAIL;
    }

    if (!imgTarget.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "resize: Invalid target image");
        return ESP_FAIL;
    }

    const CImage *first = (&img < &imgTarget) ? &img : &imgTarget;
    const CImage *second = (&img < &imgTarget) ? &imgTarget : &img;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "resize: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Get image properties
    const uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = imgTarget.getImgData();

    if (!stbir_resize_uint8(imgData, img.getWidth(), img.getHeight(), 0, imgDataTarget, newWidth, newHeight, 0, img.getChannels())) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "resize: Failed to resize image");
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t CImageMod::grayscale(CImage &img, bool overwriteSource, CImage *imgTarget)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "grayscale: Invalid source image");
        return ESP_FAIL;
    }

    if (!overwriteSource) {
        if (!imgTarget || !imgTarget->isValid()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "grayscale: Invalid or missing target image");
            return ESP_FAIL;
        }

        const CImage *first = (&img < imgTarget) ? &img : imgTarget;
        const CImage *second = (&img < imgTarget) ? imgTarget : &img;

        CImageLockGuard lock1(*first);
        CImageLockGuard lock2(*second);
        if (!lock1.isLocked() || !lock2.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "grayscale: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }
    else {
        CImageLockGuard imgLock(img);
        if (!imgLock.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "grayscale: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }

    const int width = img.getWidth();
    const int height = img.getHeight();
    const int channels = img.getChannels();

    if (channels != 3) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "grayscale: Image is not in RGB format");
        return ESP_FAIL;
    }

    uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = overwriteSource ? imgData : imgTarget->getImgData();

    const int pixelCount = width * height;

    const float R_WEIGHT = 0.299f;
    const float G_WEIGHT = 0.587f;
    const float B_WEIGHT = 0.114f;

    for (int i = 0; i < pixelCount; ++i) {
        const uint8_t *pSource = imgData + i * channels;
        uint8_t *pTarget = imgDataTarget + i * channels;

        const float gray = R_WEIGHT * (float)pSource[0] + G_WEIGHT * (float)pSource[1] + B_WEIGHT * (float)pSource[2];
        pTarget[0] = (uint8_t)gray;
        pTarget[1] = (uint8_t)gray;
        pTarget[2] = (uint8_t)gray;
    }

    return ESP_OK;
}


esp_err_t CImageMod::normalize(CImage &img, bool overwriteSource, CImage *imgTarget)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "normalize: Invalid source image");
        return ESP_FAIL;
    }

    if (!overwriteSource) {
        if (!imgTarget || !imgTarget->isValid()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "normalize: Invalid or missing target image");
            return ESP_FAIL;
        }

        const CImage *first = (&img < imgTarget) ? &img : imgTarget;
        const CImage *second = (&img < imgTarget) ? imgTarget : &img;

        CImageLockGuard lock1(*first);
        CImageLockGuard lock2(*second);
        if (!lock1.isLocked() || !lock2.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "normalize: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }
    else {
        CImageLockGuard imgLock(img);
        if (!imgLock.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "normalize: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }

    const int width = img.getWidth();
    const int height = img.getHeight();
    const int channels = img.getChannels();
    uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = overwriteSource ? imgData : imgTarget->getImgData();

    int pixelCount = width * height;

    // Find the min and max pixel values in the image
    int minVal = 255, maxVal = 0;

    for (int i = 0; i < pixelCount * channels; ++i) {
        int pixelVal = imgData[i];
        if (pixelVal < minVal) {
            minVal = pixelVal;
        }
        if (pixelVal > maxVal) {
            maxVal = pixelVal;
        }
    }

    int range = maxVal - minVal;

    // If range is 0, all pixels are the same, no normalization needed
    if (range == 0) {
        return ESP_OK;
    }

    // Normalize the pixel values using the precomputed min and max values
    for (int i = 0; i < pixelCount * channels; ++i) {
        imgDataTarget[i] = (uint8_t)(((imgData[i] - minVal) * 255) / range);
    }

    return ESP_OK;
}


esp_err_t CImageMod::negative(CImage &img, bool overwriteSource, CImage *imgTarget)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "negative: Invalid source image");
        return ESP_FAIL;
    }

    if (!overwriteSource) {
        if (!imgTarget || !imgTarget->isValid()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "negative: Invalid or missing target image");
            return ESP_FAIL;
        }

        const CImage *first = (&img < imgTarget) ? &img : imgTarget;
        const CImage *second = (&img < imgTarget) ? imgTarget : &img;

        CImageLockGuard lock1(*first);
        CImageLockGuard lock2(*second);
        if (!lock1.isLocked() || !lock2.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "negative: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }
    else {
        CImageLockGuard imgLock(img);
        if (!imgLock.isLocked()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "negative: Could not acquire lock");
            return ESP_ERR_TIMEOUT;
        }
    }

    // Get image properties
    const int width = img.getWidth();
    const int height = img.getHeight();
    const int channels = img.getChannels();
    uint8_t *imgData = img.getImgData();
    uint8_t *imgDataTarget = overwriteSource ? imgData : imgTarget->getImgData();

    for (int i = 0; i < width * height * channels; i++) {
        imgDataTarget[i] = 255 - imgData[i];
    }

    return ESP_OK;
}


esp_err_t IRAM_ATTR CImageMod::drawRect(CImage &img, int x, int y, int dx, int dy, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawRect: Invalid source image");
        return ESP_FAIL;
    }

    CImageLockGuard imgLock(img);
    if (!imgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawRect: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Precompute helpers
    const int xEnd = x + dx;
    const int yEnd = y + dy;

    // Draw top & bottom
    for (int t = 0; t < thickness; ++t) {
        for (int i = x - t; i <= xEnd + t; ++i) {
            img.setPixelColor(i, y - t, r, g, b);    // Top
            img.setPixelColor(i, yEnd + t, r, g, b); // Bottom
        }
    }

    // Draw left & right
    for (int t = 0; t < thickness; ++t) {
        for (int i = y - t; i <= yEnd + t; ++i) {
            img.setPixelColor(x - t, i, r, g, b);    // Left
            img.setPixelColor(xEnd + t, i, r, g, b); // Right
        }
    }

    return ESP_OK;
}


esp_err_t IRAM_ATTR CImageMod::drawLine(CImage &img, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawLine: Invalid source image");
        return ESP_FAIL;
    }

    CImageLockGuard imgLock(img);
    if (!imgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawLine: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Precompute helpers
    const int dx = abs(x2 - x1);
    const int dy = -abs(y2 - y1);
    const int sx = (x1 < x2) ? 1 : -1;
    const int sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;
    int e2;

    const uint8_t halfThickness = thickness / 2;

    // Draw
    do {
        for (int tX = -halfThickness; tX <= halfThickness; ++tX) {
            for (int tY = -halfThickness; tY <= halfThickness; ++tY) {
                int pixelX = x1 + tX;
                int pixelY = y1 + tY;
                img.setPixelColor(pixelX, pixelY, r, g, b);
            }
        }

        if (x1 == x2 && y1 == y2) { // Exit loop
            break;
        }

        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    } while (true);

    return ESP_OK;
}


esp_err_t IRAM_ATTR CImageMod::drawCircle(CImage &img, int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawCircle: Invalid source image");
        return ESP_FAIL;
    }

    CImageLockGuard imgLock(img);
    if (!imgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawCircle: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Precompute helper
    const float angleStep = 1.0f / rad; // Step size based on radius

    // Draw
    for (float angle = 0; angle < 2 * M_PI; angle += angleStep) {
        for (int t = -thickness / 2; t <= thickness / 2; ++t) {
            const int _x = x + (rad + t) * cos(angle);
            const int _y = y + (rad + t) * sin(angle);
            img.setPixelColor(_x, _y, r, g, b);
        }
    }

    return ESP_OK;
}


esp_err_t IRAM_ATTR CImageMod::drawEllipse(CImage &img, int x, int y, int radX, int radY, uint8_t r, uint8_t g, uint8_t b,
                                           uint8_t thickness)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawEllipse: Invalid source image");
        return ESP_FAIL;
    }

    CImageLockGuard imgLock(img);
    if (!imgLock.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "drawEllipse: Could not acquire lock");
        return ESP_ERR_TIMEOUT;
    }

    // Precompute helpers
    const float angleStep = 1.0f / std::max(radX, radY); // Step size based on larger radius

    // Draw
    for (float angle = 0; angle < 2 * M_PI; angle += angleStep) {
        for (int t = -thickness / 2; t <= thickness / 2; ++t) {
            const int _x = x + (radX + t) * cos(angle);
            const int _y = y + (radX + t) * sin(angle);
            img.setPixelColor(_x, _y, r, g, b);
        }
    }

    return ESP_OK;
}
