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
    const char* APIName = "camera:v2"; // API name and version
    char _query[384];
    char _valuechar[30], _flashtime[30], _filename[100];
    std::string task;
    std::string fn = "/sdcard/";
    int flashtime = 0;

    // Default usage message when handler gets called without any parameter
    const std::string RESTUsageInfo =
        "Handler usage:<br>"
        "1. Set camera parameter:<br>"
        "-  '/camera?task=set_parameter&flashtime=0.1&flashintensity=1&brightness=-2&contrast=0& "
            "saturation=0&sharpness=1&exposurecontrolmode=0&autoexposurelevel=0&manualexposurevalue=1200& "
            "gaincontrolmode=0&manualgainvalue=2&specialeffect=0&mirror=false&flip=false&zoommode=0&zoomx=0&zoomy=0'<br>"
        "2. Capture image<br>"
        "  - '/camera?task=capture' : Capture without flashlight<br>"
        "  - '/camera?task=capture_with_flashlight&flashtime=1000' : Capture with flashlight (flashtime in ms)<br>"
        "  - '/camera?task=capture_to_file&flashtime=1000&filename=/img_tmp/filename.jpg' : \
            Capture image with flashlight (flashtime in ms) and save '/img_tmp/filename.jpg' onto SD-card<br>"
        "3. Control Flashlight<br>"
        "  - '/camera?task=flashlight_on' : Flashlight on<br>"
        "  - '/camera?task=flashlight_off' : Flashlight off<br>"
        "4. '/camera?task=api_name' : Print API name and version<br>";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
        if (httpd_query_key_value(_query, "flashtime", _flashtime, sizeof(_flashtime)) == ESP_OK) {
            flashtime = std::max(0, atoi(_flashtime));
        }
        if (httpd_query_key_value(_query, "filename", _filename, sizeof(_filename)) == ESP_OK) {
            fn.append(_filename);
        }
    }
    else {  // if no parameter is provided, print handler usage
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, RESTUsageInfo.c_str());
        return ESP_OK;
    }

    if (task.compare("api_name") == 0) {
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("set_parameter") == 0) {
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

        httpd_resp_sendstr(req, "001: Camera parameter set");
        return ESP_OK;
    }
    else if (task.compare("capture") == 0) {
        int save_flashTime = Camera.getFlashTime();
        Camera.setFlashTime(0);
        esp_err_t result = Camera.captureToHTTP(req);
        Camera.setFlashTime(save_flashTime);

        if (result == ESP_OK) {
            httpd_resp_sendstr(req, "002: Capture without flashlight successful");
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Camera capture error");
        }
        return result;
    }
    else if (task.compare("capture_with_flashlight") == 0) {
        if (flashtime == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                "E93: No flashtime provided, e.g. '/capture?task=capture_with_flashlight&flashtime=1000'");
            return ESP_FAIL;
        }

        int save_flashTime = Camera.getFlashTime();
        Camera.setFlashTime(flashtime);
        esp_err_t result = Camera.captureToHTTP(req);
        Camera.setFlashTime(save_flashTime);

        if (result == ESP_OK) {
            httpd_resp_sendstr(req, "003: Capture with flashlight successful");
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Camera capture error");
        }

        return result;
    }
    else if (task.compare("capture_to_file") == 0) {
        if (flashtime == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                "E93: No flashtime provided, e.g. '/capture?task=capture_to_file&flashtime=1000&filename=/img_tmp/test.jpg'");
            return ESP_FAIL;
        }

        if (fn.compare("/sdcard/") == 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                "E94: No destination provided, e.g. '/capture?task=capture_to_file&flashtime=1000&filename=/img_tmp/test.jpg'");
            return ESP_FAIL;
        }

        int save_flashTime = Camera.getFlashTime();
        Camera.setFlashTime(flashtime);
        esp_err_t result = Camera.captureToFile(fn);
        Camera.setFlashTime(save_flashTime);

        if (result == ESP_OK) {
            httpd_resp_sendstr(req, "004: Capture to file successful");
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Camera capture error");
        }
        return result;
    }
    else if (task.compare("flashlight_on") == 0) {
        Camera.setFlashlight(true);
        httpd_resp_sendstr(req, "005: Flashlight on");
        return ESP_OK;
    }
    else if (task.compare("flashlight_off") == 0) {
        Camera.setFlashlight(false);
        httpd_resp_sendstr(req, "006: Flashlight off");
        return ESP_OK;
    }
    else if (task.compare("stream") == 0) {
        Camera.captureToStream(req, false);
        httpd_resp_sendstr(req, "007: Camera livestream");
        return ESP_OK;
    }
    else if (task.compare("stream_flashlight") == 0) {
        Camera.captureToStream(req, true);
        httpd_resp_sendstr(req, "008: Camera livestream with flashlight");
        return ESP_OK;
    }
    else {
        httpd_resp_sendstr(req, "E90: Task not found");
        return ESP_ERR_NOT_FOUND;
    }
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
}