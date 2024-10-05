#include "ClassFlowControl.h"
#include "../../include/defines.h"

#include "freertos/task.h"
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include "configClass.h"
#include "connect_wlan.h"
#include "ClassLogFile.h"
#include "time_sntp.h"
#include "helper.h"
#include "system.h"
#include "statusled.h"
#include "server_ota.h"
#include "server_help.h"
#include "MainFlowControl.h"
#include "gpioControl.h"

#ifdef ENABLE_MQTT
    #include "interface_mqtt.h"
    #include "server_mqtt.h"
#endif //ENABLE_MQTT


static const char* TAG = "FLOWCTRL";

//#define DEBUG_DETAIL_ON

std::vector<SequenceData *>ClassFlow::sequenceData = {};


ClassFlowControl::ClassFlowControl()
{
    cfgClassPtr = ConfigClass::getInstance();
    FlowControlImage.clear();
    FlowControlImage.shrink_to_fit();
    FlowControlPublish.clear();
    FlowControlPublish.shrink_to_fit();
    flowtakeimage = NULL;
    flowalignment = NULL;
    flowdigit = NULL;
    flowanalog = NULL;
    flowpostprocessing = NULL;

    #ifdef ENABLE_MQTT
    flowMQTT = NULL;
    #endif //ENABLE_MQTT

    #ifdef ENABLE_INFLUXDB
	flowInfluxDBv1 = NULL;
	flowInfluxDBv2 = NULL;
    #endif //ENABLE_INFLUXDB

    setActualProcessState(std::string(FLOW_NO_TASK));
    flowStateErrorInRow = 0;
    flowStateDeviationInRow = 0;
}


ClassFlowControl::~ClassFlowControl()
{
    deinitFlow();
}


bool ClassFlowControl::loadParameter()
{
    // sectionLog.debug --> Set log level
    // ***************************
    if (cfgClassPtr->get()->sectionLog.debug.logLevel == ESP_LOG_ERROR) {
        LogFile.setLogLevel(ESP_LOG_ERROR);
    }
    else if (cfgClassPtr->get()->sectionLog.debug.logLevel == ESP_LOG_WARN) {
        LogFile.setLogLevel(ESP_LOG_WARN);
    }
    else if (cfgClassPtr->get()->sectionLog.debug.logLevel == ESP_LOG_INFO) {
        LogFile.setLogLevel(ESP_LOG_INFO);
    }
    else if (cfgClassPtr->get()->sectionLog.debug.logLevel == ESP_LOG_DEBUG) {
        LogFile.setLogLevel(ESP_LOG_DEBUG);
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Invalid log level set. Use default log level ERROR");
        LogFile.setLogLevel(ESP_LOG_ERROR);
    }
    // If system reboot was not triggered by user and reboot was caused by execption -> keep log level to DEBUG
    if (!getIsPlannedReboot() && (esp_reset_reason() == ESP_RST_PANIC)) {
        LogFile.setLogLevel(ESP_LOG_DEBUG);
    }

    LogFile.setLogFileRetention(cfgClassPtr->get()->sectionLog.debug.logFilesRetention);
    LogFile.setDebugFilesRetention(cfgClassPtr->get()->sectionLog.debug.debugFilesRetention);


    // sectionLog.data --> Set data logging
    // ***************************
    LogFile.enableDataLogToSD(cfgClassPtr->get()->sectionLog.data.enabled);
    LogFile.setDataLogRetention(cfgClassPtr->get()->sectionLog.data.dataFilesRetention);


    // sectionNetwork.time --> Set time server / time zone
    // ***************************
    reconfigureTime(cfgClassPtr->get()->sectionNetwork.time.ntp.timeSyncEnabled, cfgClassPtr->get()->sectionNetwork.time.ntp.timeServer,
                    cfgClassPtr->get()->sectionNetwork.time.timeZone);

    return true;
}


