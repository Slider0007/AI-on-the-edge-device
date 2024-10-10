#include "webserver.h"

#include <string.h>
#include <stdio.h>
#include <netdb.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_private/esp_clk.h>
#include <cJSON.h>

#ifdef TASK_ANALYSIS_ON
#include "psram.h"
#endif

#include "../main/version.h" // Only include once
#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "server_file.h"
#include "server_help.h"
#include "time_sntp.h"
#include "connect_wlan.h"
#include "helper.h"
#include "system.h"
#include "interface_mqtt.h"
#include "interface_influxdbv1.h"
#include "interface_influxdbv2.h"


static const char *TAG = "MAIN_SERVER";

httpd_handle_t server = NULL;
struct HttpServerData *httpServerData = NULL;

extern std::string deviceStartTimestamp;


esp_err_t handler_get_info(httpd_req_t *req)
{
    const char* APIName = "info:v4"; // API name and version
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
        if (cJSON_AddStringToObject(cJSONObject, "datalogging_sdcard_status", LogFile.getDataLogToSDStatus() ? "Enabled" : "Disabled") == NULL)
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
        if (cJSON_AddStringToObject(cJSONObject, "influxdbv1_status", ConfigClass::getInstance()->get()->sectionInfluxDBv1.enabled ?
                                        (getInfluxDBv1isEncrypted() ? "Enabled (Encrypted)" : "Enabled") : "Disabled") == NULL)
            retVal = ESP_FAIL;

        if (cJSON_AddStringToObject(cJSONObject, "influxdbv2_status", ConfigClass::getInstance()->get()->sectionInfluxDBv2.enabled ?
                                        (getInfluxDBv2isEncrypted() ? "Enabled (Encrypted)" : "Enabled") : "Disabled") == NULL)
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
        if (cJSON_AddNumberToObject(cJSONObject, "wlan_rssi", getWifiRssi()) == NULL)
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
        if (cJSON_AddStringToObject(cJSONObject, "camera_type", cameraCtrl.getCamType().c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(cJSONObject, "camera_frequency", cameraCtrl.getCamFrequencyMhz()) == NULL)
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
        if (cJSON_AddNumberToObject(cJSONObject, "config_version", getConfigVersion()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(cJSONObject, "idf_version", getIDFVersion().c_str()) == NULL)
            retVal = ESP_FAIL;

        // Print to preallocted buffer
        char *jsonData = ((struct HttpServerData *)req->user_ctx)->scratch;
        if (!cJSON_PrintPreallocated(cJSONObject, jsonData, WEBSERVER_SCRATCH_BUFSIZE, 1))
            retVal = ESP_FAIL;
        cJSON_Delete(cJSONObject);

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "application/json");

        if (retVal == ESP_OK) {
            httpd_resp_send(req, jsonData, strlen(jsonData));
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
        httpd_resp_sendstr(req, LogFile.getDataLogToSDStatus() ? "Enabled" : "Disabled");
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
        httpd_resp_sendstr(req, ConfigClass::getInstance()->get()->sectionInfluxDBv1.enabled ? (getInfluxDBv1isEncrypted() ?
                                "Enabled (Encrypted)" : "Enabled") : "Disabled");
        return ESP_OK;
    }
    else if (type.compare("influxdbv2_status") == 0) {
        httpd_resp_sendstr(req, ConfigClass::getInstance()->get()->sectionInfluxDBv2.enabled ? (getInfluxDBv2isEncrypted() ?
                                "Enabled (Encrypted)" : "Enabled") : "Disabled");
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
        httpd_resp_sendstr(req, std::to_string(getWifiRssi()).c_str());
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
        httpd_resp_sendstr(req, cameraCtrl.getCamType().c_str());
        return ESP_OK;
    }
    else if (type.compare("camera_frequency") == 0) {
        httpd_resp_sendstr(req, std::to_string(cameraCtrl.getCamFrequencyMhz()).c_str());
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
    else if (type.compare("config_version") == 0) {
        httpd_resp_sendstr(req, std::to_string(getConfigVersion()).c_str());
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

    char *basePath = (char*) ((struct HttpServerData *)req->user_ctx)->basePathRoot;
    std::string filetosend(basePath);

    const char *filename = getPathFromUri(filepath, basePath,
                                             req->uri  + sizeof("/img_tmp/") - 1, sizeof(filepath));
    ESP_LOGD(TAG, "1 uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    filetosend = filetosend + "/img_tmp/" + std::string(filename);
    ESP_LOGD(TAG, "File to upload: %s", filetosend.c_str());

    esp_err_t res = sendFile(req, filetosend);
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

    char *basePath = (char*) ((struct HttpServerData *)req->user_ctx)->basePathRoot;
    std::string filetosend(basePath);

    const char *filename = getPathFromUri(filepath, basePath,
                                             req->uri  + sizeof("/img_tmp/") - 1, sizeof(filepath));
    ESP_LOGD(TAG, "1 uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    filetosend = std::string(filename);
    ESP_LOGD(TAG, "File to upload: %s", filetosend.c_str());

    // Serve raw.jpg
    if (filetosend == "raw.jpg")
        return flowctrl.sendRawJPG(req);

    // Serve alg.jpg, alg_roi.jpg or digit and analog ROIs
    return flowctrl.getJPGStream(filetosend, req);

    // File was not served already --> serve with img_tmp_handler
    return handler_img_tmp(req);
}


esp_err_t handler_main(httpd_req_t *req)
{
    char filepath[50];
    char *basePath = (char*) ((struct HttpServerData *)req->user_ctx)->basePathRoot;
    std::string filetosend(basePath);

    const char *filename = getPathFromUri(filepath, basePath,
                                             req->uri - 1, sizeof(filepath));
    ESP_LOGD(TAG, "uri: %s, filename: %s, filepath: %s", req->uri, filename, filepath);

    if ((strcmp(req->uri, "/") == 0)) {
        filetosend = filetosend + "/html/index.html";
    }
    else {
        filetosend += "/html" + std::string(req->uri);
        int pos = filetosend.find("?");
        if (pos != std::string::npos){
            filetosend = filetosend.substr(0, pos);
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
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Critical error(s) occured, redirect to reduced web interface");

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
            ESP_LOGD(TAG, "System is in setup mode. Redirect from index.html --> setup.html");
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

    return sendFile(req, filetosend);
}


httpd_handle_t startWebserver(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 10240;
    config.core_id = 1;
    config.max_open_sockets = 5; // With default value 7: Error "httpd_accept_conn: error in accept"
    config.max_uri_handlers = 23; // Max number of URI handler
    config.lru_purge_enable = true; // Cut old connections if new ones are needed
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting webserver on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        return server;
    }

    ESP_LOGE(TAG, "Failed to start webserver");
    return NULL;
}


void allocateWebserverHelperMemory(void)
{
    httpServerData = (HttpServerData*)heap_caps_calloc(1, sizeof(struct HttpServerData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!httpServerData) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to allocate memory for webserver data");
    }
}


void registerWebserverUri(httpd_handle_t server, const char *basePath)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    // Validate file storage base path
    if (!basePath) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "basePath for root folder not set");
        //return ESP_ERR_INVALID_ARG;
    }

    strlcpy(httpServerData->basePathRoot, basePath, sizeof(httpServerData->basePathRoot));

    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;

    camuri.uri       = "/info";
    camuri.handler   = handler_get_info;
    camuri.user_ctx  = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/img_tmp/*";
    camuri.handler   = handler_img_tmp_virtual;
    camuri.user_ctx  = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/*";
    camuri.handler   = handler_main;
    camuri.user_ctx  = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);
}
