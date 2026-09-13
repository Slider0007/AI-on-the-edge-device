#include "CImageTplMatch.h"

#include "../../include/defines.h"

#include <algorithm>
#include <cmath>

#include "CImageMod.h"
#include "ClassLogFile.h"
#include "helper.h"


static const char *TAG = "IMG_TPLMATCH";


TplMatchStatus IRAM_ATTR CImageTplMatch::invokeTplMatch(CImage &img, CImage &imgTarget, AlignmentMarker &marker1, AlignmentMarker &marker2,
                                                        TplMatchAlgorithm tplMatchAlgorithm)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "invokeTplMatch: Invalid source image");
        return TPL_MATCH_ERROR_IMAGE;
    }

    if (!marker1.markerImage->isValid() || !marker2.markerImage->isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "invokeTplMatch: Invalid marker image");
        return TPL_MATCH_ERROR_IMAGE;
    }

    switch (tplMatchAlgorithm) {
        case TplMatchAlgorithm::SAD: {
            TplMatchStatus retVal = tplMatchBySad(img, marker1);
            if (retVal != TPL_MATCH_OK) {
                return retVal;
            }

            retVal = tplMatchBySad(img, marker2);
            if (retVal != TPL_MATCH_OK) {
                return retVal;
            }
            break;
        }
        default:
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "invokeTplMatch: Unknown matching algorithm");
            return TPL_MATCH_UNKNOWN_ALGORITHM;
    }

    const int deltaX1 = marker1.targetX - marker1.foundX;
    const int deltaY1 = marker1.targetY - marker1.foundY;
    const int deltaX2 = marker2.targetX - marker2.foundX;
    const int deltaY2 = marker2.targetY - marker2.foundY;

    const int result0X = marker1.targetX + deltaX1;
    const int result0Y = marker1.targetY + deltaY1;
    const int result1X = marker2.targetX + deltaX1;
    const int result1Y = marker2.targetY + deltaY1;

    const float initialAngle = atan2(marker2.foundY - marker1.foundY, marker2.foundX - marker1.foundX);
    const float actualAngle = atan2(result1Y - result0Y, result1X - result0X);
    const float angleDeviation = (actualAngle - initialAngle) * 180.0f / M_PI;

    // Check for alignment failure
    if (fabsf(angleDeviation) >= ANGLE_DEVIATION_THRESHOLD || abs(deltaX1) >= marker1.searchX || abs(deltaY1) >= marker1.searchY ||
        abs(deltaX2) >= marker2.searchX || abs(deltaY2) >= marker2.searchY) {
        marker1.errorMsg = marker2.errorMsg = "Angle dev: " + to_stringWithPrecision(angleDeviation, 1) +
                                              ", dX1: " + std::to_string(deltaX1) + ", dY1: " + std::to_string(deltaY1) +
                                              ", dX2: " + std::to_string(deltaX2) + ", dY2: " + std::to_string(deltaY2);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, marker1.errorMsg);
        return TPL_MATCH_FAILED;
    }

    // Apply corrections (linear, rotation-based or both)
    if ((deltaX1 > 0 && deltaX2 > 0 && deltaY1 > 0 && deltaY2 > 0) || (deltaX1 < 0 && deltaX2 < 0 && deltaY1 < 0 && deltaY2 < 0)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Alignment: Correction by linear X + Y only");
        CImageMod::translate(img, deltaX1, deltaY1, imgTarget, true);
    }
    else if ((deltaX1 > 0 && deltaX2 > 0) || (deltaX1 < 0 && deltaX2 < 0)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Alignment: Correction by rotation + linear X");
        CImageMod::rotate(img, angleDeviation, imgTarget, true);
        CImageMod::translate(img, deltaX1 / 2, 0, imgTarget, true); // Adjust with half translation
    }
    else if ((deltaY1 > 0 && deltaY2 > 0) || (deltaY1 < 0 && deltaY2 < 0)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Alignment: Correction by rotation + linear Y");
        CImageMod::rotate(img, angleDeviation, imgTarget, true);
        CImageMod::translate(img, 0, deltaY1 / 2, imgTarget, true); // Adjust with half translation
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Alignment: Correction by rotation only");
        CImageMod::rotate(img, angleDeviation, imgTarget, true);
    }

    // Reset error messages on success
    marker1.errorMsg.clear();
    marker2.errorMsg.clear();

    // Final log for alignment details
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "Angle dev: " + to_stringWithPrecision(angleDeviation, 1) + ", dX1: " + std::to_string(deltaX1) + ", dY1: " +
                            std::to_string(deltaY1) + ", dX2: " + std::to_string(deltaX2) + ", dY2: " + std::to_string(deltaY2));

    return TPL_MATCH_OK;
}


