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


int getFlowCycleCounter() 
{
    return cycleCounter;
}


void setTaskAutoFlowState(uint8_t _value) 
{
    taskAutoFlowState = _value;
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
    if (!Camera.getCameraInitSuccessful()) { 
        Camera.PowerResetCamera();
        esp_err_t camStatus = Camera.InitCam(); 

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
    else if (taskAutoFlowState == FLOW_TASK_STATE_INIT_DELAYED) 
    {
        const std::string zw = "002: Abort waiting delay and continue with flow initialization (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state.      
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART) 
    {
        const std::string zw = "003: Abort waiting delay, reload config and redo flow initialization (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        reloadConfig = true;
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state.
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING || 
             taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA ||
             taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Reload config and redo flow initialization got scheduled ");
        const std::string zw = "004: Reload config and redo flow initialization got scheduled (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        reloadConfig = true;
    }
    else 
    {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Reload configuration not possible. No flow task. Request rejected");
        const std::string zw = "E90: Reload config not possible. No flow task. Request rejected (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
    }
    return ESP_OK;
}


esp_err_t handler_flow_start(httpd_req_t *req) 
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_NO_AUTOSTART || 
        taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART || 
        flowctrl.getActStatus() == FLOW_INIT_FAILED) // Possibility to manual retrigger a cycle when init is already failed
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flow start triggered by REST API");
        const std::string zw = "001: Flow start triggered by REST API (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        manualFlowStart = true;

        if (taskAutoFlowState == FLOW_TASK_STATE_IDLE_AUTOSTART)
            xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_IMG_PROCESSING || 
             taskAutoFlowState == FLOW_TASK_STATE_PUBLISH_DATA ||
             taskAutoFlowState == FLOW_TASK_STATE_ADDITIONAL_TASKS) 
    {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flow start triggered by REST API got scheduled");
        const std::string zw = "002: Flow start triggered by REST API got scheduled (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());

        manualFlowStart = true;
    }
    else if (taskAutoFlowState == FLOW_TASK_STATE_INIT_DELAYED) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Flow start triggered by REST API (abort Initialization (delayed)");
        const std::string zw = "003: Flow start triggered by REST API abort initialization delay (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
        xTaskAbortDelay(xHandletask_autodoFlow); // Delay will be aborted if task is in blocked (waiting) state
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Flow start triggered by REST API. Flow not initialized. Request rejected");
        const std::string zw = "E90: Flow start triggered by REST API. Flow not initialized. Request rejected (" + getCurrentTimeString("%H:%M:%S") + ")";
        httpd_resp_send(req, zw.c_str(), zw.length());
    }

    return ESP_OK;
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


esp_err_t handler_json(httpd_req_t *req)
{
    if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");

    std::string zw = flowctrl.getJSON();
    if (zw.length() > 0) {
        httpd_resp_send(req, zw.c_str(), zw.length());
    }
    else {
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}


esp_err_t handler_process_data(httpd_req_t *req)
{
    esp_err_t retVal = ESP_OK;

    if (!bTaskAutoFlowCreated) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    cJSON *cJSONObject = cJSON_CreateObject();
    
    if (cJSONObject == NULL) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Error, JSON object cannot be created");
        return ESP_FAIL;
    }

    if (cJSON_AddStringToObject(cJSONObject, "api_name", "process_data") == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "timestamp_processed", flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_PROCESSED).c_str()) == NULL)
        retVal = ESP_FAIL; 
    if (cJSON_AddStringToObject(cJSONObject, "timestamp_fallbackvalue", flowctrl.getReadoutAll(READOUT_TYPE_TIMESTAMP_FALLBACKVALUE).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "actual_value", flowctrl.getReadoutAll(READOUT_TYPE_VALUE).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "fallback_value", flowctrl.getReadoutAll(READOUT_TYPE_FALLBACKVALUE).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "raw_value", flowctrl.getReadoutAll(READOUT_TYPE_RAWVALUE).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "value_status", flowctrl.getReadoutAll(READOUT_TYPE_VALUE_STATUS).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "rate_per_min", flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_MIN).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "rate_per_processing", flowctrl.getReadoutAll(READOUT_TYPE_RATE_PER_PROCESSING).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "process_state", flowctrl.getActStatusWithTime().c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "process_error", std::to_string(flowctrl.getActFlowError()).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "temperature", std::to_string((int)temperatureRead()).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "rssi", std::to_string(get_WIFI_RSSI()).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "uptime", getFormatedUptime(false).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(cJSONObject, "cycle_counter", std::to_string(getFlowCycleCounter()).c_str()) == NULL)
        retVal = ESP_FAIL;

    char *jsonString = cJSON_PrintBuffered(cJSONObject, 1024, 1); // Print with predefined buffer of 1024 bytes, avoid dynamic allocations
    std::string sReturnMessage = std::string(jsonString);
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


