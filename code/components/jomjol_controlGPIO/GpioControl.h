#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "../../include/defines.h"

#include <esp_log.h>
#include "hal/gpio_types.h"
#include <esp_http_server.h>
#include <map>
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "GpioPin.h"


class GpioHandler
{
    private:
        std::string configFileName;
        httpd_handle_t httpServer;
        std::map<gpio_num_t, GpioPin*> *gpioMap = NULL;
        TaskHandle_t xHandleTaskGpio = NULL;
        bool isEnabled = false;

        std::map<int, ledc_timer_t> frequencyTable;

        int readConfig();
        void clear();

        gpio_num_t resolveSparePinNr(uint8_t _sparePinNr);
        gpio_pin_mode_t resolvePinMode(std::string input);
        std::string getPinModeDecription(gpio_pin_mode_t _mode);
        gpio_int_type_t resolveIntType(std::string input);
        std::string getPinInterruptDecription(gpio_int_type_t _type);

    public:
        GpioHandler(std::string _configFileName, httpd_handle_t _httpServer);
        ~GpioHandler();
        bool init();
        void deinit();
        void ledcInitGpio(ledc_timer_t _timer, ledc_channel_t _channel, int _gpioNum, int _frequency);
        bool gpioHandlerIsEnabled() { return isEnabled; };

        int calcDutyResultionMaxValue(int frequency);
        ledc_timer_bit_t calcDutyResolution(int frequency);
        ledc_timer_t getFreeTimer(int _frequency);

        void gpioPinInterrupt(GpioResult* gpioResult);
        void gpioInputStatePolling();

        void gpioFlashlightControl(bool _state, int _intensity);

        #ifdef ENABLE_MQTT
            void handleMQTTconnect();
        #endif //ENABLE_MQTT

        void registerGpioUri();
        esp_err_t handleHttpRequest(httpd_req_t *req);
};

esp_err_t callHandleHttpRequest(httpd_req_t *req);

void gpio_handler_create(httpd_handle_t server);
bool gpio_handler_init();
void gpio_handler_deinit();
void gpio_handler_destroy();
GpioHandler* gpio_handler_get();

#endif //GPIO_CONTROL_H
