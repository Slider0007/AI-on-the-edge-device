#include "CFindTemplate.h"
#include "../../include/defines.h"

#include <esp_log.h>

#include "ClassLogFile.h"
#include "Helper.h"
#include "psram.h"


static const char* TAG = "IMG_FINDTEMPL";

//#define DEBUG_DETAIL_ON


bool IRAM_ATTR CFindTemplate::FindTemplate(strRefInfo *_ref, bool _noFast)
{
    // Load image of alignment marker which need to be compared with reference image
    rgb_template = _ref->refImage->rgb_image;
    tpl_width = _ref->refImage->width;
    tpl_height = _ref->refImage->height;
    tpl_bpp = _ref->refImage->bpp;

    if (rgb_template == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "stbi_load: Failed to load " + _ref->image_file);
        return false;
    }

    int ow, ow_start, ow_stop;
    int oh, oh_start, oh_stop;

    if (_ref->search_x == 0)
    {
        _ref->search_x = width;
        _ref->found_x = 0;
    }

    if (_ref->search_y == 0)
    {
        _ref->search_y = height;
        _ref->found_y = 0;
    }

    ow_start = _ref->target_x - _ref->search_x;
    ow_start = std::max(ow_start, 0);
    ow_stop = _ref->target_x + _ref->search_x;
    if ((ow_stop + tpl_width) > width)
        ow_stop = width - tpl_width;
    ow = ow_stop - ow_start + 1;

    oh_start = _ref->target_y - _ref->search_y;
    oh_start = std::max(oh_start, 0);
    oh_stop = _ref->target_y + _ref->search_y;
    if ((oh_stop + tpl_height) > height)
        oh_stop = height - tpl_height;
    oh = oh_stop - oh_start + 1;

    if (_ref->alignment_algo == 2 && _ref->fastalg_x > 0 && _ref->fastalg_y > 0 && !_noFast) {
        //ESP_LOGD(TAG, "FindTemplate - use FASTALGO");
        bool isSimilar = CalculateSimularities(_ref);
        #ifdef DEBUG_DETAIL_ON
            std::string zw = "\t" + _ref->image_file + "\t x_y: \t" + std::to_string(_ref->fastalg_x) + "\t" + std::to_string(_ref->fastalg_y);
            //LogFile.WriteToDedicatedFile("/sdcard/alignment.txt", zw);
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Fast Algo: " + zw);
        #endif

        if (isSimilar)
        {
            _ref->found_x = _ref->fastalg_x;
            _ref->found_y = _ref->fastalg_y;

            //free_psram_heap(std::string(TAG) + _ref->image_file, rgb_template);  // Keep alignment image refImage in RAM
            return true;
        }
    }

    int SADsum = 0;
    int SADmin = tpl_width * tpl_height * 255;
    int _anzchannels;

    if (_ref->alignment_algo == 0 || _ref->alignment_algo == 2) { // 0 = "Default" (only R-channel) OR 2 = FAST ALGO (only if already processed but no match)
        //ESP_LOGD(TAG, "FindTemplate - DEFAULT - only R channel");
        _anzchannels = 1;
    }
    else {
        //ESP_LOGD(TAG, "FindTemplate - HIGH ACCURACY - process all channels");
        _anzchannels = channels;
    }

    //ESP_LOGD(TAG, "FindTemplate - Process STANDARD algo");
    for (int xouter = ow_start; xouter <= ow_stop; xouter++)
    {
        for (int youter = oh_start; youter <= oh_stop; ++youter)
        {
            SADsum = 0;
            for (int tpl_x = 0; tpl_x < tpl_width; tpl_x++)
            {
                for (int tpl_y = 0; tpl_y < tpl_height; tpl_y++)
                {
                    stbi_uc* p_org = rgb_image + (channels * ((youter + tpl_y) * width + (xouter + tpl_x)));
                    stbi_uc* p_tpl = rgb_template + (channels * (tpl_y * tpl_width + tpl_x));
                    for (int _ch = 0; _ch < _anzchannels; ++_ch)
                    {
                        SADsum += labs(p_tpl[_ch] - p_org[_ch]);
                    }
                }
            }

            if (SADsum < SADmin) {
                SADmin = SADsum;
                _ref->found_x = xouter;
                _ref->found_y = youter;
            }
        }
    }

    // Save actual values to be prepared if switch to FAST ALGO
    _ref->fastalg_x = _ref->found_x;
    _ref->fastalg_y = _ref->found_y;

    // Print results
    std::string zw = "SADsum:" + std::to_string(SADsum) + ", X:"+ std::to_string(_ref->found_x) + ", Y:" + std::to_string(_ref->found_y);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "STANDARD Algo results: " + zw);

    //free_psram_heap(std::string(TAG) + _ref->image_file, rgb_template);   // Keep alignment image refImage in RAM
    return false;
}



bool IRAM_ATTR CFindTemplate::CalculateSimularities(strRefInfo* _ref)
{
    int anz = 0;
    long SADsum = 0;
    int SADNorm;

    for (int xouter = 0; xouter <= tpl_width; xouter++)
    {
        for (int youter = 0; youter <= tpl_height; ++youter)
        {
            stbi_uc* p_org = rgb_image + (channels * ((youter + _ref->fastalg_y) * width + (xouter + _ref->fastalg_x)));
            stbi_uc* p_tpl = rgb_template + (channels * (youter * tpl_width + xouter));
            for (int _ch = 0; _ch < channels; ++_ch)
            {
                SADsum += labs(p_tpl[_ch] - p_org[_ch]);
                anz++;
            }
        }
    }

    // normalize by number of sums
    SADNorm = SADsum/anz;

    // Print results
    std::string zw = "SADThreshold:" + std::to_string(_ref->fastalg_SADThreshold) + ", SADNorm:" + std::to_string(SADNorm) +
                     ", X:" + std::to_string(_ref->fastalg_x) + ", Y:" + std::to_string(_ref->fastalg_y);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "FAST Algo results: " + zw);

    // Evaluate results
    if (SADNorm <= _ref->fastalg_SADThreshold) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "FAST Algo: Match found");
        return true;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "FAST Algo: No match (SADNorm>SADThreshold) -> Use STANDARD Algo");
        return false;
    }
}
