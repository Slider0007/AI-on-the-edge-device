#include "configClass.h"
#include "../../include/defines.h"

#include <lwip/sockets.h>
#include <arpa/inet.h>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "cJsonUtils.h"
#include "configMigration.h"
#include "webserver.h"
#include "MainFlowControl.h"
#include "psram.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "gpioControl.h"
#include "time_sntp.h"


static const char *TAG = "CONFIG";

ConfigClass ConfigClass::cfgClass;


static bool isValidIpAddress(const char *ipAddress)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) == 1;
}


ConfigClass::ConfigClass()
{
    // Create a FreeRTOS mutex semaphore to protect JSON buffer & global hooks
    cfgMutex = xSemaphoreCreateMutex();

    if (cfgMutex == nullptr) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ConfigClass: Failed to create mutex");
    }

    static_assert(WEBSERVER_SCRATCH_BUFSIZE >= CONFIG_HANDLING_CJSON_STRING_BUFFER_SIZE,
                  "Webserver scratch buffer must be at least as large as the config JSON string buffer");

    // Use preallocted buffer to avoid fragmentation and reduce internal RAM usage using SPIRAM
    cJsonObjectBuffer = (uint8_t *)heap_caps_calloc(1, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    jsonBuffer = (char *)heap_caps_calloc(1, CONFIG_HANDLING_CJSON_STRING_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!cJsonObjectBuffer || !jsonBuffer) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ConfigClass: Failed to allocate PSRAM buffers");

        if (cJsonObjectBuffer) {
            heap_caps_free(cJsonObjectBuffer);
            cJsonObjectBuffer = nullptr;
        }
        if (jsonBuffer) {
            heap_caps_free(jsonBuffer);
            jsonBuffer = nullptr;
        }
    }

    initCjsonHooks();
}


ConfigClass::~ConfigClass()
{
    clearCfgDataTemp();
    clearCfgData();

    if (cJsonObjectBuffer) {
        heap_caps_free(cJsonObjectBuffer);
        cJsonObjectBuffer = nullptr;
    }

    if (jsonBuffer) {
        heap_caps_free(jsonBuffer);
        jsonBuffer = nullptr;
    }

    if (cfgMutex != nullptr) {
        vSemaphoreDelete(cfgMutex);
        cfgMutex = nullptr;
    }
}


//**************************************************************************************************
// Read configuration from file (JSON notation)
//**************************************************************************************************
void ConfigClass::readConfigFile(bool unityTest, std::string unityTestData)
{
    std::string content;
    bool fallbackCfgChecked = false;

    if (unityTest) {
        clearCfgDataTemp();
        content = std::move(unityTestData);
    }
    else {
        if (readFileToString(CONFIG_PERSISTENCE_FILE, content)) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Config file found");
        }
    }

    if (content.empty()) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No persistent config data | Check for fallback config file");

        if (readFileToString(CONFIG_PERSISTENCE_FILE_FALLBACK, content)) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Fallback config file found");
        }

        if (content.empty()) {
            content = "{}";
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "No persistent config data | Use default config");
            // Continue to try to restore WLAN config from NVS, otherwise Access Point is getting started to reconfigure.
        }

        fallbackCfgChecked = true;
    }

    // Attempt primary parse
    bool parseSuccess = parseJsonFromFile(content.c_str(), unityTest);

    if (!parseSuccess) {
        copyFile(CONFIG_PERSISTENCE_FILE, CONFIG_PERSISTENCE_FILE_INVALID);

        if (!fallbackCfgChecked) { // Try read fallback file if not yet read
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Invalid persistent config data | Check for fallback config file");

            content.clear();

            if (readFileToString(CONFIG_PERSISTENCE_FILE_FALLBACK, content)) {
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "Fallback config file found");
                parseSuccess = parseJsonFromFile(content.c_str(), unityTest);
            }
        }

        if (!parseSuccess) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid persistent config data | Use default config");

            clearCfgDataTemp();
            parseJsonFromFile("{}", unityTest);
        }
    }
}


// Helper: Parse JSON string from file using Thread-Local PSRAM arena and extract into class state
bool ConfigClass::parseJsonFromFile(const char *jsonStr, bool unityTest)
{
    if (!cfgMutex || !cJsonObjectBuffer) {
        return false;
    }

    if (jsonStr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: JSON input is null");
        return false;
    }

    CfgMutexGuard lock(cfgMutex, pdMS_TO_TICKS(5000));
    if (!lock.isAcquired()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: Failed to acquire cfgMutex: Timeout");
        return false;
    }

    // Parse JSON and update internal configuration
    {
        cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

        cJsonObject = cJSON_Parse(jsonStr);
        if (cJsonObject != nullptr) {
            if (parseConfig(true, unityTest) != ESP_OK) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: Failed to update configuration");
                return false;
            }
        }
        else {
            const char *errPtr = cJSON_GetErrorPtr();
            if (errPtr != nullptr) {
                char errorMsg[96] = {};
                snprintf(errorMsg, sizeof(errorMsg), "parseJsonFromFile: Parse JSON error near: %.20s", errPtr);
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, std::string(errorMsg));
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: Parse JSON failed");
            }
            return false;
        }
    } // Free parse arena

    // Serialize updated configuration
    {
        cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

        if (serializeConfig(unityTest) != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: Failed to serialize configuration");
            return false;
        }
    } // Free serialization arena

    // Persist updated configuration
    if (!unityTest && writeConfigFile() != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseJsonFromFile: Failed to write configuration file");
        return false;
    }

    return true;
}


//**************************************************************************************************
// Parse JSON string and save to internal struct
//**************************************************************************************************
esp_err_t ConfigClass::parseConfig(bool init, bool unityTest)
{
    if (!cJsonObject) {
        return ESP_ERR_INVALID_ARG;
    }

    parseSectionConfig(init);
    parseSectionOperationMode();
    parseSectionTakeImage();
    parseSectionImageAlignment();
    parseSectionSequences(init);
    parseSectionRoi("digit", cfgDataTemp.sectionDigit, "_dig");
    parseSectionRoi("analog", cfgDataTemp.sectionAnalog, "_ana");
    parseSectionPostProcessing();
    parseSectionMqtt(unityTest);
    parseSectionInfluxDBv1(unityTest);
    parseSectionInfluxDBv2(unityTest);
    parseSectionWebhook(unityTest);
    parseSectionGpio(init);
    parseSectionLogging();
    parseSectionNetwork(init, unityTest);
    parseSectionSystem();
    parseSectionWebUi(unityTest);

    if (init) {
        if (!unityTest) {
            migrateConfiguration(cJsonObject);
        }
        cfgData = cfgDataTemp;
    }

    cJSON_Delete(cJsonObject);
    cJsonObject = NULL;

    return ESP_OK;
}


void ConfigClass::parseSectionConfig(bool init)
{
    cJSON *config = cJSON_GetObjectItem(cJsonObject, "config");

    cJsonUtils::parseInt(config, {"version"}, cfgDataTemp.sectionConfig.version);

    if (init) {
        cJsonUtils::parseString(config, {"lastmodified"}, cfgDataTemp.sectionConfig.lastModified);
    }
    else {
        cfgDataTemp.sectionConfig.lastModified = getCurrentTimeString(TIME_FORMAT_OUTPUT);
    }
}

void ConfigClass::parseSectionOperationMode()
{
    cJSON *opMode = cJSON_GetObjectItem(cJsonObject, "operationmode");
    auto &section = cfgDataTemp.sectionOperationMode;

    int mode;
    if (cJsonUtils::parseInt(opMode, {"opmode"}, mode)) {
        cfgDataTemp.sectionOperationMode.opMode = (mode < OPMODE_SETUP || mode >= OPMODE_MAX) ? OPMODE_AUTO : mode;
    }

    cJsonUtils::parseFloatClamped(opMode, {"automaticprocessinterval"}, section.automaticProcessInterval, 0.01f,
                                  std::numeric_limits<float>::max());

    cJsonUtils::parseBool(opMode, {"usedemoimages"}, section.useDemoImages);
}

void ConfigClass::parseSectionTakeImage()
{
    cJSON *takeImage = cJSON_GetObjectItem(cJsonObject, "takeimage");
    auto &section = cfgDataTemp.sectionTakeImage;

    cJsonUtils::parseIntClampedMin(takeImage, {"flashlight", "flashtime"}, section.flashlight.flashTime, 100);
    cJsonUtils::parseIntClamped(takeImage, {"flashlight", "flashintensity"}, section.flashlight.flashIntensity, 0, 100);

    cJsonUtils::parseIntClamped(takeImage, {"camera", "cameramodel"}, section.camera.cameraModel, camera_model_t(0), camera_model_t(14));
    cJsonUtils::parseIntClamped(takeImage, {"camera", "camerafrequency"}, section.camera.cameraFrequency, 6, 20);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "imagequality"}, section.camera.imageQuality, 8, 63);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "brightness"}, section.camera.brightness, -2, 2);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "contrast"}, section.camera.contrast, -2, 2);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "saturation"}, section.camera.saturation, -2, 2);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "sharpness"}, section.camera.sharpness, -3, 3);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "exposurecontrolmode"}, section.camera.exposureControlMode, 0, 2);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "autoexposurelevel"}, section.camera.autoExposureLevel, -5, 5);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "manualexposurevalue"}, section.camera.manualExposureValue, 0, 1920);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "gaincontrolmode"}, section.camera.gainControlMode, 0, 1);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "manualgainvalue"}, section.camera.manualGainValue, 0, 30);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "specialeffect"}, section.camera.specialEffect, 0, 7);
    cJsonUtils::parseBool(takeImage, {"camera", "mirrorimage"}, section.camera.mirrorImage);
    cJsonUtils::parseBool(takeImage, {"camera", "flipimage"}, section.camera.flipImage);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "zoomfactor"}, section.camera.zoomFactor, 1000, 4000);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "zoomoffsetx"}, section.camera.zoomOffsetX, -960, 960);
    cJsonUtils::parseIntClamped(takeImage, {"camera", "zoomoffsety"}, section.camera.zoomOffsetY, -720, 720);

    cJsonUtils::parseBool(takeImage, {"debug", "saverawimages"}, section.debug.saveRawImages);
    if (cJsonUtils::parseString(takeImage, {"debug", "rawimageslocation"}, section.debug.rawImagesLocation)) {
        configClassHelper::validatePath(section.debug.rawImagesLocation);
    }
    cJsonUtils::parseIntClampedMin(takeImage, {"debug", "rawimagesretention"}, section.debug.rawImagesRetention, 0);
}

