#include "ClassControlCamera.h"
#include "../../include/defines.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <nvs_flash.h>
#include <sys/param.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_log.h>

#include "ov2640_sharpness.h"
#include "psram.h"
#include "helper.h"
#include "statusled.h"
#include "CImageBasis.h"
#include "ClassLogFile.h"
#include "server_ota.h"
#include "gpioControl.h"
#include "MainFlowControl.h"


static const char *TAG = "CAMCTRL";

ClassControlCamera cameraCtrl;

/* Camera live stream */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


static camera_config_t cameraConfig = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000, // Frequency (20Mhz)

    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,    // YUV422, GRAYSCALE, RGB565, JPEG
    .frame_size = FRAMESIZE_VGA,       // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 12,                // 0-63 lower number means higher quality
    .fb_count = 1,                     // if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM, // The location where the frame buffer will be allocated */
    .grab_mode = CAMERA_GRAB_LATEST    // only from new esp32cam version
};


ClassControlCamera::ClassControlCamera()
{
    outputFrameSizeWidth = CAMERA_OUTPUT_WINDOW_SIZE_WIDTH;
    outputFrameSizeHeight = CAMERA_OUTPUT_WINDOW_SIZE_HEIGHT;

    cameraInitSuccessful = false;

    demoMode = false;

#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    ledcInitFlashlightDefault();
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_PWM
}


esp_err_t ClassControlCamera::initCam()
{
    if (cameraInitSuccessful) {
        deinitCam(); // De-init in case it was already initialized
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // Init camera
    esp_err_t err = esp_camera_init(&cameraConfig);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Camera init failed: " + intToHexString(err));

        if (err == ESP_ERR_NOT_FOUND) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "No camera detected, check camera and electrical connection");
        }
        else if (err == ESP_ERR_NOT_SUPPORTED) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Detected camera model or JPEG format is not supported");
        }

        return err;
    }
    cameraInitSuccessful = true;

    vTaskDelay(pdMS_TO_TICKS(100));

    // Set camera model in config struct
    ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.cameraModel = getCamModel();
    ConfigClass::getInstance()->reinitConfig();

    // Get actual config
    CfgData::SectionTakeImage::Camera paramCameraInternal = ConfigClass::getInstance()->get()->sectionTakeImage.camera;
    CfgData::SectionTakeImage::Flashlight paramFlashlightInternal = ConfigClass::getInstance()->get()->sectionTakeImage.flashlight;

    // Set sensor framesize dimension
    sensorFrameSizeWidth = resolution[camera_sensor[paramCameraInternal.cameraModel].max_size].width;
    sensorFrameSizeHeight = resolution[camera_sensor[paramCameraInternal.cameraModel].max_size].height;

    return ESP_OK;
}


esp_err_t ClassControlCamera::deinitCam()
{
    cameraInitSuccessful = false;
    esp_camera_deinit(); // De-init in case it was already initialized (returns ESP_FAIL if deinit is already done)
    powerResetCamera();

    return ESP_OK;
}


void ClassControlCamera::powerResetCamera()
{
#if PWDN_GPIO_NUM == -1 // Use reset only if pin is available
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "No power down pin availbale to reset camera");
#else
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Resetting camera by power down");
    gpio_config_t conf;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << PWDN_GPIO_NUM;
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&conf);

    // Be careful, logic is inverted compared to reset pin
    gpio_set_level(PWDN_GPIO_NUM, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(PWDN_GPIO_NUM, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif // PWDN_GPIO_NUM == -1
}


bool ClassControlCamera::testCamera(void)
{
    bool retval;
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Camera framebuffer check failed");
        return false;
    }

    esp_camera_fb_return(fb);
    return true;
}


void ClassControlCamera::printCamInfo(void)
{
    // Print camera infos
    // ********************************************
    char caminfo[96];
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "printCamInfo: Failed to get control structure");
        return;
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);

    sprintf(caminfo, "TYPE: %s, PID: 0x%02x, VER: 0x%02x, MIDL: 0x%02x, MIDH: 0x%02x, FREQ: %dMhz", info->name, s->id.PID, s->id.VER,
            s->id.MIDH, s->id.MIDL, s->xclk_freq_hz / 1000000);
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Info: " + std::string(caminfo));
}