esp_err_t handler_value(httpd_req_t *req)
{
    bool _fullInfo = false;
    bool _singleInfo = false;
    bool _numberspecific = false;
    std::string _type = "";
    std::string zw;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo =
        "00: Handler usage:<br>"
        "1. Return data from all number sequences:<br>"
        " - Actual Value:   /value?all=true&type=value<br>"
        " - Fallback Value: /value?all=true&type=fallback<br>"
        " - Raw Value:      /value?all=true&type=raw<br>"
        " - Value Status:   /value?all=true&type=status<br><br>"
        "2. Return data from a specific number sequence with e.g. name \"main\":<br>"
        " - Actual Value:   /value?all=true&type=value&numbersname=main<br>"
        " - Raw Value:      /value?all=true&type=raw&numbersname=main<br>"
        " - Fallback Value: /value?all=true&type=fallback&numbersname=main<br>"
        " - Value Status:   /value?all=true&type=status&numbersname=main<br><br>"
        "3. Retrieve WebUI recognition page content, use /value?full=true<br>";

    // Default return error message when no return is programmed
    std::string sReturnMessage = "E90: Uninitialized";

    char _query[100];
    char _value[10];
    char _numbersname[50];

    if (httpd_req_get_url_query_str(req, _query, 100) == ESP_OK) {
        //ESP_LOGD(TAG, "Query: %s", _query);
        if (httpd_query_key_value(_query, "all", _value, 10) == ESP_OK) {
            #ifdef DEBUG_DETAIL_ON
                ESP_LOGD(TAG, "all found: %s", _value);
            #endif
            _singleInfo = true;
            _fullInfo = false;
        }

        if (httpd_query_key_value(_query, "type", _value, 10) == ESP_OK) {
            //ESP_LOGD(TAG, "type found: %s", _value);
            _type = std::string(_value);
        }

        if (httpd_query_key_value(_query, "numbersname", _numbersname, 50) == ESP_OK) {
            //ESP_LOGD(TAG, "numbersname found: %s", _numbersname);
            _numberspecific = true;
        }

        if (httpd_query_key_value(_query, "full", _value, 10) == ESP_OK) {
            //ESP_LOGD(TAG, "full found: %s", _value);
            _fullInfo = true;
            _singleInfo = false;
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_send(req, RESTUsageInfo.c_str(), RESTUsageInfo.length());
        return ESP_OK;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    /* number sequence results */
    /**********************************************/
    if (_singleInfo) {
        if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E92: Request rejected, flow not initialized");
            return ESP_FAIL;
        }
        httpd_resp_set_type(req, "text/plain");

        if (!_numberspecific) {
            if (_type == "value")
                zw = flowctrl.getReadoutAll(READOUT_TYPE_VALUE);
            else if (_type == "fallback")
                zw = flowctrl.getReadoutAll(READOUT_TYPE_FALLBACKVALUE);
            else if (_type == "raw")
                zw = flowctrl.getReadoutAll(READOUT_TYPE_RAWVALUE);
            else if (_type == "status")
                zw = flowctrl.getReadoutAll(READOUT_TYPE_VALUE_STATUS);
            else {
                sReturnMessage = "E92: Type not found";
                httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
                return ESP_ERR_NOT_FOUND;
            }
        }
        else {
            int positon = flowctrl.getNumbersNamePosition(std::string(_numbersname));

            if (positon < 0) {
                sReturnMessage = "E93: Numbersname not found";
                httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
                return ESP_ERR_NOT_FOUND;
            }

            if (_type == "value")
                zw = flowctrl.getNumbersValue(positon, READOUT_TYPE_VALUE);
            else if (_type == "fallback")
                zw = flowctrl.getNumbersValue(positon, READOUT_TYPE_FALLBACKVALUE);
            else if (_type == "raw")
                zw = flowctrl.getNumbersValue(positon, READOUT_TYPE_RAWVALUE);
            else if (_type == "status")
                zw = flowctrl.getNumbersValue(positon, READOUT_TYPE_VALUE_STATUS);
            else {
                sReturnMessage = "E92: Type not found";
                httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
                return ESP_ERR_NOT_FOUND;
            }
        }

        ESP_LOGD(TAG, "TYPE: %s, RESULT: %s", _type.c_str(), zw.c_str());

        httpd_resp_send(req, zw.c_str(), zw.length());
        return ESP_OK;
    }

    /* WebUI - Recognition page */
    /**********************************************/
    else if (_fullInfo) {
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
    }
    else {
        sReturnMessage = "E91: Request incomplete<br> "
                            "Call /value to show REST API usage info and/or check documentation";
        httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
        return ESP_ERR_NOT_FOUND;
    }

    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}


esp_err_t handler_editflow(httpd_req_t *req)
{
    char _query[200];
    char _valuechar[30];
    std::string _task;

    if (httpd_req_get_url_query_str(req, _query, 200) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, 30) == ESP_OK) {
            //ESP_LOGD(TAG, "task is found: %s", _valuechar);
            _task = std::string(_valuechar);
        }
    }  

    if (_task.compare("namenumbers") == 0) {
        //ESP_LOGD(TAG, "Get NUMBER list");
        return get_numbers_file_handler(req);
    }

    if (_task.compare("data") == 0) {
        //ESP_LOGD(TAG, "Get data list");
        return get_data_file_handler(req);
    }

    if (_task.compare("tflite") == 0) {
        //ESP_LOGD(TAG, "Get tflite list");
        return get_tflite_file_handler(req);
    }

    if (_task.compare("copy") == 0) {
        std::string in, out, zw;

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
        zw = "Copy Done";
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, zw.c_str(), zw.length()); 
    }

    if (_task.compare("cutref") == 0) {
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

        zw = "CutImage Done";
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, zw.c_str(), zw.length()); 
    }

    if (_task.compare("test_take") == 0) {
        if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
            return ESP_FAIL;
        }
        
        std::string _host = "";
        std::string _bri = "";
        std::string _con = "";
        std::string _sat = "";
        std::string _int = "";
        int bri = -100;
        int sat = -100;
        int con = -100;
        int intens = -100;

        if (httpd_query_key_value(_query, "host", _valuechar, 30) == ESP_OK) {
            _host = std::string(_valuechar);
        }
        if (httpd_query_key_value(_query, "int", _valuechar, 30) == ESP_OK) {
            _int = std::string(_valuechar);
            intens = stoi(_int);
        }
        if (httpd_query_key_value(_query, "bri", _valuechar, 30) == ESP_OK) {
            _bri = std::string(_valuechar);
            bri = stoi(_bri);
        }
        if (httpd_query_key_value(_query, "con", _valuechar, 30) == ESP_OK) {
            _con = std::string(_valuechar);
            con = stoi(_con);
        }
        if (httpd_query_key_value(_query, "sat", _valuechar, 30) == ESP_OK) {
            _sat = std::string(_valuechar);
            sat = stoi(_sat);
        }


        //ESP_LOGD(TAG, "Parameter host: %s", _host.c_str());
        //std::string zwzw = "Do " + _task + " start\n"; ESP_LOGD(TAG, zwzw.c_str());
        Camera.SetBrightnessContrastSaturation(bri, con, sat);
        Camera.SetLEDIntensity(intens);
        ESP_LOGD(TAG, "test_take - vor TakeImage");
        std::string zw = flowctrl.doSingleStep("[TakeImage]", _host);
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, zw.c_str(), zw.length()); 
    } 


    if (_task.compare("test_align") == 0) {
        if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
            return ESP_FAIL;
        }
        std::string _host = "";
        if (httpd_query_key_value(_query, "host", _valuechar, 30) == ESP_OK) {
            _host = std::string(_valuechar);
        }
        //ESP_LOGD(TAG, "Parameter host: %s", _host.c_str());
        //std::string zwzw = "Do " + _task + " start\n"; ESP_LOGD(TAG, zwzw.c_str());
        std::string zw = flowctrl.doSingleStep("[Alignment]", _host);
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, zw.c_str(), zw.length()); 
    }

    return ESP_OK;
}


