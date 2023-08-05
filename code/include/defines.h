#pragma once
#ifndef defines_h
#define defines_h

/////////////////////////////////////////////
////          Global definitions         ////
/////////////////////////////////////////////

    //********* debug options :  *************

    //can be set in platformio with -D OPTION_TO_ACTIVATE

    //#define DEBUG_DETAIL_ON 
    //#define DEBUG_DISABLE_BROWNOUT_DETECTOR
    //#define DEBUG_ENABLE_SYSINFO
    //#define DEBUG_ENABLE_PERFMON
    //#define DEBUG_HIMEM_MEMORY_CHECK
    // need [env:esp32cam-dev-himem]
    //=> CONFIG_SPIRAM_BANKSWITCH_ENABLE=y
    //=> CONFIG_SPIRAM_BANKSWITCH_RESERVE=4


    // use himem //https://github.com/jomjol/AI-on-the-edge-device/issues/1842
    #if (CONFIG_SPIRAM_BANKSWITCH_ENABLE)
        #define USE_HIMEM_IF_AVAILABLE 1
    #endif

    /* Uncomment this to generate task list with stack sizes using the /heap handler
        PLEASE BE AWARE: The following CONFIG parameters have to to be set in 
        sdkconfig.defaults before use of this function is possible!!
        CONFIG_FREERTOS_USE_TRACE_FACILITY=1
        CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
        CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
    */
    // server_tflite.cpp
    //#define TASK_ANALYSIS_ON


    //Memory leak tracing
    //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/heap_debug.html#heap-information
    //need CONFIG_HEAP_TRACING_STANDALONE=y or #define CONFIG_HEAP_TRACING_STANDALONE
    //all setup is predifined in [env:esp32cam-dev-task-analysis]
    //#define HEAP_TRACING_MAIN_WIFI || HEAP_TRACING_MAIN_START //enable heap tracing per function in main.cpp
    //all defines in [env:esp32cam-dev-task-analysis]
    //#define HEAP_TRACING_MAIN_WIFI
    //#define HEAP_TRACING_MAIN_START
    //#define HEAP_TRACING_CLASS_FLOW_CNN_GENERAL_DO_ALING_AND_CUT

    /* Uncomment this to keep the logfile open for appending.
    * If commented out, the logfile gets opened/closed for each log measage (old behaviour) */
    // ClassLogFile
    //#define KEEP_LOGFILE_OPEN_FOR_APPENDING


    /* General DEBUG FLAG */
    //#define DEBUG_DETAIL_ON 

  //****************************************

    //compiler optimization for tflite-micro-esp-examples
    #define XTENSA
    //#define CONFIG_IDF_TARGET_ARCH_XTENSA     //not needed with platformio/espressif32 @ 5.2.0


    //Statusled + ClassControllCamera
    #define BLINK_GPIO GPIO_NUM_33              // PIN for red board LED


    //ClassControllCamera
    #define FLASH_GPIO GPIO_NUM_4               // PIN for flashlight LED
    #define USE_PWM_LEDFLASH                    // if __LEDGLOBAL is defined, a global variable is used for LED control, otherwise locally and each time a new
    #define CAM_LIVESTREAM_REFRESHRATE 500      // Camera livestream feature: Waiting time in milliseconds to refresh image


    //ClassControllCamera + ClassFlowTakeImage
    #define CAMERA_MODEL_AI_THINKER
    #define BOARD_ESP32CAM_AITHINKER
    #define DEMO_IMAGE_SIZE 30000 // Max size of demo image in bytes

    //server_GPIO
    #define __LEDGLOBAL


    //server_GPIO + server_file + SoftAP
    #define CONFIG_FILE "/sdcard/config/config.ini"
    #define CONFIG_FILE_BACKUP "/sdcard/config/config.bak"


    //interface_mqtt + read_wlanini
    #define __HIDE_PASSWORD


    //ClassFlowControll + Main + SoftAP
    #define WLAN_CONFIG_FILE "/sdcard/wlan.ini"


    //main
    #define __SD_USE_ONE_LINE_MODE__

    // server_file + Helper
     #define FILE_PATH_MAX (255) //Max length a file path can have on storage
    

    //server_file +(ota_page.html + upload_script.html)
    #define MAX_FILE_SIZE   (8000*1024) // 8 MB Max size of an individual file. Make sure this value is same as that set in upload_script.html and ota_page.html!
    #define MAX_FILE_SIZE_STR "8MB"
         
    #define LOGFILE_LAST_PART_BYTES 80 * 1024 // 80 kBytes  // Size of partial log file to return 

    #define SERVER_FILER_SCRATCH_BUFSIZE  4096 
    #define SERVER_HELPER_SCRATCH_BUFSIZE  8192
    #define SERVER_OTA_SCRATCH_BUFSIZE  1024 


    //server_file + server_help
    #define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)


    //server_ota
    #define HASH_LEN 32 // SHA-256 digest length
    #define OTA_URL_SIZE 256


    //ClassFlow + ClassFlowImage + server_tflite
    #define LOGFILE_TIME_FORMAT "%Y%m%d-%H%M%S"
    #define LOGFILE_TIME_FORMAT_DATE_EXTR substr(0, 8)
    #define LOGFILE_TIME_FORMAT_HOUR_EXTR substr(9, 2)


    //ClassFlowControll
    #define READOUT_TYPE_TIMESTAMP_PROCESSED     0
    #define READOUT_TYPE_TIMESTAMP_FALLBACKVALUE 1
    #define READOUT_TYPE_VALUE                   2
    #define READOUT_TYPE_FALLBACKVALUE           3
    #define READOUT_TYPE_RAWVALUE                4
    #define READOUT_TYPE_ERROR                   5
    #define READOUT_TYPE_RATE_PER_MIN            6
    #define READOUT_TYPE_RATE_PER_ROUND          7

    //ClassFlowMQTT
    #define LWT_TOPIC        "connection"
    #define LWT_CONNECTED    "connected"
    #define LWT_DISCONNECTED "connection lost"


    //ClassFlowPostProcessing + Influxdb + Influxdbv2
    #define TIME_FORMAT_OUTPUT "%Y-%m-%dT%H:%M:%S%z"
    #define FALLBACKVALUE_TIME_FORMAT_INPUT "%d-%d-%dT%d:%d:%d"


    //CImageBasis
    #define HTTP_BUFFER_SENT 1024
    #define MAX_JPG_SIZE 128000

    //make_stb + stb_image_resize + stb_image_write + stb_image //do not work if not in make_stb.cpp
    //#define STB_IMAGE_IMPLEMENTATION
    //#define STB_IMAGE_WRITE_IMPLEMENTATION
    //#define STB_IMAGE_RESIZE_IMPLEMENTATION
    #define STBI_ONLY_JPEG // (save 2% of Flash, but breaks the alignment mark generation, see https://github.com/jomjol/AI-on-the-edge-device/issues/1721)


    //interface_influxdb
    #define MAX_HTTP_OUTPUT_BUFFER 2048


    //server_mqtt
    #define LWT_TOPIC        "connection"
    #define LWT_CONNECTED    "connected"
    #define LWT_DISCONNECTED "connection lost"


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
    //#define WLAN_USE_MESH_ROAMING   // 802.11v (BSS Transition Management) + 802.11k (Radio Resource Management) (ca. 6kB - 8kB internal RAM neccessary)
    //#define WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES  // Client can send query to AP requesting to roam (if RSSI lower than RSSI threshold)

    /* WIFI roaming only client triggered by scanning the channels after each round (only if RSSI < RSSIThreshold) and trigger a disconnect to switch AP */
    #define WLAN_USE_ROAMING_BY_SCANNING


    //ClassFlowCNNGeneral
    #define Analog_error                        3  // 0.3
    #define Digital_Uncertainty                 2  // 0.2
    #define DigitalBand                         3  // 0.3
    #define Digital_Transition_Area_Predecessor 7  // 9.3 - 0.7
    #define Digital_Transition_Area_Forward     97 // 9.7 - Pre-run zero crossing only happens from approx. 9.7 onwards


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
    #define FLOW_NO_TASK                "No Flow Task"
    #define FLOW_START_FLOW_TASK        "Start Flow Task"
    #define FLOW_FLOW_TASK_FAILED       "Flow Creation Failed"
    #define FLOW_INIT_DELAYED           "Initialization - Delayed"
    #define FLOW_INIT                   "Initialization"
    #define FLOW_INIT_FAILED            "Initialization Failed"
    #define FLOW_SETUP_MODE             "Setup Mode"
    #define FLOW_IDLE_NO_AUTOSTART      "Idle - No Autostart"
    #define FLOW_IDLE_AUTOSTART         "Idle - Waiting for Autostart"

    #define FLOW_TAKE_IMAGE             "Take Image"
    #define FLOW_ALIGNMENT              "Image Alignment"
    #define FLOW_PROCESS_DIGIT_ROI      "ROI Digitalization - Digit"
    #define FLOW_PROCESS_ANALOG_ROI     "ROI Digitalization - Analog"
    #define FLOW_POSTPROCESSING         "Post-Processing"
    #define FLOW_PUBLISH_MQTT           "Publish to MQTT"
    #define FLOW_PUBLISH_INFLUXDB       "Publish to InfluxDBv1"
    #define FLOW_PUBLISH_INFLUXDB2      "Publish to InfluxDBv2"

    #define FLOW_ADDITIONAL_TASKS       "Additional Tasks"
    #define FLOW_AUTO_ERROR_HANDLING    "Automatic Error Handling"
    #define FLOW_INVALID_STATE          "Invalid State"

    // Process state misc
    #define FLOWSTATE_ERRORS_IN_ROW_LIMIT 3

    // ClassFlowPostProcessing.cpp: Post-Processing result value status
    #define VALUE_STATUS_000_VALID              "000 - Valid"
    #define VALUE_STATUS_001_NO_DATA_N_SUBST    "001 - No data to substitute N"
    #define VALUE_STATUS_002_RATE_TOO_HIGH_POS  "002 - Rate too high, positive"
    #define VALUE_STATUS_003_RATE_TOO_HIGH_NEG  "003 - Rate too high, negative"