void ClassControlCamera::printCamConfig(void)
{
    // Print camera config
    // ********************************************
    char camconfig[512];

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "printCamConfig: Failed to get control structure");
        return;
    }

    sprintf(camconfig,
            "ae_level:%d, aec2:%d, aec:%d, aec_value:%d, agc:%d, agc_gain:%d, awb:%d, awb_gain:%d, "
            "binning:%d, bpc:%d, brightness:%d, colorbar:%d, contrast:%d, dcw:%d, deonoise:%d, framesize:%d, "
            "gainceiling:%d, hmirror:%d, lenc:%d, quality:%d, raw_gma:%d, saturation:%d, scale:%d, sharpness:%d, "
            "special_effect:%d, vflip:%d, wb_mode:%d, wpc:%d",
            s->status.ae_level, s->status.aec2, s->status.aec, s->status.aec_value, s->status.agc, s->status.agc_gain, s->status.awb,
            s->status.awb_gain, s->status.binning, s->status.bpc, s->status.brightness, s->status.colorbar, s->status.contrast,
            s->status.dcw, s->status.denoise, s->status.framesize, s->status.gainceiling, s->status.hmirror, s->status.lenc,
            s->status.quality, s->status.raw_gma, s->status.saturation, s->status.scale, s->status.sharpness, s->status.special_effect,
            s->status.vflip, s->status.wb_mode, s->status.wpc);
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Camera config: " + std::string(camconfig));
}


esp_err_t ClassControlCamera::setCameraParameter(const CfgData::SectionTakeImage::Camera *paramCamera)
{
    paramCameraInternal = *(CfgData::SectionTakeImage::Camera *)paramCamera;

    setCameraFrequency(paramCameraInternal.cameraFrequency);
    setImageQuality(paramCameraInternal.imageQuality);
    setImageSize(paramCameraInternal.zoomFactor, paramCameraInternal.zoomOffsetX, paramCameraInternal.zoomOffsetY);
    setImageManipulation(paramCameraInternal.brightness, paramCameraInternal.contrast, paramCameraInternal.saturation,
                         paramCameraInternal.sharpness, paramCameraInternal.exposureControlMode, paramCameraInternal.autoExposureLevel,
                         paramCameraInternal.manualExposureValue, paramCameraInternal.gainControlMode, paramCameraInternal.manualGainValue,
                         paramCameraInternal.specialEffect, paramCameraInternal.mirrorImage, paramCameraInternal.flipImage);

    return ESP_OK;
}


void ClassControlCamera::setCameraFrequency(int _frequency)
{
    if (!cameraInitSuccessful) {
        return;
    }

    paramCameraInternal.cameraFrequency = _frequency;

    if (cameraConfig.xclk_freq_hz == (paramCameraInternal.cameraFrequency * 1000000)) { // If frequency is matching, return without any
                                                                                        // action
        return;
    }

    if (paramCameraInternal.cameraFrequency >= 5 && paramCameraInternal.cameraFrequency <= 20) {
        cameraConfig.xclk_freq_hz = paramCameraInternal.cameraFrequency * 1000000;
    }
    else {
        cameraConfig.xclk_freq_hz = 2000000;
    }

    initCam();
    printCamInfo();
}


void ClassControlCamera::setImageQuality(int _qual)
{
    if (!cameraInitSuccessful) {
        return;
    }

    paramCameraInternal.imageQuality = std::min(63, std::max(8, _qual)); // Limit quality from 8..63 (values lower than 8 tent to be
                                                                         // unstable)

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setSizeQuality: Failed to get control structure");
        return;
    }
    s->set_quality(s, paramCameraInternal.imageQuality);
}


