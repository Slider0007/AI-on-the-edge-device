#ifndef DEFINES_H
#define DEFINES_H

//**************************************************************************************
// ENABLE/DISABLE SOFTWARE MODULE
//**************************************************************************************

// MQTT (Default: enabled)
#ifndef ENV_DISABLE_MQTT    // Disable module by build_flag in platformio.ini
    #ifndef ENABLE_MQTT
        #define ENABLE_MQTT
    #endif
#endif


// InfluxDB v1.x + v2.x (Default: enabled)
#ifndef ENV_DISABLE_INFLUXDB // Disable module by build_flag in platformio.ini
    #ifndef ENABLE_INFLUXDB
        #define ENABLE_INFLUXDB
    #endif
#endif


// Webhook (Default: enabled)
#ifndef ENV_DISABLE_WEBHOOK // Disable module by build_flag in platformio.ini
    #ifndef ENABLE_WEBHOOK
        #define ENABLE_WEBHOOK
    #endif
#endif



// Access point for initial setup (Default: enabled)
#ifndef ENV_DISABLE_SOFTAP // Disable module by build_flag in platformio.ini
    #ifndef ENABLE_SOFTAP
        #define ENABLE_SOFTAP
    #endif
#else
    // If disabled, set CONFIG_ESP_WIFI_SOFTAP_SUPPORT=n in sdkconfig.defaults to save 28k of flash
    #define CONFIG_ESP_WIFI_SOFTAP_SUPPORT 0
#endif



//**************************************************************************************
// GLOABL DEBUG FLAGS
//**************************************************************************************

// Can also be set in platformio.ini with -D OPTION_TO_ACTIVATE
// ****************************************************
//#define DEBUG_DETAIL_ON
//#define DEBUG_DISABLE_BROWNOUT_DETECTOR


// Task memory analysis
// ****************************************************
//#define TASK_ANALYSIS_ON

/* Uncomment this to generate task list with stack sizes using the /heap handler
    PLEASE BE AWARE: The following CONFIG parameters have to to be set in
    sdkconfig.defaults before use of this function is possible!!
    CONFIG_FREERTOS_USE_TRACE_FACILITY=1
    CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
    CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
*/



//**************************************************************************************
// GLOBAL GENERAL FLAGS
//**************************************************************************************

//compiler optimization for tflite-micro-esp-examples
#define XTENSA
//#define CONFIG_IDF_TARGET_ARCH_XTENSA // not needed with platformio/espressif32 @ 5.2.0


//ConfigClass
#define CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE 32768 // Size of preallocated buffer for larger files (cJSON Object, JSON string buffer)


//ClassControlCamera
#define CAM_LIVESTREAM_REFRESHRATE 500 // Camera livestream feature: Waiting time in milliseconds to refresh image
#define DEMO_IMAGE_SIZE 30000 // Max size of demo image in bytes


//server_GPIO + server_file + SoftAP + ClassFlowControl + Main + SoftAP
#define CONFIG_PERSISTENCE_FILE "/sdcard/config/config.json" // Config persistence file for firmware v17.x and newer
#define CONFIG_PERSISTENCE_FILE_BACKUP "/sdcard/config/backup/config_json.bak" // Config persistence backup file for firmware v17.x and newer

#define CONFIG_FILE_LEGACY "/sdcard/config/config.ini" // Config file for firmware v16.x and older
#define CONFIG_FILE_BACKUP_LEGACY "/sdcard/config/backup/config_ini.bak"
#define CONFIG_WIFI_FILE_LEGACY "/sdcard/wlan.ini"
#define CONFIG_WIFI_FILE_BACKUP_LEGACY "/sdcard/config/backup/wlan_ini.bak"


// server_file + Helper
#define FILE_PATH_MAX (255) //Max length a file path can have on storage


//server_file +(ota_page.html + upload_script.html)
#define MAX_FILE_SIZE   (8000*1024) // 8 MB Max size of an individual file. Make sure this value
                                    // is same as that set in upload_script.html and ota_page.html!
#define MAX_FILE_SIZE_STR "8MB"

#define LOGFILE_LAST_PART_BYTES 80 * 1024 // 80 kBytes  // Size of partial log file to return

#define WEBSERVER_SCRATCH_BUFSIZE  4096
#define SERVER_OTA_SCRATCH_BUFSIZE  1024


//server_file + server_help
#define IS_FILE_EXT(filename, ext) \
            (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)


