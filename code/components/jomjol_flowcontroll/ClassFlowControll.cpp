#include "ClassFlowControll.h"

#include "connect_wlan.h"
#include "read_wlanini.h"

#include "freertos/task.h"

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include "ClassLogFile.h"
#include "time_sntp.h"
#include "Helper.h"
#include "statusled.h"
#include "server_ota.h"
#ifdef ENABLE_MQTT
    #include "interface_mqtt.h"
    #include "server_mqtt.h"
#endif //ENABLE_MQTT

#include "server_help.h"
#include "MainFlowControl.h"
#include "server_GPIO.h"
#include "../../include/defines.h"

static const char* TAG = "FLOWCTRL";

//#define DEBUG_DETAIL_ON


void ClassFlowControll::SetInitialParameter(void)
{
    FlowControll.clear();
    FlowControlPublish.clear();
    flowtakeimage = NULL;
    flowalignment = NULL;
    flowdigit = NULL;
    flowanalog = NULL;
    flowpostprocessing = NULL;

    #ifdef ENABLE_MQTT
    flowMQTT = NULL;
    #endif //ENABLE_MQTT

    #ifdef ENABLE_INFLUXDB
	flowInfluxDB = NULL;
	flowInfluxDBv2 = NULL;
    #endif //ENABLE_INFLUXDB
    
    AutoStart = false;
    AutoInterval = 5; // in Minutes
    SetupModeActive = false;
    disabled = false;
    readParameterDone = false;
    setActStatus(std::string(FLOW_NO_TASK));
    setActFlowError(false);
}


ClassFlowControll::ClassFlowControll()
{
    SetInitialParameter();
}


ClassFlowControll::~ClassFlowControll()
{
    DeinitFlow();
}


bool ClassFlowControll::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;


    if ((toUpper(aktparamgraph).compare("[AUTOTIMER]") != 0) && (toUpper(aktparamgraph).compare("[DEBUG]") != 0) &&
        (toUpper(aktparamgraph).compare("[SYSTEM]") != 0 && (toUpper(aktparamgraph).compare("[DATALOGGING]") != 0)))      // Paragraph passt nicht
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        splitted = ZerlegeZeile(aktparamgraph, " =");

        if ((toUpper(splitted[0]) == "AUTOSTART") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                AutoStart = true;
            else
                AutoStart = false;
        }

        if ((toUpper(splitted[0]) == "INTERVAL") && (splitted.size() > 1))
        {
            AutoInterval = std::stof(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "DATALOGACTIVE") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                LogFile.SetDataLogToSD(true);
            else
                LogFile.SetDataLogToSD(false);
        }

        if ((toUpper(splitted[0]) == "DATAFILESRETENTION") && (splitted.size() > 1))
        {
            LogFile.SetDataLogRetention(std::stoi(splitted[1]));
        }

        if ((toUpper(splitted[0]) == "LOGLEVEL") && (splitted.size() > 1))
        {
            /* matches esp_log_level_t */
            if ((toUpper(splitted[1]) == "TRUE") || (toUpper(splitted[1]) == "2"))
            {
                LogFile.setLogLevel(ESP_LOG_WARN);
            }
            else if ((toUpper(splitted[1]) == "FALSE") || (toUpper(splitted[1]) == "0") || (toUpper(splitted[1]) == "1"))
            {
                LogFile.setLogLevel(ESP_LOG_ERROR);
            }
            else if (toUpper(splitted[1]) == "3")
            {
                LogFile.setLogLevel(ESP_LOG_INFO);
            }
            else if (toUpper(splitted[1]) == "4")
            {
                LogFile.setLogLevel(ESP_LOG_DEBUG);
            }
            else 
            {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Invalid log level set. Use default log level ERROR");
                LogFile.setLogLevel(ESP_LOG_ERROR);
            }

            /* If system reboot was not triggered by user and reboot was caused by execption -> keep log level to DEBUG */
            if (!getIsPlannedReboot() && (esp_reset_reason() == ESP_RST_PANIC))
                LogFile.setLogLevel(ESP_LOG_DEBUG);
        }
        
        if ((toUpper(splitted[0]) == "LOGFILESRETENTION") && (splitted.size() > 1))
        {
            LogFile.SetLogFileRetention(std::stoi(splitted[1]));
        }

        if ((toUpper(splitted[0]) == "DEBUGFILESRETENTION") && (splitted.size() > 1))
        {
            LogFile.SetDebugFilesRetention(std::stoi(splitted[1]));
        }

        // Initial timeserver setup was already done during boot: see main.cpp -> setupTime()
        // Check timeserver here anyway due to parameter reloading without reboot
        if ((toUpper(splitted[0]) == "TIMESERVER") || (toUpper(splitted[0]) == ";TIMESERVER"))
        {
            std::string _timeServer = "";

            if (toUpper(splitted[0]) == ";TIMESERVER") { // parameter disabled
                _timeServer = ""; // Disable NTP
            }
            else if (splitted.size() <= 1) { // parameter part is empty
                _timeServer = "pool.ntp.org"; // Use Default
            }
            else {
                _timeServer = splitted[1];
            }

            setupTimeServer(_timeServer);
        }

        // Initial timezone setup was already done during boot: see main.cpp -> setupTime()
        // Check timezone here anyway due to parameter reloading without reboot
        if (toUpper(splitted[0]) == "TIMEZONE")
        {
            std::string _timeZone = "";
            if (splitted.size() <= 1) { // parameter part is empty
                _timeZone = ""; // Use Default
            }
            else {
                _timeZone = splitted[1];
            }
            setupTimeZone(_timeZone);
        }

        if ((toUpper(splitted[0]) == "HOSTNAME") && (splitted.size() > 1))
        {
            if (ChangeHostName(WLAN_CONFIG_FILE, splitted[1]))
            {
                // reboot necessary so that the new wlan.ini is also used !!!
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Please reboot to activate new hostname");
            }
        }
   
        #if (defined WLAN_USE_ROAMING_BY_SCANNING || (defined WLAN_USE_MESH_ROAMING && defined WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES))
        if ((toUpper(splitted[0]) == "RSSITHRESHOLD") || (toUpper(splitted[0]) == ";RSSITHRESHOLD"))
        {
            int _RSSIThresholdTMP;
            if ((toUpper(splitted[0]) == ";RSSITHRESHOLD") || (splitted.size() <= 1)) { // parameter disabled or parameter empty
                _RSSIThresholdTMP = 0;   // Disable function
            }
            else {
                _RSSIThresholdTMP = atoi(splitted[1].c_str());
                _RSSIThresholdTMP = std::min(0, std::max(-100, _RSSIThresholdTMP)); // Verify input limits (-100 - 0)
            }
            ChangeRSSIThreshold(WLAN_CONFIG_FILE, _RSSIThresholdTMP);
        }
        #endif

        if ((toUpper(splitted[0]) == "SETUPMODE") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "FALSE") {
                SetupModeActive = false;
            }
            else if (toUpper(splitted[1]) == "TRUE") {
                SetupModeActive = true;
            }
        }
    }

    readParameterDone = true;
    return true;
}