void ConfigClass::parseSectionImageAlignment()
{
    cJSON *imgAlign = cJSON_GetObjectItem(cJsonObject, "imagealignment");
    auto &section = cfgDataTemp.sectionImageAlignment;

    cJsonUtils::parseIntClamped(imgAlign, {"alignmentalgo"}, section.alignmentAlgo, 0, 4);
    cJsonUtils::parseIntClampedMin(imgAlign, {"searchfield", "x"}, section.searchField.x, 1);
    cJsonUtils::parseIntClampedMin(imgAlign, {"searchfield", "y"}, section.searchField.y, 1);
    cJsonUtils::parseFloatClamped(imgAlign, {"imagerotation"}, section.imageRotation, -180.0f, 180.0f);

    // Fixed-size 2-element array
    cJSON *markerArr = cJSON_GetObjectItem(imgAlign, "marker");
    for (int i = 0; i < std::min(cJSON_GetArraySize(markerArr), 2); i++) {
        cJSON *el = cJSON_GetArrayItem(markerArr, i);
        cJsonUtils::parseIntClampedMin(el, {"x"}, section.marker[i].x, 1);
        cJsonUtils::parseIntClampedMin(el, {"y"}, section.marker[i].y, 1);
    }

    cJsonUtils::parseBool(imgAlign, {"debug", "savedebuginfo"}, section.debug.saveDebugInfo);
}

void ConfigClass::parseSectionSequences(bool init)
{
    cJSON *seqArr = cJsonUtils::getNestedItem(cJsonObject, {"numbersequences", "sequence"});
    auto &master = cfgDataTemp.sectionNumberSequences.sequence;

    if (cJSON_GetArraySize(seqArr) > 0) {
        if (init) {
            for (int j = 0; j < cJSON_GetArraySize(seqArr); j++) {
                cJSON *el = cJSON_GetArrayItem(seqArr, j);
                std::string name;
                int id;
                cJsonUtils::parseString(el, {"sequencename"}, name);
                if (cJsonUtils::parseInt(el, {"sequenceid"}, id)) {
                    master.push_back({id, name});
                }
            }
        }
        else {
            // Remove sequences no longer present in the incoming JSON
            master.erase(std::remove_if(master.begin(), master.end(),
                                        [&](const SequenceList &existing) {
                                            for (int j = 0; j < cJSON_GetArraySize(seqArr); j++) {
                                                cJSON *el = cJSON_GetArrayItem(seqArr, j);
                                                int id;
                                                if (!cJsonUtils::parseInt(el, {"sequenceid"}, id)) {
                                                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Sequence ID malformed");
                                                    return false; // keep on malformed entry
                                                }
                                                if (id == existing.sequenceId) {
                                                    return false;
                                                }
                                            }
                                            return true;
                                        }),
                         master.end());

            // Add new (-1 => increment) / update existing (by id)
            for (int j = 0; j < cJSON_GetArraySize(seqArr); j++) {
                cJSON *el = cJSON_GetArrayItem(seqArr, j);
                std::string name;
                cJsonUtils::parseString(el, {"sequencename"}, name);

                int id;
                if (!cJsonUtils::parseInt(el, {"sequenceid"}, id)) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Sequence ID malformed");
                    continue;
                }

                if (id == -1) {
                    int newId = master.empty() ? 0 : master.back().sequenceId + 1;
                    master.push_back({newId, name});
                }
                else if (id > -1) {
                    if (auto *found = configClassHelper::findSequenceByIdOrName(master, id, name)) {
                        found->sequenceName = name;
                    }
                }
            }
        }

        std::sort(master.begin(), master.end(), [](const SequenceList &x, const SequenceList &y) { return x.sequenceId < y.sequenceId; });
    }
    else if (master.empty()) {
        master.push_back({0, "main"});
    }

    // Sync all underlaying  vectors with master vector
    configClassHelper::syncSequenceVectors(master, cfgDataTemp.sectionDigit.sequence, cfgDataTemp.sectionAnalog.sequence,
                                           cfgDataTemp.sectionPostProcessing.sequence, cfgDataTemp.sectionInfluxDBv1.sequence,
                                           cfgDataTemp.sectionInfluxDBv2.sequence);
}

template <typename SectionType> void ConfigClass::parseSectionRoi(const char *sectionKey, SectionType &section, const char *roiSuffix)
{
    cJSON *sectionObj = cJSON_GetObjectItem(cJsonObject, sectionKey);

    cJsonUtils::parseBool(sectionObj, {"enabled"}, section.enabled);
    cJsonUtils::parseString(sectionObj, {"model"}, section.model);
    cJsonUtils::parseFloatClamped(sectionObj, {"cnngoodthreshold"}, section.cnnGoodThreshold, 0.0f, 1.0f); // Only digit

    cJSON *seqArr = cJSON_GetObjectItem(sectionObj, "sequence");
    for (int i = 0; i < cJSON_GetArraySize(seqArr); i++) {
        cJSON *seqEl = cJSON_GetArrayItem(seqArr, i);

        std::string name;
        cJsonUtils::parseString(seqEl, {"sequencename"}, name);

        int id;
        if (!cJsonUtils::parseInt(seqEl, {"sequenceid"}, id)) {
            continue;
        }

        auto *target = configClassHelper::findSequenceByIdOrName(section.sequence, id, name);
        if (!target) {
            continue;
        }

        target->roi.clear();
        target->roi.shrink_to_fit();

        cJSON *roiArr = cJSON_GetObjectItem(seqEl, "roi");
        for (int j = 0; j < cJSON_GetArraySize(roiArr); j++) {
            cJSON *roiEl = cJSON_GetArrayItem(roiArr, j);
            RoiElement roi{};
            roi.roiName = target->sequenceName + roiSuffix + std::to_string(j + 1);

            cJsonUtils::parseIntClampedMin(roiEl, {"x"}, roi.x, 1);
            cJsonUtils::parseIntClampedMin(roiEl, {"y"}, roi.y, 1);
            cJsonUtils::parseIntClampedMin(roiEl, {"dx"}, roi.dx, 1);
            cJsonUtils::parseIntClampedMin(roiEl, {"dy"}, roi.dy, 1);
            cJsonUtils::parseBool(roiEl, {"ccw"}, roi.ccw); // only analog

            target->roi.push_back(roi);
        }
    }

    cJsonUtils::parseBool(sectionObj, {"debug", "saveroiimages"}, section.debug.saveRoiImages);
    if (cJsonUtils::parseString(sectionObj, {"debug", "roiimageslocation"}, section.debug.roiImagesLocation)) {
        configClassHelper::validatePath(section.debug.roiImagesLocation);
    }
    cJsonUtils::parseIntClampedMin(sectionObj, {"debug", "roiimagesretention"}, section.debug.roiImagesRetention, 0);
    cJsonUtils::parseIntClamped(sectionObj, {"debug", "roisavingsize"}, section.debug.roiSavingSize, RoiImageSavingSize(0),
                                RoiImageSavingSize(2));
}


void ConfigClass::parseSectionPostProcessing()
{
    cJSON *seqArr = cJsonUtils::getNestedItem(cJsonObject, {"postprocessing", "sequence"});
    auto &section = cfgDataTemp.sectionPostProcessing;


    for (int i = 0; i < cJSON_GetArraySize(seqArr); i++) {
        cJSON *el = cJSON_GetArrayItem(seqArr, i);

        std::string name;
        if (!cJsonUtils::parseString(el, {"sequencename"}, name)) {
            continue;
        }
        auto *seq = configClassHelper::findSequenceByName(section.sequence, name);
        if (!seq) {
            continue;
        }

        cJsonUtils::parseIntClamped(el, {"decimalshift"}, seq->decimalShift, -9, 9);
        cJsonUtils::parseFloatClamped(el, {"analogdigitsyncvalue"}, seq->analogDigitSyncValue, 6.0f, 9.9f);
        cJsonUtils::parseBool(el, {"extendedresolution"}, seq->extendedResolution);
        cJsonUtils::parseBool(el, {"ignoreleadingnan"}, seq->ignoreLeadingNaN);
        cJsonUtils::parseBool(el, {"checkdigitincreaseconsistency"}, seq->checkDigitIncreaseConsistency);
        cJsonUtils::parseIntClamped(el, {"maxratechecktype"}, seq->maxRateCheckType, 0, 2);
        cJsonUtils::parseFloatClamped(el, {"maxrate"}, seq->maxRate, 0.001f, std::numeric_limits<float>::max());
        cJsonUtils::parseBool(el, {"allownegativerate"}, seq->allowNegativeRate);
        cJsonUtils::parseBool(el, {"usefallbackvalue"}, seq->useFallbackValue);
        cJsonUtils::parseIntClampedMin(el, {"fallbackvalueagestartup"}, seq->fallbackValueAgeStartup, 0);
    }

    cJsonUtils::parseBool(cJsonObject, {"postprocessing", "debug", "savedebuginfo"}, section.debug.saveDebugInfo);
}


