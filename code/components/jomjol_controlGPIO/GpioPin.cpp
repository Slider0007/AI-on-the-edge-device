#include "GpioPin.h"

#include <functional>
#include "freertos/queue.h"
#include "cJSON.h"

#include "GpioControl.h"
#include "ClassLogFile.h"
#include "Helper.h"
#include "MainFlowControl.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif //ENABLE_MQTT


static const char *TAG = "GPIOPIN";

extern QueueHandle_t gpio_queue_handle;


//********************************************************************************
// GPIO Pin
//********************************************************************************
GpioPin::GpioPin(gpio_num_t _gpio, const char* _name, gpio_pin_mode_t _mode, gpio_int_type_t _interruptType,
                 int _debounceTime, int _frequency, bool _httpAccess, bool _mqttAccess, std::string _mqttTopic,
                 LedType _LEDType, int _LEDQuantity, Rgb _LEDColor, int _intensityCorrection)
{
    gpioISR.gpio = gpio = _gpio;
    name = _name;
    mode = _mode;
    interruptType = mode == GPIO_PIN_MODE_TRIGGER_CYCLE_START ? GPIO_INTR_ANYEDGE : _interruptType;
    gpioISR.debounceTime = mode == GPIO_PIN_MODE_TRIGGER_CYCLE_START ? 1000 : _debounceTime;
    frequency = _frequency;

    httpAccess = _httpAccess;
    mqttAccess = _mqttAccess;
    mqttTopic = _mqttTopic;

    LEDType = _LEDType;
    LEDQuantity = _LEDQuantity;
    LEDColor = _LEDColor;

    intensityCorrection = _intensityCorrection;
}


GpioPin::~GpioPin()
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Reset GPIO" + std::to_string((int)gpio));

    if (interruptType != GPIO_INTR_DISABLE) {
        gpio_isr_handler_remove(gpio);
    }

    gpio_reset_pin(gpio);

    if (gpio == GPIO_FLASHLIGHT_DEFAULT)
        gpio_set_direction((gpio_num_t)GPIO_FLASHLIGHT_DEFAULT, GPIO_MODE_OUTPUT);
}


static void IRAM_ATTR gpioPinISRHandler(void* arg)
{
    static TickType_t lastInterruptTime = 0;
    TickType_t interruptTime = xTaskGetTickCountFromISR(); // Depending on CONFIG_FREERTOS_HZ (100Hz)

    // If interrupts come faster than debounceTime, assume it's a bounce and ignore
    if (interruptTime - lastInterruptTime > pdMS_TO_TICKS(((struct GpioISR*)arg)->debounceTime)) {
        GpioResult gpioResult;
        gpioResult.gpio = ((struct GpioISR*)arg)->gpio;
        gpioResult.state = gpio_get_level(gpioResult.gpio);
        BaseType_t ContextSwitchRequest = pdFALSE;

        xQueueSendToBackFromISR(gpio_queue_handle, (void*)&gpioResult, &ContextSwitchRequest);

        if (ContextSwitchRequest)
            taskYIELD();
    }

    lastInterruptTime = interruptTime;
}


