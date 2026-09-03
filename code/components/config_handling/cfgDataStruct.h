#ifndef CFGDATASTRUCT_H
#define CFGDATASTRUCT_H

#include <string>
#include <vector>

#include <esp_camera.h>
#include <esp_log.h>

#include "../../include/defines.h"

//************************************************
// Enumerations
//************************************************
enum OperationMode {
    OPMODE_SETUP = -1,
    OPMODE_MANUAL = 0,
    OPMODE_AUTO = 1,
    OPMODE_MAX
};


enum AuthMode {
    AUTH_NONE = 0,
    AUTH_BASIC = 1,
    AUTH_TLS = 2,
};


enum TlsServerCertVerification {
    TLS_SERVER_CERT_VERIFICATION_NONE = 0,
    TLS_SERVER_CERT_VERIFICATION_NO_NAME_VALIDATION = 1,
    TLS_SERVER_CERT_VERIFICATION_FULL = 2,
};


enum AlignmentAlgo {
    ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH = 0,
    ALIGNALGO_ROTATE_AND_ALIGN_SAD_3CH = 1,
    ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH_SIMILAR = 2,
    ALIGNALGO_ROTATION_ONLY = 3,
    ALIGNALGO_OFF = 4,
};


enum RoiImageSavingSize {
    ROI_SAVE_FULL_SIZE = 0,
    ROI_SAVE_RESIZED = 1,
    ROI_SAVE_FULL_SIZE_AND_RESIZED = 2,
};


enum MaxRateCheckType {
    RATE_CHECK_OFF = 0,
    RATE_PER_MIN = 1,
    RATE_PER_INTERVAL = 2,
};


enum ProcessDataNotation {
    PROCESSDATA_JSON = 0,
    PROCESSDATA_TOPICS = 1,
    PROCESSDATA_JSON_AND_TOPICS = 2,
};


enum HAMeterType {
    TYPE_NONE = 0,
    WATER_M3 = 1,
    WATER_L = 2,
    WATER_GAL = 3,
    WATER_FT3 = 4,
    GAS_M3 = 5,
    GAS_FT3 = 6,
    ENERGY_WH = 7,
    ENERGY_KWH = 8,
    ENERGY_MWH = 9,
    ENERGY_GJ = 10,
    TEMPERATURE_C = 11,
    TEMPERATURE_F = 12,
    PRESSURE_BAR = 13,
    PRESSURE_PSI = 14,
    WATER_LMIN = 15,
    WATER_GALMIN = 16
};


enum WebhookUploadImage {
    WEBHOOK_PUBLISH_IMAGE_DISABLED = 0,
    WEBHOOK_PUBLISH_IMAGE_ENABLED = 1,
    WEBHOOK_PUBLISH_IMAGE_ON_ERROR_ONLY = 2
};


enum GpioSmartledType {
    LEDTYPE_WS2812 = 0,
    LEDTYPE_WS2812B_UNIVERSAL = 1,
    LEDTYPE_WS2812B_NEW_VARIANT = 2,
    LEDTYPE_WS2812B_OLD_VARIANT = 3,
    LEDTYPE_SK6812 = 4,
    LEDTYPE_WS2813 = 5
};


enum NetworkIpConfig {
    NETWORK_IP_CONFIG_DHCP = 0,
    NETWORK_IP_CONFIG_STATIC = 1,
};


enum NetworkOperationMode {
    NETWORK_OPMODE_DISABLED = -1,
    NETWORK_OPMODE_WLAN_CLIENT = 0,
    NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF = 1,
    NETWORK_OPMODE_WLAN_AP = 2,
    NETWORK_OPMODE_WLAN_AP_TIMED_OFF = 3,
#ifdef BOARD_FEATURE_ETHERNET
    NETWORK_OPMODE_ETHERNET_ONLY = 4,
    NETWORK_OPMODE_ETHERNET_FALLBACK_WLAN = 5,
#endif // BOARD_FEATURE_ETHERNET
    NETWORK_OPMODE_MAX,
};


enum HttpAuthMode {
    HTTP_AUTH_DISABLED = 0,
    HTTP_AUTH_ENABLED = 1
};


//************************************************
// Additional subelement structs
//************************************************
struct SequenceList {
    int sequenceId = -1;
    std::string sequenceName = "";
};


struct RoiElement {
    std::string roiName = "";
    int x = 1;
    int y = 1;
    int dx = 10;
    int dy = 10;
    bool ccw = false; // counter-clock-wise
};


struct RoiPerSequence {
    int sequenceId = -1;
    std::string sequenceName = "";
    std::vector<RoiElement> roi;
};


struct PostProcessingPerSequence {
    int sequenceId = -1;
    std::string sequenceName = "";
    bool enabled = true;
    int decimalShift = 0;
    float analogDigitSyncValue = 9.2;
    bool extendedResolution = true;
    bool ignoreLeadingNaN = false;
    bool checkDigitIncreaseConsistency = false;
    int maxRateCheckType = RATE_PER_MIN;
    float maxRate = 0.150;
    bool allowNegativeRate = false;
    bool useFallbackValue = true;
    int fallbackValueAgeStartup = 720;
};