std::string ClassFlowControll::doSingleStep(std::string _stepname, std::string _host){
    std::string _classname = "";
    std::string result = "";

    ESP_LOGD(TAG, "Step %s start", _stepname.c_str());

    if ((_stepname.compare("[TakeImage]") == 0) || (_stepname.compare(";[TakeImage]") == 0)){
        _classname = "ClassFlowTakeImage";
    }
    if ((_stepname.compare("[Alignment]") == 0) || (_stepname.compare(";[Alignment]") == 0)){
        _classname = "ClassFlowAlignment";
    }
    if ((_stepname.compare("[Digits") == 0) || (_stepname.compare(";[Digits]") == 0)) {
        _classname = "ClassFlowCNNGeneral - Digit";
    }
    if ((_stepname.compare("[Analog]") == 0) || (_stepname.compare(";[Analog]") == 0)){
        _classname = "ClassFlowCNNGeneral - Analog";
    }
    #ifdef ENABLE_MQTT
    if ((_stepname.compare("[MQTT]") == 0) || (_stepname.compare(";[MQTT]") == 0)){
        _classname = "ClassFlowMQTT";
    }
    #endif //ENABLE_MQTT

    #ifdef ENABLE_INFLUXDB
    if ((_stepname.compare("[InfluxDB]") == 0) || (_stepname.compare(";[InfluxDB]") == 0)){
        _classname = "ClassFlowInfluxDB";
    }
    if ((_stepname.compare("[InfluxDBv2]") == 0) || (_stepname.compare(";[InfluxDBv2]") == 0)){
        _classname = "ClassFlowInfluxDBv2";
    }
    #endif //ENABLE_INFLUXDB

    for (int i = 0; i < FlowControll.size(); ++i)
        if (FlowControll[i]->name().compare(_classname) == 0){
            if (!(FlowControll[i]->name().compare("ClassFlowTakeImage") == 0))      // if it is a TakeImage, the image does not need to be included, this happens automatically with the html query.
                FlowControll[i]->doFlow("");
            result = FlowControll[i]->getHTMLSingleStep(_host);
        }

    for (int i = 0; i < FlowControlPublish.size(); ++i)
        if (FlowControlPublish[i]->name().compare(_classname) == 0){
            FlowControlPublish[i]->doFlow("");
            result = FlowControlPublish[i]->getHTMLSingleStep(_host);
        }

    ESP_LOGD(TAG, "Step %s end", _stepname.c_str());

    return result;
}


std::string ClassFlowControll::TranslateAktstatus(std::string _input)
{
    if (_input.compare("ClassFlowTakeImage") == 0)
        return std::string(FLOW_TAKE_IMAGE);

    else if (_input.compare("ClassFlowAlignment") == 0)
        return std::string(FLOW_ALIGNMENT);

    else if (_input.compare("ClassFlowCNNGeneral - Digit") == 0)
        return std::string(FLOW_PROCESS_DIGIT_ROI);

    else if (_input.compare("ClassFlowCNNGeneral - Analog") == 0)
        return std::string(FLOW_PROCESS_ANALOG_ROI);

    else if (_input.compare("ClassFlowPostProcessing") == 0)
        return std::string(FLOW_POSTPROCESSING);

    #ifdef ENABLE_MQTT
    else if (_input.compare("ClassFlowMQTT") == 0)
        return std::string(FLOW_PUBLISH_MQTT);
    #endif //ENABLE_MQTT

    #ifdef ENABLE_INFLUXDB
    else if (_input.compare("ClassFlowInfluxDB") == 0)
        return std::string(FLOW_PUBLISH_INFLUXDB);

    else if (_input.compare("ClassFlowInfluxDBv2") == 0)
        return std::string(FLOW_PUBLISH_INFLUXDB2);
    #endif //ENABLE_INFLUXDB

    else
        return "Unkown State (" + _input + ")";
}


