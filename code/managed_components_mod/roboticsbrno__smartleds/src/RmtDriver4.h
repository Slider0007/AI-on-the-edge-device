#pragma once

#include "RmtDriver.h"

#if !SMARTLEDS_NEW_RMT_DRIVER
#include "Color.h"
#include <driver/rmt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace SmartLeds::detail {

constexpr const int CHANNEL_COUNT = RMT_CHANNEL_MAX;

class RmtDriver {
public:
    RmtDriver(const LedType& timing, int count, int pin, int channel_num, SemaphoreHandle_t finishedFlag);
    RmtDriver(const RmtDriver&) = delete;

    esp_err_t init();
    esp_err_t registerIsr(bool isFirstRegisteredChannel);
    esp_err_t unregisterIsr();
    esp_err_t transmit(const Rgb* buffer);

private:
    static void IRAM_ATTR txEndCallback(rmt_channel_t channel, void* arg);

    static void IRAM_ATTR translateSample(const void* src, rmt_item32_t* dest, size_t src_size,
        size_t wanted_rmt_items_num, size_t* out_consumed_src_bytes, size_t* out_used_rmt_items);

    const LedType& _timing;
    int _count;
    gpio_num_t _pin;
    SemaphoreHandle_t _finishedFlag;

    rmt_channel_t _channel;
    rmt_item32_t _bitToRmt[2];
    size_t _translatorSourceOffset;
};

} // namespace SmartLeds::detail
#endif // !SMARTLEDS_NEW_RMT_DRIVER
