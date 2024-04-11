#include "GpioControl.h"

#include <string>
#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_log.h"
#include "cJSON.h"

#include <sys/stat.h>
#include <vector>

#include "ClassLogFile.h"
#include "configFile.h"
#include "Helper.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#include "server_mqtt.h"
#endif //ENABLE_MQTT


static const char *TAG = "GPIOCTRL";

static GpioHandler *gpioHandler = NULL;
QueueHandle_t gpio_queue_handle = NULL;

static const gpio_num_t gpio_spare[] {GPIO_SPARE_1, GPIO_SPARE_2, GPIO_SPARE_3, GPIO_SPARE_4, GPIO_SPARE_5, GPIO_SPARE_6};
static const char* gpio_spare_usage[] {GPIO_SPARE_1_USAGE, GPIO_SPARE_2_USAGE, GPIO_SPARE_3_USAGE, 
                                        GPIO_SPARE_4_USAGE, GPIO_SPARE_5_USAGE, GPIO_SPARE_6_USAGE};


GpioHandler::GpioHandler(std::string _configFileName, httpd_handle_t _httpServer) 
{
    configFileName = _configFileName;
    httpServer = _httpServer;

    registerGpioUri();
}


GpioHandler::~GpioHandler()
{
    gpioFlashlightControl(false, 0); // Flashlight off

    if (gpioMap != NULL) {
        clear();
        delete gpioMap;
    }
}


void GpioHandler::gpioPinInterrupt(GpioResult* gpioResult)
{
    if ((gpioMap != NULL) && (gpioMap->find(gpioResult->gpio) != gpioMap->end())) {
        (*gpioMap)[gpioResult->gpio]->updatePinState(gpioResult->state);
    }
}


static void gpioHandlerTask(void *arg)
{
    while(1) {
        if(uxQueueMessagesWaiting(gpio_queue_handle)) {
            while(uxQueueMessagesWaiting(gpio_queue_handle)) {
                GpioResult gpioResult;
                xQueueReceive(gpio_queue_handle, (void*)&gpioResult, 5);
                //LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Pin interrupt: GPIO" + std::to_string((int)gpioResult.gpio) + 
                //                    ", State: " + std::to_string(gpioResult.state));
                ((GpioHandler*)arg)->gpioPinInterrupt(&gpioResult);
            }  
        }

        ((GpioHandler*)arg)->gpioInputStatePolling();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void GpioHandler::gpioInputStatePolling()
{
    if (gpioMap != NULL) {
        for(std::map<gpio_num_t, GpioPin*>::iterator it = gpioMap->begin(); it != gpioMap->end(); ++it) {
            if (it->second->getMode() == GPIO_PIN_MODE_INPUT || it->second->getMode() == GPIO_PIN_MODE_INPUT_PULLUP || 
                it->second->getMode() == GPIO_PIN_MODE_INPUT_PULLDOWN || it->second->getMode() == GPIO_PIN_MODE_TRIGGER_CYCLE_START)
            {
                it->second->updatePinState();
            }
        }
    }
}


void GpioHandler::ledcInitGpio(ledc_timer_t _timer, ledc_channel_t _channel, int _gpioNum, int _frequency)
{
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init LEDC timer " + std::to_string((int)_timer) + 
                            ", Frequency: " + std::to_string(_frequency) + 
                            ", Duty Resolution: " + std::to_string((int)calcDutyResolution(_frequency)));
    
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = { };

    ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num        = _timer;
    ledc_timer.duty_resolution  = calcDutyResolution(_frequency);
    ledc_timer.freq_hz          = _frequency;
    ledc_timer.clk_cfg          = LEDC_USE_APB_CLK;

    esp_err_t retVal = ledc_timer_config(&ledc_timer);

    if (retVal != ESP_OK)
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to init LEDC timer " + 
                    std::to_string((int)_timer) + ", Error: " +intToHexString(retVal));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = { };

    ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel        = _channel;
    ledc_channel.timer_sel      = _timer;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num       = _gpioNum;
    ledc_channel.duty           = 0; // Set duty to 0%
    ledc_channel.hpoint         = 0;

    retVal = ledc_channel_config(&ledc_channel);

    if (retVal != ESP_OK)
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to init LEDC channel " + 
                    std::to_string((int)_channel) + ", Error: " +intToHexString(retVal));
}


