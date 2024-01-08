#include "server_main.h"

#include <string>
#include <stdio.h>
#include <netdb.h>

#include "esp_log.h"
#include "esp_wifi.h"

#include "MainFlowControl.h"
#include "ClassLogFile.h"
#include "server_help.h"
#include "time_sntp.h"
#include "connect_wlan.h"
#include "read_wlanini.h"
#include "version.h"
#include "Helper.h"
#include "system.h"
#include "psram.h"


static const char *TAG = "MAIN_SERVER";

httpd_handle_t server = NULL;   
std::string starttime = "";


esp_err_t handler_get_info(httpd_req_t *req)
{
#ifdef DEBUG_DETAIL_ON      
    LogFile.WriteHeapInfo("info_get_handler - Start");    
#endif

    //LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "info_get_handler");    
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

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (_task.compare("GitBranch") == 0)
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
    else if (_task.compare("BuildTime") == 0)
    {
        httpd_resp_sendstr(req, build_time());
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
    else if (_task.compare("Hostname") == 0)
    {
        std::string zw;
        zw = std::string(wlan_config.hostname);
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("IP") == 0)
    {
        std::string zw;
        zw = getIPAddress();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SSID") == 0)
    {
        std::string zw;
        zw = getSSID();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("CycleCounter") == 0)
    {
        char formated[10] = "";    
        snprintf(formated, sizeof(formated), "%d", getFlowCycleCounter());
        httpd_resp_sendstr(req, formated);
        return ESP_OK;        
    }
    else if (_task.compare("SDCardPartitionSize") == 0)
    {
        std::string zw;
        zw = getSDCardPartitionSize();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardFreePartitionSpace") == 0)
    {
        std::string zw;
        zw = getSDCardFreePartitionSpace();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardPartitionAllocationSize") == 0)
    {
        std::string zw;
        zw = getSDCardPartitionAllocationSize();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardManufacturer") == 0)
    {
        std::string zw;
        zw = getSDCardManufacturer(); 
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardName") == 0)
    {
        std::string zw;
        zw = getSDCardName(); 
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardCapacity") == 0)
    {
        std::string zw;
        zw = getSDCardCapacity();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }
    else if (_task.compare("SDCardSectorSize") == 0)
    {
        std::string zw;
        zw = getSDCardSectorSize();
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;        
    }

    return ESP_OK;
}


esp_err_t handler_get_heap(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_get_heap - Start");       
        ESP_LOGD(TAG, "handler_get_heap uri: %s", req->uri);
    #endif

    //heap_caps_dump(MALLOC_CAP_SPIRAM);

    std::string zw = "Heap info:<br>" + getESPHeapInfo();

    #ifdef TASK_ANALYSIS_ON
        char* pcTaskList = (char*) calloc_psram_heap(std::string(TAG) + "->pcTaskList", 1, sizeof(char) * 768, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        if (pcTaskList) {
            vTaskList(pcTaskList);
            zw = zw + "<br><br>Task info:<br><pre>Name | State | Prio | Lowest stacksize | Creation order | CPU (-1=NoAffinity)<br>"
                    + std::string(pcTaskList) + "</pre>";
            free_psram_heap(std::string(TAG) + "->pcTaskList", pcTaskList);
        }
        else {
            zw = zw + "<br><br>Task info:<br>ERROR - Allocation of TaskList buffer in PSRAM failed";
        }
    #endif 

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");

    if (zw.length() > 0) 
    {
        httpd_resp_send(req, zw.c_str(), zw.length());
    }
    else 
    {
        httpd_resp_send(req, NULL, 0);
    }

    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_get_heap - Done");       
    #endif

    return ESP_OK;
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


esp_err_t handler_get_starttime(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, starttime.c_str(), starttime.length()); 

    return ESP_OK;
}



esp_err_t handler_cputemp(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_cputemp - Start");       
    #endif

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, std::to_string((int)temperatureRead()).c_str(), HTTPD_RESP_USE_STRLEN);

    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_cputemp - End");       
    #endif

    return ESP_OK;
}


esp_err_t handler_rssi(httpd_req_t *req)
{
    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_rssi - Start");       
    #endif

    if (getWIFIisConnected()) 
    {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, std::to_string(get_WIFI_RSSI()).c_str(), HTTPD_RESP_USE_STRLEN);
    }
    else 
    {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "WIFI not (yet) connected: REST API /rssi not available");
        return ESP_ERR_NOT_FOUND;
    }      

    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_rssi - End");       
    #endif

    return ESP_OK;
}


esp_err_t handler_uptime(httpd_req_t *req)
{

    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_uptime - Start");       
    #endif
    
    std::string formatedUptime = getFormatedUptime(false);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, formatedUptime.c_str(), formatedUptime.length());  

    #ifdef DEBUG_DETAIL_ON       
        LogFile.WriteHeapInfo("handler_uptime - End");       
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
    return GetJPG(filetosend, req);

    #ifdef DEBUG_DETAIL_ON      
        LogFile.WriteHeapInfo("handler_img_tmp_virtual - Done");   
    #endif

    // File was not served already --> serve with img_tmp_handler
    return handler_img_tmp(req);
}


esp_err_t handler_sysinfo(httpd_req_t *req)
{
    std::string zw;
    std::string cputemp = std::to_string((int)temperatureRead());
    std::string gitversion = libfive_git_version();
    std::string buildtime = build_time();
    std::string gitbranch = libfive_git_branch();
    std::string gittag = libfive_git_version();
    std::string gitrevision = libfive_git_revision();
    std::string htmlversion = getHTMLversion();
    char freeheapmem[11];
    sprintf(freeheapmem, "%lu", (long) getESPHeapSizeTotal());
    
    zw = std::string("[{") + 
        "\"firmware\": \"" + gitversion + "\"," +
        "\"buildtime\": \"" + buildtime + "\"," +
        "\"gitbranch\": \"" + gitbranch + "\"," +
        "\"gittag\": \"" + gittag + "\"," +
        "\"gitrevision\": \"" + gitrevision + "\"," +
        "\"html\": \"" + htmlversion + "\"," +
        "\"cputemp\": \"" + cputemp + "\"," +
        "\"hostname\": \"" + getHostname() + "\"," +
        "\"IPv4\": \"" + getIPAddress() + "\"," +
        "\"freeHeapMem\": \"" + freeheapmem + "\"" +
        "}]";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, zw.c_str(), zw.length());

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
    config.max_uri_handlers = 42; // previously 24, 20220511: 35, 20221220: 37, 2023-01-02: 38   , 2023-03-12: 40          
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

    starttime = getCurrentTimeString("%Y%m%d-%H%M%S");

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

    camuri.uri       = "/sysinfo";
    camuri.handler   = handler_sysinfo;
    camuri.user_ctx  = (void*) base_path;   // Pass server data as context
    httpd_register_uri_handler(server, &camuri);
    
    camuri.uri       = "/starttime";
    camuri.handler   = handler_get_starttime;
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
    
    camuri.uri       = "/cpu_temperature";
    camuri.handler   = handler_cputemp;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/rssi";
    camuri.handler   = handler_rssi;
    camuri.user_ctx  = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/uptime";
    camuri.handler   = handler_uptime;
    camuri.user_ctx  = NULL;
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
