#include "ClassFlowMQTT.h"

#ifdef ENABLE_MQTT
#include <sstream>
#include <iomanip>
#include <time.h>

#include "cJSON.h"

#include "MainFlowControl.h"
#include "Helper.h"
#include "ClassLogFile.h"
#include "ClassFlowControll.h"
#include "interface_mqtt.h"
#include "server_mqtt.h"


static const char *TAG = "MQTT";

extern strMqttConfig mqttConfig;
extern strHADiscoveryConfig HADiscoveryConfig;


void ClassFlowMQTT::SetInitialParameter(void)
{
    presetFlowStateHandler(true);
    previousElement = NULL;
    ListFlowControll = NULL;
    disabled = false;

    mqttConfig = {};
    HADiscoveryConfig = {};

    processDataNotation = JSON;
}


ClassFlowMQTT::ClassFlowMQTT(ClassFlowPostProcessing* _flowpostprocessing)
{
    flowpostprocessing = _flowpostprocessing;
    SetInitialParameter();
}


bool ClassFlowMQTT::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (toUpper(aktparamgraph).compare("[MQTT]") != 0)       // Paragraph does not fit MQTT
        return false;

    while (getNextLine(pfile, &aktparamgraph) && !isNewParagraph(aktparamgraph)) {
        splitted = ZerlegeZeile(aktparamgraph);
        if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
            mqttConfig.uri = splitted[1];
        }

        if ((toUpper(splitted[0]) == "MAINTOPIC") && (splitted.size() > 1)) {
            mqttConfig.mainTopic = splitted[1];
             if('/' == mqttConfig.mainTopic.back() || '\\' == mqttConfig.mainTopic.back()) // Remove slash or backslash if existing
                mqttConfig.mainTopic.pop_back();
        }

        if ((toUpper(splitted[0]) == "CLIENTID") && (splitted.size() > 1)) {
            mqttConfig.clientID = splitted[1];
        }

        if ((toUpper(splitted[0]) == "USER") && (splitted.size() > 1)) {
            mqttConfig.user = splitted[1];
        }

        if ((toUpper(splitted[0]) == "PASSWORD") && (splitted.size() > 1)) {
            mqttConfig.password = splitted[1];
        }

        if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                mqttConfig.TLSEncryption = true;
            else
                mqttConfig.TLSEncryption = false;
        }

        if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
            mqttConfig.TLSCACertFilename = "/sdcard" + splitted[1];
        }

        if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
            mqttConfig.TLSClientCertFilename = "/sdcard" + splitted[1];
        }

        if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
            mqttConfig.TLSClientKeyFilename = "/sdcard" + splitted[1];
        }

        if ((toUpper(splitted[0]) == "RETAINPROCESSDATA") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                mqttConfig.retainProcessData = true;
            else
                mqttConfig.retainProcessData = false;
        }

        if ((toUpper(splitted[0]) == "PROCESSDATANOTATION") && (splitted.size() > 1)) {
            processDataNotation = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "HOMEASSISTANTDISCOVERY") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                HADiscoveryConfig.HADiscoveryEnabled = true;
            else
                HADiscoveryConfig.HADiscoveryEnabled = false;
        }

        if ((toUpper(splitted[0]) == "HADISCOVERYPREFIX") && (splitted.size() > 1)) {
            HADiscoveryConfig.HADiscoveryPrefix = splitted[1];
            // Remove traling slash or backslash if existing
            if('/' == HADiscoveryConfig.HADiscoveryPrefix.back() || '\\' == HADiscoveryConfig.HADiscoveryPrefix.back())
                HADiscoveryConfig.HADiscoveryPrefix.pop_back();
        }


        if ((toUpper(splitted[0]) == "HASTATUSTOPIC") && (splitted.size() > 1)) {
            HADiscoveryConfig.HAStatusTopic = splitted[1];
            // Remove traling slash or backslash if existing
            if('/' == HADiscoveryConfig.HAStatusTopic.back() || '\\' == HADiscoveryConfig.HAStatusTopic.back())
                HADiscoveryConfig.HAStatusTopic.pop_back();
        }

        if ((toUpper(splitted[0]) == "HARETAINDISCOVERY") && (splitted.size() > 1)) {
            if (toUpper(splitted[1]) == "TRUE")
                HADiscoveryConfig.HARetainDiscoveryTopics = true;
            else
                HADiscoveryConfig.HARetainDiscoveryTopics = false;
        }

        if ((toUpper(splitted[0]) == "HAMETERTYPE") && (splitted.size() > 1)) {
        /* Use meter type for the device class
           Make sure it is a listed one on https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes */
            if (toUpper(splitted[1]) == "WATER_M3") {
                mqttServer_setMeterType("water", "m³", "h", "m³/h");
            }
            else if (toUpper(splitted[1]) == "WATER_L") {
                mqttServer_setMeterType("water", "L", "h", "L/h");
            }
            else if (toUpper(splitted[1]) == "WATER_FT3") {
                mqttServer_setMeterType("water", "ft³", "m", "ft³/m"); // Minutes
            }
            else if (toUpper(splitted[1]) == "WATER_GAL") {
                mqttServer_setMeterType("water", "gal", "h", "gal/h");
            }
            else if (toUpper(splitted[1]) == "GAS_M3") {
                mqttServer_setMeterType("gas", "m³", "h", "m³/h");
            }
            else if (toUpper(splitted[1]) == "GAS_FT3") {
                mqttServer_setMeterType("gas", "ft³", "m", "ft³/m"); // Minutes
            }
            else if (toUpper(splitted[1]) == "ENERGY_WH") {
                mqttServer_setMeterType("energy", "Wh", "h", "W");
            }
            else if (toUpper(splitted[1]) == "ENERGY_KWH") {
                mqttServer_setMeterType("energy", "kWh", "h", "kW");
            }
            else if (toUpper(splitted[1]) == "ENERGY_MWH") {
                mqttServer_setMeterType("energy", "MWh", "h", "MW");
            }
            else if (toUpper(splitted[1]) == "ENERGY_GJ") {
                mqttServer_setMeterType("energy", "GJ", "h", "GJ/h");
            }
            else {
                mqttServer_setMeterType("", "", "", "");
            }
        }
    }

    /* Note:
     * Originally, MQTT client was initiated here.
     * How ever we need the interval parameter from the ClassFlowControll, but that only gets started later.
     * To work around this, we delay the start and trigger it from ClassFlowControll::ReadParameter() */

    return true;
}