bool ClassFlowControl::initFlow()
{
    bool retVal = true;
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init flow");

    ConfigClass::getInstance()->reinitConfig();

    // Prepare sequence data struct
    for (const auto &sequenceCfgData : cfgClassPtr->get()->sectionNumberSequences.sequence) {
        SequenceData* sequence = new SequenceData{};
        sequence->sequenceId = sequenceCfgData.sequenceId;
        sequence->sequenceName = sequenceCfgData.sequenceName;
        sequenceData.push_back(sequence);
    }

    flowtakeimage = new ClassFlowTakeImage();
    FlowControlImage.push_back(flowtakeimage);
    if (!flowtakeimage->loadParameter()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Take Image: Init failed");
        retVal = false;
    }

    flowalignment = new ClassFlowAlignment();
    FlowControlImage.push_back(flowalignment);
    if (!flowalignment->loadParameter()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Image Alignment: Init failed");
        retVal = false;
    }

    if (cfgClassPtr->get()->sectionDigit.enabled) {
        flowdigit = new ClassFlowCNNGeneral(flowalignment, "Digit");
        FlowControlImage.push_back(flowdigit);
        if (!flowdigit->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CNN Digit: Init failed");
            retVal = false;
        }
    }

    if (cfgClassPtr->get()->sectionAnalog.enabled) {
        flowanalog = new ClassFlowCNNGeneral(flowalignment, "Analog");
        FlowControlImage.push_back(flowanalog);
        if (!flowanalog->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "CNN Analog: Init failed");
            retVal = false;
        }
    }

    if (cfgClassPtr->get()->sectionPostProcessing.enabled) {
        flowpostprocessing = new ClassFlowPostProcessing(flowtakeimage, flowdigit, flowanalog);
        FlowControlImage.push_back(flowpostprocessing);
        if (!flowpostprocessing->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Post-Processing: Init failed");
            retVal = false;
        }
    }

#ifdef ENABLE_MQTT
    if (cfgClassPtr->get()->sectionMqtt.enabled) {
        flowMQTT = new ClassFlowMQTT();
        FlowControlPublish.push_back(flowMQTT);
        if (!flowMQTT->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "MQTT: Init failed");
            retVal = false;
        }
    }
#endif //ENABLE_MQTT

#ifdef ENABLE_INFLUXDB
    if (cfgClassPtr->get()->sectionInfluxDBv1.enabled) {
        flowInfluxDBv1 = new ClassFlowInfluxDBv1();
        FlowControlPublish.push_back(flowInfluxDBv1);
        if (!flowInfluxDBv1->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv1: Init failed");
            retVal = false;
        }
    }

    if (cfgClassPtr->get()->sectionInfluxDBv2.enabled) {
        flowInfluxDBv2 = new ClassFlowInfluxDBv2();
        FlowControlPublish.push_back(flowInfluxDBv2);
        if (!flowInfluxDBv2->loadParameter()) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "InfluxDBv2: Init failed");
            retVal = false;
        }
    }
#endif //ENABLE_INFLUXDB

    // Load parameter handled in this class
    this->loadParameter();

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Init flow completed");
    LogFile.writeHeapInfo("initFlow done");

    return retVal;
}


void ClassFlowControl::deinitFlow(void)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Deinit flow");
    //LogFile.writeHeapInfo("deinitFlow start");

    #ifdef ENABLE_INFLUXDB
    delete flowInfluxDBv2;
    flowInfluxDBv2 = NULL;
    //LogFile.writeHeapInfo("After INFLUXv2");

    delete flowInfluxDBv1;
    flowInfluxDBv1 = NULL;
    //LogFile.writeHeapInfo("After INFLUX");
    #endif //ENABLE_INFLUXDB

    #ifdef ENABLE_MQTT
	delete flowMQTT;
    flowMQTT = NULL;
    //LogFile.writeHeapInfo("After MQTT");
    #endif //ENABLE_MQTT

    delete flowpostprocessing;
    flowpostprocessing = NULL;
    //LogFile.writeHeapInfo("After POSTPROC");

    delete flowanalog;
    flowanalog = NULL;
    //LogFile.writeHeapInfo("After ANALOG");

    delete flowdigit;
    flowdigit = NULL;
    //LogFile.writeHeapInfo("After DIGIT");

    delete flowalignment;
    flowalignment = NULL;
    //LogFile.writeHeapInfo("After ALIGN");

    delete flowtakeimage;
    flowtakeimage = NULL;
    //LogFile.writeHeapInfo("After TAKEIMG");

    cameraCtrl.freeMemoryOnly(); // Free user allocated memory, but no cam driver deinit
    cameraCtrl.setFlashlight(false);
    setStatusLedOff();
    //LogFile.writeHeapInfo("After camera");

    FlowControlImage.clear();
    FlowControlPublish.clear();

    FlowStateEvaluationEvent.clear();
    FlowStateEvaluationEvent.shrink_to_fit();
    FlowStatePublishEvent.clear();
    FlowStatePublishEvent.shrink_to_fit();

    for (auto &sequence : sequenceData) {
        sequence->digitRoi.clear();
        std::vector<RoiData *>().swap(sequence->digitRoi); // Ensure that memory gets freed (instead shrink_to_fit())
        sequence->analogRoi.clear();
        std::vector<RoiData *>().swap(sequence->analogRoi); // Ensure that memory gets freed (instead shrink_to_fit())
    }
    sequenceData.clear();
    std::vector<SequenceData *>().swap(sequenceData); // Ensure that memory gets freed (instead shrink_to_fit())

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Deinit flow completed");
    LogFile.writeHeapInfo("deinitFlow completed");
}