//server_ota
#define HASH_LEN 32 // SHA-256 digest length
#define OTA_URL_SIZE 256


//ClassFlow + ClassLogImage + server_tflite
#define DEFAULT_TIME_FORMAT             "%Y%m%d-%H%M%S"
#define DEFAULT_TIME_FORMAT_DATE_EXTR   substr(0, 8)
#define DEFAULT_TIME_FORMAT_HOUR_EXTR   substr(9, 2)


//ClassLogFile.cpp
#define LOG_FILE_TIME_FORMAT            "log_%Y-%m-%d.txt"
#define DATA_FILE_TIME_FORMAT           "data_%Y-%m-%d.csv"
#define DEBUG_FOLDER_TIME_FORMAT        "%Y%m%d"

#define LOG_ROOT_FOLDER                 "/sdcard/log"
#define LOG_IMAGE_RAW_ROOT_FOLDER       "/sdcard/log/source"
#define LOG_IMAGE_DIGIT_ROOT_FOLDER     "/sdcard/log/digit"
#define LOG_IMAGE_ANALOG_ROOT_FOLDER    "/sdcard/log/analog"
#define LOG_LOGS_ROOT_FOLDER            "/sdcard/log/message"
#define LOG_DATA_ROOT_FOLDER            "/sdcard/log/data"
#define LOG_DEBUG_ROOT_FOLDER           "/sdcard/log/debug"

// Uncomment this to keep the logfile open for appending.
// If commented out, the logfile gets opened/closed for each log measage
//#define KEEP_LOGFILE_OPEN_FOR_APPENDING


//ClassFlowPostProcessing + Influxdb + Influxdbv2
#define TIME_FORMAT_OUTPUT              "%Y-%m-%dT%H:%M:%S%z"
#define FALLBACKVALUE_TIME_FORMAT_INPUT "%d-%d-%dT%d:%d:%d"


//ClassFlowControl
#define READOUT_TYPE_TIMESTAMP_PROCESSED     0
#define READOUT_TYPE_TIMESTAMP_FALLBACKVALUE 1
#define READOUT_TYPE_VALUE                   2
#define READOUT_TYPE_FALLBACKVALUE           3
#define READOUT_TYPE_RAWVALUE                4
#define READOUT_TYPE_VALUE_STATUS            5
#define READOUT_TYPE_RATE_PER_MIN            6
#define READOUT_TYPE_RATE_PER_INTERVAL       7


//ClassFlowMQTT
#define MQTT_STATUS_TOPIC           "/device/status/connection"
#define MQTT_STATUS_ONLINE          "online"
#define MQTT_STATUS_OFFLINE         "offline"
#define MQTT_QOS                    1


//CImageBasis
#define HTTP_BUFFER_SENT 1024
#define MAX_JPG_SIZE 128000

//make_stb + stb_image_resize + stb_image_write + stb_image //do not work if not in make_stb.cpp
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBI_ONLY_JPEG // (save 2% of Flash, but breaks the alignment mark generation, see
                       // https://github.com/jomjol/AI-on-the-edge-device/issues/1721)


//interface_influxdb
#define MAX_HTTP_OUTPUT_BUFFER 2048


// connect_wlan.cpp
//******************************
/* WIFI roaming functionalities 802.11k+v (uses ca. 6kB - 8kB internal RAM; if SCAN CACHE activated: + 1kB / beacon)
PLEASE BE AWARE: The following CONFIG parameters have to to be set in
sdkconfig.defaults before use of this function is possible!!
CONFIG_WPA_11KV_SUPPORT=y
CONFIG_WPA_SCAN_CACHE=n
CONFIG_WPA_MBO_SUPPORT=n
CONFIG_WPA_11R_SUPPORT=n
*/
//#define WLAN_USE_MESH_ROAMING   // 802.11v (BSS Transition Management) + 802.11k (Radio Resource Management)
                                  // (ca. 6kB - 8kB internal RAM neccessary)
//#define WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES  // Client can send query to AP requesting
                                                                   // to roam (if RSSI lower than RSSI threshold)

// WIFI roaming only client triggered by scanning the channels after each
// round (only if RSSI < RSSIThreshold) and trigger a disconnect to switch AP
#define WLAN_USE_ROAMING_BY_SCANNING