void GpioPin::init()
{
    gpio_config_t io_conf;

    //set interrupt
    io_conf.intr_type = mode == GPIO_PIN_MODE_INPUT || mode == GPIO_PIN_MODE_INPUT_PULLUP ||
                        mode == GPIO_PIN_MODE_INPUT_PULLDOWN || mode == GPIO_PIN_MODE_TRIGGER_CYCLE_START ?
                                                                        interruptType : GPIO_INTR_DISABLE;

    //set input / output mode
    io_conf.mode = mode == GPIO_PIN_MODE_OUTPUT || mode == GPIO_PIN_MODE_OUTPUT_PWM ||
                   mode == GPIO_PIN_MODE_FLASHLIGHT_PWM || mode == GPIO_PIN_MODE_FLASHLIGHT_SMARTLED ||
                   mode == GPIO_PIN_MODE_FLASHLIGHT_DIGITAL ? gpio_mode_t::GPIO_MODE_OUTPUT : gpio_mode_t::GPIO_MODE_INPUT;

    //bit mask of the pins that you want to set, e.g. GPIO12
    io_conf.pin_bit_mask = (1ULL << gpio);

    //set pull-down mode
    io_conf.pull_down_en = mode == GPIO_PIN_MODE_INPUT_PULLDOWN ?
                            gpio_pulldown_t::GPIO_PULLDOWN_ENABLE : gpio_pulldown_t::GPIO_PULLDOWN_DISABLE;

    //set pull-up mode
    io_conf.pull_up_en = mode == GPIO_PIN_MODE_INPUT_PULLUP || mode == GPIO_PIN_MODE_TRIGGER_CYCLE_START ?
                            gpio_pullup_t::GPIO_PULLUP_ENABLE : gpio_pullup_t::GPIO_PULLUP_DISABLE;

    //configure GPIO with the given settings
    gpio_config(&io_conf);

    if (io_conf.intr_type != GPIO_INTR_DISABLE) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Register ISR handler for GPIO" + std::to_string((int)gpioISR.gpio) +
                                                ", Debounce time: " + std::to_string(gpioISR.debounceTime));
        gpio_isr_handler_add(gpio, gpioPinISRHandler, (void*)&gpioISR); // Hook ISR handler for specific gpio pin
    }

    pinState = (io_conf.pull_up_en == gpio_pullup_t::GPIO_PULLUP_ENABLE) ? 1 : 0;

    #ifdef ENABLE_MQTT
    if (mqttAccess && (mode == GPIO_PIN_MODE_OUTPUT || mode == GPIO_PIN_MODE_OUTPUT_PWM)) {
        // Subcribe to [mainTopic]/device/gpio/[GpioName]/ctrl
        std::function<bool(std::string, char*, int)> func = std::bind(&GpioPin::mqttControlPinState, this, std::placeholders::_1,
                                                                        std::placeholders::_2, std::placeholders::_3);
        MQTTregisterSubscribeFunction(mqttTopic + "/ctrl", func);
    }
    #endif //ENABLE_MQTT
}


void GpioPin::updatePinState(int _state)
{
    int newState = _state;
    if (_state == -1) // If no pin state provided, read pin state
        newState = gpio_get_level(gpio);

    if (newState != pinState) {
        pinState = newState;

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "updatePinState: GPIO" + std::to_string((int)gpio) +
                    ", State: " + std::to_string(pinState));

        if (mode == GPIO_PIN_MODE_TRIGGER_CYCLE_START && pinState == 0) // Pullup enabled, trigger with falling edge / low level
            triggerFlowStartByGpio();

        #ifdef ENABLE_MQTT
        mqttPublishPinState();
        #endif
    }
}


esp_err_t GpioPin::setPinState(bool _value, gpio_set_source _setSource)
{
    if (mode != GPIO_PIN_MODE_OUTPUT && mode != GPIO_PIN_MODE_FLASHLIGHT_DIGITAL) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    pinState = _value;
    esp_err_t retVal = gpio_set_level(gpio, _value);

    #ifdef ENABLE_MQTT
    mqttPublishPinState();
    #endif //ENABLE_MQTT

    return retVal;
}


esp_err_t GpioPin::setPinState(bool _value, int _intensity, gpio_set_source _setSource)
{
    if (mode != GPIO_PIN_MODE_OUTPUT_PWM && mode != GPIO_PIN_MODE_FLASHLIGHT_PWM) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    pinState = _value;

    if (_value) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ledcChannel, _intensity);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, ledcChannel); // Apply the new value
    }
    else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ledcChannel, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, ledcChannel); // Apply the new value
    }

    #ifdef ENABLE_MQTT
    mqttPublishPinState(_intensity);
    #endif //ENABLE_MQTT

    return ESP_OK;
}