/////////////////////////////////////////////
////      Conditionnal definitions       ////
/////////////////////////////////////////////

//******* camera model 
#if defined(CAMERA_MODEL_WROVER_KIT)
    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1
    #define XCLK_GPIO_NUM    21
    #define SIOD_GPIO_NUM    26
    #define SIOC_GPIO_NUM    27

    #define Y9_GPIO_NUM      35
    #define Y8_GPIO_NUM      34
    #define Y7_GPIO_NUM      39
    #define Y6_GPIO_NUM      36
    #define Y5_GPIO_NUM      19
    #define Y4_GPIO_NUM      18
    #define Y3_GPIO_NUM       5
    #define Y2_GPIO_NUM       4
    #define VSYNC_GPIO_NUM   25
    #define HREF_GPIO_NUM    23
    #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
    #define PWDN_GPIO_NUM     -1
    #define RESET_GPIO_NUM    15
    #define XCLK_GPIO_NUM     27
    #define SIOD_GPIO_NUM     25
    #define SIOC_GPIO_NUM     23

    #define Y9_GPIO_NUM       19
    #define Y8_GPIO_NUM       36
    #define Y7_GPIO_NUM       18
    #define Y6_GPIO_NUM       39
    #define Y5_GPIO_NUM        5
    #define Y4_GPIO_NUM       34
    #define Y3_GPIO_NUM       35
    #define Y2_GPIO_NUM       32
    #define VSYNC_GPIO_NUM    22
    #define HREF_GPIO_NUM     26
    #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
    #define PWDN_GPIO_NUM     GPIO_NUM_32
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM      GPIO_NUM_0
    #define SIOD_GPIO_NUM     GPIO_NUM_26
    #define SIOC_GPIO_NUM     GPIO_NUM_27

    #define Y9_GPIO_NUM       GPIO_NUM_35
    #define Y8_GPIO_NUM       GPIO_NUM_34
    #define Y7_GPIO_NUM       GPIO_NUM_39
    #define Y6_GPIO_NUM       GPIO_NUM_36
    #define Y5_GPIO_NUM       GPIO_NUM_21
    #define Y4_GPIO_NUM       GPIO_NUM_19
    #define Y3_GPIO_NUM       GPIO_NUM_18
    #define Y2_GPIO_NUM        GPIO_NUM_5
    #define VSYNC_GPIO_NUM    GPIO_NUM_25
    #define HREF_GPIO_NUM     GPIO_NUM_23
    #define PCLK_GPIO_NUM     GPIO_NUM_22