//ClassFlowCNNGeneral
#define Analog_error                        3  // 0.3
#define Digital_Uncertainty                 2  // 0.2
#define DigitalBand                         3  // 0.3
#define Digital_Transition_Area_Predecessor 7  // 9.3 - 0.7
#define Digital_Transition_Area_Forward     97 // 9.7 - Pre-run zero crossing only happens from approx. 9.7 onwards


// ClassFlowPostProcessing.cpp: Post-Processing result value status
#define VALUE_STATUS_000_VALID              "000 Valid"
#define VALUE_STATUS_W01_EMPTY_DATA         "W01 Empty data"
#define VALUE_STATUS_001_DATA_N_SUBST       "E90 No data to substitute N"
#define VALUE_STATUS_002_RATE_NEGATIVE      "E91 Rate negative"
#define VALUE_STATUS_003_RATE_TOO_HIGH_NEG  "E92 Rate too high (<)"
#define VALUE_STATUS_004_RATE_TOO_HIGH_POS  "E93 Rate too high (>)"


/* MAIN FLOW CONTROL */
/*********************/
// Flow task states
#define FLOW_TASK_STATE_INIT_DELAYED        0
#define FLOW_TASK_STATE_INIT                1
#define FLOW_TASK_STATE_SETUPMODE           2
#define FLOW_TASK_STATE_IDLE_NO_AUTOSTART   3
#define FLOW_TASK_STATE_IMG_PROCESSING      4
#define FLOW_TASK_STATE_PUBLISH_DATA        5
#define FLOW_TASK_STATE_ADDITIONAL_TASKS    6
#define FLOW_TASK_STATE_IDLE_AUTOSTART      7

// Process state names
#define FLOW_NO_TASK                "No Main Flow Task"
#define FLOW_CREATE_FLOW_TASK       "Main Flow Task Creation"
#define FLOW_FLOW_TASK_FAILED       "Flow Task Creation Failed"
#define FLOW_INIT_DELAYED           "Initialization - Delayed"
#define FLOW_INIT                   "Initialization"
#define FLOW_INIT_WAITING_TIME_SYNC "Initialization - Waiting For Time Sync"
#define FLOW_INIT_FAILED            "Initialization Failed"
#define FLOW_SETUP_MODE             "Setup Mode"
#define FLOW_IDLE_NO_AUTOSTART      "Idle - No Autostart"
#define FLOW_IDLE_AUTOSTART         "Idle - Waiting For Autostart"

#define FLOW_TAKE_IMAGE             "Take Image"
#define FLOW_ALIGNMENT              "Image Alignment"
#define FLOW_PROCESS_DIGIT_ROI      "ROI Processing - Digit"
#define FLOW_PROCESS_ANALOG_ROI     "ROI Processing - Analog"
#define FLOW_POSTPROCESSING         "Post-Processing"
#define FLOW_PUBLISH_MQTT           "Publish to MQTT"
#define FLOW_PUBLISH_INFLUXDB       "Publish to InfluxDBv1"
#define FLOW_PUBLISH_INFLUXDB2      "Publish to InfluxDBv2"
#define FLOW_PUBLISH_WEBHOOK        "Publish to Webhook"

#define FLOW_ADDITIONAL_TASKS       "Additional Tasks"
#define FLOW_POST_EVENT_HANDLING    "Post Process Event Handling"
#define FLOW_INVALID_STATE          "Invalid State"

// Process state misc
#define FLOWSTATE_ERROR_DEVIATION_IN_ROW_LIMIT   3


// SoftAP for initial setup process
#ifdef ENABLE_SOFTAP
    #define AP_ESP_WIFI_SSID      "AI-on-the-Edge"
    #define AP_ESP_WIFI_PASS      ""
    #define AP_ESP_WIFI_CHANNEL   11
    #define AP_MAX_STA_CONN       1
#endif // ENABLE_SOFTAP


// Global flashlight definitions
#define FLASHLIGHT_DEFAULT_LEDC_TIMER           LEDC_TIMER_1
#define FLASHLIGHT_DEFAULT_LEDC_CHANNEL         LEDC_CHANNEL_1
#define FLASHLIGHT_DEFAULT_FREQUENCY            (5000) // 5kHz
#define FLASHLIGHT_DEFAULT_DUTY_RESOLUTION      LEDC_TIMER_13_BIT // 13 bit resolution --> 8192: 0 .. 8191
#define FLASHLIGHT_DEFAULT_RESOLUTION_RANGE     ((1 << FLASHLIGHT_DEFAULT_DUTY_RESOLUTION) - 1) // 13 bit resolution --> 8192: 0 .. 8191
#define FLASHLIGHT_DEFAULT                      "flashlight-default"
#define FLASHLIGHT_PWM                          "flashlight-pwm"
#define FLASHLIGHT_SMARTLED                     "flashlight-smartled"
#define FLASHLIGHT_DIGITAL                      "flashlight-digital"