bool GpioHandler::init()
{
    if (gpioMap == NULL) {
        gpioMap = new std::map<gpio_num_t, GpioPin*>();
    }
    else {
        clear();
    }

    int retVal = readConfig();

    if (retVal == -1) { // Error state
        clear();
        delete gpioMap;
        gpioMap = NULL;
        return false;
    }
    else if (retVal == 0) { // GPIO disabled 
        return true;
    }

    int smartLedChannel = 0; // max. 8 channels
    int ledcChannel = 1; // max 8 channels (CH0: camera, CH1 - CH7: spare)
    bool initHandlerTask = false;

    for(std::map<gpio_num_t, GpioPin*>::iterator it = gpioMap->begin(); it != gpioMap->end(); ++it) {
        it->second->init();

        if (it->second->getMode() == GPIO_PIN_MODE_FLASHLIGHT_SMARTLED) {           
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init SmartLED (Flashlight): GPIO" + std::to_string((int)it->second->getGPIO()));
            it->second->setSmartLed(new SmartLed(it->second->getLEDType(), it->second->getLEDQuantity(), 
                                                 it->second->getGPIO(), smartLedChannel, DoubleBuffer));
            smartLedChannel++;
            if (smartLedChannel == detail::CHANNEL_COUNT) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient SmartLED channels");
                return false;
            }
        }
        else if (it->second->getMode() == GPIO_PIN_MODE_FLASHLIGHT_PWM) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init PWM (Flashlight): GPIO" + std::to_string((int)it->second->getGPIO()));

            ledc_timer_t timer = getFreeTimer(it->second->getFrequency());
            if (timer == LEDC_TIMER_MAX) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient LEDC timer");
                return false;
            }

            ledcInitGpio(timer, (ledc_channel_t)ledcChannel, it->second->getGPIO(), it->second->getFrequency());
            it->second->setLedcChannel(static_cast<ledc_channel_t>(ledcChannel));
            ledcChannel++;
            if (ledcChannel == LEDC_CHANNEL_MAX) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient LEDC channels");
                return false;
            }
        }
        else if (it->second->getMode() == GPIO_PIN_MODE_OUTPUT_PWM) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init PWM (GPIO output): GPIO" + std::to_string((int)it->second->getGPIO()));

            ledc_timer_t timer = getFreeTimer(it->second->getFrequency());
            if (timer == LEDC_TIMER_MAX) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient LEDC timer");
                return false;
            }

            ledcInitGpio(timer, (ledc_channel_t)ledcChannel, it->second->getGPIO(), it->second->getFrequency());
            it->second->setLedcChannel(static_cast<ledc_channel_t>(ledcChannel));
            ledcChannel++;
            if (ledcChannel == LEDC_CHANNEL_MAX) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient LEDC channels");
                return false;
            }
        }

        // Handler task is only needed to maintain input pin state (interrupt or polling)
        if (it->second->getMode() == GPIO_PIN_MODE_INPUT || it->second->getMode() == GPIO_PIN_MODE_INPUT_PULLUP || 
            it->second->getMode() == GPIO_PIN_MODE_INPUT_PULLDOWN || it->second->getMode() == GPIO_PIN_MODE_TRIGGER_CYCLE_START) 
        {
            initHandlerTask = true;
        }

    }

    #ifdef ENABLE_MQTT
        std::function<void()> f = std::bind(&GpioHandler::handleMQTTconnect, this);
        MQTTregisterConnectFunction("gpioHandler", f);
    #endif //ENABLE_MQTT

    // Handler task is only needed to maintain input pin state (interrupt or polling)
    if (initHandlerTask && xHandleTaskGpio == NULL) {
        gpio_queue_handle = xQueueCreate(10, sizeof(GpioResult));
        BaseType_t xReturned = xTaskCreate(&gpioHandlerTask, "gpioHandlerTask", 3 * 1024, (void *)this, 
                                            tskIDLE_PRIORITY + 4, &xHandleTaskGpio);
        
        if (xReturned != pdPASS ) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create gpioHandlerTask");
            return false;
        }
    }

    return true;
}


