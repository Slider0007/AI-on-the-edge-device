#include "server_mqtt.h"

#ifdef ENABLE_MQTT
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <esp_log.h>
#include <esp_private/esp_clk.h>
#include <cJSON.h>

#include "http_auth.h"
#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "network_main.h"
#include "connect_wlan.h"
#include "interface_mqtt.h"
#include "time_sntp.h"
#include "helper.h"
#include "system.h"
#include "gpioControl.h"

static const char *TAG = "SERVER_MQTT";


extern const char *libfive_git_version(void);
extern const char *libfive_git_revision(void);
extern const char *libfive_git_branch(void);

static bool publishHADiscoveryTopic(const strHADiscoveryData *_data, int _qos);
static bool publishHADiscoveryTopicDeviceInfo = true;
static bool publishHADiscoveryScheduled = true;
static bool publishDeviceInfoScheduled = true;

static const CfgData::SectionMqtt *cfgDataPtr = NULL;
static const std::vector<SequenceData *> *sequenceData;
static float processingInterval; // Minutes
static HAMeterConfig meterConfig;


// Publish device info topics (common topics, static, retained)
bool mqttServer_publishDeviceInfo(int _qos)
{
    if (!publishDeviceInfoScheduled) {
        return true;
    }

    if (!getMqttIsConnected()) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip publish device info, not (yet) connected to broker");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Publish device info");

    const std::string deviceInfoTopic = "/device/info/";
    bool retVal = true;

    // Prepare topic: device/info/hardware
    cJSON *cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }

    cJSON *cJSONObjectHardwareBoard = cJSON_AddObjectToObject(cJSONObject, "board");
    if (cJSONObjectHardwareBoard == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "board_type", getBoardType().c_str()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "chip_model", getChipModel().c_str()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddNumberToObject(cJSONObjectHardwareBoard, "chip_cores", getChipCoreCount()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "chip_revision", getChipRevision().c_str()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddNumberToObject(cJSONObjectHardwareBoard, "chip_frequency", esp_clk_cpu_freq() / 1000000) == NULL) {
            retVal = false;
        }
    }

    cJSON *cJSONObjectHardwareCamera = cJSON_AddObjectToObject(cJSONObject, "camera");
    if (cJSONObjectHardwareCamera == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddStringToObject(cJSONObjectHardwareCamera, "type", cameraCtrl.getCamType().c_str()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddNumberToObject(cJSONObjectHardwareCamera, "frequency", cameraCtrl.getCamFrequencyMhz()) == NULL) {
            retVal = false;
        }
    }

    cJSON *cJSONObjectHardwareSDCard = cJSON_AddObjectToObject(cJSONObject, "sdcard");
    if (cJSONObjectHardwareSDCard == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddNumberToObject(cJSONObjectHardwareSDCard, "capacity", getSDCardCapacity()) == NULL) {
            retVal = false;
        }
        if (cJSON_AddNumberToObject(cJSONObjectHardwareSDCard, "partition_size", getSDCardPartitionSize()) == NULL) {
            retVal = false;
        }
    }

    char *jsonChar = cJSON_PrintBuffered(cJSONObject, 384, 1); // Print to predefined buffer, reduce dynamic allocations
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceInfoTopic + "hardware", std::string(jsonChar), _qos, true);
        cJSON_free(jsonChar);
        jsonChar = NULL;
    }


    // Prepare topic: device/info/network
    cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }
    if (cJSON_AddStringToObject(cJSONObject, "hostname", getHostname().c_str()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddStringToObject(cJSONObject, "ipv4_address", getIpAddress().c_str()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddStringToObject(cJSONObject, "mac_address", getMac().c_str()) == NULL) {
        retVal = false;
    }

    jsonChar = cJSON_Print(cJSONObject);
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceInfoTopic + "network", std::string(jsonChar), _qos, true);
        cJSON_free(jsonChar);
        jsonChar = NULL;
    }


    // Prepare topic: device/info/version
    std::string firmwareVersion = std::string(libfive_git_version());
    if (firmwareVersion == "" || firmwareVersion == "N/A") {
        firmwareVersion = std::string(libfive_git_branch()) + " (" + std::string(libfive_git_revision()) + ")";
    }

    retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceInfoTopic + "firmware_version", firmwareVersion, _qos, true);

    if (!retVal) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to publish device info");
        return false;
    }

    publishDeviceInfoScheduled = false;
    return true;
}