int GpioPin::getPinState()
{
    return pinState;
}


#ifdef ENABLE_MQTT
bool GpioPin::mqttPublishPinState(int _pwmDuty)
{
    if (mqttAccess) {
        cJSON *cJSONObject = cJSON_CreateObject();
        if (cJSONObject == NULL) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
            return false;
        }

        bool retVal = true;

        if (cJSON_AddNumberToObject(cJSONObject, "state", pinState) == NULL)
            retVal = false;

        if (mode == GPIO_PIN_MODE_OUTPUT_PWM || mode == GPIO_PIN_MODE_FLASHLIGHT_PWM) {
            if (cJSON_AddNumberToObject(cJSONObject, "pwm_duty", _pwmDuty) == NULL)
                retVal = false;
        }

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 256, 1); // Print to predefined buffer, avoid dynamic allocations
        std::string jsonData = std::string(jsonString);
        cJSON_free(jsonString);
        cJSON_Delete(cJSONObject);

        retVal &= MQTTPublish(mqttTopic + "/state", jsonData, 1);

        if (!retVal) {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "GPIO" + std::to_string((int)gpio) + ": Failed to publish state to MQTT broker");
            return false;
        }
    }
    return true;
}


bool GpioPin::mqttControlPinState(std::string _topic, char* _data, int _data_len)
{
    //ESP_LOGI(TAG, "mqttControlPinStateHandler: topic %s, data %.*s", _topic.c_str(), _data_len, _data);
    //example: {"state": 1, "pwm_duty": 1024}

    if (_data_len == 0) {    // Check if data length > 0
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "mqttControlPinStateHandler: Handler called, but no data received");
        return false;
    }

    cJSON *jsonData = cJSON_Parse(_data);
    cJSON *pinState = cJSON_GetObjectItemCaseSensitive(jsonData, "state");

    if (mode == GPIO_PIN_MODE_OUTPUT) {
        if (cJSON_IsNumber(pinState)) {    // Check if pinState is a number
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "mqttControlPinStateHandler: state: " + std::to_string(pinState->valueint));
            esp_err_t retVal = setPinState(pinState->valueint, GPIO_SET_SOURCE_MQTT);
            if (retVal == ESP_OK) {
                cJSON_Delete(jsonData);
                return true;
            }
            else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mqttControlPinStateHandler: GPIO" + std::to_string((int)gpio) +
                        " failed to set state | Error: " + intToHexString(retVal));
            }
        }
        else {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "mqttControlPinStateHandler: state not a valid number (\"state\": 1)");
        }
    }
    else if (mode == GPIO_PIN_MODE_OUTPUT_PWM) {
        if (cJSON_IsNumber(pinState)) {    // Check if pinState is a number
            cJSON *pwmDuty = cJSON_GetObjectItemCaseSensitive(jsonData, "pwm_duty");
            if (cJSON_IsNumber(pwmDuty)) {   // Check if pwmDuty is a number
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "mqttControlPinStateHandler: state: " + std::to_string(pinState->valueint) +
                                                                                ", pwm_duty: " + std::to_string(pwmDuty->valueint));
                esp_err_t retVal = setPinState(pinState->valueint, pwmDuty->valueint, GPIO_SET_SOURCE_MQTT);
                if (retVal == ESP_OK) {
                    cJSON_Delete(jsonData);
                    return true;
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mqttControlPinStateHandler: GPIO" + std::to_string((int)gpio) +
                                        " failed to set state | Error: " + intToHexString(retVal));
                }
            }
            else {
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "mqttControlPinStateHandler: pwm_duty not a valid number (\"pwm_duty\": 1024)");
            }
        }
        else {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "mqttControlPinStateHandler: state not a valid number (\"state\": 1)");
        }

    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "mqttControlPinStateHandler: Wrong pin mode, GPIO cannot be controlled)");
    }

    cJSON_Delete(jsonData);
    return false;
}
#endif //ENABLE_MQTT
