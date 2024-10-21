#include "improvWifiProvisioning.h"
#include "../../include/defines.h"

#include <string.h>

#ifndef USB_SERIAL
#include <driver/uart.h>
#include <hal/gpio_types.h>
#else
#include <driver/usb_serial_jtag.h>
#endif // USB_SERIAL

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "improvWifi.h"
#include "connect_wlan.h"
#include "configClass.h"
#include "ClassLogFile.h"
#include "system.h"
#include "server_ota.h"
#include "softAP.h"
#include "helper.h"


static const char *TAG = "IMPROV";


static TaskHandle_t improvTaskHandle = NULL;
static ImprovWiFi *improvWifi = NULL;

#ifndef USB_SERIAL
static QueueHandle_t uartQueueHandle;
static const int evtBufferSize = (UART_HW_FIFO_LEN(DEFAULT_UART_NUM));
static const int uartBufferSize = 2 * evtBufferSize;
#else
static const int evtBufferSize = 256;
#endif // USB_SERIAL
uint8_t evtData[evtBufferSize];

extern std::string getFwVersion(void);


static void improvEventHandler(void)
{
#ifndef USB_SERIAL
    // Waiting for UART event
    uart_event_t event;

    if (xQueueReceive(uartQueueHandle, (void *)&event, (TickType_t)portMAX_DELAY) == pdPASS) {
        // ESP_LOGD(TAG, "uart[%d] event:", DEFAULT_UART_NUM);
        switch (event.type) {
            // UART receving data
            case UART_DATA:
                bzero(evtData, evtBufferSize);
                uart_read_bytes(DEFAULT_UART_NUM, evtData, event.size, portMAX_DELAY);
                improvWifi->handleSerial(evtData, event.size);
                // LogFile.writeToFile(ESP_LOG_ERROR, TAG, "IMPROV UART RX: " + std::string((char *)evtData, event.size));
                break;

            // HW FIFO overflow detected
            case UART_FIFO_OVF:
                // ESP_LOGD(TAG, "hw fifo overflow");
                uart_flush_input(DEFAULT_UART_NUM);
                xQueueReset(uartQueueHandle);
                break;

            // Ring buffer full
            case UART_BUFFER_FULL:
                //ESP_LOGD(TAG, "ring buffer full");
                uart_flush_input(DEFAULT_UART_NUM);
                xQueueReset(uartQueueHandle);
                break;

            // Other events
            default:
                // ESP_LOGD(TAG, "uart event type: %d", event.type);
                break;
        }
    }
#else
    // Waiting for USB data
    int readBytes = usb_serial_jtag_read_bytes(evtData, evtBufferSize, portMAX_DELAY);
    if (readBytes > 0) {
        improvWifi->handleSerial(evtData, readBytes);
        ESP_LOGI(TAG, "Data received"); // Workaround: Do not remove, otherwise it's not working (@TODO, FIXME, tested ESP-IDF 5.2.1)
        //LogFile.writeToFile(ESP_LOG_ERROR, TAG, "IMPROV USB RX: " + std::string((char *)evtData, readBytes));
        bzero(evtData, evtBufferSize);
        readBytes = 0;
    }
#endif // USB_SERIAL
}


static void improvTask(void *pvParameters)
{
    while (true) {
        improvEventHandler();
    }
}


#ifndef USB_SERIAL
void improvUartWrite(const unsigned char *txData, int length)
{
    uart_write_bytes(DEFAULT_UART_NUM, txData, length);
    // LogFile.writeToFile(ESP_LOG_ERROR, TAG, "IMPROV UART TX: " + std::string((char *)txData, length));
}

#else

void improvUSBWrite(const unsigned char *txData, int length)
{
    usb_serial_jtag_write_bytes(txData, length, portMAX_DELAY);

    const char newline = '\n';
    usb_serial_jtag_write_bytes(&newline, 1, portMAX_DELAY); // Force the transmission
    // LogFile.writeToFile(ESP_LOG_ERROR, TAG, "IMPROV USB TX: " + std::string((char *)txData, length));
}
#endif // USB_SERIAL