void ClassControlCamera::setImageSize(int _zoomFactor, int _zoomOffsetX, int _zoomOffsetY)
{
    if (!cameraInitSuccessful) {
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setImageSize: Failed to get control structure");
        return;
    }

    // Preload internal structure
    if (paramCameraInternal.cameraModel == CAMERA_OV2640) {
        paramCameraInternal.zoomFactor = std::clamp(_zoomFactor, 1000, 2500); // [1.0x .. 2.5x]
    }
    else if (paramCameraInternal.cameraModel == CAMERA_OV5640) {
        paramCameraInternal.zoomFactor = std::clamp(_zoomFactor, 1000, 4000); // [1.0x .. 4.0x]
    }
    else {
        paramCameraInternal.zoomFactor = 1000;
    }

    paramCameraInternal.zoomOffsetX = _zoomOffsetX;
    paramCameraInternal.zoomOffsetY = _zoomOffsetY;

    // Calculate image size (keep original ratio) based on zoom factor to realize zoomed image
    uint16_t imageWidthZoomed = (sensorFrameSizeWidth * 1000) / paramCameraInternal.zoomFactor;
    imageWidthZoomed += (imageWidthZoomed % 4); // Make it dividable by 4

    uint16_t imageHeightZoomed = (sensorFrameSizeHeight * 1000) / paramCameraInternal.zoomFactor;
    imageHeightZoomed += (imageHeightZoomed % 4); // Make it dividable by 4

    // Determine max offset values based on resulting image (with zoom factor applied)
    const int imageZoomOffsetXMax = (sensorFrameSizeWidth - imageWidthZoomed) / 2;
    const int imageZoomOffsetYMax = (sensorFrameSizeHeight - imageHeightZoomed) / 2;

    // Sanitize user provided offset values
    const int16_t imageZoomOffsetX = std::clamp(paramCameraInternal.zoomOffsetX, -1 * imageZoomOffsetXMax, imageZoomOffsetXMax);
    const int16_t imageZoomOffsetY = std::clamp(paramCameraInternal.zoomOffsetY, -1 * imageZoomOffsetYMax, imageZoomOffsetYMax);

    if (paramCameraInternal.cameraModel == CAMERA_OV2640) {
        // NOTE: No sensor offset required (x = 0, y = 0 --> see ov2640_settings.h: ratio_table -> 4x3)
        uint16_t offsetX = imageZoomOffsetXMax + imageZoomOffsetX;
        if (offsetX % 2) { // Make it odd to avoid tinted image
            offsetX += 1;
        }
        uint16_t offsetY = imageZoomOffsetYMax + imageZoomOffsetY;
        if (offsetY % 2) { // Make it odd to avoid tinted image
            offsetY += 1;
        }

        // Set customized resolution (and scale image to output resolution)
        //   NOTE 1: Function offset parameter based on image top-left (0,0). imageZoomOffsetX,Y are +/- values based on image center
        //   NOTE 2: Parameter startX --> Sensor frame size (0: 1600 x 1200)
        //   NOTE 3: Unused parameters: startY, endX, endY, scale, binning
        s->set_res_raw(s, 0, 0, 0, 0, offsetX, offsetY, imageWidthZoomed, imageHeightZoomed, outputFrameSizeWidth, outputFrameSizeHeight,
                       false, false);
    }
    else if (paramCameraInternal.cameraModel == CAMERA_OV5640) {
        // NOTE: Add sensor offset (x = 32, y = 16 --> see ov5640_settings.h: ratio_table -> 4x3)
        const uint8_t sensorOffsetX = 16; // Offset / 2
        const uint8_t sensorOffsetY = 8;  // Offset / 2

        uint16_t ispWindowXStart = sensorOffsetX + imageZoomOffsetX + (sensorFrameSizeWidth - imageWidthZoomed) / 2;
        if (ispWindowXStart < sensorOffsetX) { // If too low set to sensor offset
            ispWindowXStart = sensorOffsetX;
        }
        if (ispWindowXStart % 2) { // Make it odd to avoid tinted image
            ispWindowXStart += 1;
        }

        uint16_t ispWindowYStart = sensorOffsetY + imageZoomOffsetY + (sensorFrameSizeHeight - imageHeightZoomed) / 2;
        if (ispWindowYStart < sensorOffsetY) { // If too low set to sensor offset
            ispWindowYStart = sensorOffsetY;
        }
        if (ispWindowYStart % 2) { // Make it odd to avoid tinted image
            ispWindowYStart += 1;
        }

        const uint16_t ispWindowXEnd = ispWindowXStart + imageWidthZoomed - 1;
        const uint16_t ispWindowYEnd = ispWindowYStart + imageHeightZoomed - 1;

        // Set total sensor pixel count (incl. dark pixel) --> see ov2640_settings.h: ratio_table -> 4x3
        const uint16_t sensorTotalPixelX = 2844;
        const uint16_t sensorTotalPixelY = 1968;

#ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "SensorSize W:%d, H:%d | ImageZoomed W:%d, H:%d | Offset X:%d, Y:%d | ISPWindowStart X:%d, Y:%d",
                 sensorFrameSizeWidth, sensorFrameSizeHeight, imageWidthZoomed, imageHeightZoomed, imageZoomOffsetX, imageZoomOffsetY,
                 ispWindowXStart, ispWindowXEnd);
#endif // DEBUG_DETAIL_ON

        // Set customized resolution (and scale image to output resolution)
        //   NOTE: Function offset parameter are not used --> Offsets are applied to start values
        s->set_res_raw(s, ispWindowXStart, ispWindowYStart, ispWindowXEnd, ispWindowYEnd, 0, 0, sensorTotalPixelX, sensorTotalPixelY,
                       outputFrameSizeWidth, outputFrameSizeHeight, true, false);
    }
    else {
        s->set_framesize(s, FRAMESIZE_VGA);
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "setImageSize: Camera model not fully supported. Zoom functionality disabled");
    }
}


