#ifndef CLASSCONTROLCAMERA_H
#define CLASSCONTROLCAMERA_H

#include "../../include/defines.h"

#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>

#include <esp_http_server.h>
#include <esp_camera.h>

#include "configClass.h"
#include "CImage.h"


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

    void skipFrames(uint8_t n = 10);
    void powerCycle(void);

    void printCamInfo(void);
    void printCamConfig(void);

    bool setCameraFrequency(void);
    void setImageQuality(void);
    void setImageSize(void);
    bool setImageManipulation(void);

#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    void ledcInitFlashlightDefault(void);
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_PWM

    void setStatusLed(bool status);
    bool loadNextDemoImage(camera_fb_t *_fb);

  public:
    ClassControlCamera();
    ~ClassControlCamera();
    esp_err_t initCam(bool initialInit = false);
    esp_err_t deinitCam(void);

    esp_err_t setCameraParameter(const CfgData::SectionTakeImage::Camera *_paramCamera = NULL);

    bool getCameraInitSuccessful(void);
    camera_model_t getCamModel(void);
    std::string getCamType(void);
    std::string getCamPID(void);
    std::string getCamVersion(void);
    int getCamFrequencyMhz(void);
    void getOutputFrameSize(int &width, int &height);

    esp_err_t captureToBasisImage(CImage *_image);
    esp_err_t captureToFile(std::string _file, CfgData::SectionTakeImage::Camera *_paramCameraTemp = NULL,
                            CfgData::SectionTakeImage::Flashlight *_paramFlashlightTemp = NULL);
    esp_err_t captureToHTTP(httpd_req_t *_req, CfgData::SectionTakeImage::Camera *_paramCameraTemp = NULL,
                            CfgData::SectionTakeImage::Flashlight *_paramFlashlightTemp = NULL);
    esp_err_t captureToStream(httpd_req_t *_req, bool _flashlightOn);

    void initFlashlight(void);
    esp_err_t setFlashlightParameter(const CfgData::SectionTakeImage::Flashlight *_paramFlashlight = NULL);
    void setFlashlight(bool _status);

    void enableDemoMode(void);
    void disableDemoMode(void);
    void freeDemoMemoryOnly(void);
};


extern ClassControlCamera cameraCtrl;

#endif // CLASSCONTROLCAMERA_H
