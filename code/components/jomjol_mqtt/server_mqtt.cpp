#include "server_mqtt.h"

#ifdef ENABLE_MQTT
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "cJSON.h"

#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "connect_wlan.h"
#include "interface_mqtt.h"
#include "time_sntp.h"
#include "Helper.h"
#include "system.h"

static const char *TAG = "MQTT_SERVER";


extern const char* libfive_git_version(void);
extern const char* libfive_git_revision(void);
extern const char* libfive_git_branch(void);

extern const strMqttConfig mqttConfig;
strHADiscoveryConfig HADiscoveryConfig = {}; // Global struct for HA discovery config

static bool publishHADiscoveryTopic(const strHADiscoveryData *_data, int _qos);
static bool publishHADiscoveryTopicDeviceInfo = true;

static bool publishHADiscoveryScheduled = true;
static bool publishDeviceInfoScheduled = true;

static std::vector<NumberPost*>* numberSequences;
static float processingInterval; // Minutes
static std::string meterType, valueUnit, timeUnit, rateUnit;


// Publish device info topics (common topics, static, retained)
bool mqttServer_publishDeviceInfo(int _qos)
{
    if (!publishDeviceInfoScheduled)
        return true;

    if (!getMQTTisConnected()) {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip publish device info, not (yet) connected to broker");
        return false;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish device info");

    const std::string deviceInfoTopic = "/device/info/";
    bool retVal = true;

    // Prepare topic: device/info/hardware
    cJSON *cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }

    cJSON *cJSONObjectHardwareBoard = cJSON_AddObjectToObject(cJSONObject, "board");
    if (cJSONObjectHardwareBoard == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "board_type", getBoardType().c_str()) == NULL)
            retVal = false;
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "chip_model", getChipModel().c_str()) == NULL)
            retVal = false;
        if (cJSON_AddNumberToObject(cJSONObjectHardwareBoard, "chip_cores", getChipCoreCount()) == NULL)
            retVal = false;
        if (cJSON_AddStringToObject(cJSONObjectHardwareBoard, "chip_revision", getChipRevision().c_str()) == NULL)
            retVal = false;
        if (cJSON_AddNumberToObject(cJSONObjectHardwareBoard, "chip_frequency", esp_clk_cpu_freq()/1000000) == NULL)
            retVal = false;
    }

    cJSON *cJSONObjectHardwareCamera = cJSON_AddObjectToObject(cJSONObject, "camera");
    if (cJSONObjectHardwareCamera == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddStringToObject(cJSONObjectHardwareCamera, "type", Camera.getCamType().c_str()) == NULL)
            retVal = false;
        if (cJSON_AddNumberToObject(cJSONObjectHardwareCamera, "frequency", Camera.getCamFrequencyMhz()) == NULL)
            retVal = false;
    }

    cJSON *cJSONObjectHardwareSDCard = cJSON_AddObjectToObject(cJSONObject, "sdcard");
    if (cJSONObjectHardwareSDCard == NULL) {
        retVal = false;
    }
    else {
        if (cJSON_AddNumberToObject(cJSONObjectHardwareSDCard, "capacity", getSDCardCapacity()) == NULL)
            retVal = false;
        if (cJSON_AddNumberToObject(cJSONObjectHardwareSDCard, "partition_size", getSDCardPartitionSize()) == NULL)
            retVal = false;
    }

    char *jsonString = cJSON_PrintBuffered(cJSONObject, 384, 1); // Print to predefined buffer, avoid dynamic allocations
    std::string jsonData = std::string(jsonString);
    cJSON_free(jsonString);
    cJSON_Delete(cJSONObject);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceInfoTopic + "hardware", jsonData, _qos, true);

    // Prepare topic: device/info/network
    cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }
    if (cJSON_AddStringToObject(cJSONObject, "hostname", getHostname().c_str()) == NULL)
        retVal = false;
    if (cJSON_AddStringToObject(cJSONObject, "ipv4_address", getIPAddress().c_str()) == NULL)
        retVal = false;
    if (cJSON_AddStringToObject(cJSONObject, "mac_address", getMac().c_str()) == NULL)
        retVal = false;

    jsonString = cJSON_PrintBuffered(cJSONObject, 256, 1); // Print to predefined buffer, avoid dynamic allocations
    jsonData = std::string(jsonString);
    cJSON_free(jsonString);
    cJSON_Delete(cJSONObject);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceInfoTopic + "network", jsonData, _qos, true);

    // Prepare topic: device/info/version
    std::string firmwareVersion = std::string(libfive_git_version());
    if (firmwareVersion == "" || firmwareVersion == "N/A")
        firmwareVersion = std::string(libfive_git_branch()) + " (" + std::string(libfive_git_revision()) + ")";

    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceInfoTopic + "firmware_version", firmwareVersion, _qos, true);

    if (!retVal) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to publish device info");
        return false;
    }

    publishDeviceInfoScheduled = false;
    return true;
}


