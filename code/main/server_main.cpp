#include "server_main.h"

#include <string>
#include <stdio.h>
#include <netdb.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_private/esp_clk.h"
#include "cJSON.h"

#ifdef TASK_ANALYSIS_ON
#include "psram.h"
#endif

#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "server_help.h"
#include "time_sntp.h"
#include "connect_wlan.h"
#include "read_wlanini.h"
#include "version.h"
#include "Helper.h"
#include "system.h"
#include "interface_mqtt.h"
#include "interface_influxdb.h"


static const char *TAG = "MAIN_SERVER";

httpd_handle_t server = NULL;   
extern std::string deviceStartTimestamp;


esp_err_t handler_get_info(httpd_req_t *req)
{
    const char* APIName = "info:v3"; // API name and version
    char _query[200];
    char _valuechar[30];    
    std::string type;

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {       
        if (httpd_query_key_value(_query, "type", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            type = std::string(_valuechar);
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
        if (cJSON_AddStringToObject(cJSONObject, "process_status", getProcessStatus().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "process_interval", (int)(flowctrl.getProcessInterval() * 10) / 10.0) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "process_time", getFlowProcessingTime()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "cycle_counter", getFlowCycleCounter()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "datalogging_sdcard_status", LogFile.GetDataLogToSD() ? "Enabled" : "Disabled") == NULL)
            retVal = ESP_FAIL;
        
        #ifdef ENABLE_MQTT
        if (cJSON_AddStringToObject(cJSONObject, "mqtt_status", getMQTTisEnabled() ? (getMQTTisConnected() ? (getMQTTisEncrypted() ? 
                                        "Connected (Encrypted)" : "Connected") : "Disconnected") : "Disabled") == NULL)
            retVal = ESP_FAIL;
        #else
        if (cJSON_AddStringToObject(cJSONObject, "mqtt_status", "E01: Service not compiled (#define ENABLE_MQTT)") == NULL)
            retVal = ESP_FAIL;
        #endif

        #ifdef ENABLE_INFLUXDB
        ClassFlowInfluxDB* influxdb = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDB"));
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv1_status", influxdb ? (getInfluxDBisEncrypted() ? 
                                        "Enabled (Encrypted)" : "Enabled") : "Disabled") == NULL)
            retVal = ESP_FAIL;
        
        ClassFlowInfluxDBv2* influxdbv2 = (ClassFlowInfluxDBv2*)(flowctrl.getFlowClass("ClassFlowInfluxDBv2"));
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv2_status", influxdbv2 ? (getInfluxDBv2isEncrypted() ? 
                                        "Enabled (Encrypted)" : "Enabled") : "Disabled") == NULL)
            retVal = ESP_FAIL;
        #else
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv1_status", "E02: Service not compiled (#define ENABLE_INFLUXDB)") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv2_status", "E02: Service not compiled (#define ENABLE_INFLUXDB)") == NULL)
            retVal = ESP_FAIL;
        #endif

        if (cJSON_AddStringToObject(cJSONObject, "ntp_syncstatus", getNTPSyncStatus().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "current_time", getCurrentTimeString(TIME_FORMAT_OUTPUT).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "device_starttime", deviceStartTimestamp.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "device_uptime", getUptime()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_status", getWIFIisConnected() ? "Connected" : "Disconnected") == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "wlan_ssid", getSSID().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "wlan_rssi", get_WIFI_RSSI()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "mac_address", getMac().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "network_config", getDHCPUsage() ? "DHCP" : "Static") == NULL)
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
        if (cJSON_AddStringToObject(cJSONObject, "board_type", getBoardType().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_model", getChipModel().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "chip_cores", getChipCoreCount()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "chip_revision", getChipRevision().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "chip_frequency", esp_clk_cpu_freq()/1000000) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "chip_temp", (int)getSOCTemperature()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "camera_type", Camera.getCamType().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "camera_frequency", Camera.getCamFrequencyMhz()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_name", getSDCardName().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "sd_manufacturer", getSDCardManufacturer().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "sd_capacity", getSDCardCapacity()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "sd_sector_size", getSDCardSectorSize()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "sd_partition_alloc_size", getSDCardPartitionAllocationSize()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "sd_partition_size", getSDCardPartitionSize()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "sd_partition_free", getSDCardFreePartitionSpace()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_total_free", getESPHeapSizeTotalFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_free", getESPHeapSizeInternalFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_largest_free", getESPHeapSizeInternalLargestFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_internal_min_free", getESPHeapSizeInternalMinFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_free", getESPHeapSizeSPIRAMFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_largest_free", getESPHeapSizeSPIRAMLargestFree()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "heap_spiram_min_free", getESPHeapSizeSPIRAMMinFree()) == NULL)
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
        if (cJSON_AddNumberToObject(cJSONObject, "config_file_version", getConfigFileVersion()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "idf_version", getIDFVersion().c_str()) == NULL)
            retVal = ESP_FAIL;

        char *jsonString = cJSON_PrintBuffered(cJSONObject, 2048, 1); // Print to predefined buffer, avoid dynamic allocations
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
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Error while adding JSON elements");
            return ESP_FAIL;
        }
    }

    /* Legacy: Provide single data as text response */
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");

    if (type.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else if (type.compare("process_status") == 0) {
        httpd_resp_sendstr(req, getProcessStatus().c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_interval") == 0) {
        httpd_resp_sendstr(req, to_stringWithPrecision(flowctrl.getProcessInterval(), 1).c_str());
        return ESP_OK;        
    }
    else if (type.compare("process_time") == 0) {
        httpd_resp_sendstr(req, std::to_string(getFlowProcessingTime()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("cycle_counter") == 0) {
        httpd_resp_sendstr(req, std::to_string(getFlowCycleCounter()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("datalogging_sdcard_status") == 0) {
        httpd_resp_sendstr(req, LogFile.GetDataLogToSD() ? "Enabled" : "Disabled");
        return ESP_OK;        
    }
    
    #ifdef ENABLE_MQTT
    else if (type.compare("mqtt_status") == 0) {
        httpd_resp_sendstr(req, getMQTTisEnabled() ? (getMQTTisConnected() ? (getMQTTisEncrypted() ? 
                                "Connected (Encrypted)" : "Connected") : "Disconnected") : "Disabled");
        return ESP_OK;
    }
    #else
    else if (type.compare("mqtt_status") == 0) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "E01: Service not compiled (#define ENABLE_MQTT)");
        return ESP_FAIL;          
    }
    #endif

    #ifdef ENABLE_INFLUXDB
    else if (type.compare("influxdbv1_status") == 0) {
        ClassFlowInfluxDB* influxdb = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDB"));
        httpd_resp_sendstr(req, influxdb ? (getInfluxDBisEncrypted() ? "Enabled (Encrypted)" : "Enabled") : "Disabled");
        return ESP_OK;
    }
    else if (type.compare("influxdbv2_status") == 0) {
        ClassFlowInfluxDB* influxdbv2 = (ClassFlowInfluxDB*)(flowctrl.getFlowClass("ClassFlowInfluxDBv2"));
        httpd_resp_sendstr(req, influxdbv2 ? (getInfluxDBv2isEncrypted() ? "Enabled (Encrypted)" : "Enabled") : "Disabled");
        return ESP_OK;
    }
    #else
    else if (type.compare("influxdbv1_status") == 0) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "E02: Service not compiled (#define ENABLE_INFLUXDB)");
        return ESP_FAIL;          
    }
    else if (type.compare("influxdbv2_status") == 0) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "E02: Service not compiled (#define ENABLE_INFLUXDB)");
        return ESP_FAIL;        
    }
    #endif

    else if (type.compare("ntp_syncstatus") == 0) {
        httpd_resp_sendstr(req, getNTPSyncStatus().c_str());
        return ESP_OK;        
    }
    else if (type.compare("device_starttime") == 0) {
        httpd_resp_sendstr(req, deviceStartTimestamp.c_str());
        return ESP_OK;        
    }
    else if (type.compare("device_uptime") == 0) {
        httpd_resp_sendstr(req, std::to_string(getUptime()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("wlan_status") == 0) {
        httpd_resp_sendstr(req, getWIFIisConnected() ? "Connected" : "Disconnected");
        return ESP_OK;        
    }
    else if (type.compare("wlan_ssid") == 0) {
        httpd_resp_sendstr(req, getSSID().c_str());
        return ESP_OK;        
    }
    else if (type.compare("wlan_rssi") == 0) {
        httpd_resp_sendstr(req, std::to_string(get_WIFI_RSSI()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("mac_address") == 0) {
        httpd_resp_sendstr(req, getMac().c_str());
        return ESP_OK;        
    }
    else if (type.compare("network_config") == 0) {
        httpd_resp_sendstr(req, getDHCPUsage() ? "DHCP" : "Static");
        return ESP_OK;        
    }
    else if (type.compare("ipv4_address") == 0) {
        httpd_resp_sendstr(req, getIPAddress().c_str());
        return ESP_OK;        
    }
    else if (type.compare("netmask_address") == 0) {
        httpd_resp_sendstr(req, getNetmaskAddress().c_str());
        return ESP_OK;        
    }
    else if (type.compare("gateway_address") == 0) {
        httpd_resp_sendstr(req, getGatewayAddress().c_str());
        return ESP_OK;        
    }
    else if (type.compare("dns_address") == 0) {
        httpd_resp_sendstr(req, getDNSAddress().c_str());
        return ESP_OK;        
    }
    else if (type.compare("hostname") == 0) {
        httpd_resp_sendstr(req, getHostname().c_str());
        return ESP_OK;        
    }
    else if (type.compare("board_type") == 0) {
        httpd_resp_sendstr(req, getBoardType().c_str());
        return ESP_OK;        
    }
    else if (type.compare("chip_model") == 0) {
        httpd_resp_sendstr(req, getChipModel().c_str());
        return ESP_OK;        
    }
    else if (type.compare("chip_cores") == 0) {
        httpd_resp_sendstr(req, std::to_string(getChipCoreCount()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("chip_revision") == 0) {
        httpd_resp_sendstr(req, getChipRevision().c_str());
        return ESP_OK;        
    }
    else if (type.compare("chip_frequency") == 0) {
        httpd_resp_sendstr(req, std::to_string(esp_clk_cpu_freq()/1000000).c_str());
        return ESP_OK;        
    }
    else if (type.compare("chip_temp") == 0) {
        httpd_resp_sendstr(req, std::to_string((int)getSOCTemperature()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("camera_type") == 0) {
        httpd_resp_sendstr(req, Camera.getCamType().c_str());
        return ESP_OK;        
    }
    else if (type.compare("camera_frequency") == 0) {
        httpd_resp_sendstr(req, std::to_string(Camera.getCamFrequencyMhz()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_name") == 0) {
        httpd_resp_sendstr(req, getSDCardName().c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_manufacturer") == 0) {
        httpd_resp_sendstr(req, getSDCardManufacturer().c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_capacity") == 0) {
        httpd_resp_sendstr(req, std::to_string(getSDCardCapacity()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_sector_size") == 0) {
        httpd_resp_sendstr(req, std::to_string(getSDCardSectorSize()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_partition_alloc_size") == 0) {
        httpd_resp_sendstr(req, std::to_string(getSDCardPartitionAllocationSize()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_partition_size") == 0) {
        httpd_resp_sendstr(req, std::to_string(getSDCardPartitionSize()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("sd_partition_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getSDCardFreePartitionSpace()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_total_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeTotalFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_internal_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeInternalFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_internal_largest_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeInternalLargestFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_internal_min_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeInternalMinFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_spiram_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeSPIRAMFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_spiram_largest_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeSPIRAMLargestFree()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("heap_spiram_min_free") == 0) {
        httpd_resp_sendstr(req, std::to_string(getESPHeapSizeSPIRAMMinFree()).c_str());
        return ESP_OK;        
    }
    #ifdef TASK_ANALYSIS_ON
    else if (type.compare("task_info") == 0) {
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
    else if (type.compare("task_info") == 0) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "E01: Service not compiled (#define TASK_ANALYSIS_ON)");
        return ESP_FAIL;        
    }
    #endif
    else if (type.compare("git_branch") == 0) {
        httpd_resp_sendstr(req, libfive_git_branch());
        return ESP_OK;        
    }
    else if (type.compare("git_tag") == 0) {
        httpd_resp_sendstr(req, libfive_git_version());
        return ESP_OK;        
    }
    else if (type.compare("git_revision") == 0) {
        httpd_resp_sendstr(req, libfive_git_revision());
        return ESP_OK;        
    }
    else if (type.compare("firmware_version") == 0) {
        httpd_resp_sendstr(req, getFwVersion().c_str());
        return ESP_OK;        
    }
    else if (type.compare("html_version") == 0) {
        httpd_resp_sendstr(req, getHTMLversion().c_str());
        return ESP_OK;        
    }
    else if (type.compare("build_time") == 0) {
        httpd_resp_sendstr(req, build_time());
        return ESP_OK;        
    }
    else if (type.compare("config_file_version") == 0) {
        httpd_resp_sendstr(req, std::to_string(getConfigFileVersion()).c_str());
        return ESP_OK;        
    }
    else if (type.compare("idf_version") == 0) {
        httpd_resp_sendstr(req, getIDFVersion().c_str());
        return ESP_OK;        
    }
    else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E93: Parameter not found");
        return ESP_FAIL;  
    }
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
        return flowctrl.SendRawJPG(req);

    // Serve alg.jpg, alg_roi.jpg or digital and analog ROIs
    return flowctrl.GetJPGStream(filetosend, req);

    // File was not served already --> serve with img_tmp_handler
    return handler_img_tmp(req);
}


esp_err_t handler_main(httpd_req_t *req)
{
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
        else if (flowctrl.getStatusSetupModus()) {
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

    return ESP_OK;
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
    config.max_uri_handlers = 20; // previously 42  
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
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
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

    camuri.uri       = "/img_tmp/*";
    camuri.handler   = handler_img_tmp_virtual;
    camuri.user_ctx  = (void*) base_path;    // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/*";
    camuri.handler   = handler_main;
    camuri.user_ctx  = (void*) base_path;    // Pass server data as context
    httpd_register_uri_handler(server, &camuri);
}
