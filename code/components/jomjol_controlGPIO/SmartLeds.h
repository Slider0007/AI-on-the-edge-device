#pragma once

/*
 * A C++ driver for the WS2812 LEDs using the RMT peripheral on the ESP32.
 *
 * Jan "yaqwsx" Mrázek <email@honzamrazek.cz>
 *
 * Based on the work by Martin F. Falatic - https://github.com/FozzTexx/ws2812-demo
 */

/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <memory>
#include <cassert>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_intr_alloc.h>
#include <esp_ipc.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include <driver/rmt.h>

#include "Color.h"

namespace detail {

struct TimingParams {
    uint32_t T0H;
    uint32_t T1H;
    uint32_t T0L;
    uint32_t T1L;
    uint32_t TRS;
};

static const int DIVIDER = 4; // 8 still seems to work, but timings become marginal
static const double RMT_DURATION_NS = 12.5; // minimum time of a single RMT duration based on clock ns

} // namespace detail

using LedType = detail::TimingParams;

static const LedType LED_WS2812  = { 350, 700, 800, 600, 50000 };
static const LedType LED_WS2812B = { 400, 850, 850, 400, 50100 };
static const LedType LED_SK6812  = { 300, 600, 900, 600, 80000 };
static const LedType LED_WS2813  = { 350, 800, 350, 350, 300000 };

// Single buffer == can't touch the Rgbs between show() and wait()
enum BufferType { SingleBuffer = 0, DoubleBuffer };

enum IsrCore { CoreFirst = 0, CoreSecond = 1, CoreCurrent = 2};

class SmartLed {
public:
    // The RMT interrupt must not run on the same core as WiFi interrupts, otherwise SmartLeds
    // can't fill the RMT buffer fast enough, resulting in rendering artifacts.
    // Usually, that means you have to set isrCore == CoreSecond.
    //
    // If you use anything other than CoreCurrent, the FreeRTOS scheduler MUST be already running,
    // so you can't use it if you define SmartLed as global variable.
    SmartLed(const LedType &type, int count, int pin, int channel = 0, BufferType doubleBuffer = DoubleBuffer, IsrCore isrCore = CoreCurrent)
        : _timing(type),
          _channel((rmt_channel_t)channel),
          _count(count),
          _firstBuffer(new Rgb[count]),
          _secondBuffer( doubleBuffer ? new Rgb[ count ] : nullptr ),
          _finishedFlag(xSemaphoreCreateBinary())
    {
        assert( channel >= 0 && channel < RMT_CHANNEL_MAX );
        assert( ledForChannel( channel ) == nullptr );

        xSemaphoreGive( _finishedFlag );

        _bitToRmt[ 0 ].level0 = 1;
        _bitToRmt[ 0 ].level1 = 0;
        _bitToRmt[ 0 ].duration0 = _timing.T0H / ( detail::RMT_DURATION_NS * detail::DIVIDER );
        _bitToRmt[ 0 ].duration1 = _timing.T0L / ( detail::RMT_DURATION_NS * detail::DIVIDER );

        _bitToRmt[ 1 ].level0 = 1;
        _bitToRmt[ 1 ].level1 = 0;
        _bitToRmt[ 1 ].duration0 = _timing.T1H / ( detail::RMT_DURATION_NS * detail::DIVIDER );
        _bitToRmt[ 1 ].duration1 = _timing.T1L / ( detail::RMT_DURATION_NS * detail::DIVIDER );

        rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)pin, _channel);
        config.rmt_mode = RMT_MODE_TX;
        config.clk_div = detail::DIVIDER;
        config.mem_block_num = 1;
        config.tx_config.idle_output_en = true;

        ESP_ERROR_CHECK(rmt_config(&config));

        const auto anyAliveCached = anyAlive();
        if (!anyAliveCached && isrCore != CoreCurrent) {
            _interruptCore = isrCore;
            ESP_ERROR_CHECK(esp_ipc_call_blocking(isrCore, registerInterrupt, (void*)((intptr_t)_channel)));
        } else {
            registerInterrupt((void*)((intptr_t)_channel));
        }

        if(!anyAliveCached) {
            rmt_register_tx_end_callback(txEndCallback, NULL);
        }

        ESP_ERROR_CHECK(rmt_translator_init(_channel, translateForRmt));
        ESP_ERROR_CHECK(rmt_translator_set_context(_channel, this));

        ledForChannel( channel ) = this;
    }

    ~SmartLed() {
        ledForChannel( _channel ) = nullptr;
        if ( !anyAlive() && _interruptCore != CoreCurrent ) {
            ESP_ERROR_CHECK(esp_ipc_call_blocking(_interruptCore, unregisterInterrupt, (void*)((intptr_t)_channel)));
        } else {
            unregisterInterrupt((void*)((intptr_t)_channel));
        }
        vSemaphoreDelete( _finishedFlag );
    }

    Rgb& operator[]( int idx ) {
        return _firstBuffer[ idx ];
    }

    const Rgb& operator[]( int idx ) const {
        return _firstBuffer[ idx ];
    }

    esp_err_t show() {
        esp_err_t err = startTransmission();
        swapBuffers();
        return err;
    }

    bool wait( TickType_t timeout = portMAX_DELAY ) {
        if( xSemaphoreTake( _finishedFlag, timeout ) == pdTRUE ) {
            xSemaphoreGive( _finishedFlag );
            return true;
        }
        return false;
    }

    int size() const {
        return _count;
    }

    Rgb *begin() { return _firstBuffer.get(); }
    const Rgb *begin() const { return _firstBuffer.get(); }
    const Rgb *cbegin() const { return _firstBuffer.get(); }

    Rgb *end() { return _firstBuffer.get() + _count; }
    const Rgb *end() const { return _firstBuffer.get() + _count; }
    const Rgb *cend() const { return _firstBuffer.get() + _count; }