bool ClassControlCamera::setImageManipulation(int _brightness, int _contrast, int _saturation, int _sharpness, int _exposureControlMode,
                                              int _autoExposureLevel, int _manualExposureValue, int _gainControlMode, int _manualGainValue,
                                              int _specialEffect, bool _mirror, bool _flip)
{
    if (!cameraInitSuccessful) {
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setImageManipulation: Failed to get control structure");
        return false;
    }

    paramCameraInternal.brightness = _brightness;
    paramCameraInternal.contrast = _contrast;
    paramCameraInternal.saturation = _saturation;
    paramCameraInternal.sharpness = _sharpness;
    paramCameraInternal.exposureControlMode = _exposureControlMode;
    paramCameraInternal.autoExposureLevel = _autoExposureLevel;
    paramCameraInternal.manualExposureValue = _manualExposureValue;
    paramCameraInternal.gainControlMode = _gainControlMode;
    paramCameraInternal.manualGainValue = _manualGainValue;
    paramCameraInternal.specialEffect = _specialEffect;
    paramCameraInternal.mirrorImage = _mirror;
    paramCameraInternal.flipImage = _flip;

    // Basic image manipulation
    // *********************************************************************
    s->set_saturation(s, std::min(2, std::max(-2, paramCameraInternal.saturation))); // [-2 .. 2]
    s->set_contrast(s, std::min(2, std::max(-2, paramCameraInternal.contrast)));     // [-2 .. 2]
    s->set_brightness(s, std::min(2, std::max(-2, paramCameraInternal.brightness))); // [-2 .. 2] (IMPORTANT: Apply brightness after
                                                                                     // saturation and conrast)

    // Set special effect (0: None, 1: Negative, 2: Grayscale, 3: Reddish, 4: Greenish, 5: Blueish, 6: Sepia)
    // *********************************************************************
    if (paramCameraInternal.specialEffect >= 0 && paramCameraInternal.specialEffect <= 6) {
        s->set_special_effect(s, paramCameraInternal.specialEffect); // [0 .. 6]
    }
    // Set sepcial effect: 7: Grayscale + Negative in combination. Do grayscale on camera + negative on MCU
    else if (paramCameraInternal.specialEffect == 7) {
        s->set_special_effect(s, 2); // 2: Grayscale
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setImageManipulation: Selected special effect unknown");
        return false;
    }

    // Camera specific handling
    // *********************************************************************
    if (paramCameraInternal.cameraModel == CAMERA_OV2640) {
        // Enable contrast (and brightness), saturation and optional special effects
        // *********************************************************************
        //   Workaround: Bug in camera library: Enable bits are set without using bitwise OR logic -> only latest enabled setting is used
        //   Reference: https://esp32.com/viewtopic.php?f=19&t=14376#p93178

        // Set bit 1, 2 to enable saturation, contrast
        int registerValue = 0x06;

        // Bitwise OR of special effect enable bits
        if (paramCameraInternal.specialEffect == 1) { // Sepcial effect: 1: negative
            registerValue |= 0x40;
        }
        // Sepcial effect: 2: grayscale, 3: reddish, 4: greenish, 5: blueish, 6: sepia
        else if (paramCameraInternal.specialEffect >= 2 && paramCameraInternal.specialEffect <= 6) {
            registerValue |= 0x18;
        }
        // Sepcial effect: 7: Grayscale + Negative in combination
        //   NOTE: It's not possible to process both together on camera
        else if (paramCameraInternal.specialEffect == 7) {
            registerValue |= 0x18; // Workaround: Do grayscale on camera + negative on MCU
                                   // Disadvantage: Effect in combination not visible in other camera consumers like live stream / REST API
        }
        // Maintain DSP bank byte 0 register to keep brightness, contrast, saturation and special effect settings
        s->set_reg(s, 0xFF, 0x01, 0);             // Select DSP bank
        s->set_reg(s, 0x7C, 0xFF, 0x00);          // Select byte 0 on DSP bank (IRA_BPADDR)
        s->set_reg(s, 0x7D, 0x5E, registerValue); // Write value (IRA_BPDATA) (bitmask 0101 1110)


        // Sharpness manipulation (not implemented for this model, use customized function instead)
        // *********************************************************************
        ov2640_set_sharpness(s, std::min(3, std::max(-3, std::min(paramCameraInternal.sharpness, 3)))); // [-3 .. 3]
    }
    else if (paramCameraInternal.cameraModel == CAMERA_OV5640) {
        // Sharpness manipulation
        // *********************************************************************
        s->set_sharpness(s, std::min(3, std::max(-3, paramCameraInternal.sharpness))); // [-3 .. 3]
    }
    else {
        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                            "setImageManipulation: Camera model not fully supported. "
                            "Sharpness, brightness, contrast, saturation and special effects not properly set");
    }

    // Exposure control
    // *********************************************************************
    s->set_exposure_ctrl(s, paramCameraInternal.exposureControlMode > 0 ? 1 : 0); // Set exposure control

    if (s->status.aec) { // Auto exposure control --> Use exposure level correction
        s->set_ae_level(s, std::min(5, std::max(-5, paramCameraInternal.autoExposureLevel))); // Adjust auto exposure level [-5 .. 5]
        s->set_aec2(s, paramCameraInternal.exposureControlMode == 2 ? 1 : 0); // Switch to alternative alogrithm (aka night mode)
    }
    else { // Manual exposure control -> Use exposure value [0 .. sensorFrameHeight]
        s->set_aec_value(s, std::min((int)sensorFrameSizeHeight, std::max(0, paramCameraInternal.manualExposureValue)));
        paramCameraInternal.manualExposureValue = s->status.aec_value;
    }

    // Gain control
    // *********************************************************************
    // Auto: Auto control gain up to gainceiling parameter.
    //   Limit to max 2X to also limit brightness fluctuations. If higher gain is required, switch to manual control.
    // Manual: Manual gain control between 0 .. 30
    //   Try to keep the gain as low as possible to keep noise at a minimum. Increase manual exposure value instead.
    s->set_gain_ctrl(s, paramCameraInternal.gainControlMode == 1 ? 1 : 0); // Set gain control

    if (s->status.agc) { // Auto gain control
        s->set_gainceiling(s, GAINCEILING_2X);
    }
    else { // Manual gain control
        s->set_agc_gain(s, std::min(30, std::max(0, paramCameraInternal.manualGainValue)));
    }

    // White balance control
    // *********************************************************************
    s->set_whitebal(s, 1); // Enable auto white balance control
    s->set_awb_gain(s, 1); // Enable auto white balance gain control
    s->set_wb_mode(s, 0);  // Set white balance mode to Auto

    // Image orientation
    // *********************************************************************
    s->set_hmirror(s, paramCameraInternal.mirrorImage ? 1 : 0);
    s->set_vflip(s, paramCameraInternal.flipImage ? 1 : 0);

