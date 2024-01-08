#ifndef CLASSCONTROLLCAMERA_H
#define CLASSCONTROLLCAMERA_H

#include <string>
#include <vector>

#include <esp_http_server.h>
#include "esp_camera.h"

#include "CImageBasis.h"


typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;


class CCamera
{
    protected:
        uint8_t *demoImage; // Buffer holding the demo image in bytes
        int ActualQuality;
        framesize_t ActualResolution;
        int brightness, contrast, saturation;
        bool isFixedExposure;
        int flashduration;
        int led_intensity;

        bool CameraInitSuccessful;
        bool demoMode;
        std::vector<std::string> demoFiles;

        void LEDOnOff(bool status);
        bool loadNextDemoImage(camera_fb_t *fb);

    public:
        int image_height, image_width;
        
        CCamera();
        ~CCamera();
        void FreeMemoryOnly();
        void PowerResetCamera();
        esp_err_t InitCam();
        esp_err_t DeinitCam();
        bool testCamera(void);
        void printCamInfo(void);

        esp_err_t CaptureToBasisImage(CImageBasis *_Image, int delay = 0);
        esp_err_t CaptureToFile(std::string nm, int delay = 0);
        esp_err_t CaptureToHTTP(httpd_req_t *req, int delay = 0);
        esp_err_t CaptureToStream(httpd_req_t *req, bool FlashlightOn);

        void ledc_init(void);
        void SetCameraFrequency(int _frequency);
        void SetQualitySize(int qual, framesize_t resol);
        bool SetBrightnessContrastSaturation(int _brightness, int _contrast, int _saturation);
        framesize_t TextToFramesize(const char * text);
        void GetCameraParameter(httpd_req_t *req, int &qual, framesize_t &resol);
        void SetLEDIntensity(int _intrel);

        bool EnableAutoExposure(int flash_duration);
        bool getCameraInitSuccessful();
        void LightOnOff(bool status);

        void EnableDemoMode(void);
        void DisableDemoMode(void);
};


extern CCamera Camera;

#endif