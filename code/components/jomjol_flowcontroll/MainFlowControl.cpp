#include "MainFlowControl.h"
#include "../../include/defines.h"

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

#include "esp_log.h"
#include <esp_timer.h>
#include "esp_camera.h"

#include "cJSON.h"

#include "Helper.h"
#include "system.h"
#include "statusled.h"
#include "time_sntp.h"
#include "ClassControllCamera.h"
#include "ClassLogFile.h"
#include "server_GPIO.h"
#include "server_file.h"
#include "read_wlanini.h"
#include "connect_wlan.h"
#include "psram.h"

#ifdef ENABLE_MQTT
    #include "interface_mqtt.h"
    #include "server_mqtt.h"
#endif //ENABLE_MQTT


ClassFlowControll flowctrl;

static bool isPlannedReboot = false;
static TaskHandle_t xHandletask_autodoFlow = NULL;
static bool bTaskAutoFlowCreated = false;
static int taskAutoFlowState = FLOW_TASK_STATE_INIT;
static bool reloadConfig = false;
static bool manualFlowStart = false;
static long auto_interval = 0;
static int cycleCounter = 0;
static int FlowStateErrorsInRow = 0;
static int processingTime = 0;

static const char *TAG = "MAINCTRL";


//#define DEBUG_DETAIL_ON


void CheckIsPlannedReboot()
{
 	FILE *pfile;
    if ((pfile = fopen("/sdcard/reboot.txt", "r")) == NULL) {
		//LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Initial boot or not a planned reboot");
        isPlannedReboot = false;
	}
    else {
		LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Planned reboot");
        DeleteFile("/sdcard/reboot.txt");   // Prevent Boot Loop!!!
        isPlannedReboot = true;
	}
}


bool getIsPlannedReboot() 
{
    return isPlannedReboot;
}


void setTaskAutoFlowState(int _value) 
{
    taskAutoFlowState = _value;
}


std::string getProcessStatus(void)
{
    std::string process_status;
    
    if (taskAutoFlowState >=4 && taskAutoFlowState <= 7) 
        process_status = "Processing (Automatic)";
    else if (taskAutoFlowState == 3) 
        process_status = "Processing (Triggered Only)";
    else if (taskAutoFlowState < 3) 
        process_status = "Not Processing / Not Ready";
    else
        process_status = "Status unknown: " + taskAutoFlowState;

    return process_status;
}


int getFlowProcessingTime()
{
    return processingTime;
}


int getFlowCycleCounter() 
{
    return cycleCounter;
}


esp_err_t GetJPG(std::string _filename, httpd_req_t *req)
{
    return flowctrl.GetJPGStream(_filename, req);
}


esp_err_t GetRawJPG(httpd_req_t *req)
{
    return flowctrl.SendRawJPG(req);
}


bool isSetupModusActive() 
{
    return flowctrl.getStatusSetupModus();
}


void DeleteMainFlowTask()
{
    #ifdef DEBUG_DETAIL_ON      
        ESP_LOGD(TAG, "DeleteMainFlowTask: xHandletask_autodoFlow: %ld", (long) xHandletask_autodoFlow);
    #endif
    if( xHandletask_autodoFlow != NULL )
    {
        vTaskDelete(xHandletask_autodoFlow);
        xHandletask_autodoFlow = NULL;
    }
    #ifdef DEBUG_DETAIL_ON      
    	ESP_LOGD(TAG, "Killed: xHandletask_autodoFlow");
    #endif
}


bool doInit(void)
{
    bool bRetVal = true;

    // Deinit main flow components before init all ressources again
    // ********************************************   
    flowctrl.DeinitFlow();
    //heap_caps_dump(MALLOC_CAP_SPIRAM);

    // Init cam if init not yet done.
    // Make sure this is called between deinit and init of flow components (avoid SPIRAM fragmentation)
    // ********************************************   
    if (!Camera.getcameraInitSuccessful()) { 
        Camera.powerResetCamera();
        esp_err_t camStatus = Camera.initCam(); 

        if (camStatus != ESP_OK) // Camera init failed
            return false;
        
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Init camera successful");
        Camera.printCamInfo();
    }

    //  // Init main flow components
    // ********************************************   
    if (!flowctrl.InitFlow(CONFIG_FILE)) {
        flowctrl.DeinitFlow();
        bRetVal = false;
    }
    
    // Init GPIO handler
    // Note: GPIO handler has to be initialized before MQTT init to ensure proper topic subscription
    // ********************************************   
    gpio_handler_init();

    // Init MQTT service
    // ********************************************   
    #ifdef ENABLE_MQTT
        if (!flowctrl.StartMQTTService())
            bRetVal = false;
    #endif //ENABLE_MQTT

    //heap_caps_dump(MALLOC_CAP_INTERNAL);
    //heap_caps_dump(MALLOC_CAP_SPIRAM);

    return bRetVal;
}


#ifdef ENABLE_MQTT
esp_err_t MQTTCtrlFlowStart(std::string _topic) 
{
    if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_NO_AUTOSTART || 
        taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flow start triggered by MQTT topic " + _topic);  
        manualFlowStart = true;

        if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART)
            xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING || 
             taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA ||
             taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flow start triggered by MQTT topic "+ _topic + " got scheduled");      
        manualFlowStart = true;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Flow start triggered by MQTT topic " + _topic + ". Flow not initialized. Request rejected");
    }  

    return ESP_OK;
}
#endif //ENABLE_MQTT


