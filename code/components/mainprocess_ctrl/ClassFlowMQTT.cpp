#include "ClassFlowMQTT.h"

#ifdef ENABLE_MQTT
#include <sstream>
#include <iomanip>
#include <time.h>

#include <cJSON.h>

#include "MainFlowControl.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "ClassFlowControl.h"
#include "interface_mqtt.h"
#include "server_mqtt.h"


static const char *TAG = "MQTT";


ClassFlowMQTT::ClassFlowMQTT()
{
    presetFlowStateHandler(true);
    keepAlive = 25*60;
}


bool ClassFlowMQTT::loadParameter()
{
    // Create shorter alias to easily access config parameter
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionMqtt;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    if (cfgDataPtr->homeAssistant.meterType == WATER_M3) {
        mqttServer_setMeterType("water", "m³", "h", "m³/h");
    }
    else if (cfgDataPtr->homeAssistant.meterType == WATER_L) {
        mqttServer_setMeterType("water", "L", "h", "L/h");
    }
    else if (cfgDataPtr->homeAssistant.meterType == WATER_FT3) {
        mqttServer_setMeterType("water", "ft³", "m", "ft³/m"); // Minutes
    }
    else if (cfgDataPtr->homeAssistant.meterType == WATER_GAL) {
        mqttServer_setMeterType("water", "gal", "h", "gal/h");
    }
    else if (cfgDataPtr->homeAssistant.meterType == GAS_M3) {
        mqttServer_setMeterType("gas", "m³", "h", "m³/h");
    }
    else if (cfgDataPtr->homeAssistant.meterType == GAS_FT3) {
        mqttServer_setMeterType("gas", "ft³", "m", "ft³/m"); // Minutes
    }
    else if (cfgDataPtr->homeAssistant.meterType == ENERGY_WH) {
        mqttServer_setMeterType("energy", "Wh", "h", "W");
    }
    else if (cfgDataPtr->homeAssistant.meterType == ENERGY_KWH) {
        mqttServer_setMeterType("energy", "kWh", "h", "kW");
    }
    else if (cfgDataPtr->homeAssistant.meterType == ENERGY_MWH) {
        mqttServer_setMeterType("energy", "MWh", "h", "MW");
    }
    else if (cfgDataPtr->homeAssistant.meterType == ENERGY_GJ) {
        mqttServer_setMeterType("energy", "GJ", "h", "GJ/h");
    }
    else {
        mqttServer_setMeterType("", "", "", "");
    }

    return true;
}


