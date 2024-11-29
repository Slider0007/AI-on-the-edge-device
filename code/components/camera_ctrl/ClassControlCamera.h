#ifndef CLASSCONTROLCAMERA_H
#define CLASSCONTROLCAMERA_H

#include <string>
#include <vector>

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
    void powerResetCamera();
    esp_err_t initCam();
    esp_err_t deinitCam();

    bool testCamera(void);
    void printCamInfo(void);
    void printCamConfig(void);

    esp_err_t setCameraParameter(const CfgData::SectionTakeImage::Camera *paramCamera);
    void setCameraFrequency(int _frequency);
    void setImageQuality(int _qual);
    void setImageSize(int _zoomFactor, int _zoomOffsetX, int _zoomOffsetY);
    bool setImageManipulation(int _brightness, int _contrast, int _saturation, int _sharpness, int _exposureControlMode,
                              int _autoExposureLevel, int _manualExposureValue, int _gainControlMode, int _manualGainValue,
                              int _specialEffect, bool _mirror, bool _flip);
    bool setMirrorFlip(bool _mirror, bool _flip);

    bool getCameraInitSuccessful();
    camera_model_t getCamModel(void);
    std::string getCamType(void);
    std::string getCamPID(void);
    std::string getCamVersion(void);
    int getCamFrequencyMhz(void);
    void getOutputFrameSize(int &width, int &height);

    esp_err_t captureToBasisImage(CImageBasis *_Image);
    esp_err_t captureToFile(std::string _nm);
    esp_err_t captureToHTTP(httpd_req_t *_req);
    esp_err_t captureToStream(httpd_req_t *_req, bool _flashlightOn);

#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    void ledcInitFlashlightDefault(void);
#endif
    esp_err_t setFlashlightParameter(const CfgData::SectionTakeImage::Flashlight *paramFlashlight);
    void setFlashIntensity(int _flashIntensity);
    void setFlashTime(int _flashTime);
    int getFlashTime();
    void setFlashlight(bool _status);

    void enableDemoMode(void);
    void disableDemoMode(void);
    void freeDemoMemoryOnly();
};


extern ClassControlCamera cameraCtrl;

#endif