private:
    static IsrCore _interruptCore;

    static void registerInterrupt(void *channelVoid) {
        ESP_ERROR_CHECK(rmt_driver_install((rmt_channel_t)((intptr_t)channelVoid), 0, ESP_INTR_FLAG_IRAM));
    }

    static void unregisterInterrupt(void *channelVoid) {
        ESP_ERROR_CHECK(rmt_driver_uninstall((rmt_channel_t)((intptr_t)channelVoid)));
    }

    static SmartLed*& IRAM_ATTR ledForChannel( int channel );

    static bool anyAlive() {
        for ( int i = 0; i != 8; i++ )
            if ( ledForChannel( i ) != nullptr ) return true;
        return false;
    }

    static void IRAM_ATTR txEndCallback(rmt_channel_t channel, void *arg);

    static void IRAM_ATTR translateForRmt(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_rmt_items_num, size_t *out_consumed_src_bytes, size_t *out_used_rmt_items);

    void swapBuffers() {
        if (_secondBuffer)
            _firstBuffer.swap(_secondBuffer);
    }

    esp_err_t startTransmission() {
        // Invalid use of the library, you must wait() fir previous frame to get processed first
        if( xSemaphoreTake( _finishedFlag, 0 ) != pdTRUE )
            abort();

        _translatorSourceOffset = 0;

        auto err = rmt_write_sample(_channel, (const uint8_t *)_firstBuffer.get(), _count * 4, false);
        if(err != ESP_OK) {
            return err;
        }

        return ESP_OK;
    }

    const LedType& _timing;
    rmt_channel_t _channel;
    rmt_item32_t _bitToRmt[ 2 ];
    int _count;
    std::unique_ptr<Rgb[]> _firstBuffer;
    std::unique_ptr<Rgb[]> _secondBuffer;

    size_t _translatorSourceOffset;

    SemaphoreHandle_t _finishedFlag;
};

#ifdef CONFIG_IDF_TARGET_ESP32
#define _SMARTLEDS_SPI_HOST HSPI_HOST
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define _SMARTLEDS_SPI_HOST SPI2_HOST
#else
#error "SmartLeds SPI host not defined for this chip/esp-idf version."
#endif

class Apa102 {
public:
    struct ApaRgb {
        ApaRgb( uint8_t r = 0, uint8_t g = 0, uint32_t b = 0, uint32_t v = 0xFF )
            : v( 0xE0 | v ), b( b ), g( g ), r( r )
        {}

        ApaRgb& operator=( const Rgb& o ) {
            r = o.r;
            g = o.g;
            b = o.b;
            return *this;
        }