bool ClassFlowControl::doFlowImageEvaluation(std::string time)
{
    bool result = true;
    FlowStateEvaluationEvent.clear();
    FlowStateEvaluationEvent.shrink_to_fit();

    for (int i = 0; i < FlowControlImage.size(); ++i) {
        #ifdef DEBUG_DETAIL_ON
            LogFile.writeHeapInfo("ClassFlowControl::doFlow: " + FlowControlImage[i]->name());
        #endif

        setActualProcessState(translateActualProcessState(FlowControlImage[i]->name()));
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Process state: " + getActualProcessState());
        #ifdef ENABLE_MQTT
            MQTTPublish(mqttServer_getMainTopic() + "/process/status/process_state", getActualProcessState(), 1, false);
        #endif //ENABLE_MQTT

        if (!FlowControlImage[i]->doFlow(time)) {
            FlowStateEvaluationEvent.push_back(FlowControlImage[i]->getFlowState());

            for (int j = 0; j < FlowControlImage[i]->getFlowState()->EventCode.size(); j++) {
                if (FlowControlImage[i]->getFlowState()->EventCode[j] < 0) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Process error in state: \"" + getActualProcessState() + "\"");
                    result = false;
                    flowStateErrorInRow++;
                    flowStateDeviationInRow = 0;
                    break;
                }
                else if (FlowControlImage[i]->getFlowState()->EventCode[j] > 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Process deviation in state: \"" + getActualProcessState() + "\"");
                    flowStateDeviationInRow++;
                    flowStateErrorInRow = 0;
                }
            }

            if (!result) { // If an error occured, stop processing of further tasks
                break;
            }
        }
    }
    return result;
}


bool ClassFlowControl::doFlowPublishData(std::string time)
{
    bool result = true;
    FlowStatePublishEvent.clear();
    FlowStatePublishEvent.shrink_to_fit();

    for (int i = 0; i < FlowControlPublish.size(); ++i) {
        #ifdef DEBUG_DETAIL_ON
            LogFile.writeHeapInfo("ClassFlowControl::doFlow: " + FlowControlPublish[i]->name());
        #endif

        setActualProcessState(translateActualProcessState(FlowControlPublish[i]->name()));
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Process state: " + getActualProcessState());
        #ifdef ENABLE_MQTT
            MQTTPublish(mqttServer_getMainTopic() + "/process/status/process_state", getActualProcessState(), 1, false);
        #endif //ENABLE_MQTT

        if (!FlowControlPublish[i]->doFlow(time)) {
            FlowStatePublishEvent.push_back(FlowControlPublish[i]->getFlowState());

            for (int j = 0; j < FlowControlPublish[i]->getFlowState()->EventCode.size(); j++) {
                if (FlowControlPublish[i]->getFlowState()->EventCode[j] < 0) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Process error in state: \"" + getActualProcessState() + "\"");
                    result = false;
                    flowStateErrorInRow++;
                    flowStateDeviationInRow = 0;
                    break;
                }
                else if (FlowControlPublish[i]->getFlowState()->EventCode[j] > 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Process deviation in state: \"" + getActualProcessState() + "\"");
                    flowStateDeviationInRow++;
                    flowStateErrorInRow = 0;
                }
            }

            if (!result) { // If an error occured, stop processing of further tasks
                break;
            }
        }
    }
    return result;
}


