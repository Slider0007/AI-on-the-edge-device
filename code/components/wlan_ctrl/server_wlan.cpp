#include "server_wlan.h"
#include "../../include/defines.h"

#include <string>

#include "ClassLogFile.h"
#include "connect_wlan.h"


static const char *TAG = "SERVER_WLAN";


esp_err_t handler_wlan(httpd_req_t *req)
{
    const char *APIName = "wlan:v1"; // API name and version
    char _query[384];
    char _valuechar[30];
    std::string task;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = "Handler usage:<br>"
                                      "1. '/wlan?task=scan': Scan WLAN networks<br>"
                                      "2. '/wlan?task=api_name' : Print API name and version";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }
    else { // If no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, RESTUsageInfo.c_str());
        return ESP_OK;
    }

    if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("scan") == 0) {
        if (wifiScan(req) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Failed to scan wlan networks");
        }
        return ESP_OK;
    }
    else {
        httpd_resp_sendstr(req, "E90: Task not found");
        return ESP_ERR_NOT_FOUND;
    }
}


void registerWlanUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = {};
    camuri.method = HTTP_GET;

    camuri.uri = "/wlan";
    camuri.handler = handler_wlan;
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