        ApaRgb& operator=( const Hsv& o ) {
            *this = Rgb{ o };
            return *this;
        }

        uint8_t v, b, g, r;
    };

    static const int FINAL_FRAME_SIZE = 4;
    static const int TRANS_COUNT = 2 + 8;

    Apa102( int count, int clkpin, int datapin, BufferType doubleBuffer = SingleBuffer )
        : _count( count ),
          _firstBuffer( new ApaRgb[ count ] ),
          _secondBuffer( doubleBuffer ? new ApaRgb[ count ] : nullptr ),
          _initFrame( 0 )
    {
        spi_bus_config_t buscfg;
        memset( &buscfg, 0, sizeof( buscfg ) );
        buscfg.mosi_io_num = datapin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = clkpin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 65535;

        spi_device_interface_config_t devcfg;
        memset( &devcfg, 0, sizeof( devcfg ) );
        devcfg.clock_speed_hz = 1000000;
        devcfg.mode = 0;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = TRANS_COUNT;
        devcfg.pre_cb = nullptr;

        auto ret = spi_bus_initialize( _SMARTLEDS_SPI_HOST, &buscfg, 1 );
        assert( ret == ESP_OK );

        ret = spi_bus_add_device( _SMARTLEDS_SPI_HOST, &devcfg, &_spi );
        assert( ret == ESP_OK );

        std::fill_n( _finalFrame, FINAL_FRAME_SIZE, 0xFFFFFFFF );
    }

    ~Apa102() {
        // ToDo
    }

    ApaRgb& operator[]( int idx ) {
        return _firstBuffer[ idx ];
    }

    const ApaRgb& operator[]( int idx ) const {
        return _firstBuffer[ idx ];
    }

    void show() {
        _buffer = _firstBuffer.get();
        startTransmission();
        swapBuffers();
    }

    void wait() {
        for ( int i = 0; i != _transCount; i++ ) {
            spi_transaction_t *t;
            spi_device_get_trans_result( _spi, &t, portMAX_DELAY );
        }
    }
private:
    void swapBuffers() {
        if ( _secondBuffer )
            _firstBuffer.swap( _secondBuffer );
    }

    void startTransmission() {
        for ( int i = 0; i != TRANS_COUNT; i++ ) {
            _transactions[ i ].cmd = 0;
            _transactions[ i ].addr = 0;
            _transactions[ i ].flags = 0;
            _transactions[ i ].rxlength = 0;
            _transactions[ i ].rx_buffer = nullptr;
        }
        // Init frame
        _transactions[ 0 ].length = 32;
        _transactions[ 0 ].tx_buffer = &_initFrame;
        spi_device_queue_trans( _spi, _transactions + 0, portMAX_DELAY );
        // Data
        _transactions[ 1 ].length = 32 * _count;
        _transactions[ 1 ].tx_buffer = _buffer;
        spi_device_queue_trans( _spi, _transactions + 1, portMAX_DELAY );
        _transCount = 2;
        // End frame
        for ( int i = 0; i != 1 + _count / 32 / FINAL_FRAME_SIZE; i++ ) {
            _transactions[ 2 + i ].length = 32 * FINAL_FRAME_SIZE;
            _transactions[ 2 + i ].tx_buffer = _finalFrame;
            spi_device_queue_trans( _spi, _transactions + 2 + i, portMAX_DELAY );
            _transCount++;
        }
    }

    spi_device_handle_t _spi;
    int _count;
    std::unique_ptr< ApaRgb[] > _firstBuffer, _secondBuffer;
    ApaRgb *_buffer;

    spi_transaction_t _transactions[ TRANS_COUNT ];
    int _transCount;

    uint32_t _initFrame;
    uint32_t _finalFrame[ FINAL_FRAME_SIZE ];
};

class LDP8806 {
public:
    struct LDP8806_GRB {

        LDP8806_GRB( uint8_t g_7bit = 0, uint8_t r_7bit = 0, uint32_t b_7bit = 0 )
            : g( g_7bit ), r( r_7bit ), b( b_7bit )
        {
        }

        LDP8806_GRB& operator=( const Rgb& o ) {
            //Convert 8->7bit colour
            r = ( o.r * 127 / 256 ) | 0x80;
            g = ( o.g * 127 / 256 ) | 0x80;
            b = ( o.b * 127 / 256 ) | 0x80;
            return *this;
        }

