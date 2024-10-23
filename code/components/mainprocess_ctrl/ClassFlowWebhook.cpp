#include "ClassFlowWebhook.h"

#ifdef ENABLE_WEBHOOK
#include <sstream>
#include <time.h>

#include <esp_log.h>

#include "time_sntp.h"
#include "interface_webhook.h"
#include "ClassFlowPostProcessing.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "connect_wlan.h"


static const char* TAG = "WEBHOOK";


ClassFlowWebhook::ClassFlowWebhook(ClassFlowAlignment *_flowAlignment)
{
    presetFlowStateHandler(true);
    webhookEnable = false;

    flowAlignment = _flowAlignment;
}


bool ClassFlowWebhook::loadParameter()
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionWebhook;

    if (cfgDataPtr == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid config");
        return false;
    }

    // Check URI and database parameter
    if (cfgDataPtr->uri.empty() || cfgDataPtr->apiKey.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Init failed, missing or invalid parameter \'URI\' or \'API Key\'");
        webhookEnable = false;
        return webhookEnable;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init: URI: " + cfgDataPtr->uri + ", API Key: " + cfgDataPtr->apiKey +
         ", PublishImage: " + std::to_string(cfgDataPtr->publishImage) + ", AuthMode: " + std::to_string(cfgDataPtr->authMode));

    webhookEnable = webhookInit(cfgDataPtr);

    return webhookEnable;
}


bool ClassFlowWebhook::doFlow(std::string zwtime)
{
    if (!webhookEnable)
        return true;

    presetFlowStateHandler(false, zwtime);

    bool sequencesWithError = false;
    time_t actTime;
    struct tm timeStruct;
    time_t timeProcessedUtc = 0L;

    cJSON *jsonArray = cJSON_CreateArray();

    // Publish data
    for (const auto &sequence : sequenceData) {
        // Convert timestamp to UTC time
        time(&actTime);
        localtime_r(&actTime, &timeStruct); // Extract DST setting from actual time to consider it for timestamp evaluation
        strptime(sequence->sTimeProcessed.c_str(), TIME_FORMAT_OUTPUT, &timeStruct);
        timeProcessedUtc = mktime(&timeStruct);

        cJSON *jsonObject = cJSON_CreateObject();
        cJSON_AddStringToObject(jsonObject, "time_processed_utc", std::to_string(static_cast<long>(timeProcessedUtc)).c_str());
        cJSON_AddStringToObject(jsonObject, "timestamp_processed", sequence->sTimeProcessed.c_str());
        cJSON_AddStringToObject(jsonObject, "sequence_name", sequence->sequenceName.c_str());
        cJSON_AddStringToObject(jsonObject, "value_status", sequence->sValueStatus.c_str());

        // Skip results if result not even a number
        if (sequence->isActualValueANumber) {
            cJSON_AddStringToObject(jsonObject, "actual_value", sequence->sActualValue.c_str());
            cJSON_AddStringToObject(jsonObject, "fallback_value", sequence->sFallbackValue.c_str());
            cJSON_AddStringToObject(jsonObject, "raw_value", sequence->sRawValue.c_str());
            cJSON_AddStringToObject(jsonObject, "rate_per_min", sequence->sRatePerMin.c_str());
            cJSON_AddStringToObject(jsonObject, "rate_per_interval", sequence->sRatePerInterval.c_str());
        }

        cJSON_AddItemToArray(jsonArray, jsonObject);

        // Check if sequence has rate too high error
        if (cfgDataPtr->publishImage == WEBHOOK_PUBLISH_IMAGE_ON_ERROR_ONLY) {
            if (sequence->sValueStatus.substr(0,3) == std::string(VALUE_STATUS_003_RATE_TOO_HIGH_NEG).substr(0,3) ||
                sequence->sValueStatus.substr(0,3) == std::string(VALUE_STATUS_004_RATE_TOO_HIGH_POS).substr(0,3)) {
                sequencesWithError = true;
            }
        }
    }

    char *jsonChar = cJSON_PrintUnformatted(jsonArray);
    cJSON_Delete(jsonArray);

    if (jsonChar != NULL) {
        if ((cfgDataPtr->publishImage == WEBHOOK_PUBLISH_IMAGE_ENABLED ||
                ((cfgDataPtr->publishImage == WEBHOOK_PUBLISH_IMAGE_ON_ERROR_ONLY) && sequencesWithError))) {
            if (flowAlignment && flowAlignment->AlgROI) {
                if (webhookPublish(jsonChar, flowAlignment->AlgROI, timeProcessedUtc) != ESP_OK) {
                    setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
                }
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to publish image. No image data available (flowAlignment->AlgROI)");
                setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
                webhookPublish(jsonChar); // Fallback, only publish data
            }
        }
        else {
            if (webhookPublish(jsonChar) != ESP_OK) {
                setFlowStateHandlerEvent(1); // Set warning event code, continue process flow
            }
        }
        cJSON_free(jsonChar);
    }

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


void ClassFlowWebhook::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed

}


ClassFlowWebhook::~ClassFlowWebhook()
{
    // nothing to do
}

#endif //ENABLE_WEBHOOK