void ConfigClass::parseSectionMqtt(bool unityTest)
{
    cJSON *mqtt = cJSON_GetObjectItem(cJsonObject, "mqtt");
    auto &section = cfgDataTemp.sectionMqtt;

    cJsonUtils::parseBool(mqtt, {"enabled"}, section.enabled);
    cJsonUtils::parseString(mqtt, {"uri"}, section.uri);
    if (cJsonUtils::parseString(mqtt, {"maintopic"}, section.mainTopic)) {
        configClassHelper::validateStructure(section.mainTopic);
    }
    cJsonUtils::parseStringValidated(mqtt, {"clientid"}, section.clientID, [](const char *s) { return strlen(s) <= 23; });
    cJsonUtils::parseIntClamped(mqtt, {"authmode"}, section.authMode, 0, 2);
    cJsonUtils::parseString(mqtt, {"username"}, section.username);
    parseSecretParameter(mqtt, {"password"}, section.password, "mqtt_pw", unityTest);

    parseTlsParameters(cJSON_GetObjectItem(mqtt, "tls"), section.tls);

    cJsonUtils::parseIntClamped(mqtt, {"processdatanotation"}, section.processDataNotation, 0, 2);
    cJsonUtils::parseBool(mqtt, {"retainprocessdata"}, section.retainProcessData);

    cJsonUtils::parseBool(mqtt, {"homeassistant", "discoveryenabled"}, section.homeAssistant.discoveryEnabled);
    if (cJsonUtils::parseString(mqtt, {"homeassistant", "discoveryprefix"}, section.homeAssistant.discoveryPrefix)) {
        configClassHelper::validateStructure(section.homeAssistant.discoveryPrefix);
    }
    if (cJsonUtils::parseString(mqtt, {"homeassistant", "statustopic"}, section.homeAssistant.statusTopic)) {
        configClassHelper::validateStructure(section.homeAssistant.statusTopic);
    }
    cJsonUtils::parseIntClamped(mqtt, {"homeassistant", "metertype"}, section.homeAssistant.meterType, 0, 16);
    cJsonUtils::parseBool(mqtt, {"homeassistant", "retaindiscovery"}, section.homeAssistant.retainDiscovery);
}

void ConfigClass::parseSectionInfluxDBv1(bool unityTest)
{
    cJSON *influx = cJSON_GetObjectItem(cJsonObject, "influxdbv1");
    auto &section = cfgDataTemp.sectionInfluxDBv1;

    cJsonUtils::parseBool(influx, {"enabled"}, section.enabled);
    cJsonUtils::parseString(influx, {"uri"}, section.uri);
    cJsonUtils::parseString(influx, {"database"}, section.database);
    cJsonUtils::parseIntClamped(influx, {"authmode"}, section.authMode, 0, 2);
    cJsonUtils::parseString(influx, {"username"}, section.username);
    parseSecretParameter(influx, {"password"}, section.password, "influxdbv1_pw", unityTest);

    parseTlsParameters(cJSON_GetObjectItem(influx, "tls"), section.tls);

    cJSON *seqArr = cJSON_GetObjectItem(influx, "sequence");
    for (int i = 0; i < cJSON_GetArraySize(seqArr); i++) {
        cJSON *el = cJSON_GetArrayItem(seqArr, i);
        std::string name;
        if (!cJsonUtils::parseString(el, {"sequencename"}, name)) {
            continue;
        }
        auto *seq = configClassHelper::findSequenceByName(section.sequence, name);
        if (!seq) {
            continue;
        }
        if (cJsonUtils::parseString(el, {"measurementname"}, seq->measurementName)) {
            configClassHelper::validateStructure(seq->measurementName);
        }
        if (cJsonUtils::parseString(el, {"fieldkey1"}, seq->fieldKey1)) {
            configClassHelper::validateStructure(seq->fieldKey1);
        }
    }
}

void ConfigClass::parseSectionInfluxDBv2(bool unityTest)
{
    cJSON *influx = cJSON_GetObjectItem(cJsonObject, "influxdbv2");
    auto &section = cfgDataTemp.sectionInfluxDBv2;

    cJsonUtils::parseBool(influx, {"enabled"}, section.enabled);
    cJsonUtils::parseString(influx, {"uri"}, section.uri);
    cJsonUtils::parseString(influx, {"bucket"}, section.bucket);
    cJsonUtils::parseString(influx, {"organization"}, section.organization);
    cJsonUtils::parseIntClamped(influx, {"authmode"}, section.authMode, 1, 2);
    parseSecretParameter(influx, {"token"}, section.token, "influxdbv2_pw", unityTest);

    parseTlsParameters(cJSON_GetObjectItem(influx, "tls"), section.tls);

    cJSON *seqArr = cJSON_GetObjectItem(influx, "sequence");
    for (int i = 0; i < cJSON_GetArraySize(seqArr); i++) {
        cJSON *el = cJSON_GetArrayItem(seqArr, i);
        std::string name;
        if (!cJsonUtils::parseString(el, {"sequencename"}, name)) {
            continue;
        }
        auto *seq = configClassHelper::findSequenceByName(section.sequence, name);
        if (!seq) {
            continue;
        }
        if (cJsonUtils::parseString(el, {"measurementname"}, seq->measurementName)) {
            configClassHelper::validateStructure(seq->measurementName);
        }
        if (cJsonUtils::parseString(el, {"fieldkey1"}, seq->fieldKey1)) {
            configClassHelper::validateStructure(seq->fieldKey1);
        }
    }
}

void ConfigClass::parseSectionWebhook(bool unityTest)
{
    cJSON *webhook = cJSON_GetObjectItem(cJsonObject, "webhook");
    auto &section = cfgDataTemp.sectionWebhook;

    cJsonUtils::parseBool(webhook, {"enabled"}, section.enabled);
    cJsonUtils::parseString(webhook, {"uri"}, section.uri);
    cJsonUtils::parseString(webhook, {"apikey"}, section.apiKey);
    cJsonUtils::parseIntClamped(webhook, {"publishimage"}, section.publishImage, 0, 2);
    cJsonUtils::parseIntClamped(webhook, {"authmode"}, section.authMode, 0, 2);
    cJsonUtils::parseString(webhook, {"username"}, section.username);
    parseSecretParameter(webhook, {"password"}, section.password, "webhook_pw", unityTest);

    parseTlsParameters(cJSON_GetObjectItem(webhook, "tls"), section.tls);
}

void ConfigClass::parseSectionGpio(bool init)
{
    cJSON *gpio = cJSON_GetObjectItem(cJsonObject, "gpio");
    auto &section = cfgDataTemp.sectionGpio;

    cJsonUtils::parseBool(gpio, {"customizationenabled"}, section.customizationEnabled);

    if (init) {
        for (int i = 0; i < GPIO_SPARE_PIN_COUNT; i++) {
            if (gpio_spare[i] == -1) {
                continue;
            }
            bool exists = std::any_of(section.gpioPin.begin(), section.gpioPin.end(),
                                      [&](const GpioElement &e) { return gpio_spare[i] == (gpio_num_t)e.gpioNumber; });
            if (!exists) {
                GpioElement el;
                el.gpioNumber = (int)gpio_spare[i];
                el.gpioUsage = gpio_spare_usage[i];
                if (std::string(gpio_spare_usage[i]).substr(0, 10) == "flashlight") {
                    el.pinMode = "flashlight-default";
                }
                section.gpioPin.push_back(el);
            }
        }
    }

    std::sort(section.gpioPin.begin(), section.gpioPin.end(),
              [](const GpioElement &x, const GpioElement &y) { return x.gpioNumber < y.gpioNumber; });

    cJSON *pinArr = cJSON_GetObjectItem(gpio, "gpiopin");
    for (int i = 0; i < cJSON_GetArraySize(pinArr); i++) {
        cJSON *el = cJSON_GetArrayItem(pinArr, i);

        int gpioNumber;
        if (!cJsonUtils::parseInt(el, {"gpionumber"}, gpioNumber)) {
            continue;
        }
        auto it = std::find_if(section.gpioPin.begin(), section.gpioPin.end(),
                               [&](const GpioElement &e) { return e.gpioNumber == gpioNumber; });
        if (it == section.gpioPin.end()) {
            continue;
        }
        GpioElement &pin = *it;

        for (int k = 0; k < GPIO_SPARE_PIN_COUNT; k++) {
            if (gpio_spare[k] == pin.gpioNumber) {
                pin.gpioUsage = gpio_spare_usage[k];
            }
        }

        cJsonUtils::parseBool(el, {"pinenabled"}, pin.pinEnabled);
        cJsonUtils::parseString(el, {"pinname"}, pin.pinName);
        cJsonUtils::parseString(el, {"pinmode"}, pin.pinMode);
        cJsonUtils::parseString(el, {"capturemode"}, pin.captureMode);
        cJsonUtils::parseIntClamped(el, {"inputdebouncetime"}, pin.inputDebounceTime, 0, 5000);
        cJsonUtils::parseIntClamped(el, {"pwmfrequency"}, pin.pwmFrequency, 5, 1000000);
        cJsonUtils::parseBool(el, {"logicactivelow"}, pin.logicActiveLow);
        cJsonUtils::parseBool(el, {"exposetomqtt"}, pin.exposeToMqtt);
        cJsonUtils::parseBool(el, {"exposetorest"}, pin.exposeToRest);

        cJsonUtils::parseIntClamped(el, {"smartled", "type"}, pin.smartLed.type, 0, 5);
        cJsonUtils::parseIntClampedMin(el, {"smartled", "quantity"}, pin.smartLed.quantity, 1);
        cJsonUtils::parseIntClamped(el, {"smartled", "colorredchannel"}, pin.smartLed.colorRedChannel, 0, 255);
        cJsonUtils::parseIntClamped(el, {"smartled", "colorgreenchannel"}, pin.smartLed.colorGreenChannel, 0, 255);
        cJsonUtils::parseIntClamped(el, {"smartled", "colorbluechannel"}, pin.smartLed.colorBlueChannel, 0, 255);

        cJsonUtils::parseIntClamped(el, {"intensitycorrectionfactor"}, pin.intensityCorrectionFactor, 1, 100);
    }
}

