#include "../../include/defines.h"

#include <string>
#include <regex>

#include "nvs_flash.h"
#include "esp_psram.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#ifdef DISABLE_BROWNOUT_DETECTOR
    #include "soc/soc.h"
    #include "soc/rtc_cntl_reg.h"
#endif

#include "configClass.h"
#include "configMigration.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "system.h"
#include "statusled.h"
#include "sdcard_check.h"
#include "MainFlowControl.h"
#include "connect_wlan.h"
#include "webserver.h"
#include "time_sntp.h"
#include "gpioControl.h"
#include "server_file.h"
#include "server_ota.h"
#include "server_camera.h"

#ifdef ENABLE_MQTT
#include "server_mqtt.h"
#endif //ENABLE_MQTT

#include "openmetrics.h"

#ifdef ENABLE_SOFTAP
    #include "softAP.h"
#endif //ENABLE_SOFTAP


static const char *TAG = "MAIN";


std::string deviceStartTimestamp = "";

extern const char* GIT_TAG;
extern const char* GIT_REV;
extern const char* GIT_BRANCH;
extern const char* BUILD_TIME;

extern std::string getFwVersion(void);
extern std::string getHTMLversion(void);
extern std::string getHTMLcommit(void);

esp_err_t initNVSFlash();
esp_err_t initSDCard();


