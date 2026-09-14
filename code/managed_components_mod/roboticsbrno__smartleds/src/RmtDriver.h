#pragma once

#include <esp_system.h>
#include <stdint.h>

#if defined(ESP_IDF_VERSION)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#define SMARTLEDS_NEW_RMT_DRIVER 1
#else
#define SMARTLEDS_NEW_RMT_DRIVER 0
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
#ifndef SOC_RMT_GROUPS
#define SOC_RMT_GROUPS 1
#endif
#ifndef SOC_RMT_CHANNELS_PER_GROUP
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define SOC_RMT_CHANNELS_PER_GROUP 4
#else
#define SOC_RMT_CHANNELS_PER_GROUP 8
#endif
#endif
#ifndef HSPI_HOST
#define HSPI_HOST SPI2_HOST
#endif
#endif

#else
#define SMARTLEDS_NEW_RMT_DRIVER 0
#endif

namespace SmartLeds::detail
{

struct TimingParams {
    uint32_t T0H;
    uint32_t T1H;
    uint32_t T0L;
    uint32_t T1L;
    uint32_t TRS;
};

using LedType = TimingParams;

} // namespace SmartLeds::detail

#if SMARTLEDS_NEW_RMT_DRIVER
#include "RmtDriver5.h"
#else
#include "RmtDriver4.h"
#endif
