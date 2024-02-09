#include "../../include/defines.h"

#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include "nvs_flash.h"
#include "esp_psram.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#ifdef DISABLE_BROWNOUT_DETECTOR
    #include "soc/soc.h" 
    #include "soc/rtc_cntl_reg.h" 
#endif

#ifdef USE_HIMEM_IF_AVAILABLE
    #include "esp32/himem.h"
    #ifdef DEBUG_HIMEM_MEMORY_CHECK
        #include "himem_memory_check.h"
    #endif
#endif

//#ifdef CONFIG_HEAP_TRACING_STANDALONE
#if defined HEAP_TRACING_MAIN_WIFI || defined HEAP_TRACING_MAIN_START
    #include <esp_heap_trace.h>
    #define NUM_RECORDS 300
    static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
#endif

#include "ClassLogFile.h"
#include "connect_wlan.h"
#include "read_wlanini.h"
#include "server_main.h"
#include "MainFlowControl.h"
#include "server_file.h"
#include "server_ota.h"
#include "time_sntp.h"
#include "configFile.h"
#include "server_GPIO.h"
#include "server_camera.h"

#ifdef ENABLE_MQTT
#include "server_mqtt.h"
#endif //ENABLE_MQTT

#include "Helper.h"
#include "system.h"
#include "statusled.h"
#include "sdcard_check.h"

#ifdef ENABLE_SOFTAP
    #include "softAP.h"
#endif //ENABLE_SOFTAP


static const char *TAG = "MAIN";

extern const char* GIT_TAG;
extern const char* GIT_REV;
extern const char* GIT_BRANCH;
extern const char* BUILD_TIME;

extern std::string getFwVersion(void);
extern std::string getHTMLversion(void);
extern std::string getHTMLcommit(void);

esp_err_t initNVSFlash();
esp_err_t initSDCard();
void migrateConfiguration();