void ConfigClass::parseSectionLogging()
{
    cJSON *log = cJSON_GetObjectItem(cJsonObject, "log");
    auto &section = cfgDataTemp.sectionLog;

    cJsonUtils::parseIntClamped(log, {"debug", "loglevel"}, section.debug.logLevel, 1, 4);
    cJsonUtils::parseIntClampedMin(log, {"debug", "logfilesretention"}, section.debug.logFilesRetention, 0);
    cJsonUtils::parseIntClampedMin(log, {"debug", "debugfilesretention"}, section.debug.debugFilesRetention, 0);
    cJsonUtils::parseBool(log, {"data", "enabled"}, section.data.enabled);
    cJsonUtils::parseIntClampedMin(log, {"data", "datafilesretention"}, section.data.dataFilesRetention, 0);
}

void ConfigClass::parseSectionNetwork(bool init, bool unityTest)
{
    cJSON *network = cJSON_GetObjectItem(cJsonObject, "network");
    auto &section = cfgDataTemp.sectionNetwork;

    int opmode;
    if (cJsonUtils::parseInt(network, {"opmode"}, opmode)) {
#ifdef BOARD_FEATURE_ETHERNET
        section.opmode = (opmode < NETWORK_OPMODE_DISABLED || opmode >= NETWORK_OPMODE_MAX) ? NETWORK_OPMODE_ETHERNET_FALLBACK_WLAN
                                                                                            : opmode;
#else
        section.opmode = (opmode < NETWORK_OPMODE_DISABLED || opmode >= NETWORK_OPMODE_MAX) ? NETWORK_OPMODE_WLAN_CLIENT : opmode;
#endif
    }

    cJsonUtils::parseIntClampedMin(network, {"timedoffdelay"}, section.timedOffDelay, 1);
    cJsonUtils::parseString(network, {"hostname"}, section.hostname);

    bool ssidEmpty = false;
    std::string ssid;
    if (cJsonUtils::parseString(network, {"wlan", "ssid"}, ssid) && !ssid.empty()) {
        section.wlan.ssid = trim(ssid);
        saveDataToNVS("wlan_ssid", section.wlan.ssid);
    }
    else {
        if (init) {
            ssidEmpty = true;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "parseConfig: No SSID config, try to use SSID and password from NVS");
        }
        if (!unityTest) {
            loadDataFromNVS("wlan_ssid", section.wlan.ssid);
        }
    }

    std::string wlanPw;
    if (cJsonUtils::parseString(network, {"wlan", "password"}, wlanPw) && wlanPw != "******" && !ssidEmpty) {
        section.wlan.password = wlanPw;
        saveDataToNVS("wlan_pw", section.wlan.password);
    }
    else if (!unityTest) {
        loadDataFromNVS("wlan_pw", section.wlan.password);
    }

    cJsonUtils::parseIntClamped(network, {"wlan", "ipv4", "networkconfig"}, section.wlan.ipv4.networkConfig, 0, 1);
    cJsonUtils::parseStringValidated(network, {"wlan", "ipv4", "ipaddress"}, section.wlan.ipv4.ipAddress, isValidIpAddress);
    cJsonUtils::parseStringValidated(network, {"wlan", "ipv4", "subnetmask"}, section.wlan.ipv4.subnetMask, isValidIpAddress);
    cJsonUtils::parseStringValidated(network, {"wlan", "ipv4", "gatewayaddress"}, section.wlan.ipv4.gatewayAddress, isValidIpAddress);

    if (section.wlan.ipv4.networkConfig == NETWORK_IP_CONFIG_STATIC) {
        if (!isValidIpAddress(section.wlan.ipv4.ipAddress.c_str()) || !isValidIpAddress(section.wlan.ipv4.subnetMask.c_str()) ||
            !isValidIpAddress(section.wlan.ipv4.gatewayAddress.c_str())) {
            section.wlan.ipv4.networkConfig = NETWORK_IP_CONFIG_DHCP;
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Static network config invalid. Use DHCP as fallback");
        }
    }

    cJsonUtils::parseStringValidated(network, {"wlan", "ipv4", "dnsserver"}, section.wlan.ipv4.dnsServer, isValidIpAddress);

    cJsonUtils::parseBool(network, {"wlan", "wlanroaming", "enabled"}, section.wlan.wlanRoaming.enabled);
    cJsonUtils::parseIntClamped(network, {"wlan", "wlanroaming", "rssithreshold"}, section.wlan.wlanRoaming.rssiThreshold, -100, 0);

    cJsonUtils::parseString(network, {"wlanap", "ssid"}, section.wlanAp.ssid);
    cJsonUtils::parseStringValidated(network, {"wlanap", "password"}, section.wlanAp.password,
                                     [](const char *s) { return strlen(s) == 0 || strlen(s) >= 8; });
    cJsonUtils::parseIntClamped(network, {"wlanap", "channel"}, section.wlanAp.channel, 1, 14);
    cJsonUtils::parseStringValidated(network, {"wlanap", "ipv4", "ipaddress"}, section.wlanAp.ipv4.ipAddress, isValidIpAddress);

#ifdef BOARD_FEATURE_ETHERNET
    cJsonUtils::parseIntClamped(network, {"ethernet", "ipv4", "networkconfig"}, section.ethernet.ipv4.networkConfig, 0, 1);
    cJsonUtils::parseStringValidated(network, {"ethernet", "ipv4", "ipaddress"}, section.ethernet.ipv4.ipAddress, isValidIpAddress);
    cJsonUtils::parseStringValidated(network, {"ethernet", "ipv4", "subnetmask"}, section.ethernet.ipv4.subnetMask, isValidIpAddress);
    cJsonUtils::parseStringValidated(network, {"ethernet", "ipv4", "gatewayaddress"}, section.ethernet.ipv4.gatewayAddress,
                                     isValidIpAddress);

    if (section.ethernet.ipv4.networkConfig == NETWORK_IP_CONFIG_STATIC) {
        if (!isValidIpAddress(section.ethernet.ipv4.ipAddress.c_str()) || !isValidIpAddress(section.ethernet.ipv4.subnetMask.c_str()) ||
            !isValidIpAddress(section.ethernet.ipv4.gatewayAddress.c_str())) {
            section.ethernet.ipv4.networkConfig = NETWORK_IP_CONFIG_DHCP;
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Static network config invalid. Use DHCP as fallback");
        }
    }

    cJsonUtils::parseStringValidated(network, {"ethernet", "ipv4", "dnsserver"}, section.ethernet.ipv4.dnsServer, isValidIpAddress);
#endif

    cJsonUtils::parseString(network, {"time", "timesetmanual"}, section.time.timeSetManual);
    cJsonUtils::parseString(network, {"time", "timezone"}, section.time.timeZone);
    cJsonUtils::parseBool(network, {"time", "ntp", "timesyncenabled"}, section.time.ntp.timeSyncEnabled);
    cJsonUtils::parseString(network, {"time", "ntp", "timeserver"}, section.time.ntp.timeServer);
    cJsonUtils::parseBool(network, {"time", "processstartinterlock"}, section.time.processStartInterlock);
}

void ConfigClass::parseSectionSystem()
{
    cJsonUtils::parseIntClamped(cJSON_GetObjectItem(cJsonObject, "system"), {"cpufrequency"}, cfgDataTemp.sectionSystem.cpuFrequency, 160,
                                240);
}

void ConfigClass::parseSectionWebUi(bool unityTest)
{
    cJSON *webui = cJSON_GetObjectItem(cJsonObject, "webui");
    auto &section = cfgDataTemp.sectionWebUi;

    cJsonUtils::parseIntClamped(webui, {"httpauth", "authmode"}, section.httpAuth.authMode, 0, 1);
    cJsonUtils::parseString(webui, {"httpauth", "username"}, section.httpAuth.username);
    parseSecretParameter(webui, {"httpauth", "password"}, section.httpAuth.password, "webui_httpauth", unityTest);

    cJsonUtils::parseBool(webui, {"autorefresh", "overviewpage", "enabled"}, section.autoRefresh.overviewPage.enabled);
    cJsonUtils::parseIntClampedMin(webui, {"autorefresh", "overviewpage", "refreshtime"}, section.autoRefresh.overviewPage.refreshTime, 1);
    cJsonUtils::parseBool(webui, {"autorefresh", "datagraphpage", "enabled"}, section.autoRefresh.dataGraphPage.enabled);
    cJsonUtils::parseIntClampedMin(webui, {"autorefresh", "datagraphpage", "refreshtime"}, section.autoRefresh.dataGraphPage.refreshTime,
                                   1);
}


void ConfigClass::parseSecretParameter(cJSON *root, std::initializer_list<const char *> path, std::string &out, const char *nvsKey,
                                       bool unityTest)
{
    std::string tmpOut;
    if (cJsonUtils::parseString(root, path, tmpOut) && tmpOut != "******") {
        out = tmpOut;
        saveDataToNVS(nvsKey, out);
    }
    else if (!unityTest) {
        loadDataFromNVS(nvsKey, out);
    }
}