#ifdef DEBUG_DETAIL_ON
    printCamConfig();
#endif // DEBUG_DETAIL_ON

    return true;
}


bool ClassControlCamera::setMirrorFlip(bool _mirror, bool _flip)
{
    if (!cameraInitSuccessful) {
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setMirrorFlip: Failed to get control structure");
        return false;
    }

    paramCameraInternal.mirrorImage = _mirror;
    paramCameraInternal.flipImage = _flip;

    s->set_hmirror(s, paramCameraInternal.mirrorImage ? 1 : 0);
    s->set_vflip(s, paramCameraInternal.flipImage ? 1 : 0);

    return true;
}


bool ClassControlCamera::getCameraInitSuccessful()
{
    return cameraInitSuccessful;
}


camera_model_t ClassControlCamera::getCamModel(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCamType: Failed to get control structure");
        return CAMERA_NONE;
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);
    return info->model;
}


std::string ClassControlCamera::getCamType(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCamType: Failed to get control structure");
        return "Unknown";
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);
    return std::string(info->name);
}


std::string ClassControlCamera::getCamPID(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCamPID: Failed to get control structure");
        return "Unknown";
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);
    return intToHexString(s->id.PID);
}


std::string ClassControlCamera::getCamVersion(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCamVersion: Failed to get control structure");
        return "Unknown";
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);
    return intToHexString(s->id.VER);
}


int ClassControlCamera::getCamFrequencyMhz(void)
{
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "getCamFrequencyMhz: Failed to get control structure");
        return -1;
    }
    camera_sensor_info_t *info = esp_camera_sensor_get_info(&s->id);
    return s->xclk_freq_hz / 1000000;
}


void ClassControlCamera::getOutputFrameSize(int &width, int &height)
{
    width = outputFrameSizeWidth;
    height = outputFrameSizeHeight;
}