//*************************************************************************
// HARDWARE RELATED DEFINITIONS
//*************************************************************************

// Define BOARD type
// Define ENV_BOARD_TYPE in platformio.ini
//************************************
#if ENV_BOARD_TYPE == 1
#define BOARD_AITHINKER_ESP32CAM
#define BOARD_TYPE_NAME      "ESP32CAM"             // Keep Board type equal to main board environment name
                                                    // This is used for OTA update package verification (converted to lower case)
#elif ENV_BOARD_TYPE == 2
#define BOARD_XIAO_ESP32S3
#define BOARD_TYPE_NAME      "XIAO-ESP32S3-Sense"   // Keep Board type equal to main board environment name.
                                                    // This is used for OTA update package verification (converted to lower case)
#else
#error "Board type (ENV_BOARD_TYPE) not defined"
#define BOARD_AITHINKER_ESP32CAM
#define BOARD_TYPE_NAME      "Board unknown"
#endif


// Define CAMERA model
// Define ENV_CAMERA_MODEL in platformio.ini
//************************************
#if ENV_CAMERA_MODEL == 1
#define CAMERA_AITHINKER_ESP32CAM_OV2640
#elif ENV_CAMERA_MODEL == 2
#define CAMERA_XIAO_ESP32S3_SENSE_OV2640
#else
#define CAMERA_AITHINKER_ESP32CAM_OV2640
#error "Camera model (ENV_CAMERA_MODEL) not defined"
#endif


// Board types
//************************************
#ifdef BOARD_AITHINKER_ESP32CAM
    #define BOARD_SDCARD_SDMMC_BUS_WIDTH_1                  // Set 1 line SD card operation

    // SD card (operated with SDMMC peripheral)
    //-------------------------------------------------
    #define GPIO_SDCARD_CLK                 GPIO_NUM_14
    #define GPIO_SDCARD_CMD                 GPIO_NUM_15
    #define GPIO_SDCARD_D0                  GPIO_NUM_2
    #ifndef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        #define GPIO_SDCARD_D1              GPIO_NUM_4
        #define GPIO_SDCARD_D2              GPIO_NUM_12
    #endif
    #define GPIO_SDCARD_D3                  GPIO_NUM_13     // Needs to be high to init SD in MMC mode. After init GPIO can be used as spare GPIO


    // LEDs
    //-------------------------------------------------
    #define GPIO_STATUS_LED_ONBOARD         GPIO_NUM_33     // Onboard red status LED
    #define GPIO_FLASHLIGHT_ONBOARD         GPIO_NUM_4      // Onboard flashlight LED

    #ifdef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        #define GPIO_FLASHLIGHT_DEFAULT     GPIO_FLASHLIGHT_ONBOARD // Use onboard flashlight as default flashlight
    #else
        #define GPIO_FLASHLIGHT_DEFAULT     GPIO_NUM_13     // Onboard flashlight cannot be used if SD card operated in 4-line mode -> Define e.g. GPIO13
    #endif

    #define GPIO_FLASHLIGHT_DEFAULT_USE_PWM                 // Default flashlight LED (.e.g onboard LED) is PWM controlled
    //#define GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED          // Default flashlight SmartLED (e.g. onboard WS2812X) controlled

    #ifdef GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED
        #define GPIO_FLASHLIGHT_DEFAULT_SMARTLED_TYPE       LED_WS2812 // Flashlight default: SmartLED type
        #define GPIO_FLASHLIGHT_DEFAULT_SMARTLED_QUANTITY   1          // Flashlight default: SmartLED Quantity
    #endif


    // Serial Port: Improv Serial / Improv WiFi
    //-------------------------------------------------
    #define DEFAULT_UART_NUM        UART_NUM_0              // Use UART
    #define DEFAULT_UART_TX_PIN     GPIO_NUM_1
    #define DEFAULT_UART_RX_PIN     GPIO_NUM_3


    // Spare GPIO
    //-------------------------------------------------
    // Options for usage defintion:
    // - 'spare': Free to use
    // - 'restricted: usage': Restricted usable (WebUI expert view)
    // - 'flashlight-pwm' or 'flashlight-smartled' or 'flashlight-digital' (ON/OFF) -> Map to 'flashlight-default'
    // --> ESP32CAM: flashlight-default -> flashlight-pwm (Onboard LED, PWM controlled)
    //-------------------------------------------------
    #define GPIO_SPARE_PIN_COUNT            6

    #define GPIO_SPARE_1                    GPIO_NUM_1              // Use carefully: UART pin for debug/logging
    #define GPIO_SPARE_1_USAGE              "restricted: uart0-tx"  // Only visible when expert mode is activated

    #define GPIO_SPARE_2                    GPIO_NUM_3              // Use carefully: UART pin for debug/logging
    #define GPIO_SPARE_2_USAGE              "restricted: uart0-rx"  // Only visible when expert mode is activated

    #ifdef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        #define GPIO_SPARE_3                GPIO_FLASHLIGHT_DEFAULT // Use carefully: flashlight-default
        #if defined(GPIO_FLASHLIGHT_DEFAULT_USE_PWM)
            #define GPIO_SPARE_3_USAGE      FLASHLIGHT_PWM          // Define flashlight-default as ...
        #elif defined(GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED)
            #define GPIO_SPARE_3_USAGE      FLASHLIGHT_SMARTLED     // Define flashlight-default as ...
        #else
            #define GPIO_SPARE_3_USAGE      FLASHLIGHT_DIGITAL      // Define flashlight-default as ...
        #endif

        #define GPIO_SPARE_4                GPIO_NUM_12
        #define GPIO_SPARE_4_USAGE          "spare"
    #else
        #define GPIO_SPARE_3                GPIO_NUM_NC    // Not usable, in use for 'SD-card'
        #define GPIO_SPARE_3_USAGE          ""

        #define GPIO_SPARE_4                GPIO_NUM_NC    // Not usable, in use for 'SD-card'
        #define GPIO_SPARE_4_USAGE          ""
    #endif

    #define GPIO_SPARE_5                    GPIO_NUM_13
    #define GPIO_SPARE_5_USAGE              "spare"

    #define GPIO_SPARE_6                    GPIO_NUM_NC     // Not defined spare position
    #define GPIO_SPARE_6_USAGE              ""