void ClassFlowControl::setFlowStateError()
{
    flowStateErrorInRow++;
    flowStateDeviationInRow = 0;
}


void ClassFlowControl::clearFlowStateEventInRowCounter()
{
    flowStateErrorInRow = 0;
    flowStateDeviationInRow = 0;
}


int ClassFlowControl::getFlowStateErrorOrDeviation()
{
    if (flowStateErrorInRow >= FLOWSTATE_ERROR_DEVIATION_IN_ROW_LIMIT) {
        return MULTIPLE_ERROR_IN_ROW;
    }
    else if (flowStateDeviationInRow >= FLOWSTATE_ERROR_DEVIATION_IN_ROW_LIMIT) {
        return MULTIPLE_DEVIATION_IN_ROW;
    }
    else if (flowStateErrorInRow > 0) {
        return SINGLE_ERROR;
    }
    else if (flowStateDeviationInRow > 0) {
        return SINGLE_DEVIATION;
    }
    else {
        return NONE;
    }
}


bool ClassFlowControl::flowStateEventOccured()
{
    if (FlowStateEvaluationEvent.size() != 0 || FlowStatePublishEvent.size() != 0) {
        return true;
    }
    else {
        return false;
    }
}


void ClassFlowControl::postProcessEventHandler()
{
    for (int i = 0; i < FlowStateEvaluationEvent.size(); ++i) {
        for (int j = 0; j < FlowControlImage.size(); ++j) {
            if (FlowStateEvaluationEvent[i]->ClassName.compare(FlowControlImage[j]->name()) == 0) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, FlowStateEvaluationEvent[i]->ClassName + "-> doPostProcessEventHandling");
                FlowControlImage[j]->doPostProcessEventHandling();
                FlowControlImage[j]->presetFlowStateHandler(true); // Reinit after processing
            }
        }
    }
    // Reset of errors will be peformed before next flow starts --> functions doFlowImageEvaluation
    // FlowStateEvaluationEvent.clear();

    for (int i = 0; i < FlowStatePublishEvent.size(); ++i) {
        for (int j = 0; j < FlowControlPublish.size(); ++j) {
            if (FlowStatePublishEvent[i]->ClassName.compare(FlowControlPublish[j]->name()) == 0) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, FlowStatePublishEvent[i]->ClassName + "-> doPostProcessEventHandling");
                FlowControlPublish[j]->doPostProcessEventHandling();
                FlowControlPublish[j]->presetFlowStateHandler(true); // Reinit after processing
            }
        }
    }
    // Reset of errors will be peformed before next flow starts --> function doFlowPublishData
    // FlowStatePublishEvent.clear();
}


bool ClassFlowControl::getStatusSetupModus()
 {
    if (cfgClassPtr->get()->sectionOperationMode.opMode == OPMODE_SETUP) {
        return true;
    }

    return false;
 }


float ClassFlowControl::getProcessInterval(void)
{
    return cfgClassPtr->get()->sectionOperationMode.automaticProcessInterval;
}


bool ClassFlowControl::isAutoStart()
{
    if (cfgClassPtr->get()->sectionOperationMode.opMode == OPMODE_AUTO) {
        return true;
    }

    return false;
}


bool ClassFlowControl::isAutoStart(long &_interval)
{
    _interval = cfgClassPtr->get()->sectionOperationMode.automaticProcessInterval * 60 * 1000; // minutes -> ms

    if (cfgClassPtr->get()->sectionOperationMode.opMode == OPMODE_AUTO) {
        return true;
    }

    return false;
}


void ClassFlowControl::setActualProcessState(std::string _actualProcessState)
{
    actualProcessState = _actualProcessState;
    actualProcessStateWithTime = "[" + getCurrentTimeString("%H:%M:%S") + "] " + _actualProcessState;
}


std::string ClassFlowControl::getActualProcessStateWithTime()
{
    return actualProcessStateWithTime;
}