TplMatchStatus IRAM_ATTR CImageTplMatch::tplMatchBySad(CImage &img, AlignmentMarker &marker)
{
    if (!img.isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "tplMatchBySad: Invalid source image");
        return TPL_MATCH_ERROR_IMAGE;
    }

    if (!marker.markerImage->isValid()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "tplMatchBySad: Invalid marker image");
        return TPL_MATCH_ERROR_IMAGE;
    }

    const CImage *first = (&img < marker.markerImage) ? &img : marker.markerImage;
    const CImage *second = (&img < marker.markerImage) ? marker.markerImage : &img;

    CImageLockGuard lock1(*first);
    CImageLockGuard lock2(*second);
    if (!lock1.isLocked() || !lock2.isLocked()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "tplMatchBySad: Failed to lock");
        return TPL_MATCH_ERROR_TIMEOUT;
    }

    // Get image properties
    const int imgWidth = img.getWidth();
    const int imgHeight = img.getHeight();
    const int imgChannels = img.getChannels();
    const uint8_t *imgData = img.getImgData();

    // Get template properties
    const int tplWidth = marker.markerImage->getWidth();
    const int tplHeight = marker.markerImage->getHeight();
    const uint8_t *tplImgData = marker.markerImage->getImgData();

    // Search coordinates validation
    marker.searchX = std::clamp(marker.searchX, 1, imgWidth);
    marker.searchY = std::clamp(marker.searchY, 1, imgHeight);

    // Define search region within valid image boundaries
    const int owStart = std::clamp(marker.targetX - marker.searchX, 0, std::max(1, imgWidth - tplWidth));
    const int owStop = std::clamp(marker.targetX + marker.searchX, 0, std::max(1, imgWidth - tplWidth));
    const int ohStart = std::clamp(marker.targetY - marker.searchY, 0, std::max(1, imgHeight - tplHeight));
    const int ohStop = std::clamp(marker.targetY + marker.searchY, 0, std::max(1, imgHeight - tplHeight));

    int sadMin = INT_MAX;
    const int consideredChannels = marker.alignmentAlgo == ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH ? 1 : imgChannels;

    for (int yOuter = ohStart; yOuter <= ohStop; ++yOuter) {
        const uint8_t *pOrgStartRow = imgData + (yOuter * imgWidth * imgChannels);

        for (int xOuter = owStart; xOuter <= owStop; ++xOuter) {
            int sadSum = 0;

            // Pointer to the top-left pixel of the search region in the image
            const uint8_t *pOrgStart = pOrgStartRow + (xOuter * imgChannels);
            const uint8_t *pTplStart = tplImgData;

            for (int tplY = 0; tplY < tplHeight; ++tplY) {
                const uint8_t *pOrgRow = pOrgStart + (tplY * imgWidth * imgChannels);
                const uint8_t *pTplRow = pTplStart + (tplY * tplWidth * imgChannels);

                int tplX = 0;
                const int tplEnd = tplWidth * consideredChannels;
                for (; tplX + 4 <= tplEnd; tplX += 4) {
                    sadSum += abs(pTplRow[tplX] - pOrgRow[tplX]) + abs(pTplRow[tplX + 1] - pOrgRow[tplX + 1]) +
                              abs(pTplRow[tplX + 2] - pOrgRow[tplX + 2]) + abs(pTplRow[tplX + 3] - pOrgRow[tplX + 3]);
                }

                // Process remaining pixels due to unrollment
                for (; tplX < tplEnd; ++tplX) {
                    sadSum += abs(pTplRow[tplX] - pOrgRow[tplX]);
                }

                // Early exit if SAD is already higher than sadMin
                if (sadSum >= sadMin) {
                    break;
                }
            }

            if (sadSum < sadMin) {
                sadMin = sadSum;
                marker.foundX = xOuter;
                marker.foundY = yOuter;
            }
        }
    }

    // Log results
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "SAD result: SADmin:" + std::to_string(sadMin) + ", X:" + std::to_string(marker.foundX) +
                            ", Y:" + std::to_string(marker.foundY));

    return TPL_MATCH_OK;
}