ClassFlow* ClassFlowControll::CreateClassFlow(std::string _type)
{
    ClassFlow* cfc = NULL;

    _type = trim(_type);

    if (toUpper(_type).compare("[TAKEIMAGE]") == 0)
    {
        cfc = new ClassFlowTakeImage(&FlowControll);
        if (cfc) {
            flowtakeimage = (ClassFlowTakeImage*) cfc;
            FlowControll.push_back(cfc);
        }
    }
    else if (toUpper(_type).compare("[ALIGNMENT]") == 0)
    {
        cfc = new ClassFlowAlignment(&FlowControll);
        if (cfc) {
            flowalignment = (ClassFlowAlignment*) cfc;
            FlowControll.push_back(cfc);
        }
    }
    else if (toUpper(_type).compare("[DIGITS]") == 0)
    {
        cfc = new ClassFlowCNNGeneral(flowalignment, std::string("Digit"));
        if (cfc) {
            flowdigit = (ClassFlowCNNGeneral*) cfc;
            FlowControll.push_back(cfc);
        }
    }
    else if (toUpper(_type).compare("[ANALOG]") == 0)
    {
        cfc = new ClassFlowCNNGeneral(flowalignment, std::string("Analog"));
        if (cfc) {
            flowanalog = (ClassFlowCNNGeneral*) cfc;
            FlowControll.push_back(cfc);
        }
    }
    else if (toUpper(_type).compare("[POSTPROCESSING]") == 0)
    {
        cfc = new ClassFlowPostProcessing(&FlowControll, flowanalog, flowdigit);
        if (cfc) {
            flowpostprocessing = (ClassFlowPostProcessing*) cfc;
            FlowControll.push_back(cfc);
        }
    }

    #ifdef ENABLE_MQTT
    else if (toUpper(_type).compare("[MQTT]") == 0) 
    {
        cfc = new ClassFlowMQTT(&FlowControll);
        if(cfc) {
            flowMQTT = (ClassFlowMQTT*) cfc;
            FlowControlPublish.push_back(cfc);
        }
    }
    #endif //ENABLE_MQTT

    #ifdef ENABLE_INFLUXDB
    else if (toUpper(_type).compare("[INFLUXDB]") == 0) 
    {
        cfc = new ClassFlowInfluxDB(&FlowControll);
        if(cfc) {
            flowInfluxDB = (ClassFlowInfluxDB*) cfc;
            FlowControlPublish.push_back(cfc);
        }
    }
    else if (toUpper(_type).compare("[INFLUXDBV2]") == 0) 
    {
        cfc = new ClassFlowInfluxDBv2(&FlowControll);
        if (cfc) {
            flowInfluxDBv2 = (ClassFlowInfluxDBv2*) cfc;
            FlowControlPublish.push_back(cfc);
        }
    }
    #endif //ENABLE_INFLUXDB

    else if (toUpper(_type).compare("[AUTOTIMER]") == 0) {
        cfc = this;
    }

    else if (toUpper(_type).compare("[DATALOGGING]") == 0) {
        cfc = this;
    }

    else if (toUpper(_type).compare("[DEBUG]") == 0) {
        cfc = this;
    }

    else if (toUpper(_type).compare("[SYSTEM]") == 0) {
        cfc = this;      
    }

    return cfc;
}


bool ClassFlowControll::InitFlow(std::string config)
{   
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Init flow");

    bool bRetVal = true;
    std::string line = "";
    std::string section = "";

    ClassFlow* cfc;
    FILE* pFile;
    config = FormatFileName(config);
    pFile = fopen(config.c_str(), "r");

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(pFile, NULL, _IOFBF, 512);

    if (pFile == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InitFlow: Unable to open config file"); 
        return false;
    }

    char zw[256];
    if (fgets(zw, sizeof(zw), pFile) == NULL) {
        line = "";
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InitFlow: Config file opened, but empty or content not readable");
        fclose(pFile);
        return false;
    }
    else {
        line = std::string(zw);
    }

    while ((line.size() > 0) && !(feof(pFile)))
    {
        cfc = CreateClassFlow(line);
        if (cfc)
        {
            //ESP_LOGD(TAG, "Start ReadParameter (%s)", line.c_str());
            section = line;
            if (!cfc->ReadParameter(pFile, line)) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InitFlow: Error loading parameter of section " +
                                                         section + " -> \"" + cfc->name() + "\"");
                bRetVal = false;
            }
        }
        else
        {
            line = "";
            if (fgets(zw, sizeof(zw), pFile) && !feof(pFile))
                {
                    //ESP_LOGD(TAG, "Read: %s", zw);
                    line = std::string(zw);
                }
        }
    }
    fclose(pFile);

    if (flowtakeimage == NULL || flowalignment == NULL || flowpostprocessing == NULL || !this->readParameterDone) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "InitFlow: One mandatory parameter section [TAKEIMAGE], [ALIGNMENT], [POSTPROCESSING], "
                                                "[AUTOTIMER], [DATALOGGING], [DEBUG] or [SYSTEM] is missing. Check config file");
        bRetVal = false;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Init flow completed");
    LogFile.WriteHeapInfo("InitFlow done");

    return bRetVal;
}