esp_err_t handler_cycle_start(httpd_req_t *req) 
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_NO_AUTOSTART || 
        taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART || 
        flowctrl.getActStatus() == FLOW_INIT_FAILED) // Possibility to manual retrigger a cycle when init is already failed
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Cycle start triggered by REST API");
        const std::string zw = "001: Cycle start triggered by REST API (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        manualFlowStart = true;

        if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART)
            xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING || 
             taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA ||
             taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Cycle start triggered by REST API got scheduled");
        const std::string zw = "002: Cycle start triggered by REST API got scheduled (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());

        manualFlowStart = true;
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_INIT_DELAYED) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Cycle start triggered by REST API (abort state 'Initialization (delayed)'");
        const std::string zw = "003: Cycle start triggered by REST API abort initialization delay (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Cycle start triggered by REST API. Main task not initialized. Request rejected");
        const std::string zw = "E90: Cycle start triggered by REST API. Main task not initialized. Request rejected (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
    }

    return ESP_OK;
}


esp_err_t handler_reload_config(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (taskAutoFlowState == FLOW_TASK_STATE_INIT ||
        taskAutoFlowState == FLOW_TASK_STATE_SETUPMODE ||
        taskAutoFlowState == FLOW_TASK_STATE_IDLE_NO_AUTOSTART)
    {
        const std::string zw = "001: Reload config and redo flow initialization (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        reloadConfig = true;
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_INIT_DELAYED) {
        const std::string zw = "002: Abort waiting delay and continue with process initialization (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state.      
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART) {
        const std::string zw = "003: Abort waiting delay, reload config and reinitialize process(" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        reloadConfig = true;
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state.
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING || 
             taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA ||
             taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Reload config and schedule process reinitialization");
        const std::string zw = "004: Reload config and reinitialization got scheduled (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        reloadConfig = true;
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Reload configuration not possible. No main task. Request rejected");
        const std::string zw = "E90: Reload config not possible. No main task. Request rejected (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
    }
    return ESP_OK;
}


esp_err_t handler_fallbackvalue(httpd_req_t *req)
{
    if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E95: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = 
        "00: Handler usage:<br>"
        "- To retrieve actual Fallback Value, please provide a number sequence name only, e.g. /set_fallbackvalue?sequence=main<br>"
        "- To set Fallback Value to a new value, please provide a number sequence name and a value, e.g. /set_fallbackvalue?sequence=main&value=1234.5678<br>"
        "NOTE:<br>"
        "value >= 0.0: Set Fallback Value to provided value<br>"
        "value <  0.0: Set Fallback Value to actual RAW value (as long RAW value is a valid number, without N)";

    // Default return error message when no return is programmed
    std::string sReturnMessage = "E90: Uninitialized";

    char _query[100];
    char number_sequence[50] = "default";
    char value[20] = "";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        //ESP_LOGD(TAG, "Query: %s", _query);

        if (httpd_query_key_value(_query, "sequence", number_sequence, sizeof(number_sequence)) != ESP_OK) { // If request is incomplete
            sReturnMessage = "E91: Query parameter incomplete or not valid!<br> "
                             "Call /set_fallbackvalue to show REST API usage info and/or check documentation";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL; 
        }

        if (httpd_query_key_value(_query, "value", value, sizeof(value)) == ESP_OK) {
            //ESP_LOGD(TAG, "Value: %s", value);
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_send(req, RESTUsageInfo.c_str(), RESTUsageInfo.length());
        return ESP_OK; 
    }   

    if (strlen(value) == 0) { // If no value is povided --> return actual FallbackValue
        sReturnMessage = flowctrl.GetFallbackValue(std::string(number_sequence));

        if (sReturnMessage.empty()) {
            sReturnMessage = "E92: Number sequence not found";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL;
        }
    }
    else {
        // New value is positive: Set FallbackValue to provided value and return value
        // New value is negative and actual RAW value is a valid number: Set FallbackValue to RAW value and return value
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "REST API handler_fallbackvalue called: numbersname: " + std::string(number_sequence) + 
                                                ", value: " + std::string(value));
        if (!flowctrl.UpdateFallbackValue(value, number_sequence)) {
            sReturnMessage = "E93: Update request rejected. Please check device logs for more details";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());  
            return ESP_FAIL;
        }

        sReturnMessage = flowctrl.GetFallbackValue(std::string(number_sequence));

        if (sReturnMessage.empty()) {
            sReturnMessage = "E94: Numbers name not found";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL;
        }
    }

    httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());

    return ESP_OK;
}