        LDP8806_GRB& operator=( const Hsv& o ) {
            *this = Rgb{ o };
            return *this;
        }

        uint8_t g, r, b;
    };

    static const int LED_FRAME_SIZE_BYTES = sizeof( LDP8806_GRB );
    static const int LATCH_FRAME_SIZE_BYTES = 3;
    static const int TRANS_COUNT_MAX = 20;//Arbitrary, supports up to 600 LED

    LDP8806( int count, int clkpin, int datapin, BufferType doubleBuffer = SingleBuffer, uint32_t clock_speed_hz = 2000000 )
        : _count( count ),
          _firstBuffer( new LDP8806_GRB[ count ] ),
          _secondBuffer( doubleBuffer ? new LDP8806_GRB[ count ] : nullptr ),
          // one 'latch'/start-of-data mark frame for every 32 leds
          _latchFrames( ( count + 31 ) / 32 )
    {
        spi_bus_config_t buscfg;
        memset( &buscfg, 0, sizeof( buscfg ) );
        buscfg.mosi_io_num = datapin;
        buscfg.miso_io_num = -1;
        buscfg.sclk_io_num = clkpin;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 65535;

        spi_device_interface_config_t devcfg;
        memset( &devcfg, 0, sizeof( devcfg ) );
        devcfg.clock_speed_hz = clock_speed_hz;
        devcfg.mode = 0;
        devcfg.spics_io_num = -1;
        devcfg.queue_size = TRANS_COUNT_MAX;
        devcfg.pre_cb = nullptr;

        auto ret = spi_bus_initialize( _SMARTLEDS_SPI_HOST, &buscfg, 1 );
        assert( ret == ESP_OK );

        ret = spi_bus_add_device( _SMARTLEDS_SPI_HOST, &devcfg, &_spi );
        assert( ret == ESP_OK );

        std::fill_n( _latchBuffer, LATCH_FRAME_SIZE_BYTES, 0x0 );
    }

    ~LDP8806() {
        // noop
    }

    LDP8806_GRB& operator[]( int idx ) {
        return _firstBuffer[ idx ];
    }

    const LDP8806_GRB& operator[]( int idx ) const {
        return _firstBuffer[ idx ];
    }

    void show() {
        _buffer = _firstBuffer.get();
        startTransmission();
        swapBuffers();
    }

    void wait() {
        while ( _transCount-- ) {
            spi_transaction_t *t;
            spi_device_get_trans_result( _spi, &t, portMAX_DELAY );
        }
    }
private:
    void swapBuffers() {
        if ( _secondBuffer )
            _firstBuffer.swap( _secondBuffer );
    }

    void startTransmission() {
        _transCount = 0;
        for ( int i = 0; i != TRANS_COUNT_MAX; i++ ) {
            _transactions[ i ].cmd = 0;
            _transactions[ i ].addr = 0;
            _transactions[ i ].flags = 0;
            _transactions[ i ].rxlength = 0;
            _transactions[ i ].rx_buffer = nullptr;
        }
        // LED Data
        _transactions[ 0 ].length = ( LED_FRAME_SIZE_BYTES * 8 ) * _count;
        _transactions[ 0 ].tx_buffer = _buffer;
        spi_device_queue_trans( _spi, _transactions + _transCount, portMAX_DELAY );
        _transCount++;

        // 'latch'/start-of-data marker frames
        for ( int i = 0; i < _latchFrames; i++ ) {
            _transactions[ _transCount ].length = ( LATCH_FRAME_SIZE_BYTES * 8 );
            _transactions[ _transCount ].tx_buffer = _latchBuffer;
            spi_device_queue_trans( _spi, _transactions + _transCount, portMAX_DELAY );
            _transCount++;
        }
    }

    spi_device_handle_t _spi;
    int _count;
    std::unique_ptr< LDP8806_GRB[] > _firstBuffer, _secondBuffer;
    LDP8806_GRB *_buffer;

    spi_transaction_t _transactions[ TRANS_COUNT_MAX ];
    int _transCount;

    int _latchFrames;
    uint8_t _latchBuffer[ LATCH_FRAME_SIZE_BYTES ];
};