// Publish device status topics (common topics variable)
bool mqttServer_publishDeviceStatus(int _qos)
{
    if (!getMQTTisConnected()) {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip publish device status, not (yet) connected to broker");
        return false;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish device status");

    const std::string deviceStatusTopic = "/device/status/";
    bool retVal = true;

    retVal &= MQTTPublish(mqttConfig.mainTopic + MQTT_STATUS_TOPIC, MQTT_STATUS_ONLINE, _qos, false);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "device_uptime", std::to_string(getUptime()), _qos, false);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "wlan_rssi", std::to_string(get_WIFI_RSSI()), _qos, false);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "chip_temp", to_stringWithPrecision(getSOCTemperature(), 0), _qos, false);

    cJSON *cJSONObject = cJSON_CreateObject();
    if (cJSONObject == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }
    if (cJSON_AddNumberToObject(cJSONObject, "heap_total_free", getESPHeapSizeTotalFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_free", getESPHeapSizeInternalFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_largest_free", getESPHeapSizeInternalLargestFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_min_free", getESPHeapSizeInternalMinFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_free", getESPHeapSizeSPIRAMFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_largest_free", getESPHeapSizeSPIRAMLargestFree()) == NULL)
        retVal = false;
    if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_min_free", getESPHeapSizeSPIRAMMinFree()) == NULL)
        retVal = false;

    char *jsonString = cJSON_PrintBuffered(cJSONObject, 256, 1); // Print to predefined buffer, avoid dynamic allocations
    std::string jsonData = std::string(jsonString);
    cJSON_free(jsonString);
    cJSON_Delete(cJSONObject);

    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "heap", jsonData, _qos, false);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "sd_partition_free", std::to_string(getSDCardFreePartitionSpace()), _qos, false);
    retVal &= MQTTPublish(mqttConfig.mainTopic + deviceStatusTopic + "ntp_syncstatus", getNTPSyncStatus().c_str(), _qos, false);

    if (!retVal) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to publish device status");
        return false;
    }

    return true;
}


void mqttServer_setParameter(std::vector<NumberPost*> *_numberSequences, float _processingInterval)
{
    numberSequences = _numberSequences;
    processingInterval = _processingInterval;
}


void mqttServer_setMeterType(std::string _meterType, std::string _valueUnit, std::string _timeUnit, std::string _rateUnit)
{
    meterType = _meterType;
    valueUnit = _valueUnit;
    timeUnit = _timeUnit;
    rateUnit = _rateUnit;
}


std::string mqttServer_getTimeUnit(void)
{
    return timeUnit;
}


std::string mqttServer_getMainTopic()
{
    return mqttConfig.mainTopic;
}


void mqttServer_schedulePublishDeviceInfo()
{
    publishDeviceInfoScheduled = true;
}


void mqttServer_schedulePublishHADiscovery()
{
    publishHADiscoveryScheduled = true;
}


bool mqttServer_schedulePublishHADiscoveryFromMqtt(std::string _topic, char* _data, int _data_len)
{
    if (_data_len > 0) {    // Check if data length > 0
        if (strncmp("online", _data, _data_len) == 0) {
            publishHADiscoveryScheduled = true;
        }
    }

    return true;
}