// Publish device status topics (common topics variable)
bool mqttServer_publishDeviceStatus(int _qos)
{
    if (!getMqttIsConnected()) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip publish device status, not (yet) connected to broker");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Publish device status");

    const std::string deviceStatusTopic = "/device/status/";
    bool retVal = true;

    retVal &= publishMqttData(cfgDataPtr->mainTopic + MQTT_STATUS_TOPIC, MQTT_STATUS_ONLINE, _qos, true);
    retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "device_uptime", std::to_string(getUptime()), _qos, false);

    // Publish only, if network mode WLAN / WLAN AP
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN || getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN_AP) {
        retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "wlan_rssi", std::to_string(getWifiRssi()), _qos, false);
    }

    retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "chip_temp", to_stringWithPrecision(getSOCTemperature(), 0), _qos,
                              false);

    cJSON *cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_total_free", getESPHeapSizeTotalFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_free", getESPHeapSizeInternalFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_largest_free", getESPHeapSizeInternalLargestFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_min_free", getESPHeapSizeInternalMinFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_free", getESPHeapSizeSPIRAMFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_largest_free", getESPHeapSizeSPIRAMLargestFree()) == NULL) {
        retVal = false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_min_free", getESPHeapSizeSPIRAMMinFree()) == NULL) {
        retVal = false;
    }

    char *jsonChar = cJSON_Print(cJSONObject);
    cJSON_Delete(cJSONObject);

    if (jsonChar != NULL) {
        retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "heap", std::string(jsonChar), _qos, false);
        cJSON_free(jsonChar);
    }

    retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "sd_partition_free",
                              std::to_string(getSDCardFreePartitionSpace()), _qos, false);
    retVal &= publishMqttData(cfgDataPtr->mainTopic + deviceStatusTopic + "ntp_syncstatus", getNTPSyncStatus().c_str(), _qos, false);

    if (!retVal) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to publish device status");
        return false;
    }

    return true;
}


void mqttServer_setParameter(const CfgData::SectionMqtt *_cfgDataPtr, const std::vector<SequenceData *> *_sequenceData,
                             const float _processingInterval)
{
    cfgDataPtr = _cfgDataPtr;
    sequenceData = _sequenceData;
    processingInterval = _processingInterval;
}


void mqttServer_setHaMeterType(const int _haMeterType)
{
    // Preconfigure meter configuration
    // Check for available units:
    // https://developers.home-assistant.io/docs/core/entity/sensor/
    // https://github.com/home-assistant/core/blob/master/homeassistant/components/sensor/const.py
    // DONT'T FORGET, see further down: Device / state class validation (in loop for publish number sequence related topics)
    if (_haMeterType == WATER_M3) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "m³";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "m³/h";
    }
    else if (_haMeterType == WATER_L) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "L";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "L/h"; // Legacy: Keep, even this is not a valid unit in home assistant
    }
    else if (_haMeterType == WATER_LMIN) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "L";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "L/min";
    }
    else if (_haMeterType == WATER_GAL) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "gal";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "gal/h"; // Legacy: Keep, even this is not a valid unit in home assistant
    }
    else if (_haMeterType == WATER_GALMIN) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "gal";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "gal/min";
    }
    else if (_haMeterType == WATER_FT3) {
        meterConfig.deviceClass = "water";
        meterConfig.valueUnit = "ft³";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "ft³/min";
    }
    else if (_haMeterType == GAS_M3) {
        meterConfig.deviceClass = "gas";
        meterConfig.valueUnit = "m³";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "m³/h";
    }
    else if (_haMeterType == GAS_FT3) {
        meterConfig.deviceClass = "gas";
        meterConfig.valueUnit = "ft³";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "ft³/min";
    }
    else if (_haMeterType == ENERGY_WH) {
        meterConfig.deviceClass = "energy";
        meterConfig.valueUnit = "Wh";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "W";
    }
    else if (_haMeterType == ENERGY_KWH) {
        meterConfig.deviceClass = "energy";
        meterConfig.valueUnit = "kWh";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "kW";
    }
    else if (_haMeterType == ENERGY_MWH) {
        meterConfig.deviceClass = "energy";
        meterConfig.valueUnit = "MWh";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "MW";
    }
    else if (_haMeterType == ENERGY_GJ) {
        meterConfig.deviceClass = "energy";
        meterConfig.valueUnit = "GJ";
        meterConfig.timeUnit = "h";
        meterConfig.rateUnit = "GJ/h"; //  Legacy: Keep, even this is not a valid unit in home assistant
    }
    else if (_haMeterType == TEMPERATURE_C) {
        meterConfig.deviceClass = "temperature";
        meterConfig.valueUnit = "°C";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "K/min";
    }
    else if (_haMeterType == TEMPERATURE_F) {
        meterConfig.deviceClass = "temperature";
        meterConfig.valueUnit = "°F";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "K/min";
    }
    else if (_haMeterType == PRESSURE_BAR) {
        meterConfig.deviceClass = "pressure";
        meterConfig.valueUnit = "bar";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "bar/min";
    }
    else if (_haMeterType == PRESSURE_PSI) {
        meterConfig.deviceClass = "pressure";
        meterConfig.valueUnit = "psi";
        meterConfig.timeUnit = "min";
        meterConfig.rateUnit = "psi/min";
    }
    else {
        meterConfig.deviceClass = "";
        meterConfig.valueUnit = "";
        meterConfig.timeUnit = "Unknown";
        meterConfig.rateUnit = "";
    }
}


