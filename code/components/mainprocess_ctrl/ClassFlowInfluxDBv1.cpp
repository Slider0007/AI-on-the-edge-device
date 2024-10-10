#include "ClassFlowInfluxDBv1.h"

#ifdef ENABLE_INFLUXDB
#include <sstream>
#include <time.h>

#include <esp_log.h>

#include "time_sntp.h"
#include "interface_influxdbv1.h"
#include "ClassFlowPostProcessing.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "connect_wlan.h"


static const char* TAG = "INFLUXDBV1";


ClassFlowInfluxDBv1::ClassFlowInfluxDBv1()
{
    presetFlowStateHandler(true);
    flowpostprocessing = NULL;
    InfluxDBenable = false;
}


bool ClassFlowInfluxDBv1::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionInfluxDBv1;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    // Link sequence related influxdb config into sequenceData
    for (auto &sequence : sequenceData) {
        for (const auto &seqCfgData : cfgDataPtr->sequence) {
            if (sequence->sequenceId == seqCfgData.sequenceId) {
                sequence->paramInfluxDBv1 = &seqCfgData;
                break;
            }
        }

        if (sequence->paramInfluxDBv1 == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid sequence config");
            return false;
        }
    }

    // Check URI and database parameter
    if (cfgDataPtr->uri.empty() || cfgDataPtr->database.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init failed, missing or invalid parameter \'URI\' or \'Database\'");
        InfluxDBenable = false;
        return InfluxDBenable;
    }

    // Check measurementname and fieldkey
    for (const auto &sequence : cfgDataPtr->sequence) {
        if (sequence.measurementName.empty() || sequence.fieldKey1.empty()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init failed, missing or invalid parameter \'Measurement Name\' or \'Field Key\' for sequence: " +
                                                        sequence.sequenceName);
            InfluxDBenable = false;
            return InfluxDBenable;
        }
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init: URI: " + cfgDataPtr->uri + ", Database: " + cfgDataPtr->database +
                        ", AuthMode: " + std::to_string(cfgDataPtr->authMode));

    InfluxDBenable = influxDBv1Init(cfgDataPtr);

    return InfluxDBenable;
}


bool ClassFlowInfluxDBv1::doFlow(std::string zwtime)
{
    if (!InfluxDBenable)
        return true;

    presetFlowStateHandler(false, zwtime);

    for (const auto &sequence : sequenceData) {
        if (!sequence->isActualValueANumber) {
            continue;
        }

        if (ESP_OK != influxDBv1Publish(sequence->paramInfluxDBv1->measurementName, sequence->paramInfluxDBv1->fieldKey1,
                            sequence->sActualValue, sequence->sTimeProcessed)) {
            setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
        }
    }

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


void ClassFlowInfluxDBv1::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed

}


bool ClassFlowInfluxDBv1::isInfluxDBEnabled(void)
{
    return InfluxDBenable;
}


ClassFlowInfluxDBv1::~ClassFlowInfluxDBv1()
{
    // nothing to do
}

#endif //ENABLE_INFLUXDB