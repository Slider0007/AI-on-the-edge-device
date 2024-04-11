#ifndef GPIO_PIN_H
#define GPIO_PIN_H

#include "../../include/defines.h"

#include "hal/gpio_types.h"
#include <string>
#include "driver/ledc.h"
#include "SmartLeds.h"


typedef enum {
    GPIO_PIN_MODE_DISABLED              = 0,
    GPIO_PIN_MODE_INPUT                 = 1,
    GPIO_PIN_MODE_INPUT_PULLUP          = 2,
    GPIO_PIN_MODE_INPUT_PULLDOWN        = 3,
    GPIO_PIN_MODE_OUTPUT                = 4,
    GPIO_PIN_MODE_OUTPUT_PWM            = 5,
    GPIO_PIN_MODE_FLASHLIGHT_PWM        = 6,
    GPIO_PIN_MODE_FLASHLIGHT_SMARTLED   = 7,
    GPIO_PIN_MODE_FLASHLIGHT_DIGITAL    = 8,
    GPIO_PIN_MODE_TRIGGER_CYCLE_START   = 9,
    GPIO_PIN_MODE_MAX                   = 10
} gpio_pin_mode_t;


typedef enum {
    GPIO_SET_SOURCE_INTERNAL = 0,
    GPIO_SET_SOURCE_MQTT     = 1,
    GPIO_SET_SOURCE_HTTP     = 2,
    GPIO_SET_SOURCE_MAX      = 3
} gpio_set_source;


struct GpioResult {
    gpio_num_t gpio;
    int state;
};


struct GpioISR {
    gpio_num_t gpio;
    int debounceTime;
};


class GpioPin
{
    private:
        int pinState = -1;

        gpio_num_t gpio;
        const char* name;
        gpio_pin_mode_t mode;
        gpio_int_type_t interruptType;
        int frequency;
        GpioISR gpioISR;

        bool httpAccess = false;
        bool mqttAccess = false;
        std::string mqttTopic;

        ledc_channel_t ledcChannel = LEDC_CHANNEL_MAX;
        SmartLed *smartLed = NULL;
        LedType LEDType;
        int LEDQuantity;
        Rgb LEDColor;

        int intensityCorrection;
    
    public:
        GpioPin(gpio_num_t _gpio, const char* _name, gpio_pin_mode_t _mode, gpio_int_type_t _interruptType, 
                int _debounceTime, int _frequency, bool _httpAccess, bool _mqttAccess, std::string _mqttTopic, 
                LedType _LEDType, int _LEDQuantity, Rgb _LEDColor, int _intensityCorrection);
        ~GpioPin();
        void init();

        void updatePinState(int state = -1);
        esp_err_t setPinState(bool _state, gpio_set_source _setSource);
        esp_err_t setPinState(bool _state, int _ledIntensity, gpio_set_source _setSource);
        int getPinState();

        #ifdef ENABLE_MQTT
        bool mqttPublishPinState(int _pwmDuty = 0);
        bool mqttControlPinState(std::string _topic, char* data, int data_len);
        #endif

        gpio_num_t getGPIO() { return gpio; };
        gpio_pin_mode_t getMode() { return mode; };
        gpio_int_type_t getInterruptType() { return interruptType; };
        int getFrequency() { return frequency; };
        bool getHttpAccess() { return httpAccess; };

        void setLedcChannel (ledc_channel_t _ledcChannel) { ledcChannel = _ledcChannel; };
        ledc_channel_t getLedcChannel () { return ledcChannel; };
        void setSmartLed(SmartLed* _smartLed) { smartLed = _smartLed; };
        SmartLed* getSmartLed() { return smartLed; };
        LedType getLEDType() { return LEDType; };
        int getLEDQuantity() { return LEDQuantity; };
        Rgb getLEDColor() { return LEDColor; };

        int getIntensityCorrection() { return intensityCorrection; };
};

#endif //GPIO_PIN_H