std::string mqttServer_getTimeUnit(void)
{
    return meterConfig.timeUnit;
}


std::string mqttServer_getMainTopic()
{
    return ConfigClass::getInstance()->get()->sectionMqtt.mainTopic; // Return value from global struct to ensure valid result at any time
}


void mqttServer_schedulePublishDeviceInfo()
{
    publishDeviceInfoScheduled = true;
}


void mqttServer_schedulePublishHADiscovery()
{
    publishHADiscoveryScheduled = true;
}


bool mqttServer_schedulePublishHADiscoveryFromMqtt(std::string _topic, char *_data, int _data_len)
{
    if (_data_len > 0) { // Check if data length > 0
        if (strncmp("online", _data, _data_len) == 0) {
            publishHADiscoveryScheduled = true;
        }
    }

    return true;
}


esp_err_t handler_mqtt(httpd_req_t *req)
{
    const char *APIName = "mqtt:v2"; // API name and version
    char _query[64];
    char _valuechar[30];
    std::string task;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = "Handler usage:<br>"
                                      "1. Schedule publication of Home Assistant discovery MQTT topics:<br>"
                                      " - '/mqtt?task=publish_ha_discovery'<br>"
                                      "2. Schedule publication of device info MQTT topics:<br>"
                                      " - '/mqtt?task=publish_device_info'<br>"
                                      "3. Print API name and version<br>"
                                      " - '/mqtt?task=api_name'";

    httpd_resp_set_type(req, "text/plain");

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }
    else { // if no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, RESTUsageInfo.c_str());
        return ESP_OK;
    }

    if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("publish_ha_discovery") == 0) {
        publishHADiscoveryScheduled = true;
        httpd_resp_sendstr(req, "001: Publication of HA discovery topics scheduled during state 'Publish to MQTT'");
        return ESP_OK;
    }
    else if (task.compare("publish_device_info") == 0) {
        publishDeviceInfoScheduled = true;
        httpd_resp_sendstr(req, "002: Publication of device info topics scheduled during state 'Publish to MQTT'");
        return ESP_OK;
    }
    else {
        httpd_resp_sendstr(req, "E90: Task not found");
        return ESP_ERR_NOT_FOUND;
    }
}