int GpioHandler::readConfig() 
{
    ConfigFile configFile = ConfigFile(configFileName); 
    std::string line = "";
    bool disabledLine = false;
    bool eof = false;

    #ifdef ENABLE_MQTT
    const std::string gpioMQTTMainTopic = mqttServer_getMainTopic() + "/device/gpio";
    #endif // ENABLE_MQTT

    isEnabled = false;
        
    while ((!configFile.GetNextParagraph(line, disabledLine, eof) || (line.compare("[GPIO]") != 0)) && !eof) {}

    if (disabledLine || eof) {
        #if defined(GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED)
        // Special case: Flashlight default uses SmartLED functionality -> init smartLED functionality only
        for (int i= 0; i < GPIO_SPARE_PIN_COUNT; ++i) {
            if (strcmp(gpio_spare_usage[i], FLASHLIGHT_SMARTLED) == 0) {
                GpioPin* gpioPin = new GpioPin((gpio_num_t)gpio_spare[i], ("gpio" + std::to_string((int)gpio_spare[i])).c_str(), 
                                        GPIO_PIN_MODE_FLASHLIGHT_SMARTLED, GPIO_INTR_DISABLE, 200, 5000, false, false, "", 
                                        GPIO_FLASHLIGHT_DEFAULT_SMARTLED_TYPE, GPIO_FLASHLIGHT_DEFAULT_SMARTLED_QUANTITY, 
                                        Rgb{255,255,255}, 100);
                (*gpioMap)[(gpio_num_t)gpio_spare[i]] = gpioPin;
                return 1;
            }
        }

        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Default flashlight configured as SmartLED, but no valid GPIO config found");
        return -1;
        #endif

        return 0;
    }
    
    std::vector<std::string> splitted;
    bool gpioInstallISR = false;

    while (configFile.getNextLine(&line, disabledLine, eof) && !configFile.isNewParagraph(line)) {
        splitted = ZerlegeZeile(line);

        if (splitted[0].find_first_of("IO") > 0) // Parameter disabled
            continue;

        if (splitted.size() >= 13) { // Parameter enabled and enough parameter detected
            gpio_num_t gpioNr = (gpio_num_t)std::stoi(splitted[0].substr(2, 2).c_str());

            gpio_pin_mode_t pinMode = resolvePinMode(toLower(splitted[1]));
            if (pinMode == GPIO_PIN_MODE_DISABLED) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: Invalid GPIO pin mode");
                return -1;
            }

            gpio_int_type_t intType = resolveIntType(toLower(splitted[2]));
            if (intType != GPIO_INTR_DISABLE) {
                gpioInstallISR = true;
            }

            int debounceTime = std::stoi(splitted[3].c_str());
            if (debounceTime < 0 || debounceTime > 5000) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: Debounce time out of range (0 .. 5000ms)");
                return -1;
            }

            int frequency = std::stoi(splitted[4].c_str());
            if (frequency < 5 || frequency > 1000000) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: Frequency out of range (5Hz .. 1MHz)");
                return -1;
            }

            bool httpAccess = (toUpper(splitted[6]) == "TRUE");

            LedType LEDType = LED_WS2812;
            if (splitted[7] == "WS2812")
                LEDType = LED_WS2812;
            else if (splitted[7] == "WS2812B")
                LEDType = LED_WS2812B;
            else if (splitted[7] == "WS2812B-NEW")
                LEDType = LED_WS2812B_NEWVARIANT;
            else if (splitted[7] == "WS2812B-OLD")
                LEDType = LED_WS2812B_OLDVARIANT;
            else if (splitted[7] == "SK6812")
                LEDType = LED_SK6812;
            else if (splitted[7] == "WS2813")
                LEDType = LED_WS2813;
            else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: Unknown LED type");
                return -1;
            }

            int LEDQuantity = std::stoi(splitted[8]);
            if (LEDQuantity < 1 ) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: LED quantity out of range (> 1)");
                return -1;
            }
            Rgb LEDColor = Rgb{(uint8_t)std::stoi(splitted[9]), (uint8_t)std::stoi(splitted[10]), (uint8_t)std::stoi(splitted[11])};

            int intensityCorrection = std::stoi(splitted[12]);
            if (intensityCorrection < 1 || intensityCorrection > 100) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: LED intensity correction out of range (1 .. 100)");
                return -1;
            }

            char gpioName[32];
            if (splitted.size() >= 14)
                strcpy(gpioName, trim(splitted[13]).c_str());
            else
                sprintf(gpioName, "gpio%d", gpioNr);

            #ifdef ENABLE_MQTT 
                bool mqttAccess = (toUpper(splitted[5]) == "TRUE");           
                std::string mqttTopic = mqttAccess ? (gpioMQTTMainTopic + "/" + gpioName) : "";
            #else
                bool mqttAccess = false;
                std::string mqttTopic = "";
            #endif

            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Pin Config: GPIO" + std::to_string((int)gpioNr) +
                    ", Name: " + std::string(gpioName) + ", Mode: " + splitted[1] + ", Interrupt Type: " + 
                    splitted[2] + ", Debounce Time: " + std::to_string(debounceTime) + ", Frequency: " + 
                    std::to_string(frequency) + ", HTTP Access: " + std::to_string(httpAccess) + 
                    ", MQTT Access: " + std::to_string(mqttAccess) +", MQTT Topic: " + mqttTopic + 
                    ", LED Type: " + splitted[7] + ", LED Quantity: " + std::to_string(LEDQuantity) + 
                    ", LED Color: R:" + std::to_string(LEDColor.r) + " | G:" + std::to_string(LEDColor.g) + 
                    " | B:" + std::to_string(LEDColor.b) + ", LED Intensity Correction: " + 
                    std::to_string(intensityCorrection));
            
            GpioPin* gpioPin = new GpioPin(gpioNr, gpioName, pinMode, intType, debounceTime, frequency, httpAccess, 
                                    mqttAccess, mqttTopic, LEDType, LEDQuantity, LEDColor, intensityCorrection);
            (*gpioMap)[gpioNr] = gpioPin;
        }
        else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "readConfig: " + splitted[0] + ": Parameter set incomplete");
            return -1;
        }
    }

    if (gpioInstallISR)
        gpio_install_isr_service(ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);

    isEnabled = true;
    return 1;
}