void ClassFlowControll::DeinitFlow(void)
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Deinit flow");
    //LogFile.WriteHeapInfo("DeinitFlow start");

    Camera.FreeMemoryOnly(); // ClassControlCamera: Free any additional user allocated memory, but no cam driver deinit
    Camera.LightOnOff(false);
    StatusLEDOff();
    //LogFile.WriteHeapInfo("After camera");

    gpio_handler_destroy();
    //LogFile.WriteHeapInfo("After GPIO");
    
    #ifdef ENABLE_MQTT
	delete flowMQTT;
    flowMQTT = NULL;
    //LogFile.WriteHeapInfo("After MQTT");
    #endif //ENABLE_MQTT
    
    #ifdef ENABLE_INFLUXDB
    delete flowInfluxDB;
    flowInfluxDB = NULL;
    //LogFile.WriteHeapInfo("After INFLUX");

    delete flowInfluxDBv2;
    flowInfluxDBv2 = NULL;
    //LogFile.WriteHeapInfo("After INFLUXv2");
    #endif //ENABLE_INFLUXDB

    delete flowpostprocessing;
    flowpostprocessing = NULL;
    //LogFile.WriteHeapInfo("After POSTPROC");

    delete flowanalog;
    flowanalog = NULL;
    //LogFile.WriteHeapInfo("After ANALOG");

    delete flowdigit;
    flowdigit = NULL;
    //LogFile.WriteHeapInfo("After DIGIT");

    delete flowalignment;
    flowalignment = NULL;
    //LogFile.WriteHeapInfo("After ALIGN");

    delete flowtakeimage;
    flowtakeimage = NULL;
    //LogFile.WriteHeapInfo("After TAKEIMG");
    
    FlowControll.clear();               // Clear vector to release allocated memory
    FlowControlPublish.clear();         // Clear vector to release allocated memory
    FlowStateEvaluationEvent.clear();  // Clear vector to release allocated memory
    FlowStatePublishEvent.clear();     // Clear vector to release allocated memory
    
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Deinit flow completed"); 
    LogFile.WriteHeapInfo("DeinitFlow completed");
}


bool ClassFlowControll::doFlowImageEvaluation(std::string time)
{
    bool result = true;
    FlowStateEvaluationEvent.clear();

    for (int i = 0; i < FlowControll.size(); ++i)
    {
        #ifdef DEBUG_DETAIL_ON  
            LogFile.WriteHeapInfo("ClassFlowControll::doFlow: " + FlowControll[i]->name());
        #endif
        
        setActStatus(TranslateAktstatus(FlowControll[i]->name()));
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + getActStatus());
        #ifdef ENABLE_MQTT
            MQTTPublish(mqttServer_getMainTopic() + "/" + "status", getActStatus(), 1, false);
        #endif //ENABLE_MQTT

        if (!FlowControll[i]->doFlow(time)) {
            FlowStateEvaluationEvent.push_back(FlowControll[i]->getFlowState());
            
            for (int j = 0; j < FlowControll[i]->getFlowState()->EventCode.size(); j++) {
                if (FlowControll[i]->getFlowState()->EventCode[j] < 0) {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Error occured during processing of state \"" + getActStatus() + "\"");
                    result = false;
                    break;
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Deviation occured during processing of state \"" + getActStatus() + "\"");
                }
            }

            if (!result) // If an error occured, stop processing of further tasks
                break;
        }
    }
    return result;
}


bool ClassFlowControll::doFlowPublishData(std::string time)
{
    bool result = true;
    FlowStatePublishEvent.clear();

    for (int i = 0; i < FlowControlPublish.size(); ++i)
    {
        #ifdef DEBUG_DETAIL_ON  
            LogFile.WriteHeapInfo("ClassFlowControll::doFlow: " + FlowControlPublish[i]->name());
        #endif
        
        setActStatus(TranslateAktstatus(FlowControlPublish[i]->name()));
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + getActStatus());
        #ifdef ENABLE_MQTT
            MQTTPublish(mqttServer_getMainTopic() + "/" + "status", getActStatus(), 1, false);
        #endif //ENABLE_MQTT

        if (!FlowControlPublish[i]->doFlow(time)) {
            FlowStatePublishEvent.push_back(FlowControlPublish[i]->getFlowState());

            for (int j = 0; j < FlowControll[i]->getFlowState()->EventCode.size(); j++) {
                if (FlowControll[i]->getFlowState()->EventCode[j] < 0) {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Error occured during processing of state \"" + getActStatus() + "\"");
                    result = false;
                    break;
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Deviation occured during processing of state \"" + getActStatus() + "\"");
                }
            }

            if (!result) // If an error occured, stop processing of further tasks
                break;
        }
    }
    return result;
}