struct InfluxDBPerSequence {
    int sequenceId = -1;
    std::string sequenceName = "";
    std::string measurementName = "";
    std::string fieldKey1 = "";
};


struct GpioElement {
    int gpioNumber = -1;
    std::string gpioUsage = "";
    std::string pinName = "";
    bool pinEnabled = false;
    std::string pinMode = "input";
    std::string captureMode = "cyclic-polling";
    int inputDebounceTime = 200;
    int pwmFrequency = 5000;
    bool logicActiveLow = false;
    bool exposeToMqtt = false;
    bool exposeToRest = false;
    struct SmartLed {
        int type = LEDTYPE_WS2812;
        int quantity = 1;
        int colorRedChannel = 255;
        int colorGreenChannel = 255;
        int colorBlueChannel = 255;
    } smartLed;
    int intensityCorrectionFactor = 100;
};


struct TLSParams {
    uint8_t serverCertVerification = TLS_SERVER_CERT_VERIFICATION_FULL;
    std::string caCert = "";
    std::string clientCert = "";
    std::string clientKey = "";
};


//************************************************
// MAIN CONFIG STRUCT
// ----------------------------------------------
// Reminder: Do not forget to update unity tests!
//************************************************
struct CfgData {
    // Config File
    struct SectionConfig {
        int version = 6; // NOTE: Increment when existing parameter name changed and add migration routine
        std::string lastModified = "";
    } sectionConfig;

    // Operation Mode
    struct SectionOperationMode {
        int opMode = OPMODE_SETUP;
        float automaticProcessInterval = 1.0;
        bool useDemoImages = false;
    } sectionOperationMode;

    // Take Image
    struct SectionTakeImage {
        struct Flashlight {
            int flashTime = 2000;
            int flashIntensity = 50;
        } flashlight;
        struct Camera {
            camera_model_t cameraModel = CAMERA_OV2640;
            int cameraFrequency = 10;
            int imageQuality = 12;
            int brightness = 0;
            int contrast = 0;
            int saturation = 0;
            int sharpness = 0;
            int exposureControlMode = 1;
            int autoExposureLevel = 0;
            int manualExposureValue = 300;
            int gainControlMode = 1;
            int manualGainValue = 0;
            int specialEffect = 0;
            bool mirrorImage = false;
            bool flipImage = false;
            int zoomFactor = 1000; // 1.000x
            int zoomOffsetX = 0;
            int zoomOffsetY = 0;
        } camera;
        struct Debug {
            bool saveRawImages = false;
            std::string rawImagesLocation = "/log/source";
            int rawImagesRetention = 3;
            bool saveAllFiles = false;
        } debug;
    } sectionTakeImage;

    // Image Alignment
    struct SectionImageAlignment {
        int alignmentAlgo = ALIGNALGO_ROTATE_AND_ALIGN_SAD_1CH;
        struct SearchField {
            int x = 20;
            int y = 20;
        } searchField;
        float imageRotation = 0.0;
        struct Marker {
            int x = 1;
            int y = 1;
        } marker[2];
        struct Debug {
            bool saveDebugInfo = false;
            bool saveAllFiles = false;
        } debug;
    } sectionImageAlignment;

    // Number sequences
    struct NumberSequences {
        std::vector<SequenceList> sequence;
    } sectionNumberSequences;

    // Digit Numbers
    struct SectionDigit {
        bool enabled = true;
        std::string model = "dig-class100_0182_s2_q.tflite"; // with extention, but without path
        float cnnGoodThreshold = 0.80;
        std::vector<RoiPerSequence> sequence;
        struct Debug {
            bool saveRoiImages = false;
            std::string roiImagesLocation = "/log/digit";
            int roiImagesRetention = 3;
            int roiSavingSize = ROI_SAVE_FULL_SIZE;
            bool saveDebugInfo = false;
            bool saveAllFiles = false;
        } debug;
    } sectionDigit;

    // Analog Counter
    struct SectionAnalog {
        bool enabled = true;
        std::string model = "ana-class100_0201_s1_q.tflite"; // with extention, but without path
        float cnnGoodThreshold = 0.80;                       // placeholder: not in use
        std::vector<RoiPerSequence> sequence;
        struct Debug {
            bool saveRoiImages = false;
            std::string roiImagesLocation = "/log/analog";
            int roiImagesRetention = 3;
            int roiSavingSize = ROI_SAVE_FULL_SIZE;
            bool saveDebugInfo = false;
            bool saveAllFiles = false;
        } debug;
    } sectionAnalog;

    // Post Processing
    struct SectionPostProcessing {
        bool enabled = true;
        std::vector<PostProcessingPerSequence> sequence;
        struct Debug {
            bool saveDebugInfo = false;
        } debug;
    } sectionPostProcessing;