esp_err_t handler_editflow(httpd_req_t *req)
{
    const char* APIName = "editflow:v2"; // API name and version
    char _query[200];
    char _valuechar[30];
    std::string task;

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }

    if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else if (task.compare("data") == 0) {
        return get_data_file_handler(req);
    }
    else if (task.compare("tflite") == 0) {
        return get_tflite_file_handler(req);
    }
    else if (task.compare("copy") == 0) {
        std::string in, out;

        httpd_query_key_value(_query, "in", _valuechar, 30);
        in = std::string(_valuechar);
        httpd_query_key_value(_query, "out", _valuechar, 30);         
        out = std::string(_valuechar);  

        #ifdef DEBUG_DETAIL_ON       
            ESP_LOGD(TAG, "in: %s", in.c_str());
            ESP_LOGD(TAG, "out: %s", out.c_str());
        #endif

        in = "/sdcard" + in;
        out = "/sdcard" + out;

        CopyFile(in, out);
        
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, "Copy Done"); 
    }
    else if (task.compare("cutref") == 0) {
        if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
            return ESP_FAIL;
        }

        std::string in, out, zw;
        int x, y, dx, dy;

        httpd_query_key_value(_query, "in", _valuechar, 30);
        in = std::string(_valuechar);

        httpd_query_key_value(_query, "out", _valuechar, 30);         
        out = std::string(_valuechar);  

        httpd_query_key_value(_query, "x", _valuechar, 30);
        zw = std::string(_valuechar);  
        x = stoi(zw);              

        httpd_query_key_value(_query, "y", _valuechar, 30);
        zw = std::string(_valuechar);  
        y = stoi(zw);              

        httpd_query_key_value(_query, "dx", _valuechar, 30);
        zw = std::string(_valuechar);  
        dx = stoi(zw);  

        httpd_query_key_value(_query, "dy", _valuechar, 30);
        zw = std::string(_valuechar);  
        dy = stoi(zw);          

        #ifdef DEBUG_DETAIL_ON       
            ESP_LOGD(TAG, "in: %s", in.c_str());
            ESP_LOGD(TAG, "out: %s", out.c_str());
            ESP_LOGD(TAG, "x: %s", zw.c_str());
            ESP_LOGD(TAG, "y: %s", zw.c_str());
            ESP_LOGD(TAG, "dx: %s", zw.c_str());
            ESP_LOGD(TAG, "dy: %s", zw.c_str());
        #endif

        in = "/sdcard" + in;    // --> img_tmp/reference.jpg
        out = "/sdcard" + out;  // --> img_tmp/refX.jpg

        // Reuse allocated memory of CImageBasis element "rawImage" (ClassTakeImage.cpp)
        STBIObjectPSRAM.name="rawImage";
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.PreallocatedMemory = flowctrl.getRawImage()->RGBImageGet();
        STBIObjectPSRAM.PreallocatedMemorySize = flowctrl.getRawImage()->getMemsize();
        CAlignAndCutImage* caic = new CAlignAndCutImage("cutref1", in, true);  // CImageBasis of reference.jpg will be created first (921kB RAM needed)
        caic->CutAndSave(out, x, y, dx, dy);
        delete caic;

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, "CutImage Done"); 
    }
    else {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E91: Task not found");
        return ESP_FAIL;  
    }

    return ESP_OK;
}