bool ClassFlowControll::doFlowTakeImageOnly(std::string time)
{
    bool result = true;
    FlowStateEvaluationEvent.clear();
    
    for (int i = 0; i < FlowControll.size(); ++i)
    {
        if (FlowControll[i]->name() == "ClassFlowTakeImage") {
            setActStatus(TranslateAktstatus(FlowControll[i]->name()));
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + getActStatus());
            #ifdef ENABLE_MQTT
                MQTTPublish(mqttServer_getMainTopic() + "/" + "status", getActStatus(), 1, false);
            #endif //ENABLE_MQTT

            if (!FlowControlPublish[i]->doFlow(time)) {
                FlowStateEvaluationEvent.push_back(FlowControll[i]->getFlowState());

                for (int j = 0; j < FlowControll[i]->getFlowState()->EventCode.size(); j++) {
                    if (FlowControll[i]->getFlowState()->EventCode[j] < 0) {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Error occured during processing of state \"" + getActStatus() + "\"");
                        result = false;
                        break;
                    }
                    else {
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Deviation occured during processing of state \"" + getActStatus() + "\"");
                    }
                }

                if (!result) // If an error occured, stop processing of further tasks
                    break;
            }
        }
    }
    return result;
}


bool ClassFlowControll::FlowStateEventOccured()
{
    if (FlowStateEvaluationEvent.size() != 0 || FlowStatePublishEvent.size() != 0)
        return true;
    else
        return false;
}


void ClassFlowControll::PostProcessEventHandler()
{
    for (int i = 0; i < FlowStateEvaluationEvent.size(); ++i) {
        for (int j = 0; j < FlowControll.size(); ++j) {
            if (FlowStateEvaluationEvent[i]->ClassName.compare(FlowControll[j]->name()) == 0) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, FlowStateEvaluationEvent[i]->ClassName + "-> doPostProcessEventHandling"); 
                FlowControll[j]->doPostProcessEventHandling();
                FlowControll[j]->presetFlowStateHandler(true); // Reinit after processing
            }
        }
    }
    // Reset of errors will be peformed before next flow starts --> functions doFlowImageEvaluation, doFlowTakeImageOnly
    // FlowStateEvaluationEvent.clear();

    for (int i = 0; i < FlowStatePublishEvent.size(); ++i) {
        for (int j = 0; j < FlowControlPublish.size(); ++j) {
            if (FlowStatePublishEvent[i]->ClassName.compare(FlowControlPublish[j]->name()) == 0) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, FlowStateEvaluationEvent[i]->ClassName + "-> doPostProcessEventHandling"); 
                FlowControlPublish[j]->doPostProcessEventHandling();
                FlowControll[j]->presetFlowStateHandler(true); // Reinit after processing
            }
        }
    }
    // Reset of errors will be peformed before next flow starts --> function doFlowPublishData
    // FlowStatePublishEvent.clear();
}


bool ClassFlowControll::isAutoStart()
{
    return AutoStart;
}


bool ClassFlowControll::isAutoStart(long &_interval)
{
    _interval = AutoInterval * 60 * 1000; // AutoInterval: minutes -> ms
    return AutoStart;
}


std::string ClassFlowControll::getActStatusWithTime()
{
    return aktstatusWithTime;
}


std::string ClassFlowControll::getActStatus()
{
    return aktstatus;
}


void ClassFlowControll::setActFlowError(bool _aktflowerror)
{
    aktflowerror = _aktflowerror;
}


bool ClassFlowControll::getActFlowError()
{
    return aktflowerror;
}


void ClassFlowControll::setActStatus(std::string _aktstatus)
{
    aktstatus = _aktstatus;
    aktstatusWithTime = "[" + getCurrentTimeString("%H:%M:%S") + "] " + _aktstatus;
}


std::vector<HTMLInfo*> ClassFlowControll::GetAllDigital() 
{
    if (flowdigit)
    {
        ESP_LOGD(TAG, "ClassFlowControll::GetAllDigital - flowdigit != NULL");
        return flowdigit->GetHTMLInfo();
    }

    std::vector<HTMLInfo*> empty;
    return empty;
}


std::vector<HTMLInfo*> ClassFlowControll::GetAllAnalog()
{
    if (flowanalog)
        return flowanalog->GetHTMLInfo();

    std::vector<HTMLInfo*> empty;
    return empty;
}


t_CNNType ClassFlowControll::GetTypeDigital()
{
    if (flowdigit)
        return flowdigit->getCNNType();

    return t_CNNType::None;
}


t_CNNType ClassFlowControll::GetTypeAnalog()
{
    if (flowanalog)
        return flowanalog->getCNNType();

    return t_CNNType::None;
}


void ClassFlowControll::DigitalDrawROI(CImageBasis *_zw)
{
    if (flowdigit)
        flowdigit->DrawROI(_zw);
}


void ClassFlowControll::AnalogDrawROI(CImageBasis *_zw)
{
    if (flowanalog)
        flowanalog->DrawROI(_zw);
}


