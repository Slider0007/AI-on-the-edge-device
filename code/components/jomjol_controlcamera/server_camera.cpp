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
    char _query[384];
    char _valuechar[30];
    std::string task;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo = 
        "00: Handler usage:<br>"
        "1. Set camera parameter: http://192.168.2.68/camera?task=set_parameter&flashtime=2.0&flashintensity=1 "
        "&brightness=0&contrast=0&saturation=0&sharpness=0&autoexposurelevel=0&aec2=true&grayscale=false&negative=false "
        "&mirror=false&flip=false&zoom=true&zoommode=0&zoomx=0&zoomy=0";

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

        CameraParameter camParameter = Camera.getCameraParameter(); // Retrieve actual parameter settings

        if (httpd_query_key_value(_query, "flashtime", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.flashTime = (int)(stof(std::string(_valuechar)) * 1000); // flashTime in ms
        }
        if (httpd_query_key_value(_query, "flashintensity", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.flashIntensity = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "brightness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.brightness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "contrast", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.contrast = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "saturation", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.saturation = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "sharpness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.sharpness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "exposurecontrolmode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.exposureControlMode  = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "autoexposurelevel", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.autoExposureLevel = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "manualexposurevalue", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.manualExposureValue = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "gaincontrolmode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.gainControlMode = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "manualgainvalue", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.manualGainValue = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "specialeffect", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.specialEffect = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "mirror", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                camParameter.mirrorImage = true : camParameter.mirrorImage = false;
        }
        if (httpd_query_key_value(_query, "flip", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? 
                camParameter.flipImage = true : camParameter.flipImage = false;
        }
        if (httpd_query_key_value(_query, "zoommode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.zoomMode = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomx", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.zoomOffsetX = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomy", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            camParameter.zoomOffsetY = stoi(std::string(_valuechar));
        }

        Camera.setFlashIntensity(camParameter.flashIntensity);
        Camera.setFlashTime(camParameter.flashTime);
        Camera.setZoom(camParameter.zoomMode, camParameter.zoomOffsetX, camParameter.zoomOffsetY);
        Camera.setImageManipulation(camParameter.brightness, camParameter.contrast, camParameter.saturation, 
                                    camParameter.sharpness, camParameter.exposureControlMode, camParameter.autoExposureLevel, 
                                    camParameter.manualExposureValue, camParameter.gainControlMode, camParameter.manualGainValue, 
                                    camParameter.specialEffect, camParameter.mirrorImage, camParameter.flipImage);

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