void ConfigClass::parseTlsParameters(cJSON *tlsObj, TLSParams &tls)
{
    cJsonUtils::parseIntClamped(tlsObj, {"servercertverification"}, tls.serverCertVerification, TlsServerCertVerification(0),
                                TlsServerCertVerification(2));
    if (cJsonUtils::parseString(tlsObj, {"cacert"}, tls.caCert)) {
        configClassHelper::validateStructure(tls.caCert);
    }
    if (cJsonUtils::parseString(tlsObj, {"clientcert"}, tls.clientCert)) {
        configClassHelper::validateStructure(tls.clientCert);
    }
    if (cJsonUtils::parseString(tlsObj, {"clientkey"}, tls.clientKey)) {
        configClassHelper::validateStructure(tls.clientKey);
    }
}


//**************************************************************************************************
// Serialize internal struct to JSON string
//**************************************************************************************************
esp_err_t ConfigClass::serializeConfig(bool unityTest)
{
    cJsonObject = cJSON_CreateObject();
    if (cJsonObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "serializeConfig: Error while creating JSON object");
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;

    // Helper alias for clean inline calls
    auto addEl = [&](cJSON *obj, const char *key, const auto &val) { cJsonUtils::addElementHelper(obj, key, val, retVal); };
    auto addObj = [&](cJSON *obj, const char *key) { return cJsonUtils::addObjectHelper(obj, key, retVal); };
    auto addArr = [&](cJSON *obj, const char *key) { return cJsonUtils::addArrayHelper(obj, key, retVal); };
    auto addArrObj = [&](cJSON *arr) { return cJsonUtils::addArrayObjectHelper(arr, retVal); };


    // Config Version
    cJSON *config = addObj(cJsonObject, "config");
    addEl(config, "version", cfgDataTemp.sectionConfig.version);
    addEl(config, "lastmodified", cfgDataTemp.sectionConfig.lastModified);


    // Operation Mode
    cJSON *opmode = addObj(cJsonObject, "operationmode");
    addEl(opmode, "opmode", cfgDataTemp.sectionOperationMode.opMode);
    addEl(opmode, "automaticprocessinterval", to_stringWithPrecision(cfgDataTemp.sectionOperationMode.automaticProcessInterval, 2));
    addEl(opmode, "usedemoimages", cfgDataTemp.sectionOperationMode.useDemoImages);


    // Take Image
    cJSON *takeImage = addObj(cJsonObject, "takeimage");

    cJSON *flashlight = addObj(takeImage, "flashlight");
    addEl(flashlight, "flashtime", cfgDataTemp.sectionTakeImage.flashlight.flashTime);
    addEl(flashlight, "flashintensity", cfgDataTemp.sectionTakeImage.flashlight.flashIntensity);

    cJSON *camera = addObj(takeImage, "camera");
    addEl(camera, "cameramodel", cfgDataTemp.sectionTakeImage.camera.cameraModel);
    addEl(camera, "camerafrequency", cfgDataTemp.sectionTakeImage.camera.cameraFrequency);
    addEl(camera, "imagequality", cfgDataTemp.sectionTakeImage.camera.imageQuality);
    addEl(camera, "brightness", cfgDataTemp.sectionTakeImage.camera.brightness);
    addEl(camera, "contrast", cfgDataTemp.sectionTakeImage.camera.contrast);
    addEl(camera, "saturation", cfgDataTemp.sectionTakeImage.camera.saturation);
    addEl(camera, "sharpness", cfgDataTemp.sectionTakeImage.camera.sharpness);
    addEl(camera, "exposurecontrolmode", cfgDataTemp.sectionTakeImage.camera.exposureControlMode);
    addEl(camera, "autoexposurelevel", cfgDataTemp.sectionTakeImage.camera.autoExposureLevel);
    addEl(camera, "manualexposurevalue", cfgDataTemp.sectionTakeImage.camera.manualExposureValue);
    addEl(camera, "gaincontrolmode", cfgDataTemp.sectionTakeImage.camera.gainControlMode);
    addEl(camera, "manualgainvalue", cfgDataTemp.sectionTakeImage.camera.manualGainValue);
    addEl(camera, "specialeffect", cfgDataTemp.sectionTakeImage.camera.specialEffect);
    addEl(camera, "mirrorimage", cfgDataTemp.sectionTakeImage.camera.mirrorImage);
    addEl(camera, "flipimage", cfgDataTemp.sectionTakeImage.camera.flipImage);
    addEl(camera, "zoomfactor", cfgDataTemp.sectionTakeImage.camera.zoomFactor);
    addEl(camera, "zoomoffsetx", cfgDataTemp.sectionTakeImage.camera.zoomOffsetX);
    addEl(camera, "zoomoffsety", cfgDataTemp.sectionTakeImage.camera.zoomOffsetY);

    cJSON *takeImageDebug = addObj(takeImage, "debug");
    addEl(takeImageDebug, "saverawimages", cfgDataTemp.sectionTakeImage.debug.saveRawImages);
    addEl(takeImageDebug, "rawimageslocation", cfgDataTemp.sectionTakeImage.debug.rawImagesLocation);
    addEl(takeImageDebug, "rawimagesretention", cfgDataTemp.sectionTakeImage.debug.rawImagesRetention);


    // Image Alignment
    cJSON *imageAlignment = addObj(cJsonObject, "imagealignment");
    addEl(imageAlignment, "alignmentalgo", cfgDataTemp.sectionImageAlignment.alignmentAlgo);

    cJSON *searchField = addObj(imageAlignment, "searchfield");
    addEl(searchField, "x", cfgDataTemp.sectionImageAlignment.searchField.x);
    addEl(searchField, "y", cfgDataTemp.sectionImageAlignment.searchField.y);
    addEl(imageAlignment, "imagerotation", to_stringWithPrecision(cfgDataTemp.sectionImageAlignment.imageRotation, 1));

    cJSON *markerArr = addArr(imageAlignment, "marker");
    for (int i = 0; i < 2; ++i) {
        cJSON *markerEl = addArrObj(markerArr);
        addEl(markerEl, "x", cfgDataTemp.sectionImageAlignment.marker[i].x);
        addEl(markerEl, "y", cfgDataTemp.sectionImageAlignment.marker[i].y);
    }

    cJSON *imageAlignmentDebug = addObj(imageAlignment, "debug");
    addEl(imageAlignmentDebug, "savedebuginfo", cfgDataTemp.sectionImageAlignment.debug.saveDebugInfo);


    // Number Sequences
    cJSON *numbersequences = addObj(cJsonObject, "numbersequences");
    cJSON *sequencesArr = addArr(numbersequences, "sequence");
    for (const auto &seq : cfgDataTemp.sectionNumberSequences.sequence) {
        cJSON *seqEl = addArrObj(sequencesArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);
    }


    // Digit
    cJSON *digit = addObj(cJsonObject, "digit");
    addEl(digit, "enabled", cfgDataTemp.sectionDigit.enabled);
    addEl(digit, "model", cfgDataTemp.sectionDigit.model);
    addEl(digit, "cnngoodthreshold", to_stringWithPrecision(cfgDataTemp.sectionDigit.cnnGoodThreshold, 2));

    cJSON *digitSeqArr = addArr(digit, "sequence");
    for (const auto &seq : cfgDataTemp.sectionDigit.sequence) {
        cJSON *seqEl = addArrObj(digitSeqArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);

        cJSON *roiArr = addArr(seqEl, "roi");
        for (const auto &r : seq.roi) {
            cJSON *roiEl = addArrObj(roiArr);
            addEl(roiEl, "x", r.x);
            addEl(roiEl, "y", r.y);
            addEl(roiEl, "dx", r.dx);
            addEl(roiEl, "dy", r.dy);
        }
    }

    cJSON *digitDebug = addObj(digit, "debug");
    addEl(digitDebug, "saveroiimages", cfgDataTemp.sectionDigit.debug.saveRoiImages);
    addEl(digitDebug, "roiimageslocation", cfgDataTemp.sectionDigit.debug.roiImagesLocation);
    addEl(digitDebug, "roiimagesretention", cfgDataTemp.sectionDigit.debug.roiImagesRetention);
    addEl(digitDebug, "roisavingsize", cfgDataTemp.sectionDigit.debug.roiSavingSize);


    // Analog
    cJSON *analog = addObj(cJsonObject, "analog");
    addEl(analog, "enabled", cfgDataTemp.sectionAnalog.enabled);
    addEl(analog, "model", cfgDataTemp.sectionAnalog.model);

    cJSON *analogSeqArr = addArr(analog, "sequence");
    for (const auto &seq : cfgDataTemp.sectionAnalog.sequence) {
        cJSON *seqEl = addArrObj(analogSeqArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);

        cJSON *roiArr = addArr(seqEl, "roi");
        for (const auto &r : seq.roi) {
            cJSON *roiEl = addArrObj(roiArr);
            addEl(roiEl, "x", r.x);
            addEl(roiEl, "y", r.y);
            addEl(roiEl, "dx", r.dx);
            addEl(roiEl, "dy", r.dy);
            addEl(roiEl, "ccw", r.ccw);
        }
    }

    cJSON *analogDebug = addObj(analog, "debug");
    addEl(analogDebug, "saveroiimages", cfgDataTemp.sectionAnalog.debug.saveRoiImages);
    addEl(analogDebug, "roiimageslocation", cfgDataTemp.sectionAnalog.debug.roiImagesLocation);
    addEl(analogDebug, "roiimagesretention", cfgDataTemp.sectionAnalog.debug.roiImagesRetention);
    addEl(analogDebug, "roisavingsize", cfgDataTemp.sectionAnalog.debug.roiSavingSize);


    // Post-Processing
    cJSON *postprocessing = addObj(cJsonObject, "postprocessing");
    cJSON *postSeqArr = addArr(postprocessing, "sequence");
    for (const auto &seq : cfgDataTemp.sectionPostProcessing.sequence) {
        cJSON *seqEl = addArrObj(postSeqArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);
        addEl(seqEl, "decimalshift", seq.decimalShift);
        addEl(seqEl, "analogdigitsyncvalue", to_stringWithPrecision(seq.analogDigitSyncValue, 1));
        addEl(seqEl, "extendedresolution", seq.extendedResolution);
        addEl(seqEl, "ignoreleadingnan", seq.ignoreLeadingNaN);
        addEl(seqEl, "checkdigitincreaseconsistency", seq.checkDigitIncreaseConsistency);
        addEl(seqEl, "maxratechecktype", seq.maxRateCheckType);
        addEl(seqEl, "maxrate", to_stringWithPrecision(seq.maxRate, 3));
        addEl(seqEl, "allownegativerate", seq.allowNegativeRate);
        addEl(seqEl, "usefallbackvalue", seq.useFallbackValue);
        addEl(seqEl, "fallbackvalueagestartup", seq.fallbackValueAgeStartup);
    }

    cJSON *postDebug = addObj(postprocessing, "debug");
    addEl(postDebug, "savedebuginfo", cfgDataTemp.sectionPostProcessing.debug.saveDebugInfo);


    // MQTT
    cJSON *mqtt = addObj(cJsonObject, "mqtt");
    addEl(mqtt, "enabled", cfgDataTemp.sectionMqtt.enabled);
    addEl(mqtt, "uri", cfgDataTemp.sectionMqtt.uri);
    addEl(mqtt, "maintopic", cfgDataTemp.sectionMqtt.mainTopic);
    addEl(mqtt, "clientid", cfgDataTemp.sectionMqtt.clientID);
    addEl(mqtt, "authmode", cfgDataTemp.sectionMqtt.authMode);
    addEl(mqtt, "username", cfgDataTemp.sectionMqtt.username);
    addEl(mqtt, "password", cfgDataTemp.sectionMqtt.password.empty() ? "" : "******");

    cJSON *mqttTls = addObj(mqtt, "tls");
    addEl(mqttTls, "servercertverification", cfgDataTemp.sectionMqtt.tls.serverCertVerification);
    addEl(mqttTls, "cacert", cfgDataTemp.sectionMqtt.tls.caCert);
    addEl(mqttTls, "clientcert", cfgDataTemp.sectionMqtt.tls.clientCert);
    addEl(mqttTls, "clientkey", cfgDataTemp.sectionMqtt.tls.clientKey);

    addEl(mqtt, "processdatanotation", cfgDataTemp.sectionMqtt.processDataNotation);
    addEl(mqtt, "retainprocessdata", cfgDataTemp.sectionMqtt.retainProcessData);

    cJSON *mqttHA = addObj(mqtt, "homeassistant");
    addEl(mqttHA, "discoveryenabled", cfgDataTemp.sectionMqtt.homeAssistant.discoveryEnabled);
    addEl(mqttHA, "discoveryprefix", cfgDataTemp.sectionMqtt.homeAssistant.discoveryPrefix);
    addEl(mqttHA, "statustopic", cfgDataTemp.sectionMqtt.homeAssistant.statusTopic);
    addEl(mqttHA, "metertype", cfgDataTemp.sectionMqtt.homeAssistant.meterType);
    addEl(mqttHA, "retaindiscovery", cfgDataTemp.sectionMqtt.homeAssistant.retainDiscovery);


    // InfluxDB v1.x
    cJSON *influxv1 = addObj(cJsonObject, "influxdbv1");
    addEl(influxv1, "enabled", cfgDataTemp.sectionInfluxDBv1.enabled);
    addEl(influxv1, "uri", cfgDataTemp.sectionInfluxDBv1.uri);
    addEl(influxv1, "database", cfgDataTemp.sectionInfluxDBv1.database);
    addEl(influxv1, "authmode", cfgDataTemp.sectionInfluxDBv1.authMode);
    addEl(influxv1, "username", cfgDataTemp.sectionInfluxDBv1.username);
    addEl(influxv1, "password", cfgDataTemp.sectionInfluxDBv1.password.empty() ? "" : "******");

    cJSON *influxv1Tls = addObj(influxv1, "tls");
    addEl(influxv1Tls, "servercertverification", cfgDataTemp.sectionInfluxDBv1.tls.serverCertVerification);
    addEl(influxv1Tls, "cacert", cfgDataTemp.sectionInfluxDBv1.tls.caCert);
    addEl(influxv1Tls, "clientcert", cfgDataTemp.sectionInfluxDBv1.tls.clientCert);
    addEl(influxv1Tls, "clientkey", cfgDataTemp.sectionInfluxDBv1.tls.clientKey);

    cJSON *influxv1SeqArr = addArr(influxv1, "sequence");
    for (const auto &seq : cfgDataTemp.sectionInfluxDBv1.sequence) {
        cJSON *seqEl = addArrObj(influxv1SeqArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);
        addEl(seqEl, "measurementname", seq.measurementName);
        addEl(seqEl, "fieldkey1", seq.fieldKey1);
    }


    // InfluxDB v2.x
    cJSON *influxv2 = addObj(cJsonObject, "influxdbv2");
    addEl(influxv2, "enabled", cfgDataTemp.sectionInfluxDBv2.enabled);
    addEl(influxv2, "uri", cfgDataTemp.sectionInfluxDBv2.uri);
    addEl(influxv2, "bucket", cfgDataTemp.sectionInfluxDBv2.bucket);
    addEl(influxv2, "organization", cfgDataTemp.sectionInfluxDBv2.organization);
    addEl(influxv2, "authmode", cfgDataTemp.sectionInfluxDBv2.authMode);
    addEl(influxv2, "token", cfgDataTemp.sectionInfluxDBv2.token.empty() ? "" : "******");

    cJSON *influxv2Tls = addObj(influxv2, "tls");
    addEl(influxv2Tls, "servercertverification", cfgDataTemp.sectionInfluxDBv2.tls.serverCertVerification);
    addEl(influxv2Tls, "cacert", cfgDataTemp.sectionInfluxDBv2.tls.caCert);
    addEl(influxv2Tls, "clientcert", cfgDataTemp.sectionInfluxDBv2.tls.clientCert);
    addEl(influxv2Tls, "clientkey", cfgDataTemp.sectionInfluxDBv2.tls.clientKey);

    cJSON *influxv2SeqArr = addArr(influxv2, "sequence");
    for (const auto &seq : cfgDataTemp.sectionInfluxDBv2.sequence) {
        cJSON *seqEl = addArrObj(influxv2SeqArr);
        addEl(seqEl, "sequenceid", seq.sequenceId);
        addEl(seqEl, "sequencename", seq.sequenceName);
        addEl(seqEl, "measurementname", seq.measurementName);
        addEl(seqEl, "fieldkey1", seq.fieldKey1);
    }


    // Webhook
    cJSON *webhook = addObj(cJsonObject, "webhook");
    addEl(webhook, "enabled", cfgDataTemp.sectionWebhook.enabled);
    addEl(webhook, "uri", cfgDataTemp.sectionWebhook.uri);
    addEl(webhook, "apikey", cfgDataTemp.sectionWebhook.apiKey);
    addEl(webhook, "publishimage", cfgDataTemp.sectionWebhook.publishImage);
    addEl(webhook, "authmode", cfgDataTemp.sectionWebhook.authMode);
    addEl(webhook, "username", cfgDataTemp.sectionWebhook.username);
    addEl(webhook, "password", cfgDataTemp.sectionWebhook.password.empty() ? "" : "******");

    cJSON *webhookTls = addObj(webhook, "tls");
    addEl(webhookTls, "servercertverification", cfgDataTemp.sectionWebhook.tls.serverCertVerification);
    addEl(webhookTls, "cacert", cfgDataTemp.sectionWebhook.tls.caCert);
    addEl(webhookTls, "clientcert", cfgDataTemp.sectionWebhook.tls.clientCert);
    addEl(webhookTls, "clientkey", cfgDataTemp.sectionWebhook.tls.clientKey);


    // GPIO
    // ***************************
    cJSON *gpio = addObj(cJsonObject, "gpio");
    addEl(gpio, "customizationenabled", cfgDataTemp.sectionGpio.customizationEnabled);

    cJSON *gpioPinArr = addArr(gpio, "gpiopin");
    for (const auto &pin : cfgDataTemp.sectionGpio.gpioPin) {
        cJSON *pinEl = addArrObj(gpioPinArr);
        addEl(pinEl, "gpionumber", pin.gpioNumber);
        addEl(pinEl, "gpiousage", pin.gpioUsage);
        addEl(pinEl, "pinenabled", pin.pinEnabled);
        addEl(pinEl, "pinname", pin.pinName);
        addEl(pinEl, "pinmode", pin.pinMode);
        addEl(pinEl, "capturemode", pin.captureMode);
        addEl(pinEl, "inputdebouncetime", pin.inputDebounceTime);
        addEl(pinEl, "pwmfrequency", pin.pwmFrequency);
        addEl(pinEl, "logicactivelow", pin.logicActiveLow);
        addEl(pinEl, "exposetomqtt", pin.exposeToMqtt);
        addEl(pinEl, "exposetorest", pin.exposeToRest);

        cJSON *smartled = addObj(pinEl, "smartled");
        addEl(smartled, "type", pin.smartLed.type);
        addEl(smartled, "quantity", pin.smartLed.quantity);
        addEl(smartled, "colorredchannel", pin.smartLed.colorRedChannel);
        addEl(smartled, "colorgreenchannel", pin.smartLed.colorGreenChannel);
        addEl(smartled, "colorbluechannel", pin.smartLed.colorBlueChannel);

        addEl(pinEl, "intensitycorrectionfactor", pin.intensityCorrectionFactor);
    }


    // Logging
    // ***************************
    cJSON *log = addObj(cJsonObject, "log");

    cJSON *logDebug = addObj(log, "debug");
    addEl(logDebug, "loglevel", cfgDataTemp.sectionLog.debug.logLevel);
    addEl(logDebug, "logfilesretention", cfgDataTemp.sectionLog.debug.logFilesRetention);
    addEl(logDebug, "debugfilesretention", cfgDataTemp.sectionLog.debug.debugFilesRetention);

    cJSON *logData = addObj(log, "data");
    addEl(logData, "enabled", cfgDataTemp.sectionLog.data.enabled);
    addEl(logData, "datafilesretention", cfgDataTemp.sectionLog.data.dataFilesRetention);


    // Network
    // ***************************
    cJSON *network = addObj(cJsonObject, "network");
    addEl(network, "opmode", cfgDataTemp.sectionNetwork.opmode);
    addEl(network, "timedoffdelay", cfgDataTemp.sectionNetwork.timedOffDelay);
    addEl(network, "hostname", cfgDataTemp.sectionNetwork.hostname);

    cJSON *networkWlan = addObj(network, "wlan");
    addEl(networkWlan, "ssid", cfgDataTemp.sectionNetwork.wlan.ssid);
    addEl(networkWlan, "password", cfgDataTemp.sectionNetwork.wlan.password.empty() ? "" : "******");

    cJSON *networkWlanIpv4 = addObj(networkWlan, "ipv4");
    addEl(networkWlanIpv4, "networkconfig", cfgDataTemp.sectionNetwork.wlan.ipv4.networkConfig);
    addEl(networkWlanIpv4, "ipaddress", cfgDataTemp.sectionNetwork.wlan.ipv4.ipAddress);
    addEl(networkWlanIpv4, "subnetmask", cfgDataTemp.sectionNetwork.wlan.ipv4.subnetMask);
    addEl(networkWlanIpv4, "gatewayaddress", cfgDataTemp.sectionNetwork.wlan.ipv4.gatewayAddress);
    addEl(networkWlanIpv4, "dnsserver", cfgDataTemp.sectionNetwork.wlan.ipv4.dnsServer);

    cJSON *networkWlanRoaming = addObj(networkWlan, "wlanroaming");
    addEl(networkWlanRoaming, "enabled", cfgDataTemp.sectionNetwork.wlan.wlanRoaming.enabled);
    addEl(networkWlanRoaming, "rssithreshold", cfgDataTemp.sectionNetwork.wlan.wlanRoaming.rssiThreshold);

    cJSON *networkWlanAp = addObj(network, "wlanap");
    addEl(networkWlanAp, "ssid", cfgDataTemp.sectionNetwork.wlanAp.ssid);
    addEl(networkWlanAp, "password", cfgDataTemp.sectionNetwork.wlanAp.password);
    addEl(networkWlanAp, "channel", cfgDataTemp.sectionNetwork.wlanAp.channel);

    cJSON *networkApIpv4 = addObj(networkWlanAp, "ipv4");
    addEl(networkApIpv4, "ipaddress", cfgDataTemp.sectionNetwork.wlanAp.ipv4.ipAddress);

#ifdef BOARD_FEATURE_ETHERNET
    cJSON *networkEth = addObj(network, "ethernet");
    cJSON *networkEthIpv4 = addObj(networkEth, "ipv4");
    addEl(networkEthIpv4, "networkconfig", cfgDataTemp.sectionNetwork.ethernet.ipv4.networkConfig);
    addEl(networkEthIpv4, "ipaddress", cfgDataTemp.sectionNetwork.ethernet.ipv4.ipAddress);
    addEl(networkEthIpv4, "subnetmask", cfgDataTemp.sectionNetwork.ethernet.ipv4.subnetMask);
    addEl(networkEthIpv4, "gatewayaddress", cfgDataTemp.sectionNetwork.ethernet.ipv4.gatewayAddress);
    addEl(networkEthIpv4, "dnsserver", cfgDataTemp.sectionNetwork.ethernet.ipv4.dnsServer);
#endif // BOARD_FEATURE_ETHERNET

    cJSON *networkTime = addObj(network, "time");
    addEl(networkTime, "timezone", cfgDataTemp.sectionNetwork.time.timeZone);

    cJSON *networkTimeNtp = addObj(networkTime, "ntp");
    addEl(networkTimeNtp, "timesyncenabled", cfgDataTemp.sectionNetwork.time.ntp.timeSyncEnabled);
    addEl(networkTimeNtp, "timeserver", cfgDataTemp.sectionNetwork.time.ntp.timeServer);

    addEl(networkTime, "processstartinterlock", cfgDataTemp.sectionNetwork.time.processStartInterlock);


    // System
    // ***************************
    cJSON *system = addObj(cJsonObject, "system");
    addEl(system, "cpufrequency", cfgDataTemp.sectionSystem.cpuFrequency);


    // WebUI
    // ***************************
    cJSON *webui = addObj(cJsonObject, "webui");

    cJSON *webuiHttpAuth = addObj(webui, "httpauth");
    addEl(webuiHttpAuth, "authmode", cfgDataTemp.sectionWebUi.httpAuth.authMode);
    addEl(webuiHttpAuth, "username", cfgDataTemp.sectionWebUi.httpAuth.username);
    addEl(webuiHttpAuth, "password", cfgDataTemp.sectionWebUi.httpAuth.password.empty() ? "" : "******");

    cJSON *webuiAutorefresh = addObj(webui, "autorefresh");

    cJSON *webuiAutorefreshOverview = addObj(webuiAutorefresh, "overviewpage");
    addEl(webuiAutorefreshOverview, "enabled", cfgDataTemp.sectionWebUi.autoRefresh.overviewPage.enabled);
    addEl(webuiAutorefreshOverview, "refreshtime", cfgDataTemp.sectionWebUi.autoRefresh.overviewPage.refreshTime);

    cJSON *webuiAutorefreshDataGraph = addObj(webuiAutorefresh, "datagraphpage");
    addEl(webuiAutorefreshDataGraph, "enabled", cfgDataTemp.sectionWebUi.autoRefresh.dataGraphPage.enabled);
    addEl(webuiAutorefreshDataGraph, "refreshtime", cfgDataTemp.sectionWebUi.autoRefresh.dataGraphPage.refreshTime);

    // Print to preallocated buffer
    jsonBuffer[0] = '\0'; // Reset content
    if (!cJSON_PrintPreallocated(cJsonObject, jsonBuffer, CONFIG_HANDLING_CJSON_STRING_BUFFER_SIZE, unityTest ? 0 : 1)) {
        retVal = ESP_FAIL;
    }

    // Cleanup root cJSON structure
    cJSON_Delete(cJsonObject);
    cJsonObject = NULL;

    return retVal;
}