void GpioHandler::clear() 
{
    if (gpioMap != NULL) {
        for(std::map<gpio_num_t, GpioPin*>::iterator it = gpioMap->begin(); it != gpioMap->end(); ++it) {
            if (it->second->getSmartLed() != NULL) {
                delete it->second->getSmartLed();
                it->second->setSmartLed(NULL);
            }

            delete it->second;
        }
        gpioMap->clear();
    }
    
    frequencyTable.clear();

    // gpio_uninstall_isr_service(); can't uninstall, ISR service is also used by camera
}


void GpioHandler::deinit()
{
    #ifdef ENABLE_MQTT
    MQTTunregisterConnectFunction("gpioHandler");
    #endif //ENABLE_MQTT

    clear();

    if (xHandleTaskGpio != NULL) {
        vTaskDelete(xHandleTaskGpio);
        xHandleTaskGpio = NULL;
    }
}


void GpioHandler::gpioFlashlightControl(bool _state, int _intensity) 
{
    if (gpioMap == NULL)
        return;

    for (std::map<gpio_num_t, GpioPin*>::iterator it = gpioMap->begin(); it != gpioMap->end(); ++it) {
        if (it->second->getMode() == GPIO_PIN_MODE_FLASHLIGHT_PWM) {
            int dutyResultionMaxValue = calcDutyResultionMaxValue(it->second->getFrequency());
            int intensityValueCorrected = std::min(std::max(0, it->second->getIntensityCorrection() * 
                                                    _intensity * dutyResultionMaxValue / 10000), dutyResultionMaxValue);

            esp_err_t retVal = it->second->setPinState(_state, intensityValueCorrected, GPIO_SET_SOURCE_INTERNAL);

            if (retVal != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight PWM: GPIO" + std::to_string((int)it->first) + 
                    " failed to set state | Error: " + intToHexString(retVal));
                return;
            }
            
            if (_state) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight PWM: GPIO" + std::to_string((int)it->first) + 
                                    ", State: " + std::to_string(_state) + ", Intensity: " + std::to_string(intensityValueCorrected) + 
                                    "/" +  std::to_string(dutyResultionMaxValue));
            }
            else {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight PWM: GPIO" + std::to_string((int)it->first) + 
                                    ", State: " + std::to_string(_state));
            }
        }
        else if (it->second->getMode() == GPIO_PIN_MODE_FLASHLIGHT_SMARTLED) {
            if (_state) {
                int intensityValueCorrected = it->second->getIntensityCorrection() * _intensity * 8191 / 10000;

                Rgb LEDColorIntensityCorrected = it->second->getLEDColor();
                LEDColorIntensityCorrected.r = std::min(std::max(0, (LEDColorIntensityCorrected.r * intensityValueCorrected) / 8191), 255);
                LEDColorIntensityCorrected.g = std::min(std::max(0, (LEDColorIntensityCorrected.g * intensityValueCorrected) / 8191), 255);
                LEDColorIntensityCorrected.b = std::min(std::max(0, (LEDColorIntensityCorrected.b * intensityValueCorrected) / 8191), 255);
  
                for (int i = 0; i < it->second->getLEDQuantity(); ++i) {
                    (*it->second->getSmartLed())[i] = LEDColorIntensityCorrected;
                }

                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight SmartLED: GPIO" + std::to_string((int)it->first) + 
                                    ", State: " + std::to_string(_state) + ", Intensity: " + std::to_string(intensityValueCorrected) + 
                                    "/8191, | R: " + std::to_string(LEDColorIntensityCorrected.r) + 
                                    ", G:" + std::to_string(LEDColorIntensityCorrected.g) + 
                                    ", B:" + std::to_string(LEDColorIntensityCorrected.b));
            }
            else {
                for (int i = 0; i < it->second->getLEDQuantity(); ++i) {
                    (*it->second->getSmartLed())[i] = Rgb{0, 0, 0};
                }

                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight SmartLED: GPIO" + std::to_string((int)it->first) + 
                                        ", State: " + std::to_string(_state));
            }

            esp_err_t retVal = it->second->getSmartLed()->show();
            it->second->updatePinState(_state ? 1 : 0);

            if (retVal != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight SmartLED: GPIO" + std::to_string((int)it->first) + 
                " failed to set state | Error: " + intToHexString(retVal));
            }
        }
        else if (it->second->getMode() == GPIO_PIN_MODE_FLASHLIGHT_DIGITAL) {
            esp_err_t retVal = it->second->setPinState(_state, GPIO_SET_SOURCE_INTERNAL);

            if (retVal != ESP_OK) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight Digital: GPIO" + std::to_string((int)it->first) + 
                    " failed to set state | Error: " + intToHexString(retVal));
                return;
            }

            if (_state) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight Digital: GPIO" + std::to_string((int)it->first) + 
                                    ", State: " + std::to_string(_state));
            }
            else {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flashlight Digital: GPIO" + std::to_string((int)it->first) + 
                                    ", State: " + std::to_string(_state));
            }
        }
    }
}