esp_err_t handler_mqtt(httpd_req_t *req)
{
    const char* APIName = "mqtt:v2"; // API name and version
    char _query[64];
    char _valuechar[30];
    std::string task;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo =
        "Handler usage:<br>"
        "1. Schedule publication of Home Assistant discovery MQTT topics:<br>"
        " - '/mqtt?task=publish_ha_discovery'<br>"
        "2. Schedule publication of device info MQTT topics:<br>"
        " - '/mqtt?task=publish_device_info'<br>"
        "3. Print API name and version<br>"
        " - '/mqtt?task=api_name'";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }
    else {  // if no parameter is provided, print handler usage
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
    if (!HADiscoveryConfig.HADiscoveryEnabled || !publishHADiscoveryScheduled) // Continue if enabled and scheduled
        return true;

    if (!getMQTTisConnected()) {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip publish HA discovery, not (yet) connected to broker");
        return false;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish HA discovery | Sensor device class: " + meterType +
                                    ", Value unit: " + valueUnit + " , Rate unit: " + rateUnit);

    publishHADiscoveryTopicDeviceInfo = true; // Publish full common device info data only once
    bool publishOK = true;

    strHADiscoveryData HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "ipv4_address",
        .friendlyName = "IP Address",
        .icon = "network-outline",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "mac_address",
        .friendlyName = "MAC Address",
        .icon = "network-outline",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .isTopicJSONNotation = true,
        .topic = "/device/info/network",
        .topicName = "hostname",
        .friendlyName = "Hostname",
        .icon = "network-outline",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/device_uptime",
        .topicName = "device_uptime",
        .friendlyName = "Uptime",
        .icon = "clock-time-eight-outline",
        .unit = "s",
        .entityCategory = "diagnostic"
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
        .entityCategory = "diagnostic"
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
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);


    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/wlan_rssi",
        .topicName = "wlan_rssi",
        .friendlyName = "WLAN Signal Strength",
        .icon = "wifi",
        .unit = "dBm",
        .deviceClass = "signal_strength",
        .stateClass = "measurement",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/chip_temp",
        .topicName = "chip_temp",
        .friendlyName = "CPU Temperature",
        .icon = "thermometer",
        .unit = "°C",
        .deviceClass = "temperature",
        .stateClass = "measurement",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/device/status/ntp_syncstatus",
        .topicName = "ntp_syncstatus",
        .friendlyName = "NTP Sync Status",
        .icon = "network-outline",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_status",
        .topicName = "process_status",
        .friendlyName = "Process Status",
        .icon = "list-status",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_interval",
        .topicName = "process_interval",
        .friendlyName = "Process Interval",
        .icon = "clock-time-eight-outline",
        .unit = "min",
        .entityCategory = "diagnostic"
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
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_state",
        .topicName = "process_state",
        .friendlyName = "Process State",
        .icon = "list-status",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_error", // binary sensor for summary error indication (error after multiple events in row)
        .topicName = "process_error",
        .friendlyName = "Process Error State",
        .icon = "alert-outline",
        .deviceClass = "problem"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/status/process_error",
        .topicName = "process_error_value",
        .friendlyName = "Process Error Value",
        .icon = "alert-outline",
        .entityCategory = "diagnostic"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    // Publish number sequence related topics
    for (int i = 0; i < (*numberSequences).size(); ++i) {
        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "actual_value",
            .friendlyName = "Actual Value",
            .icon = "gauge",
            .unit = valueUnit,
            .deviceClass = meterType,
            .stateClass = (*numberSequences)[i]->allowNegativeRates ? "measurement" : "total_increasing"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "fallback_value",
            .friendlyName = "Fallback Value",
            .icon = "gauge",
            .unit = valueUnit,
            .deviceClass = meterType,
            .stateClass = "measurement",
            .entityCategory = "diagnostic"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "raw_value",
            .friendlyName = "Raw Value",
            .icon = "gauge",
            .unit = valueUnit,
            .deviceClass = meterType,
            .stateClass = "measurement",
            .entityCategory = "diagnostic"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "value_status",
            .friendlyName = "Value Status",
            .icon = "alert-circle-outline",
            .entityCategory = "diagnostic"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "rate_per_time_unit",
            .friendlyName = "Rate",
            .icon = "swap-vertical",
            .unit = rateUnit,
            .stateClass = "measurement"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "rate_per_interval",
            .friendlyName = "Rate / Interval",
            .icon = "arrow-expand-vertical",
            .unit = valueUnit != "" ? valueUnit + "/" + to_stringWithPrecision(processingInterval, 1) + "min" : "",
            .stateClass = "measurement"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

        HADiscoveryData = {};
        HADiscoveryData = {
            .numberSequenceID = i,
            .isTopicJSONNotation = true,
            .topic = "/process/data/" + std::to_string(i+1) + "/json",
            .topicName = "timestamp_processed",
            .friendlyName = "Last Processed",
            .icon = "clock-time-eight-outline",
            .deviceClass = "timestamp",
            .entityCategory = "diagnostic"
        };
        publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);
    }

    HADiscoveryData = {};
    HADiscoveryData = {
        .topic = "/process/ctrl/cycle_start",
        .topicName = "cycle_start",
        .friendlyName = "Manual Cycle Start",
        .icon = "timer-play-outline",
        .deviceClass = "update",
        .entityCategory = "config"
    };
    publishOK &= publishHADiscoveryTopic(&HADiscoveryData, _qos);

    if (!publishOK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to publish HA discovery");
        return false;
    }

    publishHADiscoveryScheduled = false;
    return true;
}