esp_err_t ClassControlCamera::captureToBasisImage(CImageBasis *_Image)
{
    if (!cameraInitSuccessful) {
        return ESP_FAIL;
    }

    if (paramFlashlightInternal.flashTime > 0) { // Switch on for defined time if a flashTime is set
        setStatusLed(true);
        setFlashlight(true);
        vTaskDelay(paramFlashlightInternal.flashTime / portTICK_PERIOD_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (paramFlashlightInternal.flashTime > 0) { // Switch off if flashlight was on
        setStatusLed(false);
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToBasisImage: Failed to get camera framebuffer");
        return ESP_FAIL;
    }

    if (demoMode) { // Use images stored on SD-Card instead of camera image
        /* Replace Framebuffer with image from SD-Card */
        loadNextDemoImage(fb);
    }

    if (_Image != NULL) {
        STBIObjectPSRAM.name = "rawImage";
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.PreallocatedMemory = _Image->getRgbImage();
        STBIObjectPSRAM.PreallocatedMemorySize = _Image->getMemsize();

        if (!_Image->loadFromMemoryPreallocated(fb->buf, fb->len)) {
            return ESP_FAIL;
        }

        // Special effect: grayscale + negative in combination
        // Workaround: Do grayscale on camera + negative on MCU
        // Disadvantage: Effect in combination not visible in other camera consumers like live stream / REST API
        if (paramCameraInternal.specialEffect == 7) {
            _Image->createNegativeImage();
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToBasisImage: rawImage not allocated");
    }
    esp_camera_fb_return(fb);

    return ESP_OK;
}


esp_err_t ClassControlCamera::captureToFile(std::string _nm)
{
    if (!cameraInitSuccessful) {
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;
    std::string ftype;

    if (paramFlashlightInternal.flashTime > 0) { // Switch on for defined time if a flashTime is set
        setStatusLed(true);
        setFlashlight(true);
        vTaskDelay(paramFlashlightInternal.flashTime / portTICK_PERIOD_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (paramFlashlightInternal.flashTime > 0) { // Switch off if flashlight was on
        setStatusLed(false);
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to get camera framebuffer");
        return ESP_FAIL;
    }

#ifdef DEBUG_DETAIL_ON
    ESP_LOGD(TAG, "w %d, h %d, size %d", fb->width, fb->height, fb->len);
#endif // DEBUG_DETAIL_ON

    _nm = formatFileName(_nm);

#ifdef DEBUG_DETAIL_ON
    ESP_LOGD(TAG, "Save Camera to: %s", _nm.c_str());
#endif // DEBUG_DETAIL_ON

    ftype = toUpper(getFileType(_nm));

#ifdef DEBUG_DETAIL_ON
    ESP_LOGD(TAG, "Filetype: %s", ftype.c_str());
#endif // DEBUG_DETAIL_ON

    uint8_t *buf = NULL;
    size_t buf_len = 0;
    bool converted = false;

    if (ftype.compare("BMP") == 0) {
        frame2bmp(fb, &buf, &buf_len);
        converted = true;
    }
    else if (ftype.compare("JPG") == 0) {
        if (fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, paramCameraInternal.imageQuality, &buf, &buf_len);
            converted = true;
            if (!jpeg_converted) {
                ESP_LOGE(TAG, "JPEG compression failed");
            }
        }
        else {
            buf_len = fb->len;
            buf = fb->buf;
        }
    }

    esp_camera_fb_return(fb);

    FILE *fp = fopen(_nm.c_str(), "wb");
    if (fp == NULL) { // If an error occurs during the file creation
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to open file " + _nm);
        retVal = ESP_FAIL;
    }
    else {
        /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
        // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
        setvbuf(fp, NULL, _IOFBF, 512);

        fwrite(buf, sizeof(uint8_t), buf_len, fp);
        fclose(fp);
    }

    if (converted) {
        free(buf);
    }

    return retVal;
}


static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;

    if (!index) {
        j->len = 0;
    }

    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
        return 0;
    }

    j->len += len;

    return len;
}


esp_err_t ClassControlCamera::captureToHTTP(httpd_req_t *_req)
{
    if (!cameraInitSuccessful) {
        return ESP_FAIL;
    }

    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();

    if (paramFlashlightInternal.flashTime > 0) {
        setStatusLed(true);
        setFlashlight(true);
        vTaskDelay(paramFlashlightInternal.flashTime / portTICK_PERIOD_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (paramFlashlightInternal.flashTime > 0) { // Switch off if flashlight was on
        setStatusLed(false);
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to get camera framebuffer");
        httpd_resp_send_500(_req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(_req, "image/jpeg");
    if (res == ESP_OK) {
        res = httpd_resp_set_hdr(_req, "Content-Disposition", "inline; filename=raw.jpg");
    }

    if (res == ESP_OK) {
        if (demoMode) { // Use images stored on SD-Card instead of camera image
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Demo mode active");
            /* Replace Framebuffer with image from SD-Card */
            loadNextDemoImage(fb);

            res = httpd_resp_send(_req, (const char *)fb->buf, fb->len);
        }
        else {
            if (fb->format == PIXFORMAT_JPEG) {
                fb_len = fb->len;
                res = httpd_resp_send(_req, (const char *)fb->buf, fb->len);
            }
            else {
                jpg_chunking_t jchunk = {_req, 0};
                res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
                httpd_resp_send_chunk(_req, NULL, 0);
                fb_len = jchunk.len;
            }
        }
    }
    esp_camera_fb_return(fb);

    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %dKB %dms", (int)(fb_len / 1024), (int)((fr_end - fr_start) / 1000));

    return res;
}


esp_err_t ClassControlCamera::captureToStream(httpd_req_t *_req, bool _flashlightOn)
{
    if (!cameraInitSuccessful) {
        return ESP_FAIL;
    }

    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start;
    char *part_buf[64];

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Live stream started");

    if (_flashlightOn) {
        setStatusLed(true);
        setFlashlight(true);
    }

    // httpd_resp_set_hdr(_req, "Access-Control-Allow-Origin", "*");  //stream is blocking web interface, only serving to local

    httpd_resp_set_type(_req, _STREAM_CONTENT_TYPE);
    httpd_resp_send_chunk(_req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    while (1) {
        fr_start = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        fb = esp_camera_fb_get();
        if (fb == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "captureToStream: Failed to get camera framebuffer");
            break;
        }
        fb_len = fb->len;

        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, sizeof(part_buf), _STREAM_PART, fb_len);
            res = httpd_resp_send_chunk(_req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(_req, (const char *)fb->buf, fb_len);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(_req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        esp_camera_fb_return(fb);

        int64_t fr_end = esp_timer_get_time();
        ESP_LOGD(TAG, "JPG: %dKB %dms", (int)(fb_len / 1024), (int)((fr_end - fr_start) / 1000));

        if (res != ESP_OK) { // Exit loop, e.g. also when closing the webpage
            break;
        }

        int64_t fr_delta_ms = (fr_end - fr_start) / 1000;
        if (CAM_LIVESTREAM_REFRESHRATE > fr_delta_ms) {
            const TickType_t xDelay = (CAM_LIVESTREAM_REFRESHRATE - fr_delta_ms) / portTICK_PERIOD_MS;
            ESP_LOGD(TAG, "Stream: sleep for: %ldms", (long)xDelay * 10);
            vTaskDelay(xDelay);
        }
    }

    setStatusLed(false);
    setFlashlight(false);

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Live stream stopped");

    return res;
}


#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
void ClassControlCamera::ledcInitFlashlightDefault(void)
{
    // Prepare GPIO for flashlight default
    gpio_config_t conf = {};
    conf.pin_bit_mask = 1LL << GPIO_FLASHLIGHT_DEFAULT;
    conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&conf);

    // Prepare LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {};

    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num = FLASHLIGHT_DEFAULT_LEDC_TIMER;            // Use TIMER 1 (TIMER0: camera)
    ledc_timer.duty_resolution = FLASHLIGHT_DEFAULT_DUTY_RESOLUTION; // 13 bit
    ledc_timer.freq_hz = FLASHLIGHT_DEFAULT_FREQUENCY;               // Use output frequency at 5 kHz
    ledc_timer.clk_cfg = LEDC_USE_APB_CLK;

    esp_err_t retVal = ledc_timer_config(&ledc_timer);

    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "Failed to init LEDC timer " + std::to_string((int)FLASHLIGHT_DEFAULT_LEDC_TIMER) +
                                ", Error: " + intToHexString(retVal));
    }

    // Prepare LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {};

    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = FLASHLIGHT_DEFAULT_LEDC_CHANNEL; // CH0: Camera, CH2 - CH7: GPIO
    ledc_channel.timer_sel = FLASHLIGHT_DEFAULT_LEDC_TIMER; // Use TIMER 1 (TIMER0: camera)
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = GPIO_FLASHLIGHT_DEFAULT; // Use default flashlight GPIO pin
    ledc_channel.duty = 0;                           // Set duty to 0%
    ledc_channel.hpoint = 0;

    retVal = ledc_channel_config(&ledc_channel);

    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "Failed to init LEDC channel " + std::to_string((int)FLASHLIGHT_DEFAULT_LEDC_CHANNEL) +
                                ", Error: " + intToHexString(retVal));
    }
}
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_PWM


esp_err_t ClassControlCamera::setFlashlightParameter(const CfgData::SectionTakeImage::Flashlight *paramFlashlight)
{
    paramFlashlightInternal = *(CfgData::SectionTakeImage::Flashlight *)paramFlashlight;

    setFlashIntensity(paramFlashlightInternal.flashIntensity);
    setFlashTime(paramFlashlightInternal.flashTime);

    return ESP_OK;
}


void ClassControlCamera::setFlashIntensity(int _flashIntensity)
{
    paramFlashlightInternal.flashIntensity = std::min(std::max(0, _flashIntensity), 100);
}


/* Set flash time in milliseconds */
void ClassControlCamera::setFlashTime(int _flashTime)
{
    paramFlashlightInternal.flashTime = std::max(0, _flashTime);
}


/* Get flash time in milliseconds */
int ClassControlCamera::getFlashTime()
{
    return paramFlashlightInternal.flashTime;
}


void ClassControlCamera::setFlashlight(bool _status)
{
    GpioHandler *gpioHandler = gpio_handler_get();
#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED
    if (gpioHandler != NULL) {
        gpioHandler->gpioFlashlightControl(_status, paramFlashlightInternal.flashIntensity);
    }
#else
    if (gpioHandler != NULL && gpioHandler->gpioHandlerIsEnabled()) {
        gpioHandler->gpioFlashlightControl(_status, paramFlashlightInternal.flashIntensity);
    }
    else {
#ifdef GPIO_FLASHLIGHT_DEFAULT_USE_PWM
        if (_status) {
            int intensityValue = (paramFlashlightInternal.flashIntensity * FLASHLIGHT_DEFAULT_RESOLUTION_RANGE) / 100;
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "Default flashlight PWM: GPIO" + std::to_string((int)GPIO_FLASHLIGHT_DEFAULT) + ", State: 1, Intensity: " +
                                    std::to_string(intensityValue) + "/" + std::to_string(FLASHLIGHT_DEFAULT_RESOLUTION_RANGE));

            ledc_set_duty(LEDC_LOW_SPEED_MODE, FLASHLIGHT_DEFAULT_LEDC_CHANNEL, intensityValue);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, FLASHLIGHT_DEFAULT_LEDC_CHANNEL); // Update duty to apply the new value
        }
        else {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "Default flashlight PWM: GPIO" + std::to_string((int)GPIO_FLASHLIGHT_DEFAULT) + ", State: 0");

            ledc_set_duty(LEDC_LOW_SPEED_MODE, FLASHLIGHT_DEFAULT_LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, FLASHLIGHT_DEFAULT_LEDC_CHANNEL);
        }
#else
        esp_rom_gpio_pad_select_gpio(GPIO_FLASHLIGHT_DEFAULT);         // Init the GPIO
        gpio_set_direction(GPIO_FLASHLIGHT_DEFAULT, GPIO_MODE_OUTPUT); // Set the GPIO as a push/pull output

        if (_status) {
            gpio_set_level(GPIO_FLASHLIGHT_DEFAULT, 1);
        }
        else {
            gpio_set_level(GPIO_FLASHLIGHT_DEFAULT, 0);
        }
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_PWM
    }
#endif // GPIO_FLASHLIGHT_DEFAULT_USE_SMARTLED
}