// Publish HA discovery topics (no retained)
bool mqttServer_publishHADiscovery(int _qos)
{
    if (!cfgDataPtr->homeAssistant.discoveryEnabled || !publishHADiscoveryScheduled) { // Continue if enabled and scheduled
        return true;
    }

    if (!getMqttIsConnected()) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip publish HA discovery, not (yet) connected to broker");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "Publish HA discovery | Meter type: " + meterConfig.deviceClass + ", Value unit: " + meterConfig.valueUnit +
                            " , Rate unit: " + meterConfig.rateUnit);

    publishHADiscoveryTopicDeviceInfo = true; // Publish full common device info data only once
    bool publishOK = true;

    strHADiscoveryData HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "ipv4_address",
        .friendlyName = "IP Address",
        .icon = "network-outline",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "mac_address",
        .friendlyName = "MAC Address",
        .icon = "network-outline",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "hostname",
        .friendlyName = "Hostname",
        .icon = "network-outline",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/device_uptime",
        .topicName = "device_uptime",
        .friendlyName = "Uptime",
        .icon = "progress-clock",
        .unit = "s",
        .deviceClass = "duration",
        .stateClass = "measurement",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/status/heap",
        .topicName = "heap_internal_free",
        .friendlyName = "Memory Internal Free",
        .icon = "memory",
        .unit = "B",
        .stateClass = "measurement",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/status/heap",
        .topicName = "heap_spiram_free",
        .friendlyName = "Memory External Free",
        .icon = "memory",
        .unit = "B",
        .stateClass = "measurement",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    // Publish only, if network mode WLAN / WLAN AP
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN || getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN_AP) {
        HADiscoveryData = {};
        HADiscoveryData = {
            .topic = "/device/status/wlan_rssi",
            .topicName = "wlan_rssi",
            .friendlyName = "WLAN Signal Strength",
            .icon = "wifi",
            .unit = "dBm",
            .deviceClass = "signal_strength",
            .stateClass = "measurement",
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);
    }

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/chip_temp",
        .topicName = "chip_temp",
        .friendlyName = "CPU Temperature",
        .icon = "thermometer",
        .unit = "°C",
        .deviceClass = "temperature",
        .stateClass = "measurement",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/ntp_syncstatus",
        .topicName = "ntp_syncstatus",
        .friendlyName = "NTP Sync Status",
        .icon = "network-outline",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_status",
        .topicName = "process_status",
        .friendlyName = "Process Status",
        .icon = "list-status",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_interval",
        .topicName = "process_interval",
        .friendlyName = "Process Interval",
        .icon = "clock-time-eight-outline",
        .unit = "min",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_time",
        .topicName = "process_time",
        .friendlyName = "Process Time",
        .icon = "clock-time-eight-outline",
        .unit = "s",
        .stateClass = "measurement",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_state",
        .topicName = "process_state",
        .friendlyName = "Process State",
        .icon = "list-status",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_error", // binary sensor for summary error indication (error after multiple events in row)
        .topicName = "process_error",
        .friendlyName = "Process Error State",
        .icon = "alert-outline",
        .deviceClass = "problem" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_error",
        .topicName = "process_error_value",
        .friendlyName = "Process Error Value",
        .icon = "alert-outline",
        .entityCategory = "diagnostic" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    // Publish number sequence related topics
    for (const auto &sequence : *sequenceData) {
        // Device / state class validation
        // Check for valid device class / state class combinations:
        // https://github.com/home-assistant/core/blob/master/homeassistant/components/sensor/const.py
        if (meterConfig.deviceClass == "water" || meterConfig.deviceClass == "gas") {
            meterConfig.valueStateClass = sequence->paramPostProc->allowNegativeRate ? "total" : "total_increasing";

            // Legacy: L/h and gal/h are not valid units in home assistant: Use device class `None`
            ((meterConfig.rateUnit != "L/h") && (meterConfig.rateUnit != "gal/h")) ? meterConfig.rateDeviceClass = "volume_flow_rate"
                                                                                   : meterConfig.rateDeviceClass = "";
        }
        else if (meterConfig.deviceClass == "energy") {
            meterConfig.valueStateClass = sequence->paramPostProc->allowNegativeRate ? "total" : "total_increasing";

            // Legacy: GJ/h is not a valid unit in home assistant: Use device class `None`
            meterConfig.rateUnit != "GJ/h" ? meterConfig.rateDeviceClass = "power" : meterConfig.rateDeviceClass = "";
        }
        else if (meterConfig.deviceClass == "temperature" || meterConfig.deviceClass == "pressure") {
            meterConfig.valueStateClass = "measurement";
            meterConfig.rateDeviceClass = "";
        }
        else {
            meterConfig.valueStateClass = "measurement";
            meterConfig.rateDeviceClass = "";
        }

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "actual_value",
            .friendlyName = "Actual Value",
            .icon = "gauge",
            .unit = meterConfig.valueUnit,
            .deviceClass = meterConfig.deviceClass,
            .stateClass = meterConfig.valueStateClass //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        if (sequence->paramPostProc->useFallbackValue) {
            HADiscoveryData = {};
            HADiscoveryData = {
                .structureName = sequence->sequenceName,
                .isTopicJSONNotation = true,
                .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
                .topicName = "fallback_value",
                .friendlyName = "Fallback Value",
                .icon = "gauge",
                .unit = meterConfig.valueUnit,
                .deviceClass = meterConfig.deviceClass,
                .stateClass = meterConfig.valueStateClass,
                .entityCategory = "diagnostic" //
            };
            publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);
        }

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "raw_value",
            .friendlyName = "Raw Value",
            .icon = "gauge",
            // Special case: If value could have 'NaN' parts (e.g. 2N234.333)
            // --> Use value as string, no long-term statistics possible
            //.unit = meterConfig.valueUnit,
            //.deviceClass = meterConfig.deviceClass,
            //.stateClass = meterConfig.valueStateClass,
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "value_status",
            .friendlyName = "Value Status",
            .icon = "alert-circle-outline",
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "rate_per_time_unit",
            .friendlyName = "Rate",
            .icon = "swap-vertical",
            .unit = meterConfig.rateUnit,
            .deviceClass = meterConfig.rateDeviceClass,
            .stateClass = "measurement" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "rate_per_interval",
            .friendlyName = "Rate / Interval (" + to_stringWithPrecision(processingInterval, 1) + "min)",
            .icon = "arrow-expand-vertical",
            .unit = meterConfig.valueUnit,
            // Special case: This rate is not using any common rate unit
            //.deviceClass = "", // Use device class 'None'
            .stateClass = "measurement",
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = sequence->sequenceName,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
            .topicName = "timestamp_processed",
            .friendlyName = "Last Processed",
            .icon = "clock-time-eight-outline",
            .deviceClass = "timestamp",
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);
    }

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/ctrl/cycle_start",
        .topicName = "cycle_start",
        .friendlyName = "Manual Cycle Start",
        .icon = "timer-play-outline" //
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    // Publish GPIO state topic
    for (const auto &gpioPin : ConfigClass::getInstance()->get()->sectionGpio.gpioPin) {
        if (!gpioPin.pinEnabled || !gpioPin.exposeToMqtt) { // Skip if disabled or not exposed to MQTT
            continue;
        }

        std::string gpioName = gpioPin.pinName.empty() ? "gpio" + std::to_string(gpioPin.gpioNumber) : gpioPin.pinName;
        gpio_pin_mode_t pinMode = getGpioHandle()->resolvePinMode(toLower(gpioPin.pinMode));

        HADiscoveryData = {};
        HADiscoveryData = {
            .structureName = gpioName,
            .isTopicJSONNotation = true,
            .topic = "/device/gpio/" + gpioName + "/state",
            .topicName = "state",
            .friendlyName = "State",
            .entityCategory = "diagnostic" //
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        if (pinMode == GPIO_PIN_MODE_OUTPUT_PWM) {
            HADiscoveryData = {};
            HADiscoveryData = {
                .structureName = gpioName,
                .isTopicJSONNotation = true,
                .topic = "/device/gpio/" + gpioName + "/state",
                .topicName = "pwm_duty",
                .friendlyName = "PWM Duty",
                .entityCategory = "diagnostic" //
            };
            publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);
        }
    }

    if (!publishOK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to publish HA discovery");
        return false;
    }

    publishHADiscoveryScheduled = false;
    return true;
}