//**************************************************************************************************
// Persist configuration
//**************************************************************************************************
bool ConfigClass::persistConfig()
{
    if (!cJsonObjectBuffer || !jsonBuffer || !cfgMutex) {
        return false;
    }

    CfgMutexGuard lock(cfgMutex, pdMS_TO_TICKS(5000));
    if (!lock.isAcquired()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "persistConfig: Failed to acquire cfgMutex - Timeout");
        return false;
    }

    cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

    if (serializeConfig() != ESP_OK) {
        return false;
    }

    return (writeConfigFile() == ESP_OK);
}


//**************************************************************************************************
// Write configuration to file (JSON string)
//**************************************************************************************************
esp_err_t ConfigClass::writeConfigFile()
{
    FILE *file = fopen(CONFIG_PERSISTENCE_FILE, "w");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "writeConfigFile: Failed to open config file");
        return ESP_FAIL;
    }

    const size_t bufLength = strlen(jsonBuffer);

    bool writeFailed = fwrite(jsonBuffer, 1, bufLength, file) != bufLength;
    if (!writeFailed) {
        writeFailed = fflush(file) != 0;
    }
    if (!writeFailed) {
        writeFailed = fsync(fileno(file)) != 0;
    }
    if (fclose(file) != 0) {
        writeFailed = true;
    }
    if (writeFailed) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "writeConfigFile: Failed to write config file");
        return ESP_FAIL;
    }

    if (!copyFile(CONFIG_PERSISTENCE_FILE, CONFIG_PERSISTENCE_FILE_FALLBACK)) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "writeConfigFile: Failed to update fallback config file");
    }

    return ESP_OK;
}


