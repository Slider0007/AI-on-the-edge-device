#include "server_camera.h"
#include "../../include/defines.h"

#include <string>

#include "esp_log.h"
#include "esp_camera.h"

#include "ClassControllCamera.h"
#include "ClassLogFile.h"


static const char *TAG = "CAM_SERVER";


esp_err_t handler_camera(httpd_req_t *req)
{
    char _query[300];
    char _valuechar[30];
    std::string task;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = 
        "00: Handler usage:<br>"
        "- Set camera parameter: /camera?task=set_parameter&flashtime=2.0&flashintensity=1"
        "&brightness=0&contrast=0&saturation=0&sharpness=0NaNfalse&negative=false"
        "&autoexposurelevel=0&aec2=true&zoom=false";

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_send(req, RESTUsageInfo.c_str(), RESTUsageInfo.length());
        return ESP_OK; 
    }   
    
    if (task.compare("set_parameter") == 0) {
        if (!Camera.getcameraInitSuccessful()) {
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                                "Camera not initialized: REST API /lighton not available");
            return ESP_ERR_NOT_FOUND;
        }
        
        int flashIntensity = 50;
        int flashTime = 2000; // Flashtime in ms
        int brightness = 0;
        int saturation = 0;
        int contrast = 0;
        int sharpness = 0;
        int autoExposureLevel = 0;
        bool aec2 = false;
        int autoExposure2Value = 300;
        #ifdef GRAYSCALE_AS_DEFAULT
        bool grayscale = true;
        #else
        bool grayscale = false;
        #endif
        bool negative = false;
        bool mirror = false;
        bool flip = false;
        bool zoom = false;
        int zoomMode = 0;
        int zoomOffsetX = 0;
        int zoomOffsetY = 0;

        if (httpd_query_key_value(_query, "flashintensity", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            flashIntensity = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "flashtime", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            flashTime = (int)(stof(std::string(_valuechar)) * 1000); // flashTime in ms
        }
        if (httpd_query_key_value(_query, "brightness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            brightness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "contrast", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            contrast = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "saturation", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            saturation = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "sharpness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            sharpness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "autoexposurelevel", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            autoExposureLevel = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "aec2", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                aec2 = true : aec2 = false;
        }
        if (httpd_query_key_value(_query, "grayscale", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                grayscale = true : grayscale = false;
        }
        if (httpd_query_key_value(_query, "negative", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                negative = true : negative = false;
        }
        if (httpd_query_key_value(_query, "mirror", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                mirror = true : mirror = false;
        }
        if (httpd_query_key_value(_query, "flip", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                flip = true : flip = false;
        }
        if (httpd_query_key_value(_query, "zoom", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                zoom = true : zoom = false;
        }
        if (httpd_query_key_value(_query, "zoommode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            zoomMode = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomx", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            zoomOffsetX = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomy", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            zoomOffsetY = stoi(std::string(_valuechar));
        }

        Camera.setFlashIntensity(flashIntensity);
        Camera.setFlashTime(flashTime);
        Camera.setZoom(zoom, zoomMode, zoomOffsetX, zoomOffsetY);
        Camera.setImageManipulation(brightness, contrast, saturation, sharpness, 
                                    autoExposureLevel, aec2, grayscale, negative, mirror, flip);

        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_sendstr(req, "001: Parameter set");
        return ESP_OK;
    }
    else {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_sendstr(req, "E90: Task not found");
        return ESP_ERR_NOT_FOUND;
    }
}


esp_err_t handler_lightOn(httpd_req_t *req)
{
    if (!Camera.getcameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                            "Camera not initialized: REST API /lighton not available");
        return ESP_ERR_NOT_FOUND;
    }
    
    Camera.setFlashlight(true);
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}


esp_err_t handler_lightOff(httpd_req_t *req)
{
    if (!Camera.getcameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                            "Camera not initialized: REST API /lightoff not available");
        return ESP_ERR_NOT_FOUND;
    }

    Camera.setFlashlight(false);
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}


esp_err_t handler_capture(httpd_req_t *req)
{
    if (!Camera.getcameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                            "Camera not initialized: REST API /capture not available");
        return ESP_ERR_NOT_FOUND;
    }

    int save_flashTime = Camera.getFlashTime();
    Camera.setFlashTime(0);
    esp_err_t result = Camera.captureToHTTP(req);
    Camera.setFlashTime(save_flashTime);

    return result;
}


esp_err_t handler_capture_with_light(httpd_req_t *req)
{
    if (!Camera.getcameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                            "Camera not initialized: REST API /capture_with_flashlight not available");
        return ESP_ERR_NOT_FOUND;
    }

    char _query[100];
    char _valuechar[10];
    int delay = 2000;

    if (httpd_req_get_url_query_str(req, _query, 100) == ESP_OK)
    {
        ESP_LOGD(TAG, "Query: %s", _query);
        if (httpd_query_key_value(_query, "delay", _valuechar, sizeof(_valuechar)) == ESP_OK) {      
            delay = atoi(_valuechar);

            if (delay < 0)
                delay = 0;
        }
    }

    int save_flashTime = Camera.getFlashTime();
    Camera.setFlashTime(delay);
    esp_err_t result = Camera.captureToHTTP(req);
    Camera.setFlashTime(save_flashTime);

    return result;

}


esp_err_t handler_capture_save_to_file(httpd_req_t *req)
{
    if (!Camera.getcameraInitSuccessful()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                            "Camera not initialized: REST API /save not available");
        return ESP_ERR_NOT_FOUND;
    }

    std::string fn = "/sdcard/";
    char _query[100];
    char filename[100];
    char _valuechar[10];
    int delay = 2000;

    if (httpd_req_get_url_query_str(req, _query, 100) == ESP_OK)
    {
        ESP_LOGD(TAG, "Query: %s", _query);
        if (httpd_query_key_value(_query, "filename", filename, sizeof(filename)) == ESP_OK) {
            fn.append(filename);
        }
        else {
            fn.append("noname.jpg");
        }

        if (httpd_query_key_value(_query, "delay", _valuechar, sizeof(_valuechar)) == ESP_OK) {      
            delay = atoi(_valuechar);

            if (delay < 0)
                delay = 0;
        }
    }
    else {
        fn.append("noname.jpg");
    }

    int save_flashTime = Camera.getFlashTime();
    Camera.setFlashTime(delay);
    esp_err_t result = Camera.captureToFile(fn);
    Camera.setFlashTime(save_flashTime);

    const char* resp_str = (const char*) fn.c_str();
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return result;
}


void register_server_camera_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;
  
    camuri.uri       = "/camera";
    camuri.handler   = handler_camera;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/lighton";
    camuri.handler   = handler_lightOn;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

    camuri.uri       = "/lightoff";
    camuri.handler   = handler_lightOff;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);    

    camuri.uri       = "/capture";
    camuri.handler   = handler_capture;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);      

    camuri.uri       = "/capture_with_flashlight";
    camuri.handler   = handler_capture_with_light;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);  

    camuri.uri       = "/save";
    camuri.handler   = handler_capture_save_to_file;
    camuri.user_ctx  = NULL; 
    httpd_register_uri_handler(server, &camuri);    
}