#else
    #error "Camera model not selected"
#endif  //camera model

// ******* Board type   
#ifdef BOARD_WROVER_KIT // WROVER-KIT PIN Map

    #define CAM_PIN_PWDN -1  //power down is not used
    #define CAM_PIN_RESET -1 //software reset will be performed
    #define CAM_PIN_XCLK 21
    #define CAM_PIN_SIOD 26
    #define CAM_PIN_SIOC 27

    #define CAM_PIN_D7 35
    #define CAM_PIN_D6 34
    #define CAM_PIN_D5 39
    #define CAM_PIN_D4 36
    #define CAM_PIN_D3 19
    #define CAM_PIN_D2 18
    #define CAM_PIN_D1 5
    #define CAM_PIN_D0 4
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF 23
    #define CAM_PIN_PCLK 22

#endif //// WROVER-KIT PIN Map

    
#ifdef BOARD_ESP32CAM_AITHINKER // ESP32Cam (AiThinker) PIN Map

    #define CAM_PIN_PWDN 32
    #define CAM_PIN_RESET -1 //software reset will be performed
    #define CAM_PIN_XCLK 0
    #define CAM_PIN_SIOD 26
    #define CAM_PIN_SIOC 27

    #define CAM_PIN_D7 35
    #define CAM_PIN_D6 34
    #define CAM_PIN_D5 39
    #define CAM_PIN_D4 36
    #define CAM_PIN_D3 21
    #define CAM_PIN_D2 19
    #define CAM_PIN_D1 18
    #define CAM_PIN_D0 5
    #define CAM_PIN_VSYNC 25
    #define CAM_PIN_HREF 23
    #define CAM_PIN_PCLK 22

#endif // ESP32Cam (AiThinker) PIN Map

// ******* LED definition
#ifdef USE_PWM_LEDFLASH

    //// PWM für Flash-LED
    #define LEDC_TIMER              LEDC_TIMER_1 // LEDC_TIMER_0
    #define LEDC_MODE               LEDC_LOW_SPEED_MODE
    #define LEDC_OUTPUT_IO          FLASH_GPIO // Define the output GPIO
    #define LEDC_CHANNEL            LEDC_CHANNEL_1
    #define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
    #define LEDC_RESOLUTION         (1 << LEDC_TIMER_13_BIT) -1 // 13bit resolution --> 8192: 0 .. 8191
    //#define LEDC_DUTY               (195) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
    #define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

#endif //USE_PWM_LEDFLASH

//softAP
#ifdef ENABLE_SOFTAP
    #define EXAMPLE_ESP_WIFI_SSID      "AI-on-the-Edge"
    #define EXAMPLE_ESP_WIFI_PASS      ""
    #define EXAMPLE_ESP_WIFI_CHANNEL   11
    #define EXAMPLE_MAX_STA_CONN       1
#endif // ENABLE_SOFTAP

#endif // ifndef defines_h