    // MQTT service
    struct SectionMqtt {
        bool enabled = false;
        std::string uri = ""; // e.g. mqtt://192.168.x.x:1883
        std::string mainTopic = "watermeter";
        std::string clientID = "watermeter";
        int authMode = AUTH_NONE;
        std::string username = "";
        std::string password = "";
        TLSParams tls;
        int processDataNotation = PROCESSDATA_JSON;
        bool retainProcessData = false;
        struct HomeAssistant {
            bool discoveryEnabled = false;
            std::string discoveryPrefix = "homeassistant";
            std::string statusTopic = "homeassistant/status";
            int meterType = WATER_M3;
            bool retainDiscovery = false;
        } homeAssistant;
    } sectionMqtt;

    // InfluxDB v1.x service
    struct SectionInfluxDBv1 {
        bool enabled = false;
        std::string uri = ""; // e.g. http://192.168.x.x:8086
        std::string database = "";
        int authMode = AUTH_NONE;
        std::string username = "";
        std::string password = "";
        TLSParams tls;
        std::vector<InfluxDBPerSequence> sequence;
    } sectionInfluxDBv1;

    // InfluxDB v2.x service
    struct SectionInfluxDBv2 {
        bool enabled = false;
        std::string uri = ""; // e.g. http://192.168.x.x:8086
        std::string bucket = "";
        std::string organization = "";
        int authMode = AUTH_BASIC; // AUTH_BASIC: Tokenized authentication
        std::string token = "";
        TLSParams tls;
        std::vector<InfluxDBPerSequence> sequence;
    } sectionInfluxDBv2;

    // Webhook service (push-based)
    struct SectionWebhook {
        bool enabled = false;
        std::string uri = ""; // e.g. http://webhook.com/1234567890
        std::string apiKey = "";
        int publishImage = WEBHOOK_PUBLISH_IMAGE_DISABLED;
        int authMode = AUTH_NONE;
        std::string username = "";
        std::string password = "";
        TLSParams tls;
    } sectionWebhook;

    // GPIO
    struct SectionGpio {
        bool customizationEnabled = false;
        std::vector<GpioElement> gpioPin;
    } sectionGpio;

    // Logging options
    struct SectionLog {
        struct Debug {
            int logLevel = ESP_LOG_WARN;
            int logFilesRetention = 5;
            int debugFilesRetention = 5;
        } debug;
        struct Data {
            bool enabled = false;
            int dataFilesRetention = 30;
        } data;
    } sectionLog;

    // Network
    struct SectionNetwork {
#ifdef BOARD_FEATURE_ETHERNET
        int opmode = NETWORK_OPMODE_ETHERNET_FALLBACK_WLAN;
#else
        int opmode = NETWORK_OPMODE_WLAN_CLIENT;
#endif // BOARD_FEATURE_ETHERNET

        int timedOffDelay = 60; // Minutes
        std::string hostname = "watermeter";
        struct Wlan {
            std::string ssid = "";
            std::string password = "";
            struct Ipv4 {
                int networkConfig = NETWORK_IP_CONFIG_DHCP;
                std::string ipAddress = "";
                std::string subnetMask = "";
                std::string gatewayAddress = "";
                std::string dnsServer = "";
            } ipv4;
            struct WlanRoaming {
                bool enabled = false;
                int rssiThreshold = -75;
            } wlanRoaming;
        } wlan;
        struct WlanAp {
            std::string ssid = "AI-on-the-Edge Device";
            std::string password = "";
            int channel = 11;
            struct Ipv4 {
                std::string ipAddress = "192.168.4.1";
            } ipv4;
        } wlanAp;
#ifdef BOARD_FEATURE_ETHERNET
        struct Ethernet {
            struct Ipv4 {
                int networkConfig = NETWORK_IP_CONFIG_DHCP;
                std::string ipAddress = "";
                std::string subnetMask = "";
                std::string gatewayAddress = "";
                std::string dnsServer = "";
            } ipv4;
        } ethernet;
#endif // BOARD_FEATURE_ETHERNET
        struct Time {
            std::string timeSetManual = ""; // Only temporary parameter | Syntax: YYYY-MM-DDTHH:MM:SS
            std::string timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
            struct Ntp {
                bool timeSyncEnabled = true;
                std::string timeServer = ""; // IP-Address or DNS name, e.g. 192.168.x.x OR fritz.box
            } ntp;
            bool processStartInterlock = true;
        } time;
    } sectionNetwork;

    // System
    struct SectionSystem {
        int cpuFrequency = 160;
    } sectionSystem;

    // WebUI
    struct SectionWebUi {
        struct HttpAuth {
            int authMode = HTTP_AUTH_DISABLED;
            std::string username = "aiote";
            std::string password = "";
        } httpAuth;
        struct AutoRefresh {
            struct OverviewPage {
                bool enabled = true;
                int refreshTime = 5;
            } overviewPage;
            struct DataGraphPage {
                bool enabled = false;
                int refreshTime = 60;
            } dataGraphPage;
        } autoRefresh;
    } sectionWebUi;
};

#endif // CFGDATASTRUCT_H