static bool publishHADiscoveryTopic(const strHADiscoveryData *_data, const int _qos)
{
    // Use MQTT maintopic without path structure as nodeID
    std::string nodeID = mqttConfig.mainTopic;
    if ((mqttConfig.mainTopic.find_last_of('/')) != -1)
        nodeID = mqttConfig.mainTopic.substr(mqttConfig.mainTopic.find_last_of('/')+1);

    // Add name prefix for number sequences to make ID and friendly names unique
    std::string topicNameID = _data->topicName;
    std::string friendlyName = _data->friendlyName;
    if (_data->numberSequenceID > -1 && _data->numberSequenceID < (*numberSequences).size()) {
        topicNameID = (*numberSequences)[_data->numberSequenceID]->name + "_" + _data->topicName;
        friendlyName = (*numberSequences)[_data->numberSequenceID]->name + ": " + _data->friendlyName;
    }

    // Define configuration topic
    std::string configurationTopic;
    if (_data->deviceClass == "problem" && _data->topicName == "process_error") { // Special case: process_error -> configure as binary sensor
        configurationTopic = HADiscoveryConfig.HADiscoveryPrefix + "/binary_sensor/" + nodeID + "/" + topicNameID + "/config";
    }
    else if (_data->topic == "/process/ctrl/cycle_start") { // Special case: cyle_start command -> configure as button
        configurationTopic = HADiscoveryConfig.HADiscoveryPrefix + "/button/" + nodeID + "/" + topicNameID + "/config";
    }
    else {
        configurationTopic = HADiscoveryConfig.HADiscoveryPrefix + "/sensor/" + nodeID + "/" + topicNameID + "/config";
    }

    // Define payload for configuration topic
    // See https://www.home-assistant.io/docs/mqtt/discovery/
    std::string payload =
        "{\"~\":\"" + mqttConfig.mainTopic + "\","  +
        "\"uniq_id\":\"" + nodeID + "_" + topicNameID + "\"," +
        //"\"obj_id\":\"" + nodeID + "_" + topicNameID + "\"," + // This used to generate the entity ID
        "\"name\":\"" + friendlyName + "\",";

    if (!_data->icon.empty())
        payload += "\"ic\":\"mdi:" + _data->icon + "\",";

    if (_data->topic == "/process/ctrl/cycle_start") { // Special case: cyle_start command
        payload += "\"cmd_t\":\"~" + _data->topic + "\","; // Add command topic
        payload += "\"pl_prs\":\"1\",";
    }
    else {
        payload += "\"stat_t\":\"~" + _data->topic + "\","; // Add status topic
    }

    // Add status topic template for JSON notation or process error binary topic
    if (_data->isTopicJSONNotation)
        payload += "\"val_tpl\":\"{{value_json." + _data->topicName + "}}\",";
    // Signal a problem only if multiple process errors (-2) or process deviation (2) in row occured
    else if (_data->deviceClass == "problem" && _data->topicName == "process_error") // Special binary sensor
        payload += "\"val_tpl\":\"{{ 'ON' if '-2' in value or '2' in value else 'OFF'}}\",";

    payload += "\"qos\":\"1\",";

    if (!_data->unit.empty())
        payload += "\"unit_of_meas\":\"" + _data->unit + "\",";

    if (!_data->deviceClass.empty())
        payload += "\"dev_cla\":\"" + _data->deviceClass + "\",";

    if (!_data->stateClass.empty())
        payload += "\"stat_cla\":\"" + _data->stateClass + "\",";

    if (!_data->entityCategory.empty())
        payload += "\"ent_cat\":\"" + _data->entityCategory + "\",";

    payload +=
        "\"avty_t\":\"~" + std::string(MQTT_STATUS_TOPIC) + "\""; // Use default values for available: "online" / not available: "offline"

    // Publish complete general device info only once
    if (publishHADiscoveryTopicDeviceInfo) {
        std::string firmwareVersion = std::string(libfive_git_version());
        if (firmwareVersion == "" || firmwareVersion == "N/A")
            firmwareVersion = std::string(libfive_git_branch()) + " (" + std::string(libfive_git_revision()) + ")";

        payload += std::string(", \"dev\": {")  +
            "\"ids\":[\"" + nodeID + "\"],"  +
            "\"name\":\"" + nodeID + "\","  +
            "\"mdl\":\"AI-on-the-Edge device [" + getBoardType() + "]\","  +
            "\"mf\":\"AI-on-the-Edge\","  +
            "\"sw\":\"" + firmwareVersion + " [SLFork]\","  +
            "\"cu\":\"http://" + getIPAddress() + "\"}";
    }
    else { // Publish device reference only to group data together
        payload += std::string(", \"dev\": {")  +
            "\"ids\":[\"" + nodeID + "\"]}";
    }

    payload += "}";

    if (MQTTPublish(configurationTopic, payload, _qos, HADiscoveryConfig.HARetainDiscoveryTopics)) {
        publishHADiscoveryTopicDeviceInfo = false;
        return true;
    }
    return false;
}


void register_server_mqtt_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t uri = { };
    uri.method    = HTTP_GET;
    uri.uri       = "/mqtt";
    uri.handler   = handler_mqtt;
    uri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &uri);
}

#endif //ENABLE_MQTT