esp_err_t handler_process_data(httpd_req_t *req)
{
    if (!bTaskAutoFlowCreated) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    const char* APIName = "process_data:v2"; // API name and version
    char _query[200];
    char _valuechar[30];    
    std::string type, number_sequence;

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {        
        if (httpd_query_key_value(_query, "type", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            type = std::string(_valuechar);
        }
        if (httpd_query_key_value(_query, "sequence", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            number_sequence = std::string(_valuechar);
        }
    }
    else { // default - no parameter set: send data as JSON
        esp_err_t retVal = ESP_OK;
        std::string sReturnMessage;
        cJSON *cJSONObject = cJSON_CreateObject();
        
        if (cJSONObject == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Error, JSON object cannot be created");
            return ESP_FAIL;
        }

        if (cJSON_AddStringToObject(cJSONObject, "api_name", APIName) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "number_sequences", std::to_string(flowctrl.getNumbersSize()).c_str()) == NULL)
            retVal = ESP_FAIL;

        cJSON *cJSONObjectTimestampProcessed = cJSON_AddObjectToObject(cJSONObject, "timestamp_processed");
        if (cJSONObjectTimestampProcessed == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectTimestampProcessed, "inline", flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_PROCESSED).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectTimestampProcessedSequence = cJSON_AddObjectToObject(cJSONObjectTimestampProcessed, "sequence");
            if (cJSONObjectTimestampProcessedSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectTimestampProcessedSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_TIMESTAMP_PROCESSED).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectTimestampFallbackValue = cJSON_AddObjectToObject(cJSONObject, "timestamp_fallbackvalue");
        if (cJSONObjectTimestampFallbackValue == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectTimestampFallbackValue, "inline", flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_FALLBACKVALUE).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectTimestampFallbackValueSequence = cJSON_AddObjectToObject(cJSONObjectTimestampFallbackValue, "sequence");
            if (cJSONObjectTimestampFallbackValueSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectTimestampFallbackValueSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_TIMESTAMP_FALLBACKVALUE).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectActualValue = cJSON_AddObjectToObject(cJSONObject, "actual_value");
        if (cJSONObjectActualValue == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectActualValue, "inline", flowctrl.getReadoutAll(READOUT_TYPE_VALUE).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectActualValueSequence = cJSON_AddObjectToObject(cJSONObjectActualValue, "sequence");
            if (cJSONObjectActualValueSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectActualValueSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_VALUE).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }
        
        cJSON *cJSONObjectFallbackValue = cJSON_AddObjectToObject(cJSONObject, "fallback_value");
        if (cJSONObjectFallbackValue == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectFallbackValue, "inline", flowctrl.getReadoutAll(READOUT_TYPE_FALLBACKVALUE).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectFallbackValueSequence = cJSON_AddObjectToObject(cJSONObjectFallbackValue, "sequence");
            if (cJSONObjectFallbackValueSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectFallbackValueSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_FALLBACKVALUE).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectRawValue = cJSON_AddObjectToObject(cJSONObject, "raw_value");
        if (cJSONObjectRawValue == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectRawValue, "inline", flowctrl.getReadoutAll(READOUT_TYPE_RAWVALUE).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectRawValueSequence = cJSON_AddObjectToObject(cJSONObjectRawValue, "sequence");
            if (cJSONObjectRawValueSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectRawValueSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_RAWVALUE).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectValueStatus = cJSON_AddObjectToObject(cJSONObject, "value_status");
        if (cJSONObjectValueStatus == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectValueStatus, "inline", flowctrl.getReadoutAll(READOUT_TYPE_VALUE_STATUS).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectValueStatusSequence = cJSON_AddObjectToObject(cJSONObjectValueStatus, "sequence");
            if (cJSONObjectValueStatusSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectValueStatusSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_VALUE_STATUS).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectRatePerMin = cJSON_AddObjectToObject(cJSONObject, "rate_per_minute");
        if (cJSONObjectRatePerMin == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectRatePerMin, "inline", flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_MIN).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectRatePerMinSequence = cJSON_AddObjectToObject(cJSONObjectRatePerMin, "sequence");
            if (cJSONObjectRatePerMinSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectRatePerMinSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_RATE_PER_MIN).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        cJSON *cJSONObjectRatePerProcessing = cJSON_AddObjectToObject(cJSONObject, "rate_per_processing");
        if (cJSONObjectRatePerProcessing == NULL) {
            retVal = ESP_FAIL;
        }
        else {
            if (cJSON_AddStringToObject(cJSONObjectRatePerProcessing, "inline", flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_PROCESSING).c_str()) == NULL)
                retVal = ESP_FAIL;
            cJSON *cJSONObjectRatePerProcessingSequence = cJSON_AddObjectToObject(cJSONObjectRatePerProcessing, "sequence");
            if (cJSONObjectRatePerProcessingSequence == NULL) {
                retVal = ESP_FAIL;
            }
            else {
                for(int i = 0; i < flowctrl.getNumbersSize(); i++) {
                    if (cJSON_AddStringToObject(cJSONObjectRatePerProcessingSequence, flowctrl.getNumbersName(i).c_str(), 
                                                flowctrl.getNumbersValue(i, READOUT_TYPE_RATE_PER_PROCESSING).c_str()) == NULL)
                    retVal = ESP_FAIL;
                }
            }
        }

        if (cJSON_AddStringToObject(cJSONObject, "process_status", getProcessStatus().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_interval", to_stringWithPrecision(flowctrl.getProcessingInterval(),1).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_time", std::to_string(getFlowProcessingTime()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_state", flowctrl.getActStatusWithTime().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_error", flowctrl.getActFlowError() ? (FlowStateErrorsInRow < FLOWSTATE_ERRORS_IN_ROW_LIMIT ? 
                "-1" : "-2") : "0") == NULL) // 0: No error, -1 (E01): One error occured, -2 (E02): Three errors in a row
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "device_uptime", std::to_string(getUptime()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "cycle_counter", std::to_string(getFlowCycleCounter()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_rssi", std::to_string(get_WIFI_RSSI()).c_str()) == NULL)
            retVal = ESP_FAIL;

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 1024 + flowctrl.getNumbersSize() * 512, 1); // Print with predefined buffer, avoid dynamic allocations
        sReturnMessage = std::string(jsonString);
        cJSON_free(jsonString);  
        cJSON_Delete(cJSONObject);

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "application/json");

        if (retVal == ESP_OK)
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
        else
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Error while adding JSON elements");

        return retVal;
    }

    /* Legacy: Provide single data as text response */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (type.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else if (type.compare("number_sequences") == 0) {
        httpd_resp_sendstr(req, std::to_string(flowctrl.getNumbersSize()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("timestamp_processed") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_PROCESSED).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_TIMESTAMP_PROCESSED).c_str());
            return ESP_OK;  
        }    
    }
    else if (type.compare("timestamp_fallbackvalue") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_FALLBACKVALUE).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_TIMESTAMP_FALLBACKVALUE).c_str());
            return ESP_OK;  
        }  
    }
    else if (type.compare("actual_value") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_VALUE).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_VALUE).c_str());
            return ESP_OK;  
        }
    }
    else if (type.compare("fallback_value") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_FALLBACKVALUE).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_FALLBACKVALUE).c_str());
            return ESP_OK;  
        }   
    }
    else if (type.compare("raw_value") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_RAWVALUE).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_RAWVALUE).c_str());
            return ESP_OK;  
        }  
    }
    else if (type.compare("value_status") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_VALUE_STATUS).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_VALUE_STATUS).c_str());
            return ESP_OK;  
        }   
    }
    else if (type.compare("rate_per_minute") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_MIN).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_RATE_PER_MIN).c_str());
            return ESP_OK;  
        }
    }
    else if (type.compare("rate_per_processing") == 0) {
        if (number_sequence.empty()) {
            httpd_resp_sendstr(req, flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_PROCESSING).c_str());
            return ESP_OK;
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(number_sequence);

            if (positon < 0) {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Number sequence not found");
                return ESP_FAIL;
            }
            httpd_resp_sendstr(req, flowctrl.getNumbersValue(positon, READOUT_TYPE_RATE_PER_PROCESSING).c_str());
            return ESP_OK;  
        }   
    }
    else if (type.compare("process_status") == 0) {
        httpd_resp_sendstr(req, getProcessStatus().c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_interval") == 0) {
        httpd_resp_sendstr(req, to_stringWithPrecision(flowctrl.getProcessingInterval(),1).c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_time") == 0) {
        httpd_resp_sendstr(req, std::to_string(getFlowProcessingTime()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_state") == 0) {
        httpd_resp_sendstr(req, flowctrl.getActStatusWithTime().c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_error") == 0) {
        httpd_resp_sendstr(req, flowctrl.getActFlowError() ? (FlowStateErrorsInRow < FLOWSTATE_ERRORS_IN_ROW_LIMIT ? 
                "E01: Process error occured" : "E02: Multiple process errors in row") : "000: No process error");
        return ESP_OK;        
    }
    else if (type.compare("device_uptime") == 0) {
        httpd_resp_sendstr(req, std::to_string(getUptime()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("cycle_counter") == 0) {
        httpd_resp_sendstr(req, std::to_string(getFlowCycleCounter()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("wlan_rssi") == 0) {
        httpd_resp_sendstr(req, std::to_string(get_WIFI_RSSI()).c_str());
        return ESP_OK;        
    }
    else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E93: Parameter not found");
        return ESP_FAIL;  
    }
}


esp_err_t handler_recognition_details(httpd_req_t *req)
{
    const char* APIName = "recognition_details:v1"; // API name and version
    char _query[100];
    char _valuechar[30];    
    std::string type, zw;
    
    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {        
        if (httpd_query_key_value(_query, "type", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            type = std::string(_valuechar);
        }
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/html");

    if (type.compare("api_name") == 0) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }

    /*++++++++++++++++++++++++++++++++++++++++*/
    /* Page details */
    std::string txt = "<!DOCTYPE html><html lang=\"en\" xml:lang=\"en\"><head><meta charset=\"UTF-8\"><title>Recognition Details</title></head>\n";
    txt += "<body style=\"width:660px;max-width:660px;font-family:arial;padding:0px 10px;font-size:100%;-webkit-text-size-adjust:100%; text-size-adjust:100%;\">";
    txt += "<h2 style=\"font-size:1.5em;margin-block-start:0.0em;margin-block-end:0.2em;\">Recognition Details</h2>\n";
    txt += "<details id=\"desc_details\" style=\"font-size:16px;text-align:justify;margin-right:10px;\">\n";
    txt += "<summary><strong>CLICK HERE</strong> for more information</summary>\n";
    txt += "<p>On this page recognition details including the underlaying ROI image are visualized. "
            "<br><strong>Be aware: The visualized infos are representing the last fully completed image evaluation of a digitalization cycle.</strong></p>";
    txt += "<p>\"Raw Value\" represents the value which gets extracted and combined from all the single image results but without "
            "correction of any of the post-processing checks / alogrithms. The result after post-processing validation is represented with "
            "\"Value\". In the sections \"Digit ROI\" and \"Analog ROI\" all single \"raw results\" of the respective ROI images (digit styled ROI and "
            "analog styled ROI) are visualized separated per number sequence. The taken image which was used for processing (including the overlays "
            "to highlight the relevant areas) is visualized at the bottom of this page.</p>";
    txt += "</details><hr>\n";

    if (taskAutoFlowState < 3 || taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING) { // Display message if flow is not initialized or image processing active
        txt += "<h4>"
                "Image recognition details are only accessable if initialization is completed and no image evaluation is ongoing. "
                "Wait a few moments and refresh this page.</h4> Current state: " + flowctrl.getActStatus();
        httpd_resp_sendstr_chunk(req, txt.c_str());
    }
    else {
        /*++++++++++++++++++++++++++++++++++++++++*/
        /* Result */
        txt += "<h4 style=\"font-size:16px;background-color:lightgray;padding:5px;margin-top:40px;\">Result</h4>\n";
        txt += "<table style=\"width:660px;border-collapse:collapse;table-layout:fixed;\">";
        txt += "<tr><td style=\"font-weight:bold;width:40%;padding:3px 5px;text-align:left;vertical-align:middle;border:1px solid lightgrey\">Number Sequence</td>"
                "<td style=\"font-weight:bold;padding:3px 5px;text-align:left;vertical-align:middle;border:1px solid lightgrey\">Raw Value</td>"
                "<td style=\"font-weight:bold;padding:3px 5px;text-align:left;vertical-align:middle;border:1px solid lightgrey\">Actual Value</td></tr>";
        for (int i = 0; i < flowctrl.getNumbersSize(); ++i) {
            txt += "<tr><td style=\"padding:3px 5px; text-align:left;vertical-align:middle;border:1px solid lightgrey\">" +
                flowctrl.getNumbersName(i) + "</td><td style=\"padding:3px 5px;text-align:left;vertical-align:middle;border:1px solid lightgrey\">" +
                flowctrl.getReadout(true, false, i) + "</td><td style=\"padding:3px 5px;text-align:left;vertical-align:middle;border:1px solid lightgrey\">" +
                flowctrl.getReadout(false, true, i) + "</td></tr>";
        }
        txt += "</table>\n";
        httpd_resp_sendstr_chunk(req, txt.c_str());

        /*++++++++++++++++++++++++++++++++++++++++*/
        /* Digital ROI */
        txt = "<h4 style=\"font-size:16px;background-color:lightgray;padding:5px;\">Digit ROI</h4>\n";
        txt += "<table style=\"border-spacing:5px;\">\n";

        std::vector<HTMLInfo*> htmlinfo;
        htmlinfo = flowctrl.GetAllDigital();

        int sequence = -1;
        for (int i = 0; i < htmlinfo.size(); ++i) {
            if (htmlinfo[i]->position == 0) {     // New line when a new number sequence begins
                txt += "<tr><td style=\"font-weight:bold;vertical-align:bottom;\" colspan=\"3\">Number Sequence: " + htmlinfo[i]->name + "</td></tr>\n";
                txt += "<tr style=\"text-align:center;vertical-align:top;\">\n";
                sequence++;
            }

            if (flowctrl.GetTypeDigital() == Digital) {
                if (htmlinfo[i]->val == 10)
                    zw = "NaN";
                else
                    zw = std::to_string((int) htmlinfo[i]->val);
            }
            else {
                if (htmlinfo[i]->val >= 10.0) {
                    zw = "0.0";
                }
                else {
                    zw = to_stringWithPrecision(htmlinfo[i]->val, 1);
                }
            }

            if (htmlinfo[i]->val >= -0.1) // Only show image if result is set, otherwise text "No Image"
                txt += "<td style=\"width:150px;\"><h4 style=\"margin-block-start:0.5em;margin-block-end:0.0em;\">" +
                        zw + "</h4><p style=\"margin-block-start:0.5em;margin-block-end:1.33em;\"><img "
                        "style=\"max-width:" + to_stringWithPrecision(640/(flowctrl.getNumbersROISize(sequence, 1) + 1), 0) + "px\" src=\"/img_tmp/" +
                        htmlinfo[i]->filename_org + "\"></p></td>\n";
            else
                txt += "<td style=\"width:150px;\"><h4 style=\"margin-block-start:0.5em;margin-block-end:0.0em;\">" +
                        zw + "</h4><p style=\"margin-block-start:0.5em;margin-block-end:1.33em;\">No Image</p></td>\n";

            delete htmlinfo[i];
        }

        if (htmlinfo.size() == 0)
            txt += "<tr><td>Digit ROI processing deactivated</td>";

        htmlinfo.clear();

        txt += "</tr></table>\n";
        httpd_resp_sendstr_chunk(req, txt.c_str());

        /*++++++++++++++++++++++++++++++++++++++++*/
        /* Analog ROI */
        txt = "<h4 style=\"font-size:16px;background-color:lightgray;padding:5px;\">Analog ROI</h4>\n";
        txt += "<table style=\"border-spacing:5px;\">\n";

        sequence = -1;
        htmlinfo = flowctrl.GetAllAnalog();
        for (int i = 0; i < htmlinfo.size(); ++i) {
            if (htmlinfo[i]->position == 0) {     // New line when a new number sequence begins
                txt += "<tr><td style=\"font-weight:bold;vertical-align:bottom;\" colspan=\"3\">Number Sequence: " +
                        htmlinfo[i]->name + "</td></tr>\n";
                txt += "<tr style=\"text-align:center;vertical-align:top;\">\n";
                sequence++;
            }

            if (htmlinfo[i]->val >= 10.0) {
                    zw = "0.0";
            }
            else {
                zw = to_stringWithPrecision(htmlinfo[i]->val, 1);
            }

            if (htmlinfo[i]->val >= -0.1) // Only show image if result is set, otherwise text "No Image"
                txt += "<td style=\"width:150px;\"><h4 style=\"margin-block-start:0.5em;margin-block-end:0.0em;\">" +
                        zw + "</h4><p style=\"margin-block-start:0.5em;margin-block-end:1.33em;\"><img "
                        "style=\"max-width:" + to_stringWithPrecision(640/(flowctrl.getNumbersROISize(sequence, 2) + 1), 0) + "px\" src=\"/img_tmp/" +
                        htmlinfo[i]->filename_org + "\"></p></td>\n";
            else
                txt += "<td style=\"width:150px;\"><h4 style=\"margin-block-start:0.5em;margin-block-end:0.0em;\">" +
                        zw + "</h4><p style=\"margin-block-start:0.5em;margin-block-end:1.33em;\">No Image</p></td>\n";

            delete htmlinfo[i];
        }

        if (htmlinfo.size() == 0)
            txt += "<tr><td>Analog ROI processing deactivated</td>";

        htmlinfo.clear();

        txt += "</tr></table>\n";
        httpd_resp_sendstr_chunk(req, txt.c_str());

        /*++++++++++++++++++++++++++++++++++++++++*/
        /* Show ALG_ROI image */
        txt = "<h4 style=\"font-size:16px;background-color:lightgray;padding:5px;\">Processed Image (incl. Overlays)</h4>\n";
        txt += "<img src=\"/img_tmp/alg_roi.jpg\">\n";
        txt += "</body></html>\n";
        httpd_resp_sendstr_chunk(req, txt.c_str());
    }

    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}


void task_autodoFlow(void *pvParameter)
{
    int64_t fr_start = 0;
    time_t cycleStartTime = 0;
    bTaskAutoFlowCreated = true;

    while (true)
    {
        // FLOW INITIALIZATION - DELAYED
        // Delay flow initialization if reboot was triggered by software exception
        // Note: Init and logging of the event is handled already in "main.cpp"
        // ********************************************
        if (taskAutoFlowState == FLOW_TASK_STATE_INIT_DELAYED) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Process state: " + std::string(FLOW_INIT_DELAYED));
            flowctrl.setActStatus(std::string(FLOW_INIT_DELAYED));
            flowctrl.setActFlowError(true);
            // Right now, it's not possible to provide state via MQTT because mqtt service is not yet started

            vTaskDelay(60*5000 / portTICK_PERIOD_MS); // Wait 5 minutes to give time to do an OTA update or fetch the log 

            taskAutoFlowState = FLOW_TASK_STATE_INIT; // Continue to FLOW INIT
        }

        // FLOW INITIALIZATION
        // ********************************************
        else if (taskAutoFlowState == FLOW_TASK_STATE_INIT) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_INIT));
            flowctrl.setActStatus(std::string(FLOW_INIT));
            // Right now, it's not possible to provide state via MQTT because mqtt service is not yet started

            flowctrl.setActFlowError(false); // Reset existing process_error

            if (!doInit()) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Process state: " + std::string(FLOW_INIT_FAILED));
                flowctrl.setActStatus(std::string(FLOW_INIT_FAILED));
                flowctrl.setActFlowError(true);
                #ifdef ENABLE_MQTT
                if (getMQTTisConnected())
                    MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
                #endif //ENABLE_MQTT

                while (true) {                                      // Waiting for a REQUEST
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    if (reloadConfig) { // Possibility to manual retrigger a cycle with parameter reload when init is already failed
                        reloadConfig = false;
                        manualFlowStart = false; // parameter reload has higher prio
                        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger: Reload configuration");
                        taskAutoFlowState = FLOW_TASK_STATE_INIT;   // Repeat FLOW INIT
                        break;
                    }
                    else if (manualFlowStart) { // Possibility to manual retrigger a cycle with manual start when init is already failed
                        manualFlowStart = false;
                        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger: Start process (manual trigger)");
                        taskAutoFlowState = FLOW_TASK_STATE_INIT;   // Repeat FLOW INIT
                        break;
                    }
                }
            }
            else {
                flowctrl.setActFlowError(false);
                taskAutoFlowState = FLOW_TASK_STATE_SETUPMODE;      // Continue to test if SETUP is ACTIVE
            }
        }

        // SETUP MODE CHECK
        // ********************************************
        else if (taskAutoFlowState == FLOW_TASK_STATE_SETUPMODE) {

            if (isSetupModusActive())
            {
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_SETUP_MODE));
                flowctrl.setActStatus(std::string(FLOW_SETUP_MODE));
                #ifdef ENABLE_MQTT
                    MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
                #endif //ENABLE_MQTT

                //std::string zw_time = getCurrentTimeString(DEFAULT_TIME_FORMAT);
                //flowctrl.doFlowTakeImageOnly(zw_time);    // Start only ClassFlowTakeImage to capture images

                while (true) {                              // Waiting for a REQUEST
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    if (reloadConfig) {
                        reloadConfig = false;
                        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger: Reload configuration");
                        taskAutoFlowState = FLOW_TASK_STATE_INIT;       // Setup Mode done --> Do FLOW INIT
                        break;
                    }
                }
            }
            else {
                taskAutoFlowState = FLOW_TASK_STATE_IDLE_NO_AUTOSTART;  // Continue to test if AUTOSTART is TRUE
            }
        }

        // AUTOSTART CHECK
        // ********************************************      
        else if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_NO_AUTOSTART) {
    
            if (!flowctrl.isAutoStart(auto_interval)) {
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_IDLE_NO_AUTOSTART));
                flowctrl.setActStatus(std::string(FLOW_IDLE_NO_AUTOSTART));
                #ifdef ENABLE_MQTT
                    MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
                #endif //ENABLE_MQTT

                while (true) {                              // Waiting for a REQUEST
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    if (reloadConfig) {
                        reloadConfig = false;
                        manualFlowStart = false;    // Reload config has higher prio
                        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger: Reload configuration");
                        taskAutoFlowState = FLOW_TASK_STATE_INIT;           // Return to state "FLOW INIT"
                        break;
                    }
                    else if (manualFlowStart) { 
                        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Start process (manual trigger)");
                        taskAutoFlowState = FLOW_TASK_STATE_IMG_PROCESSING; // Start manual triggered single cycle of "FLOW PROCESSING"  
                        break;
                    }
                }   
            }
            else {
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Start process (automatic trigger)");
                taskAutoFlowState = FLOW_TASK_STATE_IMG_PROCESSING;         // Continue to state "FLOW PROCESSING"
            }
        }

        // IMAGE PROCESSING / EVALUATION
        // ********************************************     
        else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING) {       
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "----------------------------------------------------------------"); // Clear separation between runs
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Cycle #" + std::to_string(++cycleCounter) + " started"); 
            cycleStartTime = getUptime();
            fr_start = esp_timer_get_time();

            flowctrl.setActFlowError(false); // Reset process_error at prcoess start
                   
            if (flowctrl.doFlowImageEvaluation(getCurrentTimeString(DEFAULT_TIME_FORMAT))) {
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Image evaluation completed (" + 
                                    std::to_string(getUptime() - cycleStartTime) + "s)");
            }
            else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Image evaluation process error occured");
                flowctrl.setActFlowError(true);
            }

            taskAutoFlowState = FLOW_TASK_STATE_PUBLISH_DATA;               // Continue with TASKS after FLOW FINISHED
        }

        // PUBLISH DATA / RESULTS
        // ******************************************** 
        else if (taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA) {  

            if (!flowctrl.doFlowPublishData(getCurrentTimeString(DEFAULT_TIME_FORMAT))) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Publish data process error occured"); 
                flowctrl.setActFlowError(true);
            }
            taskAutoFlowState = FLOW_TASK_STATE_ADDITIONAL_TASKS;           // Continue with TASKS after FLOW FINISHED
        }

        // ADDITIONAL TASKS
        // Process further tasks after image is fully processed and results are published
        // ********************************************
        else if (taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) {
            // Post process handling (if neccessary)
            // ********************************************
            if (flowctrl.FlowStateEventOccured()) {

                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_POST_EVENT_HANDLING));
                flowctrl.setActStatus(std::string(FLOW_POST_EVENT_HANDLING));
                #ifdef ENABLE_MQTT
                    MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
                #endif

                #ifdef ENABLE_MQTT
                    // Provide flow error indicator to MQTT interface (error occured 3 times in a row)
                    FlowStateErrorsInRow++;
                    if (FlowStateErrorsInRow >= FLOWSTATE_ERRORS_IN_ROW_LIMIT) {
                        MQTTPublish(mqttServer_getMainTopic() + "/" + "process_error", "true", 1, false);
                    }
                #endif //ENABLE_MQTT
            
                flowctrl.PostProcessEventHandler();
                LogFile.RemoveOldDebugFiles();
            }
            else {
                FlowStateErrorsInRow = 0;
                #ifdef ENABLE_MQTT
                    MQTTPublish(mqttServer_getMainTopic() + "/" + "process_error", "false", 1, false);
                #endif //ENABLE_MQTT
            }

            // Additional tasks
            // ********************************************
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_ADDITIONAL_TASKS));
            flowctrl.setActStatus(std::string(FLOW_ADDITIONAL_TASKS));
            #ifdef ENABLE_MQTT
                MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
            #endif //ENABLE_MQTT

            // Cleanup outdated log and data files (retention policy)  
            LogFile.RemoveOldLogFile();
            LogFile.RemoveOldDataLog();
 
            // CPU Temp -> Logfile
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "CPU Temperature: " + std::to_string((int)temperatureRead()) + "°C");
            
            // WIFI Signal Strength (RSSI) -> Logfile
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "WIFI Signal (RSSI): " + std::to_string(get_WIFI_RSSI()) + "dBm");

            processingTime = (int)(getUptime() - cycleStartTime);
            // Cycle finished -> Logfile
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Cycle #" + std::to_string(cycleCounter) + 
                    " completed (" + std::to_string(processingTime) + "s)");
           
            // Check if time is synchronized (if NTP is configured)
            if (getUseNtp() && !getTimeIsSet()) {
                LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Time server is configured, but time is not yet set");
                StatusLED(TIME_CHECK, 1, false);
            }

            // WIFI roaming handling (if activated)
            // ********************************************
            // Trigger client triggered roaming query
            #if (defined WLAN_USE_MESH_ROAMING && defined WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES)
                wifiRoamingQuery();
            #endif
        
            // Scan channels and check if an AP with better RSSI is available, then disconnect and try to reconnect to AP with better RSSI
            // NOTE: Keep this at the end of this state, because scan is done in blocking mode and this takes ca. 1,5 - 2s.
            #ifdef WLAN_USE_ROAMING_BY_SCANNING
                wifiRoamByScanning();
            #endif

            // Check if triggerd reload config or manually triggered single cycle
            // ********************************************    
            if (taskAutoFlowState == FLOW_TASK_STATE_INIT) {
                reloadConfig = false; // reload by post process event handler has higher prio
                manualFlowStart = false; // Reload config has higher prio
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "PostProcessEventHandler trigger: Reload configuration");
            }
            else if (reloadConfig) {
                reloadConfig = false;
                manualFlowStart = false; // Reload config has higher prio
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Manual trigger: Reload configuration");
                taskAutoFlowState = FLOW_TASK_STATE_INIT;                   // Return to state "FLOW INIT"
            }
            else if (manualFlowStart) {
                manualFlowStart = false;
                if (flowctrl.isAutoStart()) {
                    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Start process (manual trigger)");
                    taskAutoFlowState = FLOW_TASK_STATE_IMG_PROCESSING;         // Continue with next "FLOW PROCESSING" cycle"
                }
                else {
                    taskAutoFlowState = FLOW_TASK_STATE_IDLE_NO_AUTOSTART;      // Return to state "Idle (NO AUTOSTART)"
                }
            }
            else {
                taskAutoFlowState = FLOW_TASK_STATE_IDLE_AUTOSTART;         // Continue to state "Idle (AUTOSTART / WAITING STATE)"
            }
        }

        // IDLE / WAIT STATE
        // "Wait state" until autotimer is elapsed to restart next cycle
        // ********************************************
        else if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_IDLE_AUTOSTART));
            flowctrl.setActStatus(std::string(FLOW_IDLE_AUTOSTART));
            #ifdef ENABLE_MQTT
                MQTTPublish(mqttServer_getMainTopic() + "/" + "status", flowctrl.getActStatus(), 1, false);
            #endif //ENABLE_MQTT

            int64_t fr_delta_ms = (esp_timer_get_time() - fr_start) / 1000;
            if (auto_interval > fr_delta_ms)
            {
                const TickType_t xDelay = (auto_interval - fr_delta_ms)  / portTICK_PERIOD_MS;
                ESP_LOGD(TAG, "Autoflow: sleep for: %ldms", (long) xDelay * CONFIG_FREERTOS_HZ/portTICK_PERIOD_MS);
                vTaskDelay(xDelay);   
            }

            // Check if reload config is triggered by REST API
            // ********************************************    
            if (reloadConfig) {                     
                reloadConfig = false;
                manualFlowStart = false; // Reload config has higher prio
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger: Reload configuration");
                taskAutoFlowState = FLOW_TASK_STATE_INIT;               // Return to state "FLOW INIT"
            }
            else if (manualFlowStart) {
                manualFlowStart = false;
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Start process (manual trigger)");
                taskAutoFlowState = FLOW_TASK_STATE_IMG_PROCESSING;     // Continue with next "FLOW PROCESSING" cycle"
            }
            else {
                taskAutoFlowState = FLOW_TASK_STATE_IMG_PROCESSING;     // Continue with next "FLOW PROCESSING" cycle
            }
        }

        // INVALID STATE
        // ********************************************
        else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "taskAutoFlowState: Invalid state called. Programming error");
            flowctrl.setActStatus(std::string(FLOW_INVALID_STATE));
        }
    }

    // Delete task if it exits from the loop above
    // ********************************************
    vTaskDelete(NULL);
    xHandletask_autodoFlow = NULL;
}


