#include "server_camera.h"
#include "../../include/defines.h"

#include <string>

#include <esp_log.h>
#include "esp_camera.h"

#include "http_auth.h"
#include "ClassControlCamera.h"
#include "ClassLogFile.h"


static const char *TAG = "SERVER_CAM";


esp_err_t handler_camera(httpd_req_t *req)
{
    const char *APIName = "camera:v3"; // API name and version
    char _query[384];
    char _valuechar[30], _flashtime[30], _filename[100];
    std::string task;
    bool saveToFile = false;
    std::string fn = "/sdcard/";
    int flashtime = 0;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo =
        "Handler usage:<br>"
        "1. Capture (single shot) image with temporary camera parameter set (partial or full parameter set supported):<br>"
        "(Adding 'filename' parameter save image to SD card with given filename instead of response image)<br>"
        "- '/camera?task=capture&flashtime=2000&flashintensity=50&brightness=0&contrast=0&"
        "saturation=0&sharpness=1&exposurecontrolmode=1&autoexposurelevel=0&manualexposurevalue=300&"
        "gaincontrolmode=1&manualgainvalue=0&specialeffect=0&mirror=false&flip=false&zoomfactor=1000&"
        "zoomx=0&zoomy=0&filename=/img_tmp/filename.jpg'<br>"
        "2. Control flashlight:<br>"
        "- '/camera?task=flashlight_on' : Flashlight on<br>"
        "- '/camera?task=flashlight_off' : Flashlight off<br>"
        "3. Print API name and version:<br>"
        "- '/camera?task=api_name'";

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }
    else { // if no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, RESTUsageInfo.c_str());
        return ESP_OK;
    }

    if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("capture") == 0) {
        if (!cameraCtrl.getCameraInitSuccessful()) {
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Camera not initialized");
            return ESP_ERR_NOT_FOUND;
        }

        // Load actual parameter settings to allow partial parameter updates
        CfgData::SectionTakeImage::Camera paramCamera = ConfigClass::getInstance()->get()->sectionTakeImage.camera;
        CfgData::SectionTakeImage::Flashlight paramFlashlight = ConfigClass::getInstance()->get()->sectionTakeImage.flashlight;

        if (httpd_query_key_value(_query, "flashtime", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramFlashlight.flashTime = stoi(std::string(_valuechar)); // flashTime in ms
        }
        if (httpd_query_key_value(_query, "flashintensity", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramFlashlight.flashIntensity = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "brightness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.brightness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "contrast", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.contrast = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "saturation", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.saturation = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "sharpness", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.sharpness = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "exposurecontrolmode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.exposureControlMode = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "autoexposurelevel", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.autoExposureLevel = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "manualexposurevalue", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.manualExposureValue = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "gaincontrolmode", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.gainControlMode = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "manualgainvalue", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.manualGainValue = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "specialeffect", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.specialEffect = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "mirror", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? paramCamera.mirrorImage = true
                                                                                  : paramCamera.mirrorImage = false;
        }
        if (httpd_query_key_value(_query, "flip", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            (std::string(_valuechar) == "1" || std::string(_valuechar) == "true") ? paramCamera.flipImage = true
                                                                                  : paramCamera.flipImage = false;
        }
        if (httpd_query_key_value(_query, "zoomfactor", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.zoomFactor = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomx", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.zoomOffsetX = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "zoomy", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            paramCamera.zoomOffsetY = stoi(std::string(_valuechar));
        }
        if (httpd_query_key_value(_query, "filename", _filename, sizeof(_filename)) == ESP_OK) {
            fn.append(_filename);
            saveToFile = true;
        }

        // Capture image (single shot) with modified config
        esp_err_t retVal = ESP_FAIL;
        if (!saveToFile) {
            retVal = cameraCtrl.captureToHTTP(req, &paramCamera, &paramFlashlight);
        }
        else {
            retVal = cameraCtrl.captureToFile(fn, &paramCamera, &paramFlashlight);
            if (retVal == ESP_OK) {
                httpd_resp_sendstr(req, "001: Capture to file successful");
            }
        }

        return retVal;
    }
    else if (task.compare("flashlight_on") == 0) {
        cameraCtrl.setFlashlight(true);
        httpd_resp_sendstr(req, "002: Flashlight on");
        return ESP_OK;
    }
    else if (task.compare("flashlight_off") == 0) {
        cameraCtrl.setFlashlight(false);
        httpd_resp_sendstr(req, "003: Flashlight off");
        return ESP_OK;
    }
    else if (task.compare("stream") == 0) {
        if (!cameraCtrl.getCameraInitSuccessful()) {
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Camera not initialized");
            return ESP_ERR_NOT_FOUND;
        }
        cameraCtrl.captureToStream(req, false);
        httpd_resp_sendstr(req, "004: Camera livestream");
        return ESP_OK;
    }
    else if (task.compare("stream_flashlight") == 0) {
        if (!cameraCtrl.getCameraInitSuccessful()) {
            httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Camera not initialized");
            return ESP_ERR_NOT_FOUND;
        }
        cameraCtrl.captureToStream(req, true);
        httpd_resp_sendstr(req, "005: Camera livestream with flashlight");
        return ESP_OK;
    }
    else {
        httpd_resp_sendstr(req, "E90: Task not found");
        return ESP_ERR_NOT_FOUND;
    }
}


void registerCameraUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = {};
    camuri.method = HTTP_GET;

    camuri.uri = "/camera";
    camuri.handler = HTTP_AUTH_BASIC(handler_camera);
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