extern "C" void app_main(void)
{
    deviceStartTimestamp = getCurrentTimeString(TIME_FORMAT_OUTPUT);

    #ifdef DISABLE_BROWNOUT_DETECTOR
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    #endif

    // ********************************************
    // Highlight start of app_main
    // ********************************************
    ESP_LOGI(TAG, "================ Start app_main =================");

    // Init NVS flash
    // ********************************************
    if (ESP_OK != initNVSFlash()) {
        ESP_LOGE(TAG, "Device init aborted");
        return; // Stop here, NVS is needed for proper operation
    }

    // Init SD card
    // ********************************************
    if (ESP_OK != initSDCard()) {
        ESP_LOGE(TAG, "Device init aborted");
        return; // Stop here, SD card is needed for proper operation
    }

    // SD card: Create log directories (if not already existing)
    // ********************************************
    LogFile.createLogDirectories(); // mandatory for logging + image saving

    // ********************************************
    // Highlight start of logfile logging
    // Default Log Level: INFO -> Everything which needs to be logged during boot should be have level INFO, WARN OR ERROR
    // ********************************************
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "=================================================");
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "==================== Start ======================");
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "=================================================");

    // SD card: Create further mandatory directories (if not already existing)
    // Correct creation of these folders will be checked with function "checkSdCardFolderFilePresence"
    // ********************************************
    makeDir("/sdcard/config");           // mandatory for config handling
    makeDir("/sdcard/config/certs");     // mandatory for TLS encryption
    makeDir("/sdcard/config/models");    // mandatory for TFLite models
    makeDir("/sdcard/firmware");         // mandatory for OTA firmware update
    makeDir("/sdcard/img_tmp");          // mandatory for setting up alignment marker
    makeDir("/sdcard/demo");             // mandatory for demo mode

    // Check for updates
    // Note: OTA status check only necessary if OTA rollback feature is enabled
    // ********************************************
    #ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    checkOTAPartitionState();
    #endif
    checkOTAUpdate();

    // Migrate parameter from older version to actual version
    // Do migration task before first parameter usage
    // ********************************************
    migrateConfiguration();

    // Load persistent config from file (json notation)
    ConfigClass::getInstance()->readConfigFile();

    // Check for missing configuration
    // ********************************************
    #ifdef ENABLE_SOFTAP
        checkStartAPMode();
    #endif

    // SD card: basic R/W check
    // ********************************************
    int iSDCardStatus = checkSdCardRW();
    if (iSDCardStatus < 0) {
        if (iSDCardStatus <= -1 && iSDCardStatus >= -2) { // write error
            setStatusLed(SDCARD_CHECK, 1, true);
        }
        else if (iSDCardStatus <= -3 && iSDCardStatus >= -5) { // read error
            setStatusLed(SDCARD_CHECK, 2, true);
        }
        else if (iSDCardStatus == -6) { // delete error
            setStatusLed(SDCARD_CHECK, 3, true);
        }
        setSystemStatusFlag(SYSTEM_STATUS_SDCARD_CHECK_BAD); // reduced web interface going to be loaded
    }

    // SD card: Check presence of some mandatory folders / files
    // ********************************************
    if (!checkSdCardFolderFilePresence()) {
        setStatusLed(SDCARD_CHECK, 4, true);
        setSystemStatusFlag(SYSTEM_STATUS_FOLDER_CHECK_BAD); // reduced web interface going to be loaded
    }

    // Check version information
    // ********************************************
    LogFile.writeToFile(ESP_LOG_INFO, TAG, getFwVersion() + " | Build time: " + std::string(BUILD_TIME) + " | WebUI: " + getHTMLversion());

    if (getHTMLcommit().substr(0, 7) == "?")
        LogFile.writeToFile(ESP_LOG_WARN, TAG, std::string("Failed to read file html/version.txt to parse WebUI version"));

    if (getHTMLcommit().substr(0, 7) != std::string(GIT_REV).substr(0, 7)) { // Compare the first 7 characters of both hashes
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "WebUI version (" + getHTMLcommit() + ") does not match firmware version (" + std::string(GIT_REV) + ")");
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Recommendation: Repeat OTA update using AI-on-the-edge-device__board_type__*.zip");
    }

    // Check reboot reason
    // ********************************************
    checkIsPlannedReboot();
    if (!getIsPlannedReboot() && (esp_reset_reason() == ESP_RST_PANIC)) {  // If system reboot was not triggered by user and reboot was caused by execption
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Reset reason: " + getResetReason());
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Device was rebooted due to a software exception! Log level is set to DEBUG until the next reboot. "
                                               "Flow init is delayed by 5 minutes to check the logs or do an OTA update");
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Keep device running until crash occurs again and check logs after device is up again");
        LogFile.setLogLevel(ESP_LOG_DEBUG);
        setTaskAutoFlowState(FLOW_TASK_STATE_INIT_DELAYED);
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Reset reason: " + getResetReason());
    }

    // Allocate webserver memory (in an early stage to avoid memory fragmentation)
    allocateWebserverHelperMemory();

    // Set CPU Frequency (default: 160Mhz)
    // Start before WLAN init to avoid frequency changes after WLAN init
    // ********************************************
    setCPUFrequency();

    // Init time (as early as possible, but wifi needs to be connected to sync time. no hardware clock available)
    // Status of time sync will be checked after every cycle (MainFlowControl.cpp)
    // ********************************************
    initTime();

    // Init WIFI service
    // ********************************************
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init WIFI service");
    esp_err_t retVal = initWifiStation();
    if (retVal != ESP_OK) {
        if (retVal == ESP_ERR_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Device init aborted");
            setStatusLed(WLAN_INIT, 1, true);
            return;
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "WIFI init failed. Device init aborted");
            setStatusLed(WLAN_INIT, 2, true);
            return;
        }
    }

    // Set log level for wifi component to WARN level (default: INFO; only relevant for serial console)
    // ********************************************
    esp_log_level_set("wifi", ESP_LOG_WARN);

    // Init external PSRAM
    // ********************************************
    esp_err_t PSRAMStatus = esp_psram_init();
    if (PSRAMStatus == ESP_FAIL) {  // Failed to init PSRAM
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "PSRAM init failed (" + std::to_string(PSRAMStatus) + ")! PSRAM not found or defective");
        setSystemStatusFlag(SYSTEM_STATUS_PSRAM_BAD);
        setStatusLed(PSRAM_INIT, 1, true);
    }
    else { // PSRAM init not failed --> continue to check PSRAM size
        size_t psram_size = esp_psram_get_size();
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "PSRAM size: " + std::to_string(psram_size) + " byte (" + std::to_string(psram_size/1024/1024) +
                                               "MB / " + std::to_string(psram_size/1024/1024*8) + "MBit)");

        // Check PSRAM size
        // ********************************************
        if (psram_size < (4*1024*1024)) { // PSRAM is below 4 MBytes (32Mbit)
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "PSRAM size >= 4MB (32Mbit) is mandatory to run this application");
            setSystemStatusFlag(SYSTEM_STATUS_PSRAM_BAD);
            setStatusLed(PSRAM_INIT, 2, true);
        }
        else { // PSRAM size OK --> continue to check heap size
            size_t _hsize = getESPHeapSizeTotalFree();
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Total heap: " + std::to_string(_hsize) + " byte");

            // Check heap memory
            // ********************************************
            if (_hsize < 4000000) { // Check available Heap memory for a bit less than 4 MB (a test on a good device showed 4187558 bytes to be available)
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Total heap >= 4000000 byte is mandatory to run this application");
                setSystemStatusFlag(SYSTEM_STATUS_HEAP_TOO_SMALL);
                setStatusLed(PSRAM_INIT, 3, true);
            }
            else { // HEAP size OK --> continue to camera init
                // Set SPIRAM memory category
                size_t SPIRAMFree = getESPHeapSizeSPIRAMFree();
                if (SPIRAMFree >= 32000000)
                    setSPIRAMCategory(SPIRAMCategory_32MB);
                else if (SPIRAMFree >= 16000000)
                    setSPIRAMCategory(SPIRAMCategory_16MB);
                else if (SPIRAMFree >= 8000000)
                    setSPIRAMCategory(SPIRAMCategory_8MB);
                else
                    setSPIRAMCategory(SPIRAMCategory_4MB);

                // Init camera
                // ********************************************
                esp_err_t camStatus = cameraCtrl.initCam();
                cameraCtrl.setFlashlight(false);

                // Check camera init
                // ********************************************
                if (camStatus != ESP_OK) { // Camera init failed, try to reinit during flow init (MainFlowControl.cpp -> doInit())
                    setStatusLed(CAM_INIT, 1, false);
                }
                else { // ESP_OK -> Camera init OK
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init camera successful");
                    cameraCtrl.printCamInfo();
                }
            }
        }
    }

    // Init SOC temperature sensor (if supported by hardware)
    // ********************************************
    #ifdef SOC_TEMP_SENSOR_SUPPORTED
    initSOCTemperatureSensor();
    #endif

    // Print Device info
    // ********************************************
    printDeviceInfo();

    // Print SD-Card info
    // ********************************************
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "SD card info: Name: " + getSDCardName() + ", Capacity: " +
            std::to_string(getSDCardCapacity()) + "MB, Free: " + std::to_string(getSDCardFreePartitionSpace()) + "MB");


    // Start webserver + register URI handler
    // ********************************************
    ESP_LOGD(TAG, "starting servers");
    server = startWebserver();
    registerConfigFileUri(server);
    registerCameraUri(server);
    registerMainFlowTaskUri(server);
    registerFileserverUri(server, "/sdcard");
    registerOtaRebootUri(server);
    #ifdef ENABLE_MQTT
    registerMqttUri(server);
    #endif //ENABLE_MQTT
    registerOpenmetricsUri(server);
    createGpioHandler(server);
    registerWebserverUri(server, "/sdcard");

    // Check basic device init status
    // ********************************************
    if (getSystemStatus() == 0) { // Continue with regular boot sequence --> MainProcessControl.cpp -> createMainFlowTask()
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Basic device initialization completed");
        createMainFlowTask(); // Create main task
    }
    // Critical error(s) occured which do not allow to continue with regular boot sequence.
    // Provding only a reduced web interface for diagnostic purpose. Reduced web interface and interlock: server_main.cpp -> hello_main_handler()
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Basic device initialization failed");
    }
}