//**************************************************************************************************
// Clear internal structs
//**************************************************************************************************
template <typename ConfigDataType> inline void clearConfigDataStructure(ConfigDataType &data)
{
    // Clear and release outer sequence vectors
    data.sectionNumberSequences.sequence.clear();
    decltype(data.sectionNumberSequences.sequence)().swap(data.sectionNumberSequences.sequence);

    // Clear inner and outer vectors for Digit
    for (auto &seq : data.sectionDigit.sequence) {
        seq.roi.clear();
        decltype(seq.roi)().swap(seq.roi);
    }
    data.sectionDigit.sequence.clear();
    decltype(data.sectionDigit.sequence)().swap(data.sectionDigit.sequence);

    // Clear inner and outer vectors for Analog
    for (auto &seq : data.sectionAnalog.sequence) {
        seq.roi.clear();
        decltype(seq.roi)().swap(seq.roi);
    }
    data.sectionAnalog.sequence.clear();
    decltype(data.sectionAnalog.sequence)().swap(data.sectionAnalog.sequence);

    // Clear remaining sections
    data.sectionPostProcessing.sequence.clear();
    decltype(data.sectionPostProcessing.sequence)().swap(data.sectionPostProcessing.sequence);

    data.sectionInfluxDBv1.sequence.clear();
    decltype(data.sectionInfluxDBv1.sequence)().swap(data.sectionInfluxDBv1.sequence);

    data.sectionInfluxDBv2.sequence.clear();
    decltype(data.sectionInfluxDBv2.sequence)().swap(data.sectionInfluxDBv2.sequence);

    data.sectionGpio.gpioPin.clear();
    decltype(data.sectionGpio.gpioPin)().swap(data.sectionGpio.gpioPin);
}