gpio_num_t GpioHandler::resolveSparePinNr(uint8_t _sparePinNr) 
{
    for (int i = 0; i < GPIO_SPARE_PIN_COUNT; ++i) {
        if (gpio_spare[i] == _sparePinNr)
            return gpio_spare[i];
    }
    return GPIO_NUM_NC;
}


gpio_pin_mode_t GpioHandler::resolvePinMode(std::string input) 
{
    if (input == "disabled")
        return GPIO_PIN_MODE_DISABLED;
    else if (input == "input")
        return GPIO_PIN_MODE_INPUT;
    if (input == "input-pullup")
        return GPIO_PIN_MODE_INPUT_PULLUP;
    else if (input == "input-pulldown")
        return GPIO_PIN_MODE_INPUT_PULLDOWN;
    else if (input == "output")
        return GPIO_PIN_MODE_OUTPUT;
    else if (input == "output-pwm")
        return GPIO_PIN_MODE_OUTPUT_PWM;
    else if (input == FLASHLIGHT_PWM)
        return GPIO_PIN_MODE_FLASHLIGHT_PWM;
    else if (input == FLASHLIGHT_SMARTLED)
        return GPIO_PIN_MODE_FLASHLIGHT_SMARTLED;
    else if (input == FLASHLIGHT_DIGITAL)
        return GPIO_PIN_MODE_FLASHLIGHT_DIGITAL;
    else if (input == FLASHLIGHT_DEFAULT) {
        #if defined(GPIO_FLASHLIGHT_DEFAULT_USE_PWM)
            return GPIO_PIN_MODE_FLASHLIGHT_PWM;
        #elif defined(GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED)
            return GPIO_PIN_MODE_FLASHLIGHT_SMARTLED;
        #else
            return GPIO_PIN_MODE_FLASHLIGHT_DIGITAL;
        #endif
    }
    else if (input == "trigger-cycle-start")
        return GPIO_PIN_MODE_TRIGGER_CYCLE_START;

    return GPIO_PIN_MODE_DISABLED;
}