#ifdef ENABLE_MQTT
bool ClassFlowControll::StartMQTTService() 
{
    /* Start the MQTT service */
        for (int i = 0; i < FlowControlPublish.size(); ++i) {
            if (FlowControlPublish[i]->name().compare("ClassFlowMQTT") == 0) {
                return ((ClassFlowMQTT*) (FlowControlPublish[i]))->Start(AutoInterval);
            }  
        } 
    return false;
}
#endif //ENABLE_MQTT


std::string ClassFlowControll::getJSON()
{
    return flowpostprocessing->GetJSON();
}


/* Return all available numbers names (number sequences)*/
std::string ClassFlowControll::getNumbersName()
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return "";
    }
    
    return flowpostprocessing->getNumbersName();
}


/* Return numbers name of a given array position number */
std::string ClassFlowControll::getNumbersName(int _number)
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return "";
    }
    
    return (*flowpostprocessing->GetNumbers())[_number]->name;
}


/* Return number of number sequences */
int ClassFlowControll::getNumbersSize()
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return -1;
    }
    
    return flowpostprocessing->GetNumbers()->size();
}


/* Return array postion of a given numbers name (number sequence) */
int ClassFlowControll::getNumbersNamePosition(std::string _name)
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return -1;
    }
    
    for (int i = 0; i < getNumbersSize(); ++i) {
        if ((*flowpostprocessing->GetNumbers())[i]->name == _name)
            return i;
    }

    return -1;
}


/* Return value for a given numbers name (number sequence) and value type */
std::string ClassFlowControll::getNumbersValue(std::string _name, int _type)
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return "";
    }

    int pos = getNumbersNamePosition(_name); // Search numbers name array position
    if (pos < 0) {
        return "";
    }

    switch (_type) {
        case READOUT_TYPE_VALUE:
            return (*flowpostprocessing->GetNumbers())[pos]->sActualValue;

        case READOUT_TYPE_RAWVALUE:
            return (*flowpostprocessing->GetNumbers())[pos]->sRawValue;

        case READOUT_TYPE_FALLBACKVALUE:
            return (*flowpostprocessing->GetNumbers())[pos]->sFallbackValue;

        case READOUT_TYPE_VALUE_STATUS:
            return (*flowpostprocessing->GetNumbers())[pos]->sValueStatus;

        default:
            return "";
    }

    return "";
}


/* Return value for a given numbers name array position and value type */
std::string ClassFlowControll::getNumbersValue(int _position, int _type)
{
    if (flowpostprocessing == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Request rejected. Flowpostprocessing not available"); 
        return "";
    }

    if (_position < 0 || _position > getNumbersSize()) {
        return "";
    }

    switch (_type) {
        case READOUT_TYPE_VALUE:
            return (*flowpostprocessing->GetNumbers())[_position]->sActualValue;

        case READOUT_TYPE_RAWVALUE:
            return (*flowpostprocessing->GetNumbers())[_position]->sRawValue;

        case READOUT_TYPE_FALLBACKVALUE:
            return (*flowpostprocessing->GetNumbers())[_position]->sFallbackValue;

        case READOUT_TYPE_VALUE_STATUS:
            return (*flowpostprocessing->GetNumbers())[_position]->sValueStatus;

        default:
            return "";
    }

    return "";
}


/* Return values for all numbers names (number sequences) and a given value type */
std::string ClassFlowControll::getReadoutAll(int _type)
{
    std::string out = "";
    if (flowpostprocessing)
    {
        std::vector<NumberPost*> *numbers = flowpostprocessing->GetNumbers();

        for (int i = 0; i < (*numbers).size(); ++i)
        {
            out = out + (*numbers)[i]->name + "\t";
            switch (_type) {
                case READOUT_TYPE_TIMESTAMP_PROCESSED:
                    out = out + (*numbers)[i]->sTimeProcessed;
                    break;
                case READOUT_TYPE_TIMESTAMP_FALLBACKVALUE:
                    out = out + (*numbers)[i]->sTimeFallbackValue;
                    break;
                case READOUT_TYPE_VALUE:
                    out = out + (*numbers)[i]->sActualValue;
                    break;
                case READOUT_TYPE_FALLBACKVALUE:
                    if (flowpostprocessing->getUseFallbackValue()) {
                        if ((*numbers)[i]->isFallbackValueValid)
                            out = out + (*numbers)[i]->sFallbackValue;
                        else
                            out = out + "Outdated or age indeterminable";                
                    }
                    else
                        out = out + "Deactivated";
                    break;
                case READOUT_TYPE_RAWVALUE:
                    out = out + (*numbers)[i]->sRawValue;
                    break;
                case READOUT_TYPE_VALUE_STATUS:
                    out = out + (*numbers)[i]->sValueStatus;
                    break;
                case READOUT_TYPE_RATE_PER_MIN:
                    out = out + (*numbers)[i]->sRatePerMin;
                    break;
                case READOUT_TYPE_RATE_PER_PROCESSING:
                    out = out + (*numbers)[i]->sRatePerProcessing;
                    break;
            }
            if (i < (*numbers).size()-1)
                out = out + "\r\n";
        }
    //    ESP_LOGD(TAG, "OUT: %s", out.c_str());
    }

    return out;
}	


