#ifndef CLASSCONTROLCAMERA_H
#define CLASSCONTROLCAMERA_H

#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>

#include <esp_http_server.h>
#include <esp_camera.h>

#include "configClass.h"
#include "CImageBasis.h"


typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;


class ClassControlCamera
{
  protected:
    SemaphoreHandle_t camMutex;
    CfgData::SectionTakeImage::Camera paramCameraInternal;
    CfgData::SectionTakeImage::Flashlight paramFlashlightInternal;
    bool cameraInitSuccessful;
    uint16_t sensorFrameSizeWidth, sensorFrameSizeHeight;
    uint16_t outputFrameSizeWidth, outputFrameSizeHeight;

    bool demoMode;
    std::vector<std::string> demoFiles;

    void setStatusLed(bool status);
    bool loadNextDemoImage(camera_fb_t *_fb);

  public:
    ClassControlCamera();
    ~ClassControlCamera();
    esp_err_t initCam(void);
    esp_err_t deinitCam(void);

    void skipFrames(uint8_t n = 10);
    void powerCycle(void);

    void printCamInfo(void);
    void printCamConfig(void);

    esp_err_t setCameraParameter(const CfgData::SectionTakeImage::Camera *_paramCamera);
    void setCameraFrequency(int _frequency);
    void setImageQuality(int _qual);
    void setImageSize(int _zoomFactor, int _zoomOffsetX, int _zoomOffsetY);
    bool setImageManipulation(int _brightness, int _contrast, int _saturation, int _sharpness, int _exposureControlMode,
                              int _autoExposureLevel, int _manualExposureValue, int _gainControlMode, int _manualGainValue,
                              int _specialEffect, bool _mirror, bool _flip);
    bool setMirrorFlip(bool _mirror, bool _flip);

    bool getCameraInitSuccessful(void);
    camera_model_t getCamModel(void);
    std::string getCamType(void);
    std::string getCamPID(void);
    std::string getCamVersion(void);
    int getCamFrequencyMhz(void);
    void getOutputFrameSize(int &width, int &height);

    esp_err_t captureToBasisImage(CImageBasis *_image);
    esp_err_t captureToFile(std::string _file, CfgData::SectionTakeImage::Camera *_paramCameraTemp = NULL,
                            CfgData::SectionTakeImage::Flashlight *_paramFlashlightTemp = NULL);
    esp_err_t captureToHTTP(httpd_req_t *_req, CfgData::SectionTakeImage::Camera *_paramCameraTemp = NULL,
                            CfgData::SectionTakeImage::Flashlight *_paramFlashlightTemp = NULL);
    esp_err_t captureToStream(httpd_req_t *_req, bool _flashlightOn);

    void initFlashlight(void);
#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    void ledcInitFlashlightDefault(void);
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    esp_err_t setFlashlightParameter(const CfgData::SectionTakeImage::Flashlight *_paramFlashlight);
    void setFlashIntensity(int _flashIntensity);
    void setFlashTime(int _flashTime);
    int getFlashTime(void);
    void setFlashlight(bool _status);

    void enableDemoMode(void);
    void disableDemoMode(void);
    void freeDemoMemoryOnly(void);
};


extern ClassControlCamera cameraCtrl;

#endif // CLASSCONTROLCAMERA_H
