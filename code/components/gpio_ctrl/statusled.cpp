#include "statusled.h"
#include "../../include/defines.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <driver/ledc.h>

#include "ClassLogFile.h"

#ifdef GPIO_STATUS_LED_ONBOARD_USE_SMARTLED
#include "gpioControl.h"
#endif // GPIO_STATUS_LED_ONBOARD_USE_SMARTLED


static const char *TAG = "STATUSLED";

static TaskHandle_t xHandleTaskStatusLED = nullptr;
static struct StatusLEDData StatusLEDData = {};
static SemaphoreHandle_t xStatusLedMutex = nullptr;


static void applyPhysicalLedState(bool status)
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

static void task_StatusLED(void *pvParameter)
{
    while (true) {
        struct StatusLEDData StatusLEDDataInt = {};

        if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
            // Check if a cancellation request arrived or processing dropped low
            if (!StatusLEDData.bProcessingRequest) {
                applyPhysicalLedState(false);
                xHandleTaskStatusLED = nullptr;
                xSemaphoreGive(xStatusLedMutex);
                break;
            }

            // Snapshot the fresh operational parameters atomically
            StatusLEDDataInt = StatusLEDData;
            StatusLEDData.bRequestPending = false;
            xSemaphoreGive(xStatusLedMutex);
        }

        // Execute the full structural pattern sequence pass
        for (int i = 0; i < 2;) {
            if (!StatusLEDDataInt.bInfinite) {
                ++i;
            }

            // Source Blinks
            for (int j = 0; j < StatusLEDDataInt.iSourceBlinkCnt; ++j) {
                applyPhysicalLedState(true);
                vTaskDelay(pdMS_TO_TICKS(StatusLEDDataInt.iBlinkTime));
                applyPhysicalLedState(false);
                vTaskDelay(pdMS_TO_TICKS(StatusLEDDataInt.iBlinkTime));
            }

            vTaskDelay(pdMS_TO_TICKS(500)); // Delay between module code and error code

            // Code Blinks
            for (int j = 0; j < StatusLEDDataInt.iCodeBlinkCnt; ++j) {
                applyPhysicalLedState(true);
                vTaskDelay(pdMS_TO_TICKS(StatusLEDDataInt.iBlinkTime));
                applyPhysicalLedState(false);
                vTaskDelay(pdMS_TO_TICKS(StatusLEDDataInt.iBlinkTime));
            }

            vTaskDelay(pdMS_TO_TICKS(1500)); // Delay to signal new round
        }

        // Check for request
        if (xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) == pdTRUE) {
            if (StatusLEDData.bRequestPending && StatusLEDData.bProcessingRequest) {
                xSemaphoreGive(xStatusLedMutex);
                continue;
            }

            applyPhysicalLedState(false);
            StatusLEDData.bProcessingRequest = false;
            xHandleTaskStatusLED = nullptr;
            xSemaphoreGive(xStatusLedMutex);
            break;
        }
    }

    vTaskDelete(nullptr);
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

    if (!xHandleTaskStatusLED) {
        BaseType_t xReturned = xTaskCreate(&task_StatusLED, "task_StatusLED", 2048, nullptr, tskIDLE_PRIORITY + 2, &xHandleTaskStatusLED);
        if (xReturned != pdPASS) {
            xHandleTaskStatusLED = nullptr;
            StatusLEDData.bProcessingRequest = false;
            StatusLEDData.bRequestPending = false;
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "task_StatusLED failed to create");
            LogFile.writeHeapInfo("task_StatusLED failed");
        }
    }

    xSemaphoreGive(xStatusLedMutex);
}

void setStatusLed(bool status)
{
    if (xStatusLedMutex == nullptr || xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    // Set pin only if in idle
    if (!xHandleTaskStatusLED && !StatusLEDData.bProcessingRequest) {
        applyPhysicalLedState(status);
    }

    xSemaphoreGive(xStatusLedMutex);
}

void forceStatusLedOff(void)
{
    if (xStatusLedMutex == nullptr || xSemaphoreTake(xStatusLedMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    // Set control structure state variables to safely trigger task teardown on its next turn
    StatusLEDData.bProcessingRequest = false;
    StatusLEDData.bRequestPending = false;
    StatusLEDData.bInfinite = false;
    StatusLEDData.iSourceBlinkCnt = 0;
    StatusLEDData.iCodeBlinkCnt = 0;

    applyPhysicalLedState(false);

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

    applyPhysicalLedState(false); // Force LED to off
}
