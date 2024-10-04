#include "ClassFlowInfluxDBv2.h"

#ifdef ENABLE_INFLUXDB
#include <sstream>
#include <time.h>

#include <esp_log.h>

#include "time_sntp.h"
#include "interface_influxdbv2.h"
#include "ClassFlowPostProcessing.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "connect_wlan.h"


static const char* TAG = "INFLUXDBV2";


ClassFlowInfluxDBv2::ClassFlowInfluxDBv2()
{
    presetFlowStateHandler(true);
    flowpostprocessing = NULL;
    InfluxDBenable = false;
}


bool ClassFlowInfluxDBv2::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionInfluxDBv2;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    // Link sequence related influxdb config into sequenceData
    for (auto &sequence : sequenceData) {
        for (const auto &seqCfgData : cfgDataPtr->sequence) {
            if (sequence->sequenceId == seqCfgData.sequenceId) {
                sequence->paramInfluxDBv2 = &seqCfgData;
                break;
            }
        }

        if (sequence->paramInfluxDBv1 == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid sequence config");
            return false;
        }
    }

    // Check URI and database parameter
    if (cfgDataPtr->uri.empty() || cfgDataPtr->bucket.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init failed, missing or invalid parameter \'URI\' or \'Bucket\'");
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

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init: URI: " + cfgDataPtr->uri + ", Bucket: " + cfgDataPtr->bucket +
                            ", AuthMode: " + std::to_string(cfgDataPtr->authMode));

    InfluxDBenable = influxDBv2Init(cfgDataPtr);

    return InfluxDBenable;
}


bool ClassFlowInfluxDBv2::doFlow(std::string zwtime)
{
    if (!InfluxDBenable)
        return true;

    presetFlowStateHandler(false, zwtime);

    for (const auto &sequence : sequenceData) {
        if (!sequence->isActualValueANumber) {
            continue;
        }

        if (ESP_OK != influxDBv2Publish(sequence->paramInfluxDBv2->measurementName, sequence->paramInfluxDBv2->fieldKey1,
                            sequence->sActualValue, sequence->sTimeProcessed)) {
            setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
        }
    }

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


void ClassFlowInfluxDBv2::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed

}


bool ClassFlowInfluxDBv2::isInfluxDBEnabled(void)
{
    return InfluxDBenable;
}


ClassFlowInfluxDBv2::~ClassFlowInfluxDBv2()
{
    // nothing to do
}

#endif //ENABLE_INFLUXDB