/* Return values for numbers names (number sequences) of a given array positon and a given return type */
std::string ClassFlowControll::getReadout(bool _rawvalue = false, bool _noerror = false, int _number = 0)
{
    if (flowpostprocessing)
        return flowpostprocessing->getReadoutParam(_rawvalue, _noerror, _number);

    std::string zw = "";
    std::string result = "";

    for (int i = 0; i < FlowControll.size(); ++i)
    {
        zw = FlowControll[i]->getReadout();
        if (zw.length() > 0)
        {
            if (result.length() == 0)
                result = zw;
            else
                result = result + "\t" + zw;
        }
    }

    return result;
}


std::string ClassFlowControll::GetFallbackValue(std::string _number)	
{
    if (flowpostprocessing)
    {
        return flowpostprocessing->GetFallbackValue(_number);   
    }

    return std::string("");    
}


bool ClassFlowControll::UpdateFallbackValue(std::string _newvalue, std::string _numbers)
{
    double newvalueAsDouble;
    char* p;

    _newvalue = trim(_newvalue);
    //ESP_LOGD(TAG, "Input UpdateFallbackValue: %s", _newvalue.c_str());

    if (_newvalue.substr(0,8).compare("0.000000") == 0 || _newvalue.compare("0.0") == 0 || _newvalue.compare("0") == 0) {
        newvalueAsDouble = 0;   // preset to value = 0
    }
    else {
        newvalueAsDouble = strtod(_newvalue.c_str(), &p);
        if (newvalueAsDouble == 0) {
            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "UpdateFallbackValue: No valid value for processing: " + _newvalue);
            return false;
        }
    }
    
    if (flowpostprocessing) {
        if (flowpostprocessing->SetFallbackValue(newvalueAsDouble, _numbers))
            return true;
        else
            return false;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "UpdateFallbackValue: ERROR - Class Post-Processing not initialized");
        return false;
    }
}


int ClassFlowControll::CleanTempFolder() {
    const char* folderPath = "/sdcard/img_tmp";
    
    ESP_LOGD(TAG, "Clean up temporary folder to avoid damage of sdcard sectors: %s", folderPath);
    DIR *dir = opendir(folderPath);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir: %s", folderPath);
        return -1;
    }

    struct dirent *entry;
    int deleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        std::string path = std::string(folderPath) + "/" + entry->d_name;
		if (entry->d_type == DT_REG) {
			if (unlink(path.c_str()) == 0) {
				deleted ++;
			} else {
				ESP_LOGE(TAG, "can't delete file: %s", path.c_str());
			}
        } else if (entry->d_type == DT_DIR) {
			deleted += removeFolder(path.c_str(), TAG);
		}
    }
    closedir(dir);
    ESP_LOGD(TAG, "%d files deleted", deleted);
    
    return 0;
}


CImageBasis* ClassFlowControll::getRawImage()
{
    return flowtakeimage->rawImage;
}


esp_err_t ClassFlowControll::SendRawJPG(httpd_req_t *req)
{
    if (flowtakeimage) {
        return flowtakeimage->SendRawJPG(req);
    }
    else {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "flowtakeimage not available: Raw image cannot be served");
        return ESP_ERR_NOT_FOUND;
    }
}