bool ClassFlowMQTT::initMqtt(float _processingInterval)
{
    std::stringstream stream;

    mqttServer_setParameter(cfgDataPtr, &sequenceData, _processingInterval);
    mqttServer_schedulePublishDeviceInfo();

    if (cfgDataPtr->homeAssistant.discoveryEnabled)
        mqttServer_schedulePublishHADiscovery();

    keepAlive = _processingInterval * 60 * 2.5; // Seconds, make sure it is greater than 2 processing cycles!

    stream << std::fixed << std::setprecision(1) << "Process interval: " << _processingInterval <<
            "min -> MQTT keepAlive: " << ((float)keepAlive/60) << "min";
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, stream.str());

    if (MQTT_Configure(cfgDataPtr, keepAlive)) {
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
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Skip process state: Not connected to broker");
        setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
        return false;
    }

    // Publish device info / status + HA Discovery
    retValCommon &= mqttServer_publishHADiscovery(MQTT_QOS);
    retValCommon &= mqttServer_publishDeviceInfo(MQTT_QOS);
    retValCommon &= mqttServer_publishDeviceStatus(MQTT_QOS);

    // Publish process status
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Publish process status");
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/process_status",
                                getProcessStatus().c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/process_interval",
                                to_stringWithPrecision(flowctrl.getProcessInterval(), 1).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/process_time",
                                std::to_string(getFlowProcessingTime()).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/process_state",
                                flowctrl.getActualProcessState().c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/process_error",
                                std::to_string(flowctrl.getFlowStateErrorOrDeviation()).c_str(), MQTT_QOS, false);
    retValStatus &= MQTTPublish(cfgDataPtr->mainTopic + "/process/status/cycle_counter",
                                std::to_string(getFlowCycleCounter()).c_str(), MQTT_QOS, false);

    if (!retValStatus)
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Failed to publish process status");

    // Publish process data per sequence
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Publish process data");

    retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/number_sequences",
                              std::to_string(sequenceData.size()), MQTT_QOS, cfgDataPtr->retainProcessData);

    for (const auto &sequence : sequenceData) {
        if (cfgDataPtr->processDataNotation == PROCESSDATA_JSON || cfgDataPtr->processDataNotation == PROCESSDATA_JSON_AND_TOPICS ||
                cfgDataPtr->homeAssistant.discoveryEnabled) {
            cJSON *cJSONObject = cJSON_CreateObject();
            if (cJSONObject == NULL) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
                return false;
            }
            if (cJSON_AddStringToObject(cJSONObject, "sequence_name", sequence->sequenceName.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "actual_value", sequence->sActualValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "fallback_value", sequence->sFallbackValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "raw_value", sequence->sRawValue.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "value_status", sequence->sValueStatus.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "rate_per_minute", sequence->sRatePerMin.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "rate_per_interval", sequence->sRatePerInterval.c_str()) == NULL)
                retValData = false;
            if (cfgDataPtr->homeAssistant.discoveryEnabled) { // Only used for Home Assistant integration
                if (cJSON_AddStringToObject(cJSONObject, "rate_per_time_unit", (mqttServer_getTimeUnit() == "h") ?
                            to_stringWithPrecision(sequence->ratePerMin * 60, sequence->decimalPlaceCount).c_str() :
                                                sequence->sRatePerMin.c_str()) == NULL)
                    retValData = false;
            }
            if (cJSON_AddStringToObject(cJSONObject, "timestamp_processed", sequence->sTimeProcessed.c_str()) == NULL)
                retValData = false;
            if (cJSON_AddStringToObject(cJSONObject, "timestamp_fallbackvalue", sequence->sTimeFallbackValue.c_str()) == NULL)
                retValData = false;

            char *jsonChar = cJSON_PrintBuffered(cJSONObject, 512, 1); // Print to predefined buffer, reduce dynamic allocations
            cJSON_Delete(cJSONObject);

            if (jsonChar != NULL) {
                retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/json",
                                        std::string(jsonChar), MQTT_QOS, cfgDataPtr->retainProcessData);
                cJSON_free(jsonChar);
            }
        }

        if (cfgDataPtr->processDataNotation == PROCESSDATA_TOPICS || cfgDataPtr->processDataNotation == PROCESSDATA_JSON_AND_TOPICS) {
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/sequence_name",
                                        sequence->sequenceName.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/actual_value",
                                        sequence->sActualValue.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/fallback_value",
                                        sequence->sFallbackValue.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/raw_value",
                                        sequence->sRawValue.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/value_status",
                                        sequence->sValueStatus.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/rate_per_minute",
                                        sequence->sRatePerMin.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/rate_per_interval",
                                        sequence->sRatePerInterval.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            if (cfgDataPtr->homeAssistant.discoveryEnabled) { // Only used for Home Assistant integration
                retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/rate_per_time_unit",
                            (mqttServer_getTimeUnit() == "h") ? to_stringWithPrecision(sequence->ratePerMin * 60,
                            sequence->decimalPlaceCount).c_str() : sequence->sRatePerMin.c_str(),
                            MQTT_QOS, cfgDataPtr->retainProcessData);
            }
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/timestamp_processed",
                                        sequence->sTimeProcessed.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
            retValData &= MQTTPublish(cfgDataPtr->mainTopic + "/process/data/" + std::to_string(sequence->sequenceId) + "/timestamp_fallbackvalue",
                                        sequence->sTimeFallbackValue.c_str(), MQTT_QOS, cfgDataPtr->retainProcessData);
        }
    }

    if (!retValData)
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Failed to publish process data");

    if (!retValCommon || !retValStatus || !retValData) { // If publishing of one of the clusters failed
        setFlowStateHandlerEvent(2); // Set warning event code, continue process flow
    }

    if (!getFlowState()->isSuccessful)
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