extern "C" void app_main(void)
{
    //#ifdef CONFIG_HEAP_TRACING_STANDALONE
    #if defined HEAP_TRACING_MAIN_WIFI || defined HEAP_TRACING_MAIN_START
        //register a buffer to record the memory trace
        ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );
    #endif
        
    TickType_t xDelay;
        
    #ifdef DISABLE_BROWNOUT_DETECTOR
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    #endif

    #ifdef HEAP_TRACING_MAIN_START
        ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
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
    LogFile.CreateLogDirectories(); // mandatory for logging + image saving

    // ********************************************
    // Highlight start of logfile logging
    // Default Log Level: INFO -> Everything which needs to be logged during boot should be have level INFO, WARN OR ERROR
    // ********************************************
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "=================================================");
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "==================== Start ======================");
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "=================================================");

    // SD card: basic R/W check
    // ********************************************
    int iSDCardStatus = SDCardCheckRW();
    if (iSDCardStatus < 0) {
        if (iSDCardStatus <= -1 && iSDCardStatus >= -2) { // write error
            StatusLED(SDCARD_CHECK, 1, true);
        }
        else if (iSDCardStatus <= -3 && iSDCardStatus >= -5) { // read error
            StatusLED(SDCARD_CHECK, 2, true);
        }
        else if (iSDCardStatus == -6) { // delete error
            StatusLED(SDCARD_CHECK, 3, true);
        }
        setSystemStatusFlag(SYSTEM_STATUS_SDCARD_CHECK_BAD); // reduced web interface going to be loaded
    }

    // Migrate parameter in config.ini to new naming (firmware 15.0 and newer)
    // ********************************************
    migrateConfiguration();

    // Init time (as early as possible, but SD card needs to be initialized)
    // ********************************************
    setupTime();    // NTP time service: Status of time synchronization will be checked after every cycle (server_tflite.cpp)

    // Set CPU Frequency (default: 160Mhz)
    // ********************************************
    setCPUFrequency();

    // SD card: Create further mandatory directories (if not already existing)
    // Correct creation of these folders will be checked with function "SDCardCheckFolderFilePresence"
    // ********************************************
    MakeDir("/sdcard/firmware");         // mandatory for OTA firmware update
    MakeDir("/sdcard/img_tmp");          // mandatory for setting up alignment marker
    MakeDir("/sdcard/demo");             // mandatory for demo mode
    MakeDir("/sdcard/config/certs");     // mandatory for TLS encryption

    // Check for updates
    // Note: OTA status check only necessary if OTA rollback feature is enabled
    // ********************************************
    #ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    CheckOTAPartitionState();
    #endif
    CheckOTAUpdate();

    // Start SoftAP for initial remote setup
    // Note: Start AP if no wlan.ini and/or config.ini available, e.g. SD card empty; function does not exit anymore until reboot
    // ********************************************
    #ifdef ENABLE_SOFTAP
        CheckStartAPMode(); 
    #endif

    // SD card: Check presence of some mandatory folders / files
    // ********************************************
    if (!SDCardCheckFolderFilePresence()) {
        StatusLED(SDCARD_CHECK, 4, true);
        setSystemStatusFlag(SYSTEM_STATUS_FOLDER_CHECK_BAD); // reduced web interface going to be loaded
    }

    // Check version information
    // ********************************************
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, getFwVersion() + " | Build time: " + std::string(BUILD_TIME) + " | WebUI: " + getHTMLversion());

    if (getHTMLcommit().substr(0, 7) == "?")
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, std::string("Failed to read file html/version.txt to parse WebUI version"));
 
    if (getHTMLcommit().substr(0, 7) != std::string(GIT_REV).substr(0, 7)) { // Compare the first 7 characters of both hashes
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "WebUI version (" + getHTMLcommit() + ") does not match firmware version (" + std::string(GIT_REV) + ")");
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Recommendation: Repeat OTA update using AI-on-the-edge-device__update__*.zip");    
    }

    // Check reboot reason
    // ********************************************
    CheckIsPlannedReboot();
    if (!getIsPlannedReboot() && (esp_reset_reason() == ESP_RST_PANIC)) {  // If system reboot was not triggered by user and reboot was caused by execption 
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Reset reason: " + getResetReason());
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Device was rebooted due to a software exception! Log level is set to DEBUG until the next reboot. "
                                               "Flow init is delayed by 5 minutes to check the logs or do an OTA update"); 
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Keep device running until crash occurs again and check logs after device is up again");
        LogFile.setLogLevel(ESP_LOG_DEBUG);
        setTaskAutoFlowState(FLOW_TASK_STATE_INIT_DELAYED);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Reset reason: " + getResetReason());
    }

    #ifdef HEAP_TRACING_MAIN_START
        ESP_ERROR_CHECK( heap_trace_stop() );
        heap_trace_dump(); 
    #endif
    
    #ifdef HEAP_TRACING_MAIN_WIFI
        ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
    #endif

    // Read WLAN parameter and start WIFI
    // ********************************************
    int iWLANStatus = LoadWlanFromFile(WLAN_CONFIG_FILE);
    if (iWLANStatus == 0) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "WLAN config loaded, init WIFI");
        if (wifi_init_sta() != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "WIFI init failed. Device init aborted");
            StatusLED(WLAN_INIT, 3, true);
            return;
        }
    }
    else if (iWLANStatus == -1) {  // wlan.ini not available, potentially empty or content not readable
        StatusLED(WLAN_INIT, 1, true);
        return; // No way to continue without reading the wlan.ini
    }
    else if (iWLANStatus == -2) { // SSID not configured
        StatusLED(WLAN_INIT, 2, true);
        return; // No way to continue with empty SSID
    }

    xDelay = 2000 / portTICK_PERIOD_MS;
    ESP_LOGD(TAG, "main: sleep for: %ldms", (long) xDelay * CONFIG_FREERTOS_HZ/portTICK_PERIOD_MS);
    vTaskDelay( xDelay );

    // manual reset the time
    // ********************************************
    if (!time_manual_reset_sync())
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Manual Time Sync failed during startup" );

    // Set log level for wifi component to WARN level (default: INFO; only relevant for serial console)
    // ********************************************
    esp_log_level_set("wifi", ESP_LOG_WARN);
  
    #ifdef HEAP_TRACING_MAIN_WIFI
        ESP_ERROR_CHECK( heap_trace_stop() );
        heap_trace_dump(); 
    #endif   

    #ifdef USE_HIMEM_IF_AVAILABLE
        #ifdef DEBUG_HIMEM_MEMORY_CHECK
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Himem mem check : " + himem_memory_check() );
            ESP_LOGD(TAG, "Himem mem check %s", himem_memory_check().c_str());
        #endif
    #endif
   
    // Init external PSRAM
    // ********************************************
    esp_err_t PSRAMStatus = esp_psram_init();
    if (PSRAMStatus == ESP_FAIL) {  // ESP_FAIL -> Failed to init PSRAM
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "PSRAM init failed (" + std::to_string(PSRAMStatus) + ")! PSRAM not found or defective");
        setSystemStatusFlag(SYSTEM_STATUS_PSRAM_BAD);
        StatusLED(PSRAM_INIT, 1, true);
    }
    else { // ESP_OK -> PSRAM init OK --> continue to check PSRAM size
        size_t psram_size = esp_psram_get_size(); // size_t psram_size = esp_psram_get_size(); // comming in IDF 5.0
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "PSRAM size: " + std::to_string(psram_size) + " byte (" + std::to_string(psram_size/1024/1024) + 
                                               "MB / " + std::to_string(psram_size/1024/1024*8) + "MBit)");

        // Check PSRAM size
        // ********************************************
        if (psram_size < (4*1024*1024)) { // PSRAM is below 4 MBytes (32Mbit)
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "PSRAM size >= 4MB (32Mbit) is mandatory to run this application");
            setSystemStatusFlag(SYSTEM_STATUS_PSRAM_BAD);
            StatusLED(PSRAM_INIT, 2, true);
        }
        else { // PSRAM size OK --> continue to check heap size
            size_t _hsize = getESPHeapSizeTotal();
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Total heap: " + std::to_string(_hsize) + " byte");

            // Check heap memory
            // ********************************************
            if (_hsize < 4000000) { // Check available Heap memory for a bit less than 4 MB (a test on a good device showed 4187558 bytes to be available)
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Total heap >= 4000000 byte is mandatory to run this application");
                setSystemStatusFlag(SYSTEM_STATUS_HEAP_TOO_SMALL);
                StatusLED(PSRAM_INIT, 3, true);
            }
            else { // HEAP size OK --> continue to camera init
                // Init camera
                // ********************************************
                esp_err_t camStatus = Camera.InitCam();
                Camera.LightOnOff(false);

                // Check camera init
                // ********************************************
                if (camStatus != ESP_OK) { // Camera init failed, try to reinit during flow init (MainFlowControl.cpp -> doInit())
                    StatusLED(CAM_INIT, 1, false);
                }
                else { // ESP_OK -> Camera init OK
                    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init camera successful");
                    Camera.printCamInfo();
                }
            }
        }
    }

    // Print Device info
    // ********************************************
    printDeviceInfo();
    
    // Print SD-Card info
    // ********************************************
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "SD card info: Name: " + getSDCardName() + ", Capacity: " + 
                        getSDCardCapacity() + "MB, Free: " + getSDCardFreePartitionSpace() + "MB");


    // Start webserver + register handler
    // ********************************************
    ESP_LOGD(TAG, "starting servers");
    server = start_webserver();   
    register_server_camera_uri(server); 
    register_server_main_flow_task_uri(server);
    register_server_file_uri(server, "/sdcard");
    register_server_ota_sdcard_uri(server);
    #ifdef ENABLE_MQTT
        register_server_mqtt_uri(server);
    #endif //ENABLE_MQTT

    gpio_handler_create(server);

    ESP_LOGD(TAG, "Before reg server main");
    register_server_main_uri(server, "/sdcard");

    // Check basic device init status
    // ********************************************
    if (getSystemStatus() == 0) { // Continue with regular boot sequence
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Basic device initialization completed");
        CreateMainFlowTask(); // Create main flow task
    }
    // Critical error(s) occured which do not allow to continue with regular boot sequence.
    // Provding only a reduced web interface for diagnostic purpose. Reduced web interface and interlock: server_main.cpp -> hello_main_handler()
    else { 
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Basic device initialization failed");
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
            StatusLED(SDCARD_NVS_INIT, 4, true);
        } 
        else if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
            ESP_LOGE(TAG, "NVS flash init failed. No free NVS pages found");
            StatusLED(SDCARD_NVS_INIT, 5, true);
        }
        else {
            ESP_LOGE(TAG, "NVS flash init failed. Check error code");
            StatusLED(SDCARD_NVS_INIT, 6, true);
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
            StatusLED(SDCARD_NVS_INIT, 1, true);
        } 
        else if (ret == 263) { // Error code: 0x107 --> usually: SD not found
            ESP_LOGE(TAG, "SD card init failed. Check if SD card is properly inserted into SD card slot or try another card");
            StatusLED(SDCARD_NVS_INIT, 2, true);
        }
        else {
            ESP_LOGE(TAG, "SD card init failed. Check error code or try another card");
            StatusLED(SDCARD_NVS_INIT, 3, true);
        }
        return ret;
    }

    SaveSDCardInfo(card);
    return ret;
}