bool ClassFlowMQTT::Start(float _processingInterval)
{
    std::stringstream stream;

    mqttServer_setParameter(flowpostprocessing->GetNumbers(), _processingInterval);
    mqttServer_schedulePublishDeviceInfo();

    if (HADiscoveryConfig.HADiscoveryEnabled)
        mqttServer_schedulePublishHADiscovery();

    mqttConfig.keepAlive = _processingInterval * 60 * 2.5; // Seconds, make sure it is greater than 2 processing cycles!

    stream << std::fixed << std::setprecision(1) << "Process interval: " << _processingInterval <<
            "min -> MQTT keepAlive: " << ((float)mqttConfig.keepAlive/60) << "min";
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, stream.str());

    if (MQTT_Configure()) {
        MQTT_Init();
        return true;
    }

    return false;
}


bool ClassFlowMQTT::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);
    bool retValCommon = true, retValStatus = true, retValData = true;

    if (!getMQTTisConnected()) {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Skip publish, not (yet) connected to broker");
        return true;
    }

    // Publish device info / status + HA Discovery
    retValCommon &= mqttServer_publishHADiscovery(MQTT_QOS);
    retValCommon &= mqttServer_publishDeviceInfo(MQTT_QOS);
    retValCommon &= mqttServer_publishDeviceStatus(MQTT_QOS);

    // Publish process status
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish process status");
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/process_status",
                                getProcessStatus().c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/process_interval",
                                to_stringWithPrecision(flowctrl.getProcessInterval(), 1).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/process_time",
                                std::to_string(getFlowProcessingTime()).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/process_state",
                                flowctrl.getActStatus().c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/process_error",
                                std::to_string(flowctrl.getFlowStateErrorOrDeviation()).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(mqttConfig.mainTopic + "/process/status/cycle_counter",
                                std::to_string(getFlowCycleCounter()).c_str(), MQTT_QOS, false);

    if (!retValStatus)
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Failed to publish process status");

    // Publish process data per sequence
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to retrieve process data");
        return false;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publish process data");

    std::vector<NumberPost*> *numberSequences = flowpostprocessing->GetNumbers();

    retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/number_sequences",
                              std::to_string((*numberSequences).size()), MQTT_QOS, mqttConfig.retainProcessData);

    for (int i = 0; i < (*numberSequences).size(); ++i) {
        if (processDataNotation == JSON || processDataNotation == JSON_AND_TOPICS || HADiscoveryConfig.HADiscoveryEnabled) {
            cJSON *cJSONObject = cJSON_CreateObject();
            if (cJSONObject == NULL) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
                return false;
            }
            if (cJSON_AddStringToObject(cJSONObject, "sequence_name", (*numberSequences)[i]->name.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "actual_value", (*numberSequences)[i]->sActualValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "fallback_value", (*numberSequences)[i]->sFallbackValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "raw_value", (*numberSequences)[i]->sRawValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "value_status", (*numberSequences)[i]->sValueStatus.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "rate_per_minute", (*numberSequences)[i]->sRatePerMin.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "rate_per_interval", (*numberSequences)[i]->sRatePerInterval.c_str()) == NULL)
                retValData = false;
            if (HADiscoveryConfig.HADiscoveryEnabled) { // Only used for Home Assistant integration
                if (cJSON_AddStringToObject(cJSONObject, "rate_per_time_unit", (mqttServer_getTimeUnit() == "h") ?
                            to_stringWithPrecision((*numberSequences)[i]->ratePerMin * 60, (*numberSequences)[i]->decimalPlaceCount).c_str() :
                                                (*numberSequences)[i]->sRatePerMin.c_str()) == NULL)
                    retValData = false;
            }
            if (cJSON_AddStringToObject(cJSONObject, "timestamp_processed", (*numberSequences)[i]->sTimeProcessed.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "timestamp_fallbackvalue", (*numberSequences)[i]->sTimeFallbackValue.c_str()) == NULL)
                retValData = false;

            char *jsonString = cJSON_PrintBuffered(cJSONObject, 512, 1); // Print to predefined buffer, avoid dynamic allocations
            std::string jsonData = std::string(jsonString);
            cJSON_free(jsonString);
            cJSON_Delete(cJSONObject);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/json",
                                      jsonData, MQTT_QOS, mqttConfig.retainProcessData);
        }

        if (processDataNotation == TOPICS || processDataNotation == JSON_AND_TOPICS) {
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/sequence_name",
                                        (*numberSequences)[i]->name.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/actual_value",
                                        (*numberSequences)[i]->sActualValue.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/fallback_value",
                                        (*numberSequences)[i]->sFallbackValue.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/raw_value",
                                        (*numberSequences)[i]->sRawValue.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/value_status",
                                        (*numberSequences)[i]->sValueStatus.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/rate_per_minute",
                                        (*numberSequences)[i]->sRatePerMin.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/rate_per_interval",
                                        (*numberSequences)[i]->sRatePerInterval.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            if (HADiscoveryConfig.HADiscoveryEnabled) { // Only used for Home Assistant integration
                retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/rate_per_time_unit",
                            (mqttServer_getTimeUnit() == "h") ? to_stringWithPrecision((*numberSequences)[i]->ratePerMin * 60,
                            (*numberSequences)[i]->decimalPlaceCount).c_str() : (*numberSequences)[i]->sRatePerMin.c_str(),
                            MQTT_QOS, mqttConfig.retainProcessData);
            }
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/timestamp_processed",
                                        (*numberSequences)[i]->sTimeProcessed.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
            retValData &= MQTTPublish(mqttConfig.mainTopic + "/process/data/" + std::to_string(i+1) + "/timestamp_fallbackvalue",
                                        (*numberSequences)[i]->sTimeFallbackValue.c_str(), MQTT_QOS, mqttConfig.retainProcessData);
        }
    }

    if (!retValData)
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Failed to publish process data");

    if (!retValCommon || !retValStatus || !retValData) // If publishing of one of the clusters failed
        return false;

    return true;
}


void ClassFlowMQTT::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed

}


ClassFlowMQTT::~ClassFlowMQTT()
{
    MQTTdestroy_client(true);
}

#endif //ENABLE_MQTT