std::string ClassFlowControl::getActualProcessState()
{
    return actualProcessState;
}


std::string ClassFlowControl::translateActualProcessState(std::string classname)
{
    if (classname.compare("ClassFlowTakeImage") == 0) {
        return std::string(FLOW_TAKE_IMAGE);
    }
    else if (classname.compare("ClassFlowAlignment") == 0) {
        return std::string(FLOW_ALIGNMENT);
    }
    else if (classname.compare("ClassFlowCNNGeneral - Digit") == 0) {
        return std::string(FLOW_PROCESS_DIGIT_ROI);
    }
    else if (classname.compare("ClassFlowCNNGeneral - Analog") == 0) {
        return std::string(FLOW_PROCESS_ANALOG_ROI);
    }
    else if (classname.compare("ClassFlowPostProcessing") == 0) {
        return std::string(FLOW_POSTPROCESSING);
    }
#ifdef ENABLE_MQTT
    else if (classname.compare("ClassFlowMQTT") == 0) {
        return std::string(FLOW_PUBLISH_MQTT);
    }
#endif //ENABLE_MQTT
#ifdef ENABLE_INFLUXDB
    else if (classname.compare("ClassFlowInfluxDBv1") == 0) {
        return std::string(FLOW_PUBLISH_INFLUXDB);
    }
    else if (classname.compare("ClassFlowInfluxDBv2") == 0) {
        return std::string(FLOW_PUBLISH_INFLUXDB2);
    }
#endif //ENABLE_INFLUXDB
    else {
        return "Unkown State (" + classname + ")";
    }
}


void ClassFlowControl::drawDigitRoi(CImageBasis *image)
{
    if (flowdigit) {
        flowdigit->drawROI(image);
    }
}


void ClassFlowControl::drawAnalogRoi(CImageBasis *image)
{
    if (flowanalog) {
        flowanalog->drawROI(image);
    }
}


#ifdef ENABLE_MQTT
bool ClassFlowControl::initMqttService()
{
    if (flowMQTT == NULL) { // Service disabled
        return true;
    }

    return flowMQTT->initMqtt(cfgClassPtr->get()->sectionOperationMode.automaticProcessInterval);
}
#endif //ENABLE_MQTT


// Return values for all number sequences and a given value type
// Providing any sequence name will filter result only for mentioned sequence
std::string ClassFlowControl::getSequenceResultInline(int type, std::string sequenceName)
{
    std::string out = "";

    for (int i = 0; const auto &sequence : sequenceData) {
        if (!sequenceName.empty() && sequence->sequenceName != sequenceName) {
            continue;
        }

        out += sequence->sequenceName + "\t";
        switch (type) {
            case READOUT_TYPE_TIMESTAMP_PROCESSED:
                out += sequence->sTimeProcessed;
                break;
            case READOUT_TYPE_TIMESTAMP_FALLBACKVALUE:
                out += sequence->sTimeFallbackValue;
                break;
            case READOUT_TYPE_VALUE:
                out += sequence->sActualValue;
                break;
            case READOUT_TYPE_FALLBACKVALUE:
                out += sequence->sFallbackValue;
                break;
            case READOUT_TYPE_RAWVALUE:
                out += sequence->sRawValue;
                break;
            case READOUT_TYPE_VALUE_STATUS:
                out += sequence->sValueStatus;
                break;
            case READOUT_TYPE_RATE_PER_MIN:
                out += sequence->sRatePerMin;
                break;
            case READOUT_TYPE_RATE_PER_INTERVAL:
                out += sequence->sRatePerInterval;
                break;
        }

        if (sequenceName.empty() && i < sequenceData.size() - 1) {
            out += "\r\n";
        }

        i++;
    }

    return out;
}


std::string ClassFlowControl::getFallbackValue(std::string _sequenceName)
{
    if (flowpostprocessing) {
        return flowpostprocessing->getFallbackValue(_sequenceName);
    }

    return std::string("");
}