void ClassControlCamera::setStatusLed(bool _status)
{
    if (xHandle_task_StatusLED == NULL) { // Only if status LED is not used by higher prior status
        // Init the GPIO
        esp_rom_gpio_pad_select_gpio(GPIO_STATUS_LED_ONBOARD);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(GPIO_STATUS_LED_ONBOARD, GPIO_MODE_OUTPUT);

#ifdef GPIO_STATUS_LED_ONBOARD_LOWACTIVE
        if (!_status) {
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 1);
        }
        else {
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 0);
        }
#else
        if (_status) {
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 1);
        }
        else {
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 0);
        }
#endif // GPIO_STATUS_LED_ONBOARD_LOWACTIVE
    }
}


void ClassControlCamera::enableDemoMode()
{
    FILE *fd = fopen("/sdcard/demo/files.txt", "r");
    if (fd == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Can not start Demo mode, the folder '/sdcard/demo/' does not contain the needed files");
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "See details on https://jomjol.github.io/AI-on-the-edge-device-docs/Demo-Mode");
        return;
    }

    demoFiles.clear();
    demoFiles.reserve(1500); // Preallocate memory to ensure using a SPIRAM chunk (36kB)

    char line[50];
    while (fgets(line, sizeof(line), fd) != NULL) {
        line[strlen(line) - 1] = '\0';
        demoFiles.push_back(line);
    }
    fclose(fd);

    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                        "Using demo images (" + std::to_string(demoFiles.size()) + " files) instead of real camera image");

    /*// Print all file to log
    for (auto &file : demoFiles) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, file);
    }*/

    demoMode = true;
}