esp_err_t handler_statusflow(httpd_req_t *req)
{
    if (!bTaskAutoFlowCreated) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E90: Request rejected, flow not initialized");
        return ESP_FAIL;
    }


    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    std::string zw = flowctrl.getActStatusWithTime();
    httpd_resp_send(req, zw.c_str(), zw.length());
    return ESP_OK;
}


esp_err_t handler_processerror(httpd_req_t *req)
{
    if (taskAutoFlowState <= FLOW_TASK_STATE_INIT) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E92: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (flowctrl.getActFlowError()) {
        if (FlowStateErrorsInRow < FLOWSTATE_ERRORS_IN_ROW_LIMIT)
            httpd_resp_send(req, "E90: Process error occured", HTTPD_RESP_USE_STRLEN);
        else
            httpd_resp_send(req, "E91: Multiple process errors in row", HTTPD_RESP_USE_STRLEN);
    }
    else {
        httpd_resp_send(req, "000: No process error", HTTPD_RESP_USE_STRLEN);
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
        "- To retrieve actual Fallback Value, please provide only a numbersname, e.g. /set_fallbackvalue?numbers=main<br>"
        "- To set Fallback Value to a new value, please provide a numbersname and a value, e.g. /set_fallbackvalue?numbers=main&value=1234.5678<br>"
        "NOTE:<br>"
        "value >= 0.0: Set Fallback Value to provided value<br>"
        "value <  0.0: Set Fallback Value to actual RAW value (as long RAW value is a valid number, without N)";

    // Default return error message when no return is programmed
    std::string sReturnMessage = "E90: Uninitialized";

    char _query[100];
    char _numbersname[50] = "default";
    char _value[20] = "";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (httpd_req_get_url_query_str(req, _query, 100) == ESP_OK) {   
        //ESP_LOGD(TAG, "Query: %s", _query);

        if (httpd_query_key_value(_query, "numbers", _numbersname, 50) != ESP_OK) { // If request is incomplete
            sReturnMessage = "E91: Query parameter incomplete or not valid!<br> "
                             "Call /set_fallbackvalue to show REST API usage info and/or check documentation";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL; 
        }

        if (httpd_query_key_value(_query, "value", _value, 20) == ESP_OK) { 
            //ESP_LOGD(TAG, "Value: %s", _value);
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_send(req, RESTUsageInfo.c_str(), RESTUsageInfo.length());
        return ESP_OK; 
    }   

    if (strlen(_value) == 0) { // If no value is povided --> return actual FallbackValue
        sReturnMessage = flowctrl.GetFallbackValue(std::string(_numbersname));

        if (sReturnMessage.empty()) {
            sReturnMessage = "E92: Numbers name not found";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL;
        }
    }
    else {
        // New value is positive: Set FallbackValue to provided value and return value
        // New value is negative and actual RAW value is a valid number: Set FallbackValue to RAW value and return value
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "REST API handler_fallbackvalue called: numbersname: " + std::string(_numbersname) + 
                                                ", value: " + std::string(_value));
        if (!flowctrl.UpdateFallbackValue(_value, _numbersname)) {
            sReturnMessage = "E93: Update request rejected. Please check device logs for more details";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());  
            return ESP_FAIL;
        }

        sReturnMessage = flowctrl.GetFallbackValue(std::string(_numbersname));

        if (sReturnMessage.empty()) {
            sReturnMessage = "E94: Numbers name not found";
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_FAIL;
        }
    }

    httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
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
            cycleStartTime = getUpTime();
            fr_start = esp_timer_get_time();

            flowctrl.setActFlowError(false); // Reset process_error at prcoess start
                   
            if (flowctrl.doFlowImageEvaluation(getCurrentTimeString(DEFAULT_TIME_FORMAT))) {
                LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Image evaluation completed (" + 
                                    std::to_string(getUpTime() - cycleStartTime) + "s)");
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


            // Cycle finished -> Logfile
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Cycle #" + std::to_string(cycleCounter) + 
                    " completed (" + std::to_string(getUpTime() - cycleStartTime) + "s)");
           
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

    camuri.uri       = "/reload_config";
    camuri.handler   = handler_reload_config;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/process_data";
    camuri.handler   = handler_process_data;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/set_fallbackvalue";
    camuri.handler   = handler_fallbackvalue;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/flow_start";
    camuri.handler   = handler_flow_start;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/statusflow";
    camuri.handler   = handler_statusflow;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/process_error";
    camuri.handler   = handler_processerror;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/editflow";
    camuri.handler   = handler_editflow;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);   

    camuri.uri       = "/value";
    camuri.handler   = handler_value;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/json";
    camuri.handler   = handler_json;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);
}