std::string GpioHandler::getPinModeDecription(gpio_pin_mode_t _mode) 
{
    switch(_mode) {
        case 0:
            return "disabled";
        case 1:
            return "input";
        case 2:
            return "input-pullup";
        case 3:
            return "input-pulldown";
        case 4:
            return "output";
        case 5:
            return "output-pwm";
        case 6:
            return FLASHLIGHT_PWM;
        case 7:
            return FLASHLIGHT_SMARTLED;            
        case 8:
            return FLASHLIGHT_DIGITAL;
        case 9:
            return "trigger-cycle-start";
        default: 
            return "disabled";   
    }
}


gpio_int_type_t GpioHandler::resolveIntType(std::string input) 
{
    if ( input == "cyclic-polling" )
        return GPIO_INTR_DISABLE;
    else if ( input == "interrupt-rising-edge" )
        return GPIO_INTR_POSEDGE;
    else if ( input == "interrupt-falling-edge" )
        return GPIO_INTR_NEGEDGE;
    else if ( input == "interrupt-rising-falling" )
        return GPIO_INTR_ANYEDGE ;
    else if ( input == "interrupt-low-level" )
        return GPIO_INTR_LOW_LEVEL;
    else if ( input == "interrupt-high-level" )
        return GPIO_INTR_HIGH_LEVEL;

    return GPIO_INTR_DISABLE;
}


std::string GpioHandler::getPinInterruptDecription(gpio_int_type_t _type) 
{
    switch(_type) {
        case 0:
            return "cyclic-polling";
        case 1:
            return "interrupt-rising-edge";
        case 2:
            return "interrupt-falling-edge";
        case 3:
            return "interrupt-rising-falling";
        case 4:
            return "interrupt-low-level";
        case 5:
            return "interrupt-high-level";
        default: 
            return "cyclic-polling";   
    }
}


int GpioHandler::calcDutyResultionMaxValue(int frequency)
{
    return ((1 << calcDutyResolution(frequency)) - 1);
}


ledc_timer_bit_t GpioHandler::calcDutyResolution(int frequency)
{
    // Calculate max duty resultion derived from device clock (LEDC_USE_APB_CLK == 80Mhz)
    // Limit max duty resolution to 14 bit (due to compability with ESP32S3)
    return static_cast<ledc_timer_bit_t>(std::min((int)log2(80000000/frequency), 14));
}


ledc_timer_t GpioHandler::getFreeTimer(int _frequency)
{
    auto it = frequencyTable.find(_frequency);

    // Return timer related to already registered frequency
    if (it != frequencyTable.end()) 
        return it->second;
    
    // Insert new frequency and return timer
    if (frequencyTable.size() == 0) {
        frequencyTable.insert({_frequency, LEDC_TIMER_1});
        return LEDC_TIMER_1;
    }
    else if (frequencyTable.size() == 1) {
        frequencyTable.insert({_frequency, LEDC_TIMER_2});
        return LEDC_TIMER_2;
    }
    else if (frequencyTable.size() == 2) {
        frequencyTable.insert({_frequency, LEDC_TIMER_3});
        return LEDC_TIMER_3;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Insufficient LEDC timer");
        return LEDC_TIMER_MAX;
    }
}



// MQTT GPIO state publish
// ***********************************
#ifdef ENABLE_MQTT
void GpioHandler::handleMQTTconnect()
{
    if (gpioMap != NULL) {
        for(std::map<gpio_num_t, GpioPin*>::iterator it = gpioMap->begin(); it != gpioMap->end(); ++it) {
            it->second->mqttPublishPinState();
        }
    }
}
#endif //ENABLE_MQTT