bool ClassFlowControl::setFallbackValue(std::string _sequenceName, std::string _newvalue)
{
    double newValueAsDouble;
    char* p;

    _newvalue = trim(_newvalue);
    //ESP_LOGD(TAG, "Input setFallbackValue: %s", _newvalue.c_str());

    if (_newvalue.substr(0,8).compare("0.000000") == 0 || _newvalue.compare("0.0") == 0 || _newvalue.compare("0") == 0) {
        newValueAsDouble = 0;   // preset to value = 0
    }
    else {
        newValueAsDouble = strtod(_newvalue.c_str(), &p);
        if (newValueAsDouble == 0) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "setFallbackValue: No valid value for processing: " + _newvalue);
            return false;
        }
    }

    if (flowpostprocessing) {
        if (flowpostprocessing->setFallbackValue(newValueAsDouble, _sequenceName)) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setFallbackValue: ERROR - Class Post-Processing not initialized");
        return false;
    }
}


CImageBasis* ClassFlowControl::getRawImage()
{
    if (flowtakeimage) {
        return flowtakeimage->rawImage;
    }

    return NULL;
}


esp_err_t ClassFlowControl::sendRawJPG(httpd_req_t *req)
{
    if (flowtakeimage) {
        return flowtakeimage->sendRawJPG(req);
    }
    else {
        return cameraCtrl.captureToHTTP(req);
    }
}


