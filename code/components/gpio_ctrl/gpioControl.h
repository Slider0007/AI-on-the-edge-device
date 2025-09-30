#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "../../include/defines.h"

#include <esp_log.h>
#include <esp_http_server.h>
#include <map>
#include <hal/gpio_types.h>
#include <esp_rom_gpio.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

#include "configClass.h"
#include "gpioPin.h"


static const gpio_num_t gpio_spare[]{GPIO_SPARE_1, GPIO_SPARE_2, GPIO_SPARE_3, GPIO_SPARE_4, GPIO_SPARE_5, GPIO_SPARE_6};
static const char *gpio_spare_usage[]{GPIO_SPARE_1_USAGE, GPIO_SPARE_2_USAGE, GPIO_SPARE_3_USAGE,
                                      GPIO_SPARE_4_USAGE, GPIO_SPARE_5_USAGE, GPIO_SPARE_6_USAGE};

class GpioHandler
{
  private:
    const CfgData::SectionGpio *cfgDataPtr = NULL;
    httpd_handle_t httpServer = NULL;
    std::map<gpio_num_t, GpioPin *> *gpioMap = NULL;
    TaskHandle_t xHandleTaskGpio = NULL;
    bool gpioHandlerEnabled = false;

    esp_err_t loadParameter();
    void clearData();

    void ledcInitGpio(ledc_timer_t _timer, ledc_channel_t _channel, int _gpioNum, int _frequency);

    std::map<int, ledc_timer_t> frequencyTable;
    int calcDutyResolutionMaxValue(int frequency);
    ledc_timer_bit_t calcDutyResolution(int frequency);
    ledc_timer_t getFreeTimer(int _frequency);

    gpio_num_t resolveSparePinNr(uint8_t _sparePinNr);
    std::string getPinModeDescription(gpio_pin_mode_t _mode);
    gpio_int_type_t resolveIntType(std::string input);
    std::string getPinInterruptDescription(gpio_int_type_t _type);

#ifdef ENABLE_MQTT
    void handleMQTTconnect();
#endif // ENABLE_MQTT

  public:
    GpioHandler();
    ~GpioHandler();
    bool init();
    void deinit();
    bool gpioHandlerIsEnabled() { return gpioHandlerEnabled; };

    void gpioFlashlightControl(bool _state, int _intensity);
    void gpioStatusLedControl(bool _state);

    void gpioPinInterrupt(GpioResult *gpioResult);
    void gpioInputStatePolling();

    gpio_pin_mode_t resolvePinMode(std::string input);

    void registerGpioUri(httpd_handle_t server);
    esp_err_t handleHttpRequest(httpd_req_t *req);
};

esp_err_t callHandleHttpRequest(httpd_req_t *req);

bool initGpioHandler();
void deinitGpioHandler();
void destroyGpioHandler();
GpioHandler *getGpioHandle();

#endif // GPIO_CONTROL_H