void migrateConfiguration(void)
{
    bool migrated = false;

    if (!FileExists(CONFIG_FILE)) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Config file seems to be missing");
        return;	
    }

    std::string section = "";
	std::ifstream ifs(CONFIG_FILE);
  	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	
    /* Split config file it array of lines */
    std::vector<std::string> configLines = splitString(content);

    /* Process each line */
    for (int i = 0; i < configLines.size(); i++) {
        //ESP_LOGI(TAG, "Line %d: %s", i, configLines[i].c_str());

        if (configLines[i].find("[") != std::string::npos) { // Start of new section
            section = configLines[i];
            replaceString(section, ";", "", false); // Remove possible semicolon (just for the string comparison)
            //ESP_LOGI(TAG, "New section: %s", section.c_str());
        }

        /* Migrate parameters as needed
         * For all boolean and further more parameter, we make them enabled all the time now:
         *  1. If they where disabled, set them to their default value
         *  2. Enable them
         * Notes:
         * The migration has some simplifications:
         *  - Case Sensitiveness must be like in the initial config.ini
         *  - No Whitespace after a semicollon
         *  - Only one whitespace before/after the equal sign
         */
        if (section == "[MakeImage]") {
            migrated = migrated | replaceString(configLines[i], "[MakeImage]", "[TakeImage]"); // Rename the section itself
        }

        if (section == "[MakeImage]" || section == "[TakeImage]") {
            migrated = migrated | replaceString(configLines[i], "LogImageLocation", "RawImagesLocation");
            migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "RawImagesRetention");

            migrated = migrated | replaceString(configLines[i], ";FixedExposure = true", ";FixedExposure = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";FixedExposure", "FixedExposure"); // Enable it

            migrated = migrated | replaceString(configLines[i], ";Demo = true", ";Demo = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";Demo", "Demo"); // Enable it
        }

        if (section == "[Alignment]") {
            if (isInString(configLines[i], "AlignmentAlgo") && isInString(configLines[i], ";")) { // It is the parameter "AlignmentAlgo" and it is commented out
                migrated = migrated | replaceString(configLines[i], "highAccuracy", "default"); // Set it to its default value and enable it
                migrated = migrated | replaceString(configLines[i], "fast", "default"); // Set it to its default value and enable it
                migrated = migrated | replaceString(configLines[i], "off", "default"); // Set it to its default value and enable it
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            migrated = migrated | replaceString(configLines[i], ";FlipImageSize = true", ";FlipImageSize = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";FlipImageSize", "FlipImageSize"); // Enable it

            migrated = migrated | replaceString(configLines[i], ";InitialMirror = true", ";InitialMirror = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";InitialMirror", "InitialMirror"); // Enable it
        }

        if (section == "[Digits]") {
            if (isInString(configLines[i], "CNNGoodThreshold")) { // It is the parameter "CNNGoodThreshold"
                migrated = migrated | replaceString(configLines[i], "0.5", "0.0");
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
            migrated = migrated | replaceString(configLines[i], "LogImageLocation", "ROIImagesLocation");
            migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "ROIImagesRetention");
        }

        if (section == "[Analog]") {
            migrated = migrated | replaceString(configLines[i], "LogImageLocation", "ROIImagesLocation");
            migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "ROIImagesRetention");
            migrated = migrated | replaceString(configLines[i], "CNNGoodThreshold", ";UNUSED_PARAMETER"); // This parameter is no longer used          
            migrated = migrated | replaceString(configLines[i], "ExtendedResolution", ";UNUSED_PARAMETER"); // This parameter is no longer used
        }

        if (section == "[PostProcessing]") {
            migrated = migrated | replaceString(configLines[i], ";PreValueUse = true", ";PreValueUse = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";PreValueUse", "PreValueUse"); // Enable it
            migrated = migrated | replaceString(configLines[i], "PreValueUse", "FallbackValueUse"); // Rename it

            migrated = migrated | replaceString(configLines[i], ";PreValueAgeStartup", "PreValueAgeStartup"); // Enable it
            migrated = migrated | replaceString(configLines[i], "PreValueAgeStartup", "FallbackValueAgeStartup");

            migrated = migrated | replaceString(configLines[i], ";CheckDigitIncreaseConsistency = true", ";CheckDigitIncreaseConsistency = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";CheckDigitIncreaseConsistency", "CheckDigitIncreaseConsistency"); // Enable it

            if (isInString(configLines[i], "DecimalShift") && isInString(configLines[i], ";")) { // It is the parameter "DecimalShift" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* AllowNegativeRates has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "AllowNegativeRates") && isInString(configLines[i], ";")) { // It is the parameter "AllowNegativeRates" and it is commented out
                migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "AnalogDigitalTransitionStart") && isInString(configLines[i], ";")) { // It is the parameter "AnalogDigitalTransitionStart" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* MaxRateType has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "MaxRateType")) { // It is the parameter "MaxRateType"
                if (isInString(configLines[i], ";")) { // if disabled
                    migrated = migrated | replaceString(configLines[i], "= Off", "= RatePerMin"); // Convert it to its default value
                    migrated = migrated | replaceString(configLines[i], "= RateChange", "= RatePerMin"); // Convert it to its default value
                    migrated = migrated | replaceString(configLines[i], "= AbsoluteChange", "= RatePerMin"); // Convert it to its default value
                    migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                }
                else {
                    migrated = migrated | replaceString(configLines[i], "= Off", "= RateOff"); // Convert it to its new name
                    migrated = migrated | replaceString(configLines[i], "= RateChange", "= RatePerMin"); // Convert it to its new name
                    migrated = migrated | replaceString(configLines[i], "= AbsoluteChange", "= RatePerProcessing"); // Convert it to its new name
                }
            }

            if (isInString(configLines[i], "MaxRateValue") && isInString(configLines[i], ";")) { // It is the parameter "MaxRateValue" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* ExtendedResolution has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "ExtendedResolution") && isInString(configLines[i], ";")) { // It is the parameter "ExtendedResolution" and it is commented out
                migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
        
            /* IgnoreLeadingNaN has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "IgnoreLeadingNaN") && isInString(configLines[i], ";")) { // It is the parameter "IgnoreLeadingNaN" and it is commented out
                migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
        }

        if (section == "[MQTT]") {
            migrated = migrated | replaceString(configLines[i], ";Uri", "Uri"); // Enable it
            migrated = migrated | replaceString(configLines[i], ";MainTopic", "MainTopic"); // Enable it
            migrated = migrated | replaceString(configLines[i], ";ClientID", "ClientID"); // Enable it

            if (isInString(configLines[i], "CACert") && !isInString(configLines[i], "TLSCACert")) {
                migrated = migrated | replaceString(configLines[i], "CACert =", "TLSCACert ="); // Rename it
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "ClientCert") && !isInString(configLines[i], "TLSClientCert")) {
                migrated = migrated | replaceString(configLines[i], "ClientCert =", "TLSClientCert ="); // Rename it
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "ClientKey") && !isInString(configLines[i], "TLSClientKey")) {
                migrated = migrated | replaceString(configLines[i], "ClientKey =", "TLSClientKey ="); // Rename it
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            migrated = migrated | replaceString(configLines[i], "SetRetainFlag", "RetainMessages"); // First rename it, enable it with its default value
            migrated = migrated | replaceString(configLines[i], ";RetainMessages = true", ";RetainMessages = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";RetainMessages", "RetainMessages"); // Enable it

            migrated = migrated | replaceString(configLines[i], ";HomeassistantDiscovery = true", ";HomeassistantDiscovery = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";HomeassistantDiscovery", "HomeassistantDiscovery"); // Enable it

            if (isInString(configLines[i], "MeterType") && isInString(configLines[i], ";")) { // It is the parameter "MeterType" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (configLines[i].rfind("Topic", 0) != std::string::npos)  // only if string starts with "Topic" (Was the naming in very old version)
            {
                migrated = migrated | replaceString(configLines[i], "Topic", "MainTopic");
            }
        }

        if (section == "[InfluxDB]") {
            if (isInString(configLines[i], "Uri") && isInString(configLines[i], ";")) { // It is the parameter "Uri" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "Database") && isInString(configLines[i], ";")) { // It is the parameter "Database" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* Measurement has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Measurement") && isInString(configLines[i], ";")) { // It is the parameter "Measurement" and is it disabled
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* Fieldname has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Fieldname")) { // It is the parameter "Fieldname"
                migrated = migrated | replaceString(configLines[i], "Fieldname", "Field"); // Rename it to Field
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* Field has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Field") && isInString(configLines[i], ";")) { // It is the parameter "Field" and is it disabled
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
        }

        if (section == "[InfluxDBv2]") {
            if (isInString(configLines[i], "Uri") && isInString(configLines[i], ";")) { // It is the parameter "Uri" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "Database") && isInString(configLines[i], ";")) { // It is the parameter "Database" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
            if (isInString(configLines[i], "Database")) { // It is the parameter "Database"
                migrated = migrated | replaceString(configLines[i], "Database", "Bucket"); // Rename it to Bucket
            }
            
            /* Measurement has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Measurement") && isInString(configLines[i], ";")) { // It is the parameter "Measurement" and is it disabled
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* Fieldname has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Fieldname")) { // It is the parameter "Fieldname"
                migrated = migrated | replaceString(configLines[i], "Fieldname", "Field"); // Rename it to Field
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            /* Field has a <NUMBER> as prefix! */
            if (isInString(configLines[i], "Field") && isInString(configLines[i], ";")) { // It is the parameter "Field" and is it disabled
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
        }

        if (section == "[GPIO]") {

        }

        if (section == "[DataLogging]") {
            /* DataLogActive is true by default! */
            migrated = migrated | replaceString(configLines[i], ";DataLogActive = false", ";DataLogActive = true"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";DataLogActive", "DataLogActive"); // Enable it

            migrated = migrated | replaceString(configLines[i], "DataLogRetentionInDays", "DataFilesRetention");
        }

        if (section == "[AutoTimer]") {
            migrated = migrated | replaceString(configLines[i], ";AutoStart = true", ";AutoStart = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";AutoStart", "AutoStart"); // Enable it

            migrated = migrated | replaceString(configLines[i], "Intervall", "Interval");
        }

        if (section == "[Debug]") {
            migrated = migrated | replaceString(configLines[i], "Logfile ", "LogLevel "); // Whitespace needed so it does not match `LogfileRetentionInDays`
            /* LogLevel (resp. LogFile) was originally a boolean, but we switched it to an int
             * For both cases (true/false), we set it to level 2 (WARNING) */
            migrated = migrated | replaceString(configLines[i], "LogLevel = true", "LogLevel = 2");
            migrated = migrated | replaceString(configLines[i], "LogLevel = false", "LogLevel = 2");
            
            migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "LogfilesRetention");
        }

        if (section == "[System]") {
            if (isInString(configLines[i], "TimeServer = undefined") && isInString(configLines[i], ";")) 
            { // It is the parameter "TimeServer" and is it disabled
                migrated = migrated | replaceString(configLines[i], "undefined", "pool.ntp.org");
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "TimeZone") && isInString(configLines[i], ";")) { // It is the parameter "TimeZone" and it is commented out
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }

            if (isInString(configLines[i], "Hostname") && isInString(configLines[i], ";")) { // It is the parameter "Hostname" and is it disabled
                migrated = migrated | replaceString(configLines[i], "undefined", "watermeter");
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
                                 
            migrated = migrated | replaceString(configLines[i], "RSSIThreashold", "RSSIThreshold");

            if (isInString(configLines[i], "CPUFrequency") && isInString(configLines[i], ";")) { // It is the parameter "CPUFrequency" and is it disabled
                migrated = migrated | replaceString(configLines[i], "240", "160");
                migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
            }
            
            migrated = migrated | replaceString(configLines[i], "AutoAdjustSummertime", ";UNUSED_PARAMETER"); // This parameter is no longer used

            migrated = migrated | replaceString(configLines[i], ";SetupMode = true", ";SetupMode = false"); // Set it to its default value
            migrated = migrated | replaceString(configLines[i], ";SetupMode", "SetupMode"); // Enable it
        }
    }

    if (migrated) { // At least one replacement happened
        if (! RenameFile(CONFIG_FILE, CONFIG_FILE_BACKUP)) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create backup of config file");
            return;
        }

        FILE* pfile = fopen(CONFIG_FILE, "w");        
        for (int i = 0; i < configLines.size(); i++) {
            fwrite(configLines[i].c_str() , configLines[i].length(), 1, pfile);
            fwrite("\n" , 1, 1, pfile);
        }
        fclose(pfile);
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Config file migrated. Saved backup to " + std::string(CONFIG_FILE_BACKUP));
    }
}