// Handle HTTP GPIO request
// ***********************************
esp_err_t callHandleHttpRequest(httpd_req_t *req)
{
    GpioHandler *gpioHandler = (GpioHandler*)req->user_ctx;
    return gpioHandler->handleHttpRequest(req);
}


esp_err_t GpioHandler::handleHttpRequest(httpd_req_t *req)
{
    char query[200];
    char value[30];
    std::string respStr = "";
    std::string task = "";
    std::string requestedGpioState = "";
    int requestedGpioNum = -1;
    int requestedPWMDuty = -1;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (httpd_req_get_url_query_str(req, query, 200) == ESP_OK) {
        if (httpd_query_key_value(query, "task", value, sizeof(value)) == ESP_OK) {
            task = std::string(value);
        }

        if (httpd_query_key_value(query, "gpio", value, sizeof(value)) == ESP_OK) {
            requestedGpioNum = std::stoi(std::string(value));
        }

        if (httpd_query_key_value(query, "state", value, sizeof(value)) == ESP_OK) {
            requestedGpioState = std::string(value);
        }

        if (httpd_query_key_value(query, "pwm_duty", value, sizeof(value)) == ESP_OK) {
            requestedPWMDuty = std::stoi(value);
        }
    } 
    else {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, "Error in call. Use /gpio?gpio=12&state=1");    
        return ESP_OK;
    }

    if (task.compare("gpio_config") == 0) {
        // Prepare JSON object
        esp_err_t retVal = ESP_OK;
        cJSON *cJSONObject, *pins, *pin, *mods, *default_config, *pinModes, *pinInterrupt;

        cJSONObject = cJSON_CreateObject();         
        if (cJSONObject == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Error, JSON object cannot be created");
            return ESP_FAIL;
        }

        if (!cJSON_AddItemToObject(cJSONObject, "default_config", default_config = cJSON_CreateObject()))
            retVal = ESP_FAIL;
        
        if ((pinModes = cJSON_AddArrayToObject(default_config, "gpio_modes")) == NULL)
            retVal = ESP_FAIL;
        for (int i = 1; i < GPIO_PIN_MODE_MAX; ++i) {
            if (!cJSON_AddItemToArray(pinModes, cJSON_CreateString(getPinModeDecription((gpio_pin_mode_t)i).c_str())))
                retVal = ESP_FAIL;
        }

        if ((pinInterrupt = cJSON_AddArrayToObject(default_config, "gpio_interrupt")) == NULL)
            retVal = ESP_FAIL;
        for (int i = 0; i <= 3; ++i) {
            if (!cJSON_AddItemToArray(pinInterrupt, cJSON_CreateString(getPinInterruptDecription((gpio_int_type_t)i).c_str())))
                retVal = ESP_FAIL;
        }

        if (!cJSON_AddItemToObject(cJSONObject, "gpio", pins = cJSON_CreateArray()))
            retVal = ESP_FAIL;
        
        for (int i = 0; i < GPIO_SPARE_PIN_COUNT; ++i) {
            if (gpio_spare[i] == GPIO_NUM_NC)   // Skip not usable GPIOs
                continue;
            
            if (!cJSON_AddItemToArray(pins, pin = cJSON_CreateObject()))
                retVal = ESP_FAIL;
            if (!cJSON_AddItemToObject(pin, "name", cJSON_CreateNumber(gpio_spare[i])))
                retVal = ESP_FAIL;
            if (cJSON_AddStringToObject(pin, "usage", gpio_spare_usage[i]) == NULL)
                retVal = ESP_FAIL;
        }

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 1024, true); // Print to predefined buffer, avoid dynamic allocations
        std::string sReturnMessage = std::string(jsonString);
        cJSON_free(jsonString);  
        cJSON_Delete(cJSONObject);

        if (retVal == ESP_OK) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_sendstr(req, sReturnMessage.c_str());
            return ESP_OK;
        }
        else {
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Error while adding JSON elements");
            return ESP_FAIL;
        }
    }
    else if (task.compare("get_state") == 0 || task.compare("set_state") == 0) {           
        if (gpioMap == NULL || !isEnabled) {
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "GPIO handler not initialized or disabled");
            return ESP_FAIL;
        }

        gpio_num_t gpioNum = (gpio_num_t)resolveSparePinNr(requestedGpioNum);
        if (gpioMap->count(gpioNum) == 0) {
            respStr = "Skip request, GPIO not enabled: " + std::to_string(requestedGpioNum);
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, respStr.c_str());
            return ESP_FAIL;
        }

        if (gpioNum == GPIO_NUM_NC) {
            respStr = "Skip request, GPIO number unknown: " + std::to_string(requestedGpioNum);
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, respStr.c_str());
            return ESP_OK;
        }

        if  (!(*gpioMap)[gpioNum]->getHttpAccess()) {
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Skip request, HTTP access disabled");        
            return ESP_FAIL;    
        }

        if (task.compare("get_state") == 0) {
            requestedGpioState = "{ \"state\": " + std::to_string((*gpioMap)[gpioNum]->getPinState());
            
            if ((*gpioMap)[gpioNum]->getMode() == GPIO_PIN_MODE_OUTPUT_PWM ||
                (*gpioMap)[gpioNum]->getMode() == GPIO_PIN_MODE_FLASHLIGHT_PWM)
            {
                requestedGpioState += ", \"pwm_duty\": " + std::to_string(ledc_get_duty(LEDC_LOW_SPEED_MODE, 
                                                (*gpioMap)[gpioNum]->getLedcChannel()));
            }

            requestedGpioState += " }";

            httpd_resp_set_type(req, "text/plain");
            httpd_resp_sendstr(req, requestedGpioState.c_str());
            return ESP_OK;
        }
        else if (task.compare("set_state") == 0) {
            requestedGpioState = toUpper(requestedGpioState);
            if (requestedGpioState != "0" && requestedGpioState != "1")
            {
                respStr = "Skip request, invalid state: " + requestedGpioState;
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, respStr.c_str());        
                return ESP_FAIL;    
            }

            esp_err_t retVal = ESP_OK;
            if (requestedPWMDuty == -1) { // Use ON/OFF
                retVal = (*gpioMap)[gpioNum]->setPinState(requestedGpioState == "1", GPIO_SET_SOURCE_HTTP);
                respStr = "GPIO" + std::to_string(requestedGpioNum) + 
                        ", State: " + std::to_string((*gpioMap)[gpioNum]->getPinState());
            }
            else { // Use PWM
                requestedPWMDuty = std::min(std::max(0, requestedPWMDuty), calcDutyResultionMaxValue((*gpioMap)[gpioNum]->getFrequency()));
                retVal = (*gpioMap)[gpioNum]->setPinState(requestedGpioState == "1", requestedPWMDuty, GPIO_SET_SOURCE_HTTP); 
                respStr = "GPIO" + std::to_string(requestedGpioNum) + 
                        ", State: " + std::to_string((*gpioMap)[gpioNum]->getPinState()) + 
                        ", PWM Duty: " + std::to_string(requestedPWMDuty);
            }
            
            if (retVal != ESP_OK) {
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Skip request, wrong GPIO pin mode");
                return ESP_FAIL;
            }
            
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_sendstr(req, respStr.c_str());
            return ESP_OK;
        }
    }
    else {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "E90: Task not found");
        return ESP_FAIL;
    }
          
    return ESP_OK;    
}


void GpioHandler::registerGpioUri() 
{
    ESP_LOGI(TAG, "Registering URI handlers");
    
    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;
    camuri.uri       = "/gpio";
    camuri.handler   = callHandleHttpRequest;
    camuri.user_ctx  = (void*)this;    
    httpd_register_uri_handler(httpServer, &camuri);
}


// GPIO handler interface
// ***********************************
void gpio_handler_create(httpd_handle_t _server) 
{
    if (gpioHandler == NULL)
        gpioHandler = new GpioHandler(CONFIG_FILE, _server);
}


bool gpio_handler_init() 
{
    if (gpioHandler != NULL) {
        return (gpioHandler->init());
    }
    return false;
}


void gpio_handler_deinit()
{
    if (gpioHandler != NULL) {
        gpioHandler->deinit();
   }
}


void gpio_handler_destroy()
{
    if (gpioHandler != NULL) {
        gpio_handler_deinit();
        delete gpioHandler;
        gpioHandler = NULL;
    }
}


GpioHandler* gpio_handler_get()
{
    return gpioHandler;
}