esp_err_t initNVSFlash()
{
    ESP_LOGI(TAG, "Initializing NVS flash");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "NVS flash init failed. No NVS partition found");
            setStatusLed(SDCARD_NVS_INIT, 4, true);
        }
        else if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
            ESP_LOGE(TAG, "NVS flash init failed. No free NVS pages found");
            setStatusLed(SDCARD_NVS_INIT, 5, true);
        }
        else {
            ESP_LOGE(TAG, "NVS flash init failed. Check error code");
            setStatusLed(SDCARD_NVS_INIT, 6, true);
        }
    }

    return ret;
}


esp_err_t initSDCard()
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG,"Initializing SD card: Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // Pullup SD card D3 pin to ensure SD init using MMC mode
    // Additionally, an external pullup is needed
    gpio_set_pull_mode(GPIO_SDCARD_D3, GPIO_PULLUP_ONLY);

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    #ifdef SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = GPIO_SDCARD_CLK;
        slot_config.cmd = GPIO_SDCARD_CMD;
        slot_config.d0 = GPIO_SDCARD_D0;
    #endif

    #ifdef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        slot_config.width = 1;
    #else
        #ifdef SOC_SDMMC_USE_GPIO_MATRIX
            slot_config.d1 = GPIO_SDCARD_D1;
            slot_config.d2 = GPIO_SDCARD_D2;
            slot_config.d3 = GPIO_SDCARD_D3;
        #endif
        slot_config.width = 4;
    #endif

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 12,                         // previously -> 2022-09-21: 5, 2023-01-02: 7
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = 0
    };

    sdmmc_card_t* card;

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount FAT filesystem on SD card. Check SD card filesystem (only FAT supported) or try another card");
            setStatusLed(SDCARD_NVS_INIT, 1, true);
        }
        else if (ret == 263) { // Error code: 0x107 --> usually: SD not found
            ESP_LOGE(TAG, "SD card init failed. Check if SD card is properly inserted into SD card slot or try another card");
            setStatusLed(SDCARD_NVS_INIT, 2, true);
        }
        else {
            ESP_LOGE(TAG, "SD card init failed. Check error code or try another card");
            setStatusLed(SDCARD_NVS_INIT, 3, true);
        }
        return ret;
    }

    saveSDCardInfo(card);
    return ret;
}