esp_err_t ClassFlowControl::getJPGStream(std::string _fn, httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("ClassFlowControl::getJPGStream - Start");
    #endif

    CImageBasis *_send = NULL;
    esp_err_t result = ESP_FAIL;
    bool _sendDelete = false;

    if (_fn == "alg_roi.jpg") {
        if (getTaskAutoFlowState() == FLOW_TASK_STATE_INIT_DELAYED) {
            FILE* file = fopen("/sdcard/html/flowstate_initialization_delayed.jpg", "rb");

            if (!file) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_initialization_delayed.jpg not found");
                return ESP_FAIL;
            }

            /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
            // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
            setvbuf(file, NULL, _IOFBF, 512);

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            unsigned char* fileBuffer = (unsigned char*) malloc(fileSize);

            if (!fileBuffer) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize);
            free(fileBuffer);
        }
        else if (getTaskAutoFlowState() == FLOW_TASK_STATE_INIT) {
            FILE* file = fopen("/sdcard/html/flowstate_initialization.jpg", "rb");

            if (!file) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_initialization.jpg not found");
                return ESP_FAIL;
            }

            /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
            // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
            setvbuf(file, NULL, _IOFBF, 512);

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            unsigned char* fileBuffer = (unsigned char*) malloc(fileSize);

            if (!fileBuffer) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize);
            free(fileBuffer);
        }
        else if (getTaskAutoFlowState() == FLOW_TASK_STATE_SETUPMODE) {
            FILE* file = fopen("/sdcard/html/flowstate_setup_mode.jpg", "rb");

            if (!file) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_setup_mode.jpg not found");
                return ESP_FAIL;
            }

            /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
            // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
            setvbuf(file, NULL, _IOFBF, 512);

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            unsigned char* fileBuffer = (unsigned char*) malloc(fileSize);

            if (!fileBuffer) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize);
            free(fileBuffer);
        }
        // Show only before first cycle started or error occured, otherwise result will be shown till next start
        else if ((getActualProcessState() == std::string(FLOW_IDLE_NO_AUTOSTART) && (flowtakeimage != NULL) && !flowtakeimage->getFlowState()->getExecuted) ||
                    (getActualProcessState() == std::string(FLOW_TAKE_IMAGE) && !isAutoStart() && flowStateEventOccured())) {
            FILE* file = fopen("/sdcard/html/flowstate_idle_no_autostart.jpg", "rb");

            if (!file) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_idle_no_autostart.jpg not found");
                return ESP_FAIL;
            }

            /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
            // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
            setvbuf(file, NULL, _IOFBF, 512);

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            unsigned char* fileBuffer = (unsigned char*) malloc(fileSize);

            if (!fileBuffer) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize);
            free(fileBuffer);
        }
        else if (getActualProcessState() == std::string(FLOW_TAKE_IMAGE)) {
            if (flowalignment && flowalignment->AlgROI) {
                FILE* file = fopen("/sdcard/html/flowstate_take_image.jpg", "rb");

                if (!file) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_take_image.jpg not found");
                    return ESP_FAIL;
                }

                /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
                // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
                setvbuf(file, NULL, _IOFBF, 512);

                fseek(file, 0, SEEK_END);
                flowalignment->AlgROI->size = ftell(file);
                fseek(file, 0, SEEK_SET);

                if (flowalignment->AlgROI->size > MAX_JPG_SIZE) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_take_image.jpg (" + std::to_string(flowalignment->AlgROI->size) +
                                                            ") > allocated buffer (" + std::to_string(MAX_JPG_SIZE) + ")");
                    fclose(file);
                    return ESP_FAIL;
                }

                fread(flowalignment->AlgROI->data, flowalignment->AlgROI->size, 1, file);
                fclose(file);

                httpd_resp_set_type(req, "image/jpeg");
                result = httpd_resp_send(req, (const char *)flowalignment->AlgROI->data, flowalignment->AlgROI->size);
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: alg_roi.jpg cannot be served -> alg.jpg is going to be served");
                if (flowalignment && flowalignment->ImageBasis->imageOkay()) {
                    _send = flowalignment->ImageBasis;
                }
                else {
                    httpd_resp_send(req, NULL, 0);
                    return ESP_OK;
                }
            }
        }
        else if (getActualProcessState() == std::string(FLOW_TAKE_IMAGE) && isAutoStart() && flowStateEventOccured()) {
            FILE* file = fopen("/sdcard/html/flowstate_idle_autostart.jpg", "rb");

            if (!file) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/flowstate_idle_autostart.jpg not found");
                return ESP_FAIL;
            }

            /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
            // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
            setvbuf(file, NULL, _IOFBF, 512);

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file); /* how long is the file ? */
            fseek(file, 0, SEEK_SET); /* reset */

            unsigned char* fileBuffer = (unsigned char*) malloc(fileSize);

            if (!fileBuffer) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize);
            free(fileBuffer);
        }
        else { // Show actual image taken for all other states
            if (flowalignment && flowalignment->AlgROI) {
                httpd_resp_set_type(req, "image/jpeg");
                result = httpd_resp_send(req, (const char *)flowalignment->AlgROI->data, flowalignment->AlgROI->size);
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: alg_roi.jpg cannot be served -> alg.jpg is going to be served");
                if (flowalignment && flowalignment->ImageBasis->imageOkay()) {
                    _send = flowalignment->ImageBasis;
                }
                else {
                    httpd_resp_send(req, NULL, 0);
                    return ESP_OK;
                }
            }
        }
    }
    else if (_fn == "alg.jpg") {
        if (flowalignment && flowalignment->ImageBasis->imageOkay()) {
            _send = flowalignment->ImageBasis;
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "ClassFlowControl::getJPGStream: alg.jpg cannot be served");
            return ESP_FAIL;
        }
    }
    else {
        for (const auto &sequence : sequenceData) {
            for (const auto &roi : sequence->digitRoi) {
                if (roi->param->roiName + ".jpg" == _fn) {
                    _send = roi->imageRoiResized;
                    break;
                }

                if (roi->param->roiName + "_org.jpg" == _fn) {
                    _send = roi->imageRoi;
                    break;
                }
            }
            if (_send)
                break;

            for (const auto &roi : sequence->analogRoi) {
                if (roi->param->roiName + ".jpg" == _fn) {
                    _send = roi->imageRoiResized;
                    break;
                }

                if (roi->param->roiName + "_org.jpg" == _fn) {
                    _send = roi->imageRoi;
                    break;
                }
            }
            if (_send)
                break;
        }
    }

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("ClassFlowControl::getJPGStream - before send");
    #endif

    if (_send) {
        setContentTypeFromFile(req, _fn.c_str());
        result = _send->sendJPGtoHTTP(req);
        httpd_resp_send_chunk(req, NULL, 0); // Respond with an empty chunk to signal HTTP response completion

        if (_sendDelete) {
            delete _send;
        }

        _send = NULL;
    }

    #ifdef DEBUG_DETAIL_ON
        LogFile.writeHeapInfo("ClassFlowControl::getJPGStream - done");
    #endif

    return result;
}

