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

TaskHandle_t xHandle_task_StatusLED = nullptr;
struct StatusLEDData StatusLEDData = {};

static SemaphoreHandle_t xStatusLedMutex = nullptr;


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
    while (true) {
        struct StatusLEDData StatusLEDDataInt = {};
        bool bProcess = false;

        if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
            bProcess = StatusLEDData.bProcessingRequest;
            if (bProcess) {
                StatusLEDDataInt = StatusLEDData;
                StatusLEDData.bRequestPending = false;
            }
            xSemaphoreGive(xStatusLedMutex);
        }

        if (!bProcess) {
            break;
        }

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

        // Clear processing flags or evaluate execution bypass requests
        bool bBypassDelay = false;
        if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
            if (!StatusLEDData.bRequestPending) {
                StatusLEDData.bProcessingRequest = false;
            }
            else {
                bBypassDelay = true; // New request is pending
            }
            xSemaphoreGive(xStatusLedMutex);
        }

        if (!bBypassDelay) {
            if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
                StatusLEDData.bIsIdling = true;
                xSemaphoreGive(xStatusLedMutex);
            }

            vTaskDelay(10000 / portTICK_PERIOD_MS); // Enter waiting state for new requests

            if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
                StatusLEDData.bIsIdling = false;
                xSemaphoreGive(xStatusLedMutex);
            }
        }
    }

    if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
        xHandle_task_StatusLED = nullptr;
        StatusLEDData.bIsIdling = false;
        xSemaphoreGive(xStatusLedMutex);
    }
    vTaskDelete(nullptr); // Delete this task due to no request
}


void setStatusLed(StatusLedSource _eSource, int _iCode, bool _bInfinite)
{
    if (xStatusLedMutex == nullptr || xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    StatusLEDData.iSourceBlinkCnt = static_cast<int>(_eSource);
    StatusLEDData.iCodeBlinkCnt = _iCode;
    StatusLEDData.bInfinite = _bInfinite;
    StatusLEDData.iBlinkTime = (_eSource == AP_OR_OTA) ? 350 : 250;
    StatusLEDData.bProcessingRequest = true;
    StatusLEDData.bRequestPending = true;

    if (xHandle_task_StatusLED) {
        // Abort only if the task is asleep in the 10-second linger window
        if (StatusLEDData.bIsIdling) {
            xTaskAbortDelay(xHandle_task_StatusLED);
        }
    }
    else {
        BaseType_t xReturned = xTaskCreate(&task_StatusLED, "task_StatusLED", 2048, nullptr, tskIDLE_PRIORITY + 1, &xHandle_task_StatusLED);
        if (xReturned != pdPASS) {
            xHandle_task_StatusLED = nullptr;
            StatusLEDData.bProcessingRequest = false;
            StatusLEDData.bRequestPending = false;
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "task_StatusLED failed to create");
            LogFile.writeHeapInfo("task_StatusLED failed");
        }
    }

    xSemaphoreGive(xStatusLedMutex);
}


void forceStatusLedOff(void)
{
    if (xStatusLedMutex == nullptr || xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (xHandle_task_StatusLED) {
        vTaskDelete(xHandle_task_StatusLED); // Kill the task mid-execution immediately
        xHandle_task_StatusLED = nullptr;
    }

    // Reset the control structure state completely
    StatusLEDData.bProcessingRequest = false;
    StatusLEDData.bRequestPending = false;
    StatusLEDData.bIsIdling = false;
    StatusLEDData.bInfinite = false;
    StatusLEDData.iSourceBlinkCnt = 0;
    StatusLEDData.iCodeBlinkCnt = 0;

    setStatusLedState(false); // Force LED state to off

    xSemaphoreGive(xStatusLedMutex);
}


void initStatusLed()
{
    if (xStatusLedMutex == nullptr) {
        xStatusLedMutex = xSemaphoreCreateMutex();
    }

#ifdef GPIO_STATUS_LED_ONBOARD_USE_SMARTLED
    initGpioHandler();
#else
    esp_rom_gpio_pad_select_gpio(GPIO_STATUS_LED_ONBOARD);         // Init GPIO pin
    gpio_set_direction(GPIO_STATUS_LED_ONBOARD, GPIO_MODE_OUTPUT); // Set the GPIO as push/pull output
#endif // GPIO_STATUS_LED_ONBOARD_USE_SMARTLED

    setStatusLedState(false); // Force status LED off
}