void ConfigClass::clearCfgDataTemp()
{
    clearConfigDataStructure(cfgDataTemp);
}


void ConfigClass::clearCfgData()
{
    clearConfigDataStructure(cfgData);
}


//**************************************************************************************************
// Load data from NVS storage
//**************************************************************************************************
bool ConfigClass::loadDataFromNVS(const std::string &key, std::string &value)
{
    if (key.empty() || key.length() > 15) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: Key: " + key + ": empty / too long (max. 15)");
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t nvshandle;

    err = nvs_open("cfg_data", NVS_READONLY, &nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_open | error: " + intToHexString(err));
        return false;
    }
    else if (err != ESP_OK && (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NVS_INVALID_HANDLE)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "loadDataFromNVS: nvs_open | No data in NVS namespace 'cfg_data'");
        return false;
    }

    // Get string length
    size_t requiredSize = 0;
    err = nvs_get_str(nvshandle, key.c_str(), NULL, &requiredSize);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_get_str | Key: " + key + " length | error: " + intToHexString(err));
        nvs_close(nvshandle);
        return false;
    }

    if (requiredSize > 0) {
        char cValue[requiredSize + 1];
        err = nvs_get_str(nvshandle, key.c_str(), cValue, &requiredSize);
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_get_str | Key: " + key + " | error: " + intToHexString(err));
            nvs_close(nvshandle);
            return false;
        }
        value = std::string(cValue);
    }

    nvs_close(nvshandle);
    return true;
}


//**************************************************************************************************
// Save data to NVS storage
//**************************************************************************************************
bool ConfigClass::saveDataToNVS(const std::string &key, const std::string &value)
{
    if (key.empty() || key.length() > 15) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: Key: " + key + ": empty / too long (max. 15)");
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t nvshandle;

    err = nvs_open("cfg_data", NVS_READWRITE, &nvshandle);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: nvs_open | error : " + intToHexString(err));
        return false;
    }

    err = nvs_set_str(nvshandle, key.c_str(), value.c_str());
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: Key: " + key + " | nvs_set_str | error: " + intToHexString(err));
        nvs_close(nvshandle);
        return false;
    }

    err = nvs_commit(nvshandle);
    nvs_close(nvshandle);

    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: nvs_commit | error: " + intToHexString(err));
        return false;
    }

    return true;
}


//**************************************************************************************************
// Retrieve actual configuration via REST API (JSON notation)
//**************************************************************************************************
esp_err_t ConfigClass::getConfigRequest(httpd_req_t *req)
{
    if (!cJsonObjectBuffer || !jsonBuffer || !cfgMutex || !req) {
        return ESP_FAIL;
    }

    if (req->user_ctx == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Server context is null");
        return ESP_FAIL;
    }

    char *httpBuffer = static_cast<char *>(((struct HttpServerData *)req->user_ctx)->scratch);

    if (httpBuffer == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Scratch buffer is null");
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_FAIL;

    // Lock mutex guard (RAII)
    {
        CfgMutexGuard lock(cfgMutex, pdMS_TO_TICKS(5000));
        if (!lock.isAcquired()) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Failed to acquire cfgMutex - Timeout");
            return ESP_FAIL;
        }

        cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

        // Serialize config data into jsonBuffer
        retVal = serializeConfig();
        if (retVal == ESP_OK) {
            const size_t length = strlen(jsonBuffer);

            if (length < WEBSERVER_SCRATCH_BUFSIZE) {
                memcpy(httpBuffer, jsonBuffer, length + 1);
            }
            else {
                retVal = ESP_ERR_NO_MEM;
            }
        }
    } // Release mutex guard (RAII)

    if (retVal == ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, httpBuffer, HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to serialize configuration");
    return retVal;
}


//**************************************************************************************************
// Update configuration via REST API (JSON notation)
//**************************************************************************************************
esp_err_t ConfigClass::setConfigRequest(httpd_req_t *req, bool triggerReload)
{
    if (!cJsonObjectBuffer || !jsonBuffer || !cfgMutex || !req) {
        return ESP_FAIL;
    }

    if (req->user_ctx == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Server context is null");
        return ESP_FAIL;
    }

    char *httpBuffer = static_cast<char *>(((struct HttpServerData *)req->user_ctx)->scratch);

    if (httpBuffer == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Scratch buffer is null");
        return ESP_FAIL;
    }

    // Scratch buffer must accommodate the complete payload plus '\0'
    if (req->content_len >= WEBSERVER_SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Payload exceeds maximum buffer size");
        return ESP_FAIL;
    }

    // Receive the complete request into the webserver scratch buffer
    size_t remaining = req->content_len;
    size_t offset = 0;
    uint8_t retries = 0;

    while (remaining > 0) {
        const int received = httpd_req_recv(req, httpBuffer + offset, remaining);

        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT && ++retries < 5) {
                continue;
            }

            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Config reception failed");
            return ESP_FAIL;
        }

        retries = 0;
        offset += static_cast<size_t>(received);
        remaining -= static_cast<size_t>(received);
    }

    httpBuffer[offset] = '\0';

    esp_err_t retVal = ESP_OK;
    httpd_err_code_t httpError = HTTPD_500_INTERNAL_SERVER_ERROR;
    char errorMsg[64] = "Failed to update configuration";

    // Keep cfgMutex locked for the complete configuration update
    {
        CfgMutexGuard lock(cfgMutex, pdMS_TO_TICKS(5000));

        if (!lock.isAcquired()) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Server Error: Failed to acquire cfgMutex - Timeout");
            return ESP_FAIL;
        }

        // Copy the complete request atomically
        memcpy(jsonBuffer, httpBuffer, offset + 1);

        // Parse JSON and update internal configuration
        {
            cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

            cJsonObject = cJSON_Parse(jsonBuffer);

            if (cJsonObject == nullptr) {
                const char *errPtr = cJSON_GetErrorPtr();

                if (errPtr != nullptr) {
                    snprintf(errorMsg, sizeof(errorMsg), "Parse JSON error near: %.20s", errPtr);
                }
                else {
                    snprintf(errorMsg, sizeof(errorMsg), "Parse JSON failed");
                }

                httpError = HTTPD_400_BAD_REQUEST;
                retVal = ESP_ERR_INVALID_ARG;
            }
            else {
                retVal = parseConfig();
            }
        } // Free parse arena

        // Serialize updated configuration
        if (retVal == ESP_OK) {
            cJsonObjectArena jsonArena(cJsonObjectBuffer, CONFIG_HANDLING_CJSON_OBJECT_BUFFER_SIZE);

            retVal = serializeConfig();

            if (retVal == ESP_OK) {
                const size_t length = strlen(jsonBuffer);

                if (length < WEBSERVER_SCRATCH_BUFSIZE) {
                    memcpy(httpBuffer, jsonBuffer, length + 1);
                }
                else {
                    snprintf(errorMsg, sizeof(errorMsg), "JSON size exceeds buffer");
                    retVal = ESP_ERR_NO_MEM;
                }
            }
        } // Free serialization arena

        // Persist updated configuration
        if (retVal == ESP_OK) {
            retVal = writeConfigFile();
        }
    } // Release cfgMutex

    // HTTP response
    if (retVal == ESP_OK) {
        // Stage config reload + add custom headers
        if (triggerReload) {
            triggerReloadConfig(req);
        }

        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, httpBuffer, HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_send_err(req, httpError, errorMsg);
    return retVal;
}


esp_err_t handlerGetConfigRequest(httpd_req_t *req)
{
    const char *APIName = "config:v1"; // API name and version
    char _query[200];
    char _valuechar[30];
    std::string task;

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }

    if (task.compare("api_name") == 0) { // Response API version
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("reload") == 0) { // Load configuration and reinit process
        return triggerReloadConfig(req);
    }

    return ConfigClass::getInstance()->getConfigRequest(req); // Response with configuration
}


esp_err_t handlerSetConfigRequest(httpd_req_t *req)
{
    // Check for query parameter
    bool triggerReload = false;
    size_t queryLen = httpd_req_get_url_query_len(req);

    if (queryLen > 0) {
        char queryBuf[queryLen + 1];
        if (httpd_req_get_url_query_str(req, queryBuf, sizeof(queryBuf)) == ESP_OK) {
            char valBuf[16] = {0};
            if (httpd_query_key_value(queryBuf, "reload", valBuf, sizeof(valBuf)) == ESP_OK) {
                if (strcasecmp(valBuf, "true") == 0 || strcmp(valBuf, "1") == 0) {
                    triggerReload = true;
                }
            }
        }
    }

    // Process setConfigRequest
    return ConfigClass::getInstance()->setConfigRequest(req, triggerReload);
}


void registerConfigFileUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = {};

    camuri.uri = "/config";
    camuri.handler = HTTP_AUTH_BASIC(handlerGetConfigRequest);
    camuri.method = HTTP_GET;
    camuri.user_ctx = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri = "/config";
    camuri.handler = HTTP_AUTH_BASIC(handlerSetConfigRequest);
    camuri.method = HTTP_POST;
    camuri.user_ctx = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri = "/config";
    camuri.handler = HTTP_AUTH_BASIC(NULL);
    camuri.method = HTTP_OPTIONS;
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