#elif defined(BOARD_XIAO_ESP32S3)
    #ifndef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        #define BOARD_SDCARD_SDMMC_BUS_WIDTH_1              // Only 1 line SD card operation is supported (hardware related)
    #endif

    // SD card (operated with SDMMC peripheral)
    //-------------------------------------------------
    #define GPIO_SDCARD_CLK                 GPIO_NUM_7
    #define GPIO_SDCARD_CMD                 GPIO_NUM_9
    #define GPIO_SDCARD_D0                  GPIO_NUM_8
    #define GPIO_SDCARD_D3                  GPIO_NUM_21     // Needs to be high to init with MMC mode. After init GPIO can be used as status LED


    // LEDs
    //-------------------------------------------------
    #define GPIO_STATUS_LED_ONBOARD         GPIO_NUM_21     // Onboard yellow status LED (USER LED)
    #define GPIO_FLASHLIGHT_ONBOARD         GPIO_NUM_NC     // No onboard flashlight available

    #define GPIO_FLASHLIGHT_DEFAULT         GPIO_NUM_1      // Default flashlight GPIO pin (can be modified by activiating GPIO functionality in WebUI)

    #define GPIO_FLASHLIGHT_DEFAULT_USE_PWM                 // Default flashlight LED is PWM controlled
    //#define GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED          // Default flashlight SmartLED (e.g. onboard WS2812X) controlled

    #ifdef GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED
        #define GPIO_FLASHLIGHT_DEFAULT_SMARTLED_TYPE       LED_WS2812 // Flashlight default: SmartLED type
        #define GPIO_FLASHLIGHT_DEFAULT_SMARTLED_QUANTITY   1          // Flashlight default: SmartLED Quantity
    #endif


    // Serial Port: Improv Serial / Improv WiFi
    //-------------------------------------------------
    #define USB_SERIAL                                      // Use USB Serial/JTAG controller console


    // Spare GPIO
    //-------------------------------------------------
    // Options for usage defintion:
    // - 'spare': Free to use
    // - 'restricted: usage': Restricted usable (WebUI expert view)
    // - 'flashlight-pwm' or 'flashlight-smartled' or 'flashlight-digital' (ON/OFF) -> Map to 'flashlight-default'
    // --> ESP32CAM: flashlight-default -> flashlight-pwm (Onboard LED, PWM controlled)
    //-------------------------------------------------
    #define GPIO_SPARE_PIN_COUNT            6

    #define GPIO_SPARE_1                    GPIO_FLASHLIGHT_DEFAULT // Flashlight default
    #if defined(GPIO_FLASHLIGHT_DEFAULT_USE_PWM)
        #define GPIO_SPARE_1_USAGE          FLASHLIGHT_PWM          // Define flashlight-default as ...
    #elif defined(GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED)
        #define GPIO_SPARE_1_USAGE          FLASHLIGHT_SMARTLED     // Define flashlight-default as ...
    #else
        #define GPIO_SPARE_1_USAGE          FLASHLIGHT_DIGITAL      // Define flashlight-default as ...
    #endif

    #define GPIO_SPARE_2                    GPIO_NUM_2
    #define GPIO_SPARE_2_USAGE              "spare"

    #define GPIO_SPARE_3                    GPIO_NUM_3
    #define GPIO_SPARE_3_USAGE              "spare"

    #define GPIO_SPARE_4                    GPIO_NUM_4
    #define GPIO_SPARE_4_USAGE              "spare"

    #define GPIO_SPARE_5                    GPIO_NUM_5
    #define GPIO_SPARE_5_USAGE              "spare"

    #define GPIO_SPARE_6                    GPIO_NUM_6
    #define GPIO_SPARE_6_USAGE              "spare"

