#include "server_main.h"

#include <string>

#include "server_help.h"
#include "ClassLogFile.h"

#include "time_sntp.h"

#include "connect_wlan.h"
#include "read_wlanini.h"
#include "interface_mqtt.h"

#include "version.h"

#include "esp_wifi.h"
#include "esp_private/esp_clk.h"
#include <netdb.h>

#include "MainFlowControl.h"
#include "ClassFlowInfluxDB.h"
#include "ClassFlowInfluxDBv2.h"
#include "ClassControllCamera.h"

#include <stdio.h>

#include "Helper.h"
#include "system_info.h"
#include "cJSON.h"

httpd_handle_t server = NULL;
extern std::string deviceStartTimestamp;

static const char *TAG = "MAIN SERVER";


esp_err_t handler_get_info(httpd_req_t *req)
{
    const char* APIName = "info:v2"; // API name and version
    char _query[200];
    char _valuechar[30];    
    std::string _task;

    if (httpd_req_get_url_query_str(req, _query, 200) == ESP_OK) {
        //ESP_LOGD(TAG, "Query: %s", _query);
        
        if (httpd_query_key_value(_query, "type", _valuechar, 30) == ESP_OK) {
            //ESP_LOGD(TAG, "type is found: %s", _valuechar);
            _task = std::string(_valuechar);
        }
    }
    else { // default - no parameter set: send data as JSON
        esp_err_t retVal = ESP_OK;
        std::string sReturnMessage;
        cJSON *cJSONObject = cJSON_CreateObject();
            
        if (cJSONObject == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E90: Error, JSON object cannot be created");
            return ESP_FAIL;
        }

        if (cJSON_AddStringToObject(cJSONObject, "api_name", APIName) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_status", getProcessStatus().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "process_interval", to_stringWithPrecision(flowctrl.getProcessingInterval(),1).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "cycle_counter", std::to_string(getFlowCycleCounter()).c_str()) == NULL)
            retVal = ESP_FAIL;      
        if (cJSON_AddStringToObject(cJSONObject, "datalogging_sdcard_status", LogFile.GetDataLogToSD() ? "Enabled" : "Disabled") == NULL)
            retVal = ESP_FAIL;
        
        #ifdef ENABLE_MQTT
        if (cJSON_AddStringToObject(cJSONObject, "mqtt_status", getMQTTisEnabled() ? (getMQTTisConnected() ? "Connected" : "Disconnected") : "Disabled") == NULL)
            retVal = ESP_FAIL;
        #else
        if (cJSON_AddStringToObject(cJSONObject, "mqtt_status", "E01: Service not compiled (#define ENABLE_MQTT)") == NULL)
            retVal = ESP_FAIL;
        #endif

        #ifdef ENABLE_INFLUXDB
        ClassFlowInfluxDB* influxdb = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDB"));
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv1_status", influxdb ? "Enabled" : "Disabled") == NULL)
            retVal = ESP_FAIL;
        
        ClassFlowInfluxDBv2* influxdbv2 = (ClassFlowInfluxDBv2*)(flowctrl.getFlowClass("ClassFlowInfluxDBv2"));
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv2_status", influxdbv2 ? "Enabled" : "Disabled") == NULL)
            retVal = ESP_FAIL;
        #else
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv1_status", "E02: Service not compiled (#define ENABLE_INFLUXDB)") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv2_status", "E02: Service not compiled (#define ENABLE_INFLUXDB)") == NULL)
            retVal = ESP_FAIL;
        #endif

        if (cJSON_AddStringToObject(cJSONObject, "ntp_syncstatus", 
                                        getUseNtp() ? (getTimeWasSetOnce() ? "Synchronized" : "Sync In Progress") : "Disabled") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "current_time", getCurrentTimeString(TIME_FORMAT_OUTPUT).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "device_start_time", deviceStartTimestamp.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "device_uptime", getFormatedUptime(false).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_status", getWIFIisConnected() ? "Connected" : "Disconnected") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_ssid", getSSID().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_rssi", std::to_string(get_WIFI_RSSI()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "mac_address", getMac().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "network_config", getDHCPUsage() ? "DHCP" : "Manual") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "ipv4_address", getIPAddress().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "netmask_address", getNetmaskAddress().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "gateway_address", getGatewayAddress().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "dns_address", getDNSAddress().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "hostname", getHostname().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_model", getChipModel().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_cores", std::to_string(getChipCoreCount()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_revision", getChipRevision().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_frequency", std::to_string(esp_clk_cpu_freq()/1000000).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_temp", std::to_string((int)temperatureRead()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "camera_type", Camera.getCamType().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "camera_frequency", std::to_string(Camera.getCamFrequencyMhz()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_name", getSDCardName().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_manufacturer", getSDCardManufacturer().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_capacity", getSDCardCapacity().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_sector_size", getSDCardSectorSize().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_partition_alloc_size", getSDCardPartitionAllocationSize().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_partition_size", getSDCardPartitionSize().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_partition_free", getSDCardFreePartitionSpace().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "git_branch", libfive_git_branch()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "git_tag", libfive_git_version()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "git_revision", libfive_git_revision()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "firmware_version", getFwVersion().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "html_version", getHTMLversion().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "build_time", build_time()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "idf_version", getIDFVersion().c_str()) == NULL)
            retVal = ESP_FAIL;

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 1536, 1); // Print to predefined buffer, avoid dynamic allocations
        sReturnMessage = std::string(jsonString);
        cJSON_free(jsonString);  
        cJSON_Delete(cJSONObject);

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "application/json");

        if (retVal == ESP_OK) {
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_OK;
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Error while adding JSON elements");
            return ESP_FAIL;
        }
    }

    /* Legacy: Provide single data as text response */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (_task.compare("APIName") == 0)
    {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else if (_task.compare("ProcessStatus") == 0)
    {
        httpd_resp_sendstr(req, getProcessStatus().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ProcessInterval") == 0)
    {
        httpd_resp_sendstr(req, to_stringWithPrecision(flowctrl.getProcessingInterval(),1).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("CycleCounter") == 0)
    {
        httpd_resp_sendstr(req, std::to_string(getFlowCycleCounter()).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("DataLoggingSDCardStatus") == 0)
    {
        httpd_resp_sendstr(req, LogFile.GetDataLogToSD() ? "Enabled" : "Disabled");
        return ESP_OK;        
    }
    
    #ifdef ENABLE_MQTT
    else if (_task.compare("MQTTStatus") == 0)
    {
        httpd_resp_sendstr(req, getMQTTisEnabled() ? (getMQTTisConnected() ? "Connected" : "Disconnected") : "Disabled");
        return ESP_OK;        
    }
    #else
    else if (_task.compare("MQTTStatus") == 0)
    {
        httpd_resp_sendstr(req, "E01: Service not compiled (#define ENABLE_MQTT)");
        return ESP_OK;        
    }
    #endif

    #ifdef ENABLE_INFLUXDB
    else if (_task.compare("InfluxDBv1Status") == 0)
    {
        ClassFlowInfluxDB* influxdb = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDB"));
        httpd_resp_sendstr(req, influxdb ? "Enabled" : "Disabled");
        return ESP_OK;        
    }
    else if (_task.compare("InfluxDBv2Status") == 0)
    {
        ClassFlowInfluxDB* influxdbv2 = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDBv2"));
        httpd_resp_sendstr(req, influxdbv2 ? "Enabled" : "Disabled");
        return ESP_OK;        
    }
    #else
    else if (_task.compare("InfluxDBv1Status") == 0)
    {
        httpd_resp_sendstr(req, "E02: Service not compiled (#define ENABLE_INFLUXDB)");
        return ESP_OK;        
    }
    else if (_task.compare("InfluxDBv2Status") == 0)
    {
        httpd_resp_sendstr(req, "E02: Service not compiled (#define ENABLE_INFLUXDB)");
        return ESP_OK;        
    }
    #endif

    else if (_task.compare("NTPSyncStatus") == 0)
    {
        httpd_resp_sendstr(req, getUseNtp() ? (getTimeWasSetOnce() ? "Synchronized" : "In Progress") : "Disabled");
        return ESP_OK;        
    }
    else if (_task.compare("DeviceStartTime") == 0)
    {
        httpd_resp_sendstr(req, deviceStartTimestamp.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("DeviceUptime") == 0)
    {
        httpd_resp_sendstr(req, getFormatedUptime(false).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("WLANStatus") == 0)
    {
        httpd_resp_sendstr(req, getWIFIisConnected() ? "Connected" : "Disconnected");
        return ESP_OK;        
    }
    else if (_task.compare("WlanSSID") == 0)
    {
        httpd_resp_sendstr(req, getSSID().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("WlanRSSI") == 0)
    {
        httpd_resp_sendstr(req, std::to_string(get_WIFI_RSSI()).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("MACAddress") == 0)
    {
        httpd_resp_sendstr(req, getMac().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("NetworkConfig") == 0)
    {
        httpd_resp_sendstr(req, getDHCPUsage() ? "DHCP" : "Manual");
        return ESP_OK;        
    }
    else if (_task.compare("IPv4Address") == 0)
    {
        httpd_resp_sendstr(req, getIPAddress().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("NetmaskAddress") == 0)
    {
        httpd_resp_sendstr(req, getNetmaskAddress().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("GatewayAddress") == 0)
    {
        httpd_resp_sendstr(req, getGatewayAddress().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("DNSAddress") == 0)
    {
        httpd_resp_sendstr(req, getDNSAddress().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("Hostname") == 0)
    {
        httpd_resp_sendstr(req, getHostname().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ChipModel") == 0)
    {
        httpd_resp_sendstr(req, getChipModel().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ChipCores") == 0)
    {
        httpd_resp_sendstr(req, std::to_string(getChipCoreCount()).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ChipRevision") == 0)
    {
        httpd_resp_sendstr(req, getChipRevision().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ChipFrequency") == 0)
    {
        httpd_resp_sendstr(req, std::to_string(esp_clk_cpu_freq()/1000000).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("ChipTemp") == 0)
    {
        httpd_resp_sendstr(req, std::to_string((int)temperatureRead()).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("CameraType") == 0)
    {
        httpd_resp_sendstr(req, Camera.getCamType().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("CameraFrequency") == 0)
    {
        httpd_resp_sendstr(req, std::to_string(Camera.getCamFrequencyMhz()).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDName") == 0)
    {
        httpd_resp_sendstr(req, getSDCardName().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDManufacturer") == 0)
    {
        httpd_resp_sendstr(req, getSDCardManufacturer().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCapacity") == 0)
    {
        httpd_resp_sendstr(req, getSDCardCapacity().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDSectorSize") == 0)
    {
        httpd_resp_sendstr(req, getSDCardSectorSize().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDPartitionAllocSize") == 0)
    {
        httpd_resp_sendstr(req, getSDCardPartitionAllocationSize().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDPartitionSize") == 0)
    {
        httpd_resp_sendstr(req, getSDCardPartitionSize().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDPartitionFree") == 0)
    {
        httpd_resp_sendstr(req, getSDCardFreePartitionSpace().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("GitBranch") == 0)
    {
        httpd_resp_sendstr(req, libfive_git_branch());
        return ESP_OK;        
    }
    else if (_task.compare("GitTag") == 0)
    {
        httpd_resp_sendstr(req, libfive_git_version());
        return ESP_OK;        
    }
    else if (_task.compare("GitRevision") == 0)
    {
        httpd_resp_sendstr(req, libfive_git_revision());
        return ESP_OK;        
    }
    else if (_task.compare("FirmwareVersion") == 0)
    {
        httpd_resp_sendstr(req, getFwVersion().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HTMLVersion") == 0)
    {
        httpd_resp_sendstr(req, getHTMLversion().c_str());
        return ESP_OK;        
    }
    else if (_task.compare("BuildTime") == 0)
    {
        httpd_resp_sendstr(req, build_time());
        return ESP_OK;        
    }
    else if (_task.compare("IDFVersion") == 0)
    {
        httpd_resp_sendstr(req, getIDFVersion().c_str());
        return ESP_OK;        
    }
    else {
        httpd_resp_sendstr(req, "E92: Parameter not found");
        return ESP_OK;    
    }
}


esp_err_t handler_get_heap(httpd_req_t *req)
{
    const char* APIName = "heap:v2"; // API name and version
    char _query[200];
    char _valuechar[30];    
    std::string _task;

    //heap_caps_dump(MALLOC_CAP_SPIRAM);

    if (httpd_req_get_url_query_str(req, _query, 200) == ESP_OK) {
        //ESP_LOGD(TAG, "Query: %s", _query);
        
        if (httpd_query_key_value(_query, "type", _valuechar, 30) == ESP_OK) {
            //ESP_LOGD(TAG, "type is found: %s", _valuechar);
            _task = std::string(_valuechar);
        }
    }
    else { // default - no parameter set: send data as JSON 
        esp_err_t retVal = ESP_OK;
        std::string sReturnMessage;
        cJSON *cJSONObject = cJSON_CreateObject();
            
        if (cJSONObject == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E90: Error, JSON object cannot be created");
            return ESP_FAIL;
        }

        if (cJSON_AddStringToObject(cJSONObject, "api_name", APIName) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_total_free", std::to_string(getESPHeapSizeTotalFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_internal_free", std::to_string(getESPHeapSizeInternalFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_internal_largest_free", std::to_string(getESPHeapSizeInternalLargestFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_internal_min_free", std::to_string(getESPHeapSizeInternalMinFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_spiram_free", std::to_string(getESPHeapSizeSPIRAMFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_spiram_largest_free", std::to_string(getESPHeapSizeSPIRAMLargestFree()).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "heap_spiram_min_free", std::to_string(getESPHeapSizeSPIRAMMinFree()).c_str()) == NULL)
            retVal = ESP_FAIL;

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 320, 1); // Print to predefined buffer, avoid dynamic allocations
        sReturnMessage = std::string(jsonString);
        cJSON_free(jsonString);  
        cJSON_Delete(cJSONObject);

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "application/json");

        if (retVal == ESP_OK) {
            httpd_resp_send(req, sReturnMessage.c_str(), sReturnMessage.length());
            return ESP_OK;
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Error while adding JSON elements");
            return ESP_FAIL;
        }
    }

    /* Legacy: Provide single data as text response */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    
    if (_task.compare("APIName") == 0)
    {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else if (_task.compare("HeapTotalFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeTotalFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapInternalFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeInternalFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapInternalLargestFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeInternalLargestFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapInternalMinFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeInternalMinFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapSPIRAMFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeSPIRAMFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapSPIRAMLargestFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeSPIRAMLargestFree())).c_str());
        return ESP_OK;        
    }
    else if (_task.compare("HeapSPIRAMMinFree") == 0)
    {
        httpd_resp_sendstr(req, (std::to_string(getESPHeapSizeSPIRAMMinFree())).c_str());
        return ESP_OK;        
    }
    #ifdef TASK_ANALYSIS_ON
    else if (_task.compare("TaskInfo") == 0)
    {
        char* pcTaskList = (char*) calloc_psram_heap(std::string(TAG) + "->pcTaskList", 1, sizeof(char) * 768, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (pcTaskList) {
            vTaskList(pcTaskList);
            std::string zw = "Task info:<br><pre>Name | State | Prio | Lowest stacksize | Creation order | CPU (-1=NoAffinity)<br>"
                                + std::string(pcTaskList) + "</pre>";
            free_psram_heap(std::string(TAG) + "->pcTaskList", pcTaskList);
        }
        else {
            zw += "Task info:<br>E93: Allocation of TaskList buffer in PSRAM failed";
        }
        
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    #else
    else if (_task.compare("TaskInfo") == 0)
    {
        httpd_resp_sendstr(req, "E93: Service not compiled (#define TASK_ANALYSIS_ON)");
        return ESP_OK;        
    }
    #endif
    else {
        httpd_resp_sendstr(req, "E92: Parameter not found");
        return ESP_OK;    
    }
}


esp_err_t handler_get_stream(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_get_stream - Start");       
        ESP_LOGD(TAG, "handler_get_stream uri: %s", req->uri);
    #endif

    char _query[50];
    char _value[10];
    bool flashlightOn = false;

    if (httpd_req_get_url_query_str(req, _query, 50) == ESP_OK)
    {
        //ESP_LOGD(TAG, "Query: %s", _query);
        if (httpd_query_key_value(_query, "flashlight", _value, 10) == ESP_OK)
        {
            #ifdef DEBUG_DETAIL_ON       
                ESP_LOGD(TAG, "flashlight is found: %s", _value);
            #endif
            if (strlen(_value) > 0)
                flashlightOn = true;
        }
    }

    Camera.CaptureToStream(req, flashlightOn);

    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_get_stream - Done");       
    #endif

    return ESP_OK;
}


esp_err_t handler_main(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_main - Start");
    #endif

    char filepath[50];
    ESP_LOGD(TAG, "uri: %s\n", req->uri);
    int _pos;
    esp_err_t res;

    char *base_path = (char*) req->user_ctx;
    std::string filetosend(base_path);

    const char *filename = get_path_from_uri(filepath, base_path,
                                             req->uri - 1, sizeof(filepath));    
    ESP_LOGD(TAG, "1 uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    if ((strcmp(req->uri, "/") == 0))
    {
        {
            filetosend = filetosend + "/html/index.html";
        }
    }
    else
    {
        filetosend = filetosend + "/html" + std::string(req->uri);
        _pos = filetosend.find("?");
        if (_pos > -1){
            filetosend = filetosend.substr(0, _pos);
        }
    }

    if (filetosend == "/sdcard/html/index.html") {
        // Check basic device initialization status:
        // If critical error(s) occured which do not allow to start regular process and web interface, redirect to a reduced web interface
        if (isSetSystemStatusFlag(SYSTEM_STATUS_PSRAM_BAD) ||
            isSetSystemStatusFlag(SYSTEM_STATUS_HEAP_TOO_SMALL) ||
            isSetSystemStatusFlag(SYSTEM_STATUS_SDCARD_CHECK_BAD) ||
            isSetSystemStatusFlag(SYSTEM_STATUS_FOLDER_CHECK_BAD)) 
        {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Critical error(s) occured, redirect to reduced web interface");

            char buf[20];
            std::string message = "<h1>AI on the Edge</h1><b>Critical error(s) occured, which do not allow to start regular process and web interface:</b><br>";

            for (int i = 0; i < 32; i++) {
                if (isSetSystemStatusFlag((SystemStatusFlag_t)(1<<i))) {
                    snprintf(buf, sizeof(buf), "0x%08X", 1<<i);
                    message += std::string(buf) + "<br>";
                }
            }

            message += "<br>Please check logs with \'Log Viewer\' and/or <a href=\"https://jomjol.github.io/AI-on-the-edge-device-docs/Error-Codes\" target=_blank>jomjol.github.io/AI-on-the-edge-device-docs/Error-Codes</a> for more information.";
            message += "<br><br><button onclick=\"window.location.href='/reboot';\">Reboot</button>";
            message += "&nbsp;<button onclick=\"window.open('/ota_page.html');\">OTA Update</button>";
            message += "&nbsp;<button onclick=\"window.open('/log.html');\">Log Viewer</button>";
            message += "&nbsp;<button onclick=\"window.open('/info.html');\">Show System Info</button>";
            httpd_resp_send(req, message.c_str(), message.length());
            return ESP_OK;
        }
        else if (isSetupModusActive()) {
            ESP_LOGD(TAG, "System is in setup mode --> index.html --> setup.html");
            filetosend = "/sdcard/html/setup.html";
        }
    }

    ESP_LOGD(TAG, "Filename: %s", filename);
    
    ESP_LOGD(TAG, "File requested: %s", filetosend.c_str());

    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 414 Error */
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Filename too long");
        return ESP_FAIL;
    }

    res = send_file(req, filetosend);
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);

    if (res != ESP_OK)
        return res;

    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_main - Stop");   
    #endif

    return ESP_OK;
}


esp_err_t handler_img_tmp(httpd_req_t *req)
{
    char filepath[50];
    ESP_LOGD(TAG, "uri: %s", req->uri);

    char *base_path = (char*) req->user_ctx;
    std::string filetosend(base_path);

    const char *filename = get_path_from_uri(filepath, base_path,
                                             req->uri  + sizeof("/img_tmp/") - 1, sizeof(filepath));    
    ESP_LOGD(TAG, "1 uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    filetosend = filetosend + "/img_tmp/" + std::string(filename);
    ESP_LOGD(TAG, "File to upload: %s", filetosend.c_str());

    esp_err_t res = send_file(req, filetosend); 
    if (res != ESP_OK)
        return res;

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


esp_err_t handler_img_tmp_virtual(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_img_tmp_virtual - Start");  
    #endif

    char filepath[50];

    ESP_LOGD(TAG, "uri: %s", req->uri);

    char *base_path = (char*) req->user_ctx;
    std::string filetosend(base_path);

    const char *filename = get_path_from_uri(filepath, base_path,
                                             req->uri  + sizeof("/img_tmp/") - 1, sizeof(filepath));    
    ESP_LOGD(TAG, "1 uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    filetosend = std::string(filename);
    ESP_LOGD(TAG, "File to upload: %s", filetosend.c_str());

    // Serve raw.jpg
    if (filetosend == "raw.jpg")
        return GetRawJPG(req); 

    // Serve alg.jpg, alg_roi.jpg or digital and analog ROIs
    if (ESP_OK == GetJPG(filetosend, req))
        return ESP_OK;

    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_img_tmp_virtual - Done");   
    #endif

    // File was not served already --> serve with img_tmp_handler
    return handler_img_tmp(req);
}


httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = { };

    config.task_priority = tskIDLE_PRIORITY+3; // previously -> 2022-12-11: tskIDLE_PRIORITY+1; 2021-09-24: tskIDLE_PRIORITY+5
    config.stack_size = 10240; // previously -> 2023-01-02: 32768
    config.core_id = 1; // previously -> 2023-01-02: 0, 2022-12-11: tskNO_AFFINITY;
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_open_sockets = 5; //20210921 --> previously 7   
    config.max_uri_handlers = 32; // previously 24, 20220511: 35, 20221220: 37, 2023-01-02: 38   , 2023-03-12: 40          
    config.max_resp_headers = 8;                        
    config.backlog_conn = 5;                        
    config.lru_purge_enable = true; // this cuts old connections if new ones are needed.               
    config.recv_wait_timeout = 15; // default: 5 20210924 --> previously 30              
    config.send_wait_timeout = 15; // default: 5 20210924 --> previously 30                    
    config.global_user_ctx = NULL;                        
    config.global_user_ctx_free_fn = NULL;                
    config.global_transport_ctx = NULL;                   
    config.global_transport_ctx_free_fn = NULL;           
    config.open_fn = NULL;                                
    config.close_fn = NULL;     
//    config.uri_match_fn = NULL;                            
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        return server;
    }

    ESP_LOGE(TAG, "Failed to start webserver");
    return NULL;
}


void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}


void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}


void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}


void register_server_main_uri(httpd_handle_t server, const char *base_path)
{ 
    ESP_LOGI(TAG, "Registering URI handlers");
    
    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;
    
    camuri.uri       = "/info";
    camuri.handler   = handler_get_info;
    camuri.user_ctx  = (void*) base_path;   // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/heap";
    camuri.handler   = handler_get_heap;
    camuri.user_ctx  = NULL;   // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/stream";
    camuri.handler   = handler_get_stream;
    camuri.user_ctx  = NULL;   // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/img_tmp/*";
    camuri.handler   = handler_img_tmp_virtual;
    camuri.user_ctx  = (void*) base_path;    // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/*";
    camuri.handler   = handler_main;
    camuri.user_ctx  = (void*) base_path;    // Pass server data as context
    httpd_register_uri_handler(server, &camuri);
}