esp_err_t ClassFlowControll::GetJPGStream(std::string _fn, httpd_req_t *req)
{
    ESP_LOGD(TAG, "ClassFlowControll::GetJPGStream %s", _fn.c_str());

    #ifdef DEBUG_DETAIL_ON 
        LogFile.WriteHeapInfo("ClassFlowControll::GetJPGStream - Start");
    #endif

    CImageBasis *_send = NULL;
    esp_err_t result = ESP_FAIL;
    bool _sendDelete = false;

    if (_fn == "alg.jpg") {
        if (flowalignment && flowalignment->ImageBasis->ImageOkay()) {
            _send = flowalignment->ImageBasis;
        }
        else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: alg.jpg cannot be served");
            return ESP_FAIL;
        }
    }
    else if (_fn == "alg_roi.jpg") {
        if (getActStatus().compare(std::string(FLOW_INIT_DELAYED)) == 0) {
            FILE* file = fopen("/sdcard/html/Flowstate_initialization_delayed.jpg", "rb"); 

            if (!file) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_initialization_delayed.jpg not found");
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);  
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize); 
            free(fileBuffer);
        }
        else if (getActStatus().compare(std::string(FLOW_INIT)) == 0 ||
                    getActStatus().compare(std::string(FLOW_INIT_FAILED)) == 0) {
            FILE* file = fopen("/sdcard/html/Flowstate_initialization.jpg", "rb"); 

            if (!file) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_initialization.jpg not found");
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);  
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize); 
            free(fileBuffer);
        }
        else if (getActStatus().compare(std::string(FLOW_SETUP_MODE)) == 0) {
            FILE* file = fopen("/sdcard/html/Flowstate_setup_mode.jpg", "rb"); 

            if (!file) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_setup_mode.jpg not found");
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);  
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize); 
            free(fileBuffer);
        }
        else if ((!flowtakeimage->getFlowState()->getExecuted && getActStatus().compare(std::string(FLOW_IDLE_NO_AUTOSTART)) == 0) ||
                    (!isAutoStart() && FlowStateEventOccured() && getActStatus().compare(std::string(FLOW_TAKE_IMAGE)) == 0)) {   // Show only before first cycle started or error occured, otherwise result will be shown till next start
            FILE* file = fopen("/sdcard/html/Flowstate_idle_no_autostart.jpg", "rb"); 

            if (!file) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_idle_no_autostart.jpg not found");
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);  
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize); 
            free(fileBuffer);
        }
        else if (getActStatus().compare(std::string(FLOW_TAKE_IMAGE)) == 0) {
            if (flowalignment && flowalignment->AlgROI) {
                FILE* file = fopen("/sdcard/html/Flowstate_take_image.jpg", "rb");    

                if (!file) {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_take_image.jpg not found");
                    return ESP_FAIL;
                }

                /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
                // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
                setvbuf(file, NULL, _IOFBF, 512);

                fseek(file, 0, SEEK_END);
                flowalignment->AlgROI->size = ftell(file); /* how long is the file ? */
                fseek(file, 0, SEEK_SET); /* reset */
                
                if (flowalignment->AlgROI->size > MAX_JPG_SIZE) {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_take_image.jpg (" + std::to_string(flowalignment->AlgROI->size) +
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: alg_roi.jpg cannot be served -> alg.jpg is going to be served");
                if (flowalignment && flowalignment->ImageBasis->ImageOkay()) {
                    _send = flowalignment->ImageBasis;
                }
                else {
                    httpd_resp_send(req, NULL, 0);
                    return ESP_OK;
                }
            }
        }
        else if (isAutoStart() && FlowStateEventOccured() && (getActStatus().compare(std::string(FLOW_TAKE_IMAGE)) == 0)) {
            FILE* file = fopen("/sdcard/html/Flowstate_idle_autostart.jpg", "rb"); 

            if (!file) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "File /sdcard/html/Flowstate_idle_autostart.jpg not found");
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
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: Not enough memory to create fileBuffer: " + std::to_string(fileSize));
                fclose(file);  
                return ESP_FAIL;
            }

            fread(fileBuffer, fileSize, 1, file);
            fclose(file);

            httpd_resp_set_type(req, "image/jpeg");
            result = httpd_resp_send(req, (const char *)fileBuffer, fileSize); 
            free(fileBuffer);
        }
        else {
            if (flowalignment && flowalignment->AlgROI) {
                httpd_resp_set_type(req, "image/jpeg");
                result = httpd_resp_send(req, (const char *)flowalignment->AlgROI->data, flowalignment->AlgROI->size);
            }
            else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ClassFlowControll::GetJPGStream: alg_roi.jpg cannot be served -> alg.jpg is going to be served");
                if (flowalignment && flowalignment->ImageBasis->ImageOkay()) {
                    _send = flowalignment->ImageBasis;
                }
                else {
                    httpd_resp_send(req, NULL, 0);
                    return ESP_OK;
                }
            }
        }
    }
    else {
        std::vector<HTMLInfo*> htmlinfo;
    
        htmlinfo = GetAllDigital();
        ESP_LOGD(TAG, "After getClassFlowControll::GetAllDigital");

        for (int i = 0; i < htmlinfo.size(); ++i)
        {
            if (_fn == htmlinfo[i]->filename)
            {
                if (htmlinfo[i]->image)
                    _send = htmlinfo[i]->image;
            }

            if (_fn == htmlinfo[i]->filename_org)
            {
                if (htmlinfo[i]->image_org)
                    _send = htmlinfo[i]->image_org;
            }
            delete htmlinfo[i];
        }
        htmlinfo.clear();

        if (!_send)
        {
	        htmlinfo = GetAllAnalog();
	        ESP_LOGD(TAG, "After getClassFlowControll::GetAllAnalog");
	        
	        for (int i = 0; i < htmlinfo.size(); ++i)
	        {
	            if (_fn == htmlinfo[i]->filename)
	            {
	                if (htmlinfo[i]->image)
	                    _send = htmlinfo[i]->image;
	            }

	            if (_fn == htmlinfo[i]->filename_org)
	            {
	                if (htmlinfo[i]->image_org)
	                    _send = htmlinfo[i]->image_org;
	            }
	            delete htmlinfo[i];
	        }
	        htmlinfo.clear();
    	}
    }

    #ifdef DEBUG_DETAIL_ON 
        LogFile.WriteHeapInfo("ClassFlowControll::GetJPGStream - before send");
    #endif

    if (_send)
    {
        ESP_LOGD(TAG, "Sending file: %s", _fn.c_str());
        set_content_type_from_file(req, _fn.c_str());
        result = _send->SendJPGtoHTTP(req);
        /* Respond with an empty chunk to signal HTTP response completion */
        httpd_resp_send_chunk(req, NULL, 0);
        ESP_LOGD(TAG, "File sending complete");

        if (_sendDelete)
            delete _send;
            
        _send = NULL;  
    }

    #ifdef DEBUG_DETAIL_ON 
        LogFile.WriteHeapInfo("ClassFlowControll::GetJPGStream - done");
    #endif

    return result;
}