void CreateMainFlowTask()
{
    #ifdef DEBUG_DETAIL_ON      
            LogFile.WriteHeapInfo("CreateFlowTask: start");
    #endif

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Process state: " + std::string(FLOW_CREATE_FLOW_TASK));
    flowctrl.setActStatus(std::string(FLOW_CREATE_FLOW_TASK));

    BaseType_t xReturned = xTaskCreatePinnedToCore(&task_autodoFlow, "task_autodoFlow", 12 * 1024, NULL, tskIDLE_PRIORITY+2, &xHandletask_autodoFlow, 0);
    if( xReturned != pdPASS ) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create task_autodoFlow");
        LogFile.WriteHeapInfo("CreateFlowTask: Failed to create task");
        flowctrl.setActStatus(std::string(FLOW_FLOW_TASK_FAILED));
        flowctrl.setActFlowError(true);
    }

    #ifdef DEBUG_DETAIL_ON      
            LogFile.WriteHeapInfo("CreateFlowTask: end");
    #endif
}


void register_server_main_flow_task_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");
    
    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;

    camuri.uri       = "/cycle_start";
    camuri.handler   = handler_cycle_start;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/reload_config";
    camuri.handler   = handler_reload_config;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/set_fallbackvalue";
    camuri.handler   = handler_fallbackvalue;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/editflow";
    camuri.handler   = handler_editflow;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/process_data";
    camuri.handler   = handler_process_data;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/recognition_details";
    camuri.handler   = handler_recognition_details;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);
}