static bool publishHADiscoveryTopic(const strHADiscoveryData *_data, const int _qos)
{
    // Use MQTT maintopic without path structure as nodeID
    std::string nodeID = cfgDataPtr->mainTopic;
    if ((cfgDataPtr->mainTopic.find_last_of('/')) != -1) {
        nodeID = cfgDataPtr->mainTopic.substr(cfgDataPtr->mainTopic.find_last_of('/') + 1);
    }

    // Add name prefix for number sequences to make ID and friendly names unique
    std::string topicNameID = _data->topicName;
    std::string friendlyName = _data->friendlyName;
    if (!_data->structureName.empty()) {
        topicNameID = _data->structureName + "_" + _data->topicName;
        friendlyName = _data->structureName + ": " + _data->friendlyName;
    }

    // Define configuration topic (default component: sensor)
    std::string configurationTopic = cfgDataPtr->homeAssistant.discoveryPrefix + "/sensor/" + nodeID + "/" + topicNameID + "/config";

    if (_data->topicName == "process_error") { // Special case: Process error
        configurationTopic = cfgDataPtr->homeAssistant.discoveryPrefix + "/binary_sensor/" + nodeID + "/" + topicNameID + "/config";
    }
    else if (_data->topicName == "cycle_start") { // Special case: Cycle start
        configurationTopic = cfgDataPtr->homeAssistant.discoveryPrefix + "/button/" + nodeID + "/" + topicNameID + "/config";
    }
    else if (_data->topic.contains("/gpio/")) { // Special case: GPIO
        if (_data->topicName == "state") {      // State
            configurationTopic = cfgDataPtr->homeAssistant.discoveryPrefix + "/binary_sensor/" + nodeID + "/" + topicNameID + "/config";
        }
        else { // PWM duty
            configurationTopic = cfgDataPtr->homeAssistant.discoveryPrefix + "/sensor/" + nodeID + "/" + topicNameID + "/config";
        }
    }

    // Define payload for configuration topic
    // See https://www.home-assistant.io/docs/mqtt/discovery/
    // Abbreviations: https://www.home-assistant.io/integrations/mqtt/#supported-abbreviations-in-mqtt-discovery-messages
    std::string payload = "{\"~\":\"" + cfgDataPtr->mainTopic + "\"," + "\"uniq_id\":\"" + nodeID + "_" + topicNameID + "\"," +
                          //"\"obj_id\":\"" + nodeID + "_" + topicNameID + "\"," + // This used to generate the entity ID
                          "\"name\":\"" + friendlyName + "\",";

    if (!_data->icon.empty()) {
        payload += "\"ic\":\"mdi:" + _data->icon + "\",";
    }

    // Define command or status topic
    if (_data->topicName == "cycle_start") {               // Special case: cycle_start command
        payload += "\"cmd_t\":\"~" + _data->topic + "\","; // Add command topic
        payload += "\"pl_prs\":\"1\",";
    }
    else if (_data->topic.contains("/gpio/")) {             // Special case: GPIO
        payload += "\"stat_t\":\"~" + _data->topic + "\","; // Add status topic

        if (_data->topicName == "state") {  // GPIO state
            payload += "\"pl_on\":\"1\",";  // payload "ON"
            payload += "\"pl_off\":\"0\","; // payload "OFF"
        }
    }
    else {
        payload += "\"stat_t\":\"~" + _data->topic + "\","; // Add status topic
    }

    // Define value template
    if (_data->isTopicJSONNotation) {
        payload += "\"val_tpl\":\"{{value_json." + _data->topicName + "}}\",";
    }
    // Signal a problem only if multiple process errors (-2) or process deviation (2) in row occurred
    else if (_data->topicName == "process_error") { // Special case: process error
        payload += "\"val_tpl\":\"{{ 'ON' if '-2' in value or '2' in value else 'OFF'}}\",";
    }

    // Set QOS to "At least once" (QoS 1)
    payload += "\"qos\":\"1\",";

    if (!_data->unit.empty()) {
        payload += "\"unit_of_meas\":\"" + _data->unit + "\",";
    }

    if (!_data->deviceClass.empty()) {
        payload += "\"dev_cla\":\"" + _data->deviceClass + "\",";
    }

    if (!_data->stateClass.empty()) {
        payload += "\"stat_cla\":\"" + _data->stateClass + "\",";
    }

    if (!_data->entityCategory.empty()) {
        payload += "\"ent_cat\":\"" + _data->entityCategory + "\",";
    }

    // Define availability topic
    payload += "\"avty_t\":\"~" + std::string(MQTT_STATUS_TOPIC) + "\"";

    // Publish complete general device info only once
    if (publishHADiscoveryTopicDeviceInfo) {
        std::string firmwareVersion = std::string(libfive_git_version());
        if (firmwareVersion == "" || firmwareVersion == "N/A") {
            firmwareVersion = std::string(libfive_git_branch()) + " (" + std::string(libfive_git_revision()) + ")";
        }

        payload += std::string(", \"dev\": {") + "\"ids\":[\"" + nodeID + "\"]," + "\"name\":\"" + nodeID + "\"," +
                   "\"mdl\":\"AI-on-the-Edge device (" + getBoardType() + ")\"," + "\"mf\":\"AI-on-the-Edge\"," + "\"sw\":\"" +
                   firmwareVersion + " [SLFork]\"," + "\"cu\":\"http://" + getIpAddress() + "\"}";
    }
    else { // Publish device reference only to group data together
        payload += std::string(", \"dev\": {") + "\"ids\":[\"" + nodeID + "\"]}";
    }

    payload += "}";

    if (publishMqttData(configurationTopic, payload, _qos, cfgDataPtr->homeAssistant.retainDiscovery)) {
        publishHADiscoveryTopicDeviceInfo = false;
        return true;
    }
    return false;
}


void registerMqttUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t uri = {};
    uri.method = HTTP_GET;

    uri.uri = "/mqtt";
    uri.handler = HTTP_AUTH_BASIC(handler_mqtt);
    uri.user_ctx = NULL;
    httpd_register_uri_handler(server, &uri);
}

#endif // ENABLE_MQTT