void ClassControlCamera::disableDemoMode()
{
    demoMode = false;
    demoFiles.clear();
    std::vector<std::string>().swap(demoFiles); // Ensure that memory allocated by vector gets freed
}


bool ClassControlCamera::loadNextDemoImage(camera_fb_t *_fb)
{
    char filename[50];
    snprintf(filename, sizeof(filename), "/sdcard/demo/%s", demoFiles[getFlowCycleCounter() % demoFiles.size()].c_str());

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Using " + std::string(filename) + " as demo image");

    /* Inject saved image */

    size_t fileSize = getFileSize(filename);
    if (fileSize > DEMO_IMAGE_SIZE) {
        char buf[100];
        snprintf(buf, sizeof(buf), "Demo image (%d bytes) is larger than provided buffer (%d bytes)", (int)fileSize, DEMO_IMAGE_SIZE);
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, std::string(buf));
        return false;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "DemoImage: Failed to read file: " + std::string(filename));
        return false;
    }

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(fp, NULL, _IOFBF, 512);

    _fb->len = fread(_fb->buf, 1, fileSize, fp);
    fclose(fp);

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "DemoImage: Read " + std::to_string(_fb->len) + " bytes");

    return true;
}


/* Free only user allocated memory without deinit of cam driver */
void ClassControlCamera::freeDemoMemoryOnly()
{
    demoFiles.clear();
    std::vector<std::string>().swap(demoFiles); // Ensure that memory allocated by vector gets freed
}


ClassControlCamera::~ClassControlCamera()
{
    deinitCam();
    demoFiles.clear();
    std::vector<std::string>().swap(demoFiles); // Ensure that memory allocated by vector gets freed
}
