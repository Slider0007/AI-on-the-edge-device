#include "SmartLeds.h"

IsrCore SmartLed::_interruptCore = CoreCurrent;

SmartLed*& IRAM_ATTR SmartLed::ledForChannel(int channel) {
    static SmartLed* table[SmartLeds::detail::CHANNEL_COUNT] = {};
    assert(channel < SmartLeds::detail::CHANNEL_COUNT);
    return table[channel];
}
