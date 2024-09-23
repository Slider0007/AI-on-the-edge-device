#include "CFindTemplate.h"
#include "../../include/defines.h"

#include <esp_log.h>

#include "cfgDataStruct.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "psram.h"


static const char* TAG = "IMG_FINDTEMPL";


bool IRAM_ATTR CFindTemplate::findTemplate(AlignmentMarker *_ref, bool _noFast)
{
    // Load image of alignment marker which need to be compared with reference image
    rgb_template = _ref->markerImage->rgb_image;
    tpl_width = _ref->markerImage->width;
    tpl_height = _ref->markerImage->height;
    tpl_bpp = _ref->markerImage->bpp;

    if (rgb_template == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "stbi_load: Failed to load " + _ref->markerImageFilename);
        return false;
    }

    int ow, ow_start, ow_stop;
    int oh, oh_start, oh_stop;

    if (_ref->searchX == 0) {
        _ref->searchX = width;
        _ref->foundX = 0;
    }

    if (_ref->searchY == 0) {
        _ref->searchY = height;
        _ref->foundY = 0;
    }

    ow_start = _ref->targetX - _ref->searchX;
    ow_start = std::max(ow_start, 0);
    ow_stop = _ref->targetX + _ref->searchX;
    if ((ow_stop + tpl_width) > width) {
        ow_stop = width - tpl_width;
    }
    ow = ow_stop - ow_start + 1;

    oh_start = _ref->targetY - _ref->searchY;
    oh_start = std::max(oh_start, 0);
    oh_stop = _ref->targetY + _ref->searchY;
    if ((oh_stop + tpl_height) > height) {
        oh_stop = height - tpl_height;
    }
    oh = oh_stop - oh_start + 1;

    if (_ref->alignmentAlgo == ALIGNALGO_FAST && _ref->algoFastX > 0 && _ref->algoFastY > 0 && !_noFast) {
        //ESP_LOGD(TAG, "findTemplate - use FASTALGO");
        bool isSimilar = calcSimularities(_ref);
        #ifdef DEBUG_DETAIL_ON
            std::string zw = "\t" + _ref->markerImageFilename + "\t x_y: \t" + std::to_string(_ref->algoFastX) + "\t" + std::to_string(_ref->algoFastY);
            //LogFile.WriteToDedicatedFile("/sdcard/alignment.txt", zw);
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Fast Algo: " + zw);
        #endif

        if (isSimilar) {
            _ref->foundX = _ref->algoFastX;
            _ref->foundY = _ref->algoFastY;

            //free_psram_heap(std::string(TAG) + _ref->markerImageFilename, rgb_template);  // Keep alignment image markerImage in RAM
            return true;
        }
    }

    int SADsum = 0;
    int SADmin = tpl_width * tpl_height * 255;
    int _anzchannels;

    // 0 = "Default" (only R-channel) OR 2 = FAST ALGO (only if already processed but no match)
    if (_ref->alignmentAlgo == ALIGNALGO_DEFAULT || _ref->alignmentAlgo == ALIGNALGO_FAST) {
        //ESP_LOGD(TAG, "findTemplate - DEFAULT - only R channel");
        _anzchannels = 1;
    }
    else {
        //ESP_LOGD(TAG, "findTemplate - HIGH ACCURACY - process all channels");
        _anzchannels = channels;
    }

    //ESP_LOGD(TAG, "findTemplate - Process STANDARD algo");
    for (int xouter = ow_start; xouter <= ow_stop; xouter++) {
        for (int youter = oh_start; youter <= oh_stop; ++youter) {
            SADsum = 0;
            for (int tpl_x = 0; tpl_x < tpl_width; tpl_x++) {
                for (int tpl_y = 0; tpl_y < tpl_height; tpl_y++) {
                    stbi_uc* p_org = rgb_image + (channels * ((youter + tpl_y) * width + (xouter + tpl_x)));
                    stbi_uc* p_tpl = rgb_template + (channels * (tpl_y * tpl_width + tpl_x));
                    for (int _ch = 0; _ch < _anzchannels; ++_ch) {
                        SADsum += labs(p_tpl[_ch] - p_org[_ch]);
                    }
                }
            }

            if (SADsum < SADmin) {
                SADmin = SADsum;
                _ref->foundX = xouter;
                _ref->foundY = youter;
            }
        }
    }

    // Save actual values to be prepared if switch to FAST ALGO
    _ref->algoFastX = _ref->foundX;
    _ref->algoFastY = _ref->foundY;

    // Print results
    std::string zw = "SADsum:" + std::to_string(SADsum) + ", X:"+ std::to_string(_ref->foundX) + ", Y:" + std::to_string(_ref->foundY);
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "STANDARD Algo results: " + zw);

    //free_psram_heap(std::string(TAG) + _ref->markerImageFilename, rgb_template);   // Keep alignment image markerImage in RAM
    return false;
}



bool IRAM_ATTR CFindTemplate::calcSimularities(AlignmentMarker* _ref)
{
    int anz = 0;
    long SADsum = 0;
    int SADNorm;

    for (int xouter = 0; xouter <= tpl_width; xouter++) {
        for (int youter = 0; youter <= tpl_height; ++youter) {
            stbi_uc* p_org = rgb_image + (channels * ((youter + _ref->algoFastY) * width + (xouter + _ref->algoFastX)));
            stbi_uc* p_tpl = rgb_template + (channels * (youter * tpl_width + xouter));
            for (int _ch = 0; _ch < channels; ++_ch) {
                SADsum += labs(p_tpl[_ch] - p_org[_ch]);
                anz++;
            }
        }
    }

    // normalize by number of sums
    SADNorm = SADsum/anz;

    // Print results
    std::string zw = "SADThreshold:" + std::to_string(_ref->algoFastSADThreshold) + ", SADNorm:" + std::to_string(SADNorm) +
                     ", X:" + std::to_string(_ref->algoFastX) + ", Y:" + std::to_string(_ref->algoFastY);
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "FAST Algo results: " + zw);

    // Evaluate results
    if (SADNorm <= _ref->algoFastSADThreshold) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "FAST Algo: Match found");
        return true;
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "FAST Algo: No match (SADNorm>SADThreshold) -> Use STANDARD Algo");
        return false;
    }
}