#else
    #error "define.h: No board type defined or type unknown"
#endif //Board types


// Camera models
// Further models: https://github.com/Mjrovai/XIAO-ESP32S3-Sense/blob/main/camera_round_display_save_jpeg/camera_pins.h
//************************************
#ifdef CAMERA_AITHINKER_ESP32CAM_OV2640
    #define PWDN_GPIO_NUM       GPIO_NUM_32
    #define RESET_GPIO_NUM      -1
    #define XCLK_GPIO_NUM       GPIO_NUM_0
    #define SIOD_GPIO_NUM       GPIO_NUM_26
    #define SIOC_GPIO_NUM       GPIO_NUM_27

    #define Y9_GPIO_NUM         GPIO_NUM_35
    #define Y8_GPIO_NUM         GPIO_NUM_34
    #define Y7_GPIO_NUM         GPIO_NUM_39
    #define Y6_GPIO_NUM         GPIO_NUM_36
    #define Y5_GPIO_NUM         GPIO_NUM_21
    #define Y4_GPIO_NUM         GPIO_NUM_19
    #define Y3_GPIO_NUM         GPIO_NUM_18
    #define Y2_GPIO_NUM         GPIO_NUM_5
    #define VSYNC_GPIO_NUM      GPIO_NUM_25
    #define HREF_GPIO_NUM       GPIO_NUM_23
    #define PCLK_GPIO_NUM       GPIO_NUM_22

#elif defined(CAMERA_XIAO_ESP32S3_SENSE_OV2640)
    #define PWDN_GPIO_NUM       -1
    #define RESET_GPIO_NUM      -1
    #define XCLK_GPIO_NUM       GPIO_NUM_10
    #define SIOD_GPIO_NUM       GPIO_NUM_40
    #define SIOC_GPIO_NUM       GPIO_NUM_39

    #define Y9_GPIO_NUM         GPIO_NUM_48
    #define Y8_GPIO_NUM         GPIO_NUM_11
    #define Y7_GPIO_NUM         GPIO_NUM_12
    #define Y6_GPIO_NUM         GPIO_NUM_14
    #define Y5_GPIO_NUM         GPIO_NUM_16
    #define Y4_GPIO_NUM         GPIO_NUM_18
    #define Y3_GPIO_NUM         GPIO_NUM_17
    #define Y2_GPIO_NUM         GPIO_NUM_15
    #define VSYNC_GPIO_NUM      GPIO_NUM_38
    #define HREF_GPIO_NUM       GPIO_NUM_47
    #define PCLK_GPIO_NUM       GPIO_NUM_13
#else
    #error "define.h: No camera model defined or model unknown"
#endif //Camera models

#endif //DEFINES_H