void improvWifiScan(unsigned char *scanResponse, int bufLen, uint16_t *networkNum)
{
    esp_err_t retVal;

    // If already in AP mode, switch to station mode
    wifi_mode_t wifiMode;
    esp_wifi_get_mode(&wifiMode);

    if(wifiMode == WIFI_MODE_AP) {
        wifiDeinitAP();
        vTaskDelay(pdMS_TO_TICKS(500));

        initWifiStation();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    wifi_scan_config_t wifiScanConfig;
    memset(&wifiScanConfig, 0, sizeof(wifiScanConfig));

    wifiScanConfig.show_hidden = true; // Scan also hidden SSIDs
	wifiScanConfig.channel = 0; // Scan all channels

    // Start scan. If in wrong state disconnect first and restart scan again
    if (esp_wifi_scan_start(&wifiScanConfig, true) == ESP_ERR_WIFI_STATE) {
        wifi_ap_record_t apInfoTmp;
        int timeoutCnt = 0;

        do {
            esp_wifi_disconnect();
            vTaskDelay(pdMS_TO_TICKS(500));
            if (timeoutCnt > 20) { // Timeout 10s
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvWifiScan: Timeout, waiting for proper state to scan");
                break;
            }
            timeoutCnt++;
        } while (esp_wifi_sta_get_ap_info(&apInfoTmp) != ESP_ERR_WIFI_NOT_CONNECT && esp_wifi_sta_get_ap_info(&apInfoTmp) != ESP_OK);

        esp_wifi_scan_start(&wifiScanConfig, true); // Scan in blocking mode
    }

    *networkNum = 10; // Max. number of APs, value will be updated by function "esp_wifi_scan_get_ap_num"
	retVal = esp_wifi_scan_get_ap_num(networkNum); // Get actual found APs
    if (retVal != ESP_OK) {
      	LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvWifiScan: esp_wifi_scan_get_ap_num: Error: " + intToHexString(retVal));
		return;
    }
    wifi_ap_record_t* apInfo = new wifi_ap_record_t[*networkNum]; // Allocate necessary record datasets
	if (apInfo == NULL) {
		esp_wifi_scan_get_ap_records(0, NULL); // Free internal heap
		LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvWifiScan: Failed to allocate heap for apInfo");
		return;
	}
	else {
        retVal = esp_wifi_scan_get_ap_records(networkNum, apInfo);
    	if (retVal != ESP_OK) { // Retrieve results (and free internal heap)
			LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvWifiScan: esp_wifi_scan_get_ap_records: Error: " + intToHexString(retVal));
			delete[] apInfo;
			return;
		}
	}

    // Build response string
    scanResponse[0] = 0;
    for (int i = 0; i < *networkNum; i++) {
        char rssiStr[8] = {0};
        char cipherStr[8] = {0};
        int neededLen;

        itoa(apInfo[i].rssi, rssiStr, 10);
        if (apInfo[i].authmode != WIFI_AUTH_OPEN) {
            strcat(cipherStr, "YES");
        }
        else {
            strcat(cipherStr, "NO");
        }
        neededLen = strlen((const char *)apInfo[i].ssid) + strlen(rssiStr) + strlen(cipherStr) + 3;

        if ((bufLen - neededLen) > 0) {
            strcat((char *)scanResponse, (char *)apInfo[i].ssid);
            strcat((char *)scanResponse, (char *)",");
            strcat((char *)scanResponse, (char *)rssiStr);
            strcat((char *)scanResponse, (char *)",");
            strcat((char *)scanResponse, (char *)cipherStr);
            strcat((char *)scanResponse, (char *)"\n");

            bufLen -= neededLen;
        }
    }
}


bool improvWifiConnect(const char *ssid, const char *password)
{
    // Set new configuration
    ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ssid = std::string(ssid);
    ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password = std::string(password);
    ConfigClass::getInstance()->saveMigDataToNVS("wlan_pw", ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password);
    ConfigClass::getInstance()->persistConfig();
    ConfigClass::getInstance()->reinitConfig();

    // Get existing wifi config
    wifi_config_t wifiConfig;
    esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

    // Set new credentials
    strcpy((char*)wifiConfig.sta.ssid, (const char*)ConfigClass::getInstance()->get()->sectionNetwork.wlan.ssid.c_str());
    strcpy((char*)wifiConfig.sta.password, (const char*)ConfigClass::getInstance()->get()->sectionNetwork.wlan.password.c_str());
    esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);

    // Disconnect and reconnect
    wifi_ap_record_t apInfoTmp;
    do {
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
    } while (esp_wifi_sta_get_ap_info(&apInfoTmp) != ESP_ERR_WIFI_NOT_CONNECT);
    esp_wifi_connect();

    // Check connection state
    int timeoutCnt = 0;
    while (!getWIFIisConnected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (timeoutCnt > 30) { // Timeout 30s
            return false;
        }
        timeoutCnt++;
    }

    return true;
}


void improvInit(void)
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init started");

    esp_err_t retVal = ESP_OK;

    improvWifi = new ImprovWiFi();

#ifndef USB_SERIAL
    improvWifi->serialWrite(improvUartWrite);
#else
    improvWifi->serialWrite(improvUSBWrite);
#endif // USB_SERIAL

    ImprovTypes::ChipFamily chipFamily;
    if (getChipModel() == "ESP32") {
        chipFamily = ImprovTypes::CF_ESP32;
    }
    else if (getChipModel() == "ESP32S3") {
        chipFamily = ImprovTypes::CF_ESP32_S3;
    }
    else {
        chipFamily = ImprovTypes::CF_ESP32;
    }
    improvWifi->setDeviceInfo(chipFamily, "Firmware:", getFwVersion().c_str(), "AI-on-the-Edge Device");

    improvWifi->setCustomConnectWiFi(improvWifiConnect);
    improvWifi->setCustomScanWiFi(improvWifiScan);
    improvWifi->setCustomisConnected(getWIFIisConnected);
    improvWifi->setCustomGetLocalIpCallback(getIPAddress);

#ifndef USB_SERIAL
    // Install UART driver using an event queue
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Install UART driver");
    retVal = uart_driver_install(DEFAULT_UART_NUM, uartBufferSize, uartBufferSize, 10, &uartQueueHandle, 0);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvInit: uart_driver_install: Error: Parameter error");
    }

    retVal = uart_set_pin(DEFAULT_UART_NUM, DEFAULT_UART_TX_PIN, DEFAULT_UART_RX_PIN, -1, -1);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvInit: uart_set_pin: Error: Parameter error");
    }
#else
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Install USB serial driver");
    usb_serial_jtag_driver_config_t usbCfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usbCfg.rx_buffer_size = evtBufferSize;
    usbCfg.tx_buffer_size = evtBufferSize;
    retVal = usb_serial_jtag_driver_install(&usbCfg);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "improvInit: usb_serial_jtag_driver_install: Error: failed to install driver");
    }
#endif // USB_SERIAL

    BaseType_t xReturned = xTaskCreate(&improvTask, "improv", 4 * 1024, NULL, tskIDLE_PRIORITY + 4, &improvTaskHandle);
    if (xReturned != pdPASS) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create task 'improv'");
    }

    if (retVal != ESP_OK || xReturned != pdPASS) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init failed");
        return;
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init successful");
}


void improvDeinit(void)
{
    if (improvTaskHandle) {
        vTaskDelete(improvTaskHandle);
        improvTaskHandle = NULL;
    }

    if (improvWifi != NULL) {
        delete improvWifi;
        improvWifi = NULL;
    }

#ifndef USB_SERIAL
    uart_driver_delete(DEFAULT_UART_NUM);
#else
    usb_serial_jtag_driver_uninstall();
#endif // USB_SERIAL
}
