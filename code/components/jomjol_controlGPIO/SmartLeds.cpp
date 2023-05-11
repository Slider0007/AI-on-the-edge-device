#include "SmartLeds.h"

IsrCore SmartLed::_interruptCore = CoreCurrent;

SmartLed*& IRAM_ATTR SmartLed::ledForChannel( int channel ) {
    static SmartLed* table[8] = { nullptr };
    assert( channel < 8 );
    return table[ channel ];
}

void IRAM_ATTR SmartLed::txEndCallback(rmt_channel_t channel, void *arg) {
    xSemaphoreGiveFromISR(ledForChannel(channel)->_finishedFlag, nullptr);
}


void IRAM_ATTR SmartLed::translateForRmt(const void *src, rmt_item32_t *dest, size_t src_size,
    size_t wanted_rmt_items_num, size_t *out_consumed_src_bytes, size_t *out_used_rmt_items) {
    SmartLed *self;
    ESP_ERROR_CHECK(rmt_translator_get_context(out_used_rmt_items, (void**)&self));

    const auto& _bitToRmt = self->_bitToRmt;
    const auto src_offset = self->_translatorSourceOffset;

    auto *src_components = (const uint8_t *)src;
    size_t consumed_src_bytes = 0;
    size_t used_rmt_items = 0;

    while (consumed_src_bytes < src_size && used_rmt_items + 7 < wanted_rmt_items_num) {
        uint8_t val = *src_components;

        // each bit, from highest to lowest
        for ( uint8_t j = 0; j != 8; j++, val <<= 1 ) {
            dest->val = _bitToRmt[ val >> 7 ].val;
            ++dest;
        }

        used_rmt_items += 8;
        ++src_components;
        ++consumed_src_bytes;

        // skip alpha byte
        if(((src_offset + consumed_src_bytes) % 4) == 3) {
            ++src_components;
            ++consumed_src_bytes;

            // TRST delay after last pixel in strip
            if(consumed_src_bytes == src_size) {
                (dest-1)->duration1 = self->_timing.TRS / ( detail::RMT_DURATION_NS * detail::DIVIDER );
            }
        }
    }

    self->_translatorSourceOffset = src_offset + consumed_src_bytes;
    *out_consumed_src_bytes = consumed_src_bytes;
    *out_used_rmt_items = used_rmt_items;
}
