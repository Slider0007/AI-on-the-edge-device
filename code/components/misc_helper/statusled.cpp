#include "statusled.h"
#include "../../include/defines.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <driver/ledc.h>

#include "gpioControl.h"
#include "ClassLogFile.h"
#include "helper.h"


static const char *TAG = "STATUSLED";

TaskHandle_t xHandle_task_StatusLED = NULL;
struct StatusLEDData StatusLEDData = {};


void setStatusLedState(bool status)
{
#ifdef GPIO_STATUS_LED_ONBOARD_USE_SMARTLED
    GpioHandler *gpioHandle = getGpioHandle();
    if (gpioHandle) {
        gpioHandle->gpioStatusLedControl(status);
    }
#else

#ifdef GPIO_STATUS_LED_ONBOARD_LOWACTIVE
    gpio_set_level(GPIO_STATUS_LED_ONBOARD, status ? 0 : 1);
#else
    gpio_set_level(GPIO_STATUS_LED_ONBOARD, status ? 1 : 0);
#endif // GPIO_STATUS_LED_ONBOARD_LOWACTIVE

#endif // GPIO_STATUS_LED_ONBOARD_USE_SMARTLED
}


void task_StatusLED(void *pvParameter)
{
    // ESP_LOGD(TAG, "task_StatusLED - create");
    while (StatusLEDData.bProcessingRequest) {
        // ESP_LOGD(TAG, "task_StatusLED - start");
        struct StatusLEDData StatusLEDDataInt = StatusLEDData;

        for (int i = 0; i < 2;) { // Default: repeat 2 times
            if (!StatusLEDDataInt.bInfinite) {
                ++i;
            }

            for (int j = 0; j < StatusLEDDataInt.iSourceBlinkCnt; ++j) {
                setStatusLedState(true);
                vTaskDelay(StatusLEDDataInt.iBlinkTime / portTICK_PERIOD_MS);
                setStatusLedState(false);
                vTaskDelay(StatusLEDDataInt.iBlinkTime / portTICK_PERIOD_MS);
            }

            vTaskDelay(500 / portTICK_PERIOD_MS); // Delay between module code and error code

            for (int j = 0; j < StatusLEDDataInt.iCodeBlinkCnt; ++j) {
                setStatusLedState(true);
                vTaskDelay(StatusLEDDataInt.iBlinkTime / portTICK_PERIOD_MS);
                setStatusLedState(false);
                vTaskDelay(StatusLEDDataInt.iBlinkTime / portTICK_PERIOD_MS);
            }
            vTaskDelay(1500 / portTICK_PERIOD_MS); // Delay to signal new round
        }

        StatusLEDData.bProcessingRequest = false;
        // ESP_LOGD(TAG, "task_StatusLED - done/wait");
        vTaskDelay(10000 / portTICK_PERIOD_MS); // Wait for an upcoming request otherwise continue and delete task to save memory
    }
    // ESP_LOGD(TAG, "task_StatusLED - delete");
    xHandle_task_StatusLED = NULL;
    vTaskDelete(NULL); // Delete this task due to no request
}


void setStatusLed(StatusLedSource _eSource, int _iCode, bool _bInfinite)
{
    // ESP_LOGD(TAG, "setStatusLed - start");

    if (_eSource == WLAN_CONN) {
        StatusLEDData.iSourceBlinkCnt = WLAN_CONN;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == NETWORK_INIT) {
        StatusLEDData.iSourceBlinkCnt = NETWORK_INIT;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == SDCARD_NVS_INIT) {
        StatusLEDData.iSourceBlinkCnt = SDCARD_NVS_INIT;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == SDCARD_CHECK) {
        StatusLEDData.iSourceBlinkCnt = SDCARD_CHECK;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == CAM_INIT) {
        StatusLEDData.iSourceBlinkCnt = CAM_INIT;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == PSRAM_INIT) {
        StatusLEDData.iSourceBlinkCnt = PSRAM_INIT;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == TIME_CHECK) {
        StatusLEDData.iSourceBlinkCnt = TIME_CHECK;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 250;
        StatusLEDData.bInfinite = _bInfinite;
    }
    else if (_eSource == AP_OR_OTA) {
        StatusLEDData.iSourceBlinkCnt = AP_OR_OTA;
        StatusLEDData.iCodeBlinkCnt = _iCode;
        StatusLEDData.iBlinkTime = 350;
        StatusLEDData.bInfinite = _bInfinite;
    }

    if (xHandle_task_StatusLED && !StatusLEDData.bProcessingRequest) {
        StatusLEDData.bProcessingRequest = true;
        xTaskAbortDelay(xHandle_task_StatusLED); // Reuse still running status LED task

        /*if (xReturned == pdPASS)
            ESP_LOGD(TAG, "task_StatusLED - abort waiting delay");*/
    }
    else if (xHandle_task_StatusLED == NULL) {
        StatusLEDData.bProcessingRequest = true;
        BaseType_t xReturned = xTaskCreate(&task_StatusLED, "task_StatusLED", 1280, NULL, tskIDLE_PRIORITY + 1, &xHandle_task_StatusLED);
        if (xReturned != pdPASS) {
            xHandle_task_StatusLED = NULL;
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "task_StatusLED failed to create");
            LogFile.writeHeapInfo("task_StatusLED failed");
        }
    }
    else {
        ESP_LOGD(TAG, "task_StatusLED still processing, request skipped"); // Requests with high frequency could be skipped, but LED is only
                                                                           // helpful for static states
    }
    // ESP_LOGD(TAG, "setStatusLed - done");
}


void forceStatusLedOff(void)
{
    if (xHandle_task_StatusLED) {
        vTaskDelete(xHandle_task_StatusLED); // Delete task for setStatusLed to force stop of blinking
        xHandle_task_StatusLED = NULL;
    }

    setStatusLedState(false); // Force status LED off
}


void initStatusLed()
{
#ifdef GPIO_STATUS_LED_ONBOARD_USE_SMARTLED
    initGpioHandler();
#else
    esp_rom_gpio_pad_select_gpio(GPIO_STATUS_LED_ONBOARD);         // Init GPIO pin
    gpio_set_direction(GPIO_STATUS_LED_ONBOARD, GPIO_MODE_OUTPUT); // Set the GPIO as push/pull output
#endif // GPIO_STATUS_LED_ONBOARD_USE_SMARTLED

    setStatusLedState(false); // Force status LED off
}
