#include "server_camera.h"
#include "../../include/defines.h"

#include <string>

#include "esp_log.h"
#include "esp_camera.h"

#include "ClassControllCamera.h"
#include "ClassLogFile.h"


static const char *TAG = "CAM_SERVER";


esp_err_t handler_flashlight(httpd_req_t *req)
{
    if (!Camera.getCameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E90: Camera not initialized");
        return ESP_FAIL;
    }

    const char* APIName = "flashlight:v2"; // API name and version
    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = 
        "00: Handler usage:<br>"
        "- '/flashlight?type=api_name : Print API name and version<br>"
        "- '/flashlight?type=on' : Flashlight on<br>"
        "- '/flashlight?type=off' : Flashlight off<br>";
    char _query[100];
    char _valuechar[30];    
    std::string type;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    
    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {        
        if (httpd_query_key_value(_query, "type", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            type = std::string(_valuechar);
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, RESTUsageInfo.c_str());
        return ESP_FAIL; 
    }
    
    if (type.compare("on") == 0) {
        Camera.LightOnOff(true);
        httpd_resp_sendstr(req, "001: Flashlight on");
        return ESP_OK;        
    }
    else if (type.compare("off") == 0) {
        Camera.LightOnOff(false);
        httpd_resp_sendstr(req, "002: Flashlight off");
        return ESP_OK;        
    }
    else if (type.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E92: Parameter not found");
        return ESP_FAIL;  
    }
}


esp_err_t handler_capture(httpd_req_t *req)
{
    if (!Camera.getCameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E90: Camera not initialized");
        return ESP_FAIL;
    }

    const char* APIName = "capture:v2"; // API name and version
    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = 
        "00: Handler usage:<br>"
        "- '/capture?type=api_name : Print API name and version<br>"
        "- '/capture?type=capture' : Capture without flashlight<br>"
        "- '/capture?type=capture_with_flashlight&flashduration=1000' : Capture with flashlight (flashduration in ms)<br>"
        "- '/capture?type=capture_to_file&flashduration=1000&filename=/img_tmp/filename.jpg' : \
            Capture image with flashlight (flashduration in ms) and save '/img_tmp/filename.jpg' onto SD-card<br>";
    char _query[100];
    char _valuechar[30], _flashduration[30], _filename[100];   
    std::string type;
    std::string fn = "/sdcard/";
    int flashduration = 0;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain");
    
    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {        
        if (httpd_query_key_value(_query, "type", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            type = std::string(_valuechar);
        }
        if (httpd_query_key_value(_query, "flashduration", _flashduration, sizeof(_flashduration)) == ESP_OK) {     
            flashduration = atoi(_flashduration);
            if (flashduration < 0)
                flashduration = 0;
        }
        if (httpd_query_key_value(_query, "filename", _filename, sizeof(_filename)) == ESP_OK) {
            fn.append(_filename);
            #ifdef DEBUG_DETAIL_ON   
                ESP_LOGD(TAG, "Filename: %s", fn.c_str());
            #endif
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, RESTUsageInfo.c_str());
        return ESP_FAIL; 
    }

    if (type.compare("capture") == 0) {
        int quality;
        framesize_t res;

        Camera.GetCameraParameter(req, quality, res);
        #ifdef DEBUG_DETAIL_ON   
            ESP_LOGD(TAG, "Size: %d, Quality: %d", res, quality);
        #endif
        Camera.SetQualitySize(quality, res);

        esp_err_t result = Camera.CaptureToHTTP(req);

        if (result == ESP_OK) {
            httpd_resp_sendstr(req, "001: Capture without flashlight successful");
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Camera capture error");
        } 
        return result;        
    }
    else if (type.compare("capture_with_flashlight") == 0) {
        if (flashduration == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, 
                "E93: No flashduration provided, e.g. '/capture?type=CaptureWithFlashlight&flashduration=1000'");
            return ESP_FAIL;
        }

        int quality;
        framesize_t res;

        Camera.GetCameraParameter(req, quality, res);
        #ifdef DEBUG_DETAIL_ON   
            ESP_LOGD(TAG, "Size: %d, Quality: %d", res, quality);
        #endif
        Camera.SetQualitySize(quality, res);

        esp_err_t result = Camera.CaptureToHTTP(req, flashduration);

        if (result != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Camera capture error");
        } 
        return result;        
    }
    else if (type.compare("capture_to_file") == 0) {
        if (flashduration == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, 
                "E93: No flashduration provided, e.g. '/capture?type=CaptureWithFlashlight&flashduration=1000'");
            return ESP_FAIL;
        }

        if (fn.compare("/sdcard/") == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, 
                "E94: No destination provided, e.g. '/capture?type=CaptureWithFlashlight&flashduration=1000&filename=/img_tmp/test.jpg'");
            return ESP_FAIL;
        }
        int quality;
        framesize_t res;

        Camera.GetCameraParameter(req, quality, res);
        #ifdef DEBUG_DETAIL_ON   
            ESP_LOGD(TAG, "Size: %d, Quality: %d", res, quality);
        #endif
        Camera.SetQualitySize(quality, res);

        esp_err_t result = Camera.CaptureToFile(fn, flashduration);

        if (result == ESP_OK) {
            httpd_resp_sendstr(req, "001: Capture to file successful");
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Camera capture error");
        } 
        return result;        
    }
    if (type.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;        
    }
    else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "E94: Parameter not found");
        return ESP_FAIL;  
    }
}


void register_server_camera_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;

    camuri.uri       = "/flashlight";
    camuri.handler   = handler_flashlight;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/capture";
    camuri.handler   = handler_capture;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);      
}