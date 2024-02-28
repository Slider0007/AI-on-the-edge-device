#include "ClassControllCamera.h"
#include "../../include/defines.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <nvs_flash.h>
#include <sys/param.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_camera.h"

#include "ov2640_sharpness.h"
#include "psram.h"
#include "Helper.h"
#include "statusled.h"
#include "CImageBasis.h"
#include "ClassLogFile.h"
#include "server_ota.h"
#include "server_GPIO.h"
#include "MainFlowControl.h"


static const char *TAG = "CAM_CTRL"; 

CCamera Camera;

/* Camera live stream */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// OV2640 Camera SDE Indirect Register Access
#define OV2640_IRA_BPADDR   0x7C
#define OV2640_IRA_BPDATA   0x7D

static camera_config_t camera_config = {
    .pin_pwdn       = PWDN_GPIO_NUM,
    .pin_reset      = RESET_GPIO_NUM,
    .pin_xclk       = XCLK_GPIO_NUM,
    .pin_sccb_sda   = SIOD_GPIO_NUM,
    .pin_sccb_scl   = SIOC_GPIO_NUM,
    .pin_d7         = Y9_GPIO_NUM,
    .pin_d6         = Y8_GPIO_NUM,
    .pin_d5         = Y7_GPIO_NUM,
    .pin_d4         = Y6_GPIO_NUM,
    .pin_d3         = Y5_GPIO_NUM,
    .pin_d2         = Y4_GPIO_NUM,
    .pin_d1         = Y3_GPIO_NUM,
    .pin_d0         = Y2_GPIO_NUM,
    .pin_vsync      = VSYNC_GPIO_NUM,
    .pin_href       = HREF_GPIO_NUM,
    .pin_pclk       = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,           // Frequency (20Mhz)
    
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,     // YUV422, GRAYSCALE, RGB565, JPEG
    .frame_size = FRAMESIZE_VGA,        // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 12,                 // 0-63 lower number means higher quality
    .fb_count = 1,                      // if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,  // The location where the frame buffer will be allocated */
    .grab_mode = CAMERA_GRAB_LATEST     // only from new esp32cam version
};


void CCamera::ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = { };

    ledc_timer.speed_mode       = LEDC_MODE;
    ledc_timer.timer_num        = LEDC_TIMER;
    ledc_timer.duty_resolution  = LEDC_DUTY_RES;
    ledc_timer.freq_hz          = LEDC_FREQUENCY;   // Set output frequency at 5 kHz
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = { };

    ledc_channel.speed_mode     = LEDC_MODE;
    ledc_channel.channel        = LEDC_CHANNEL;
    ledc_channel.timer_sel      = LEDC_TIMER;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num       = LEDC_OUTPUT_IO;
    ledc_channel.duty           = 0; // Set duty to 0%
    ledc_channel.hpoint         = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


CCamera::CCamera()
{
    cameraInitSuccessful = false;

    camParameter.flashTime = 2000; // flashTime in ms
    camParameter.flashIntensity = 4095;

    camParameter.actualResolution = FRAMESIZE_VGA;
    camParameter.actualQuality = 12;

    camParameter.brightness = 0;
    camParameter.contrast = 0;
    camParameter.saturation = 0;
    camParameter.sharpness = 0;
    camParameter.exposureControlMode = 1;
    camParameter.autoExposureLevel = 0;
    camParameter.manualExposureValue = 300;
    camParameter.gainControlMode = 1;
    camParameter.manualGainValue = 0;
    camParameter.specialEffect = 0;
    camParameter.zoomMode = 0;
    camParameter.zoomOffsetX = 0;
    camParameter.zoomOffsetY = 0;

    demoMode = false;

    #ifdef GPIO_FLASHLIGHT_DEFAULT_USE_LEDC
        ledc_init();   
    #endif
}


void CCamera::powerResetCamera()
{
    #if PWDN_GPIO_NUM > -1 // Use reset only if pin is available (PWDN_GPIO_NC == -1)
        ESP_LOGD(TAG, "Resetting camera by power cycling");
        gpio_config_t conf;
        conf.intr_type = GPIO_INTR_DISABLE;
        conf.pin_bit_mask = 1LL << PWDN_GPIO_NUM;
        conf.mode = GPIO_MODE_OUTPUT;
        conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&conf);

        // carefull, logic is inverted compared to reset pin
        gpio_set_level(PWDN_GPIO_NUM, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(PWDN_GPIO_NUM, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return;
    #else
        ESP_LOGD(TAG, "Power pin not defined. Software power reset not available"); 
        return;
    #endif
}


esp_err_t CCamera::initCam()
{
    if (cameraInitSuccessful)
        deinitCam(); // De-init in case it was already initialized
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Camera init failed: " + intToHexString(err));

            if (err == ESP_ERR_NOT_FOUND)
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Camera module not found, check camera module and electrical connection");
            else if (err == ESP_ERR_NOT_SUPPORTED)
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Camera module or feature not supported");
        
        return err;
    }

    camParameter.actualResolution = camera_config.frame_size;
    camParameter.actualQuality = camera_config.jpeg_quality;

    cameraInitSuccessful = true;
    return ESP_OK;
}


esp_err_t CCamera::deinitCam()
{
    cameraInitSuccessful = false;
    esp_camera_deinit(); // De-init in case it was already initialized (returns ESP_FAIL if deinit is already done)
    powerResetCamera();

    return ESP_OK;
}


bool CCamera::testCamera(void) 
{
    bool retval;
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Camera framebuffer check failed");
        return false;
    }

    esp_camera_fb_return(fb);
    return true;
}


void CCamera::printCamInfo(void)
{
    // Print camera infos
    // ********************************************
    char caminfo[64];

    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "printCamInfo: Failed to get control structure");
        return;
    }

    sprintf(caminfo, "PID: 0x%02x, VER: 0x%02x, MIDL: 0x%02x, MIDH: 0x%02x, FREQ: %dMhz", s->id.PID, 
                s->id.VER, s->id.MIDH, s->id.MIDL, s->xclk_freq_hz/1000000);
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Camera info: " + std::string(caminfo));
}


void CCamera::printCamConfig(void)
{
    // Print camera config
    // ********************************************
    char camconfig[512];

    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "printCamConfig: Failed to get control structure");
        return;
    }        

    sprintf(camconfig, "ae_level:%d, aec2:%d, aec:%d, aec_value:%d, agc:%d, agc_gain:%d, awb:%d, awb_gain:%d, "
                "binning:%d, bpc:%d, brightness:%d, colorbar:%d, contrast:%d, dcw:%d, deonoise:%d, framesize:%d, "
                "gainceiling:%d, hmirror:%d, lenc:%d, quality:%d, raw_gma:%d, saturation:%d, scale:%d, sharpness:%d, "
                "special_effect:%d, vflip:%d, wb_mode:%d, wpc:%d", 
                s->status.ae_level, s->status.aec2, s->status.aec, s->status.aec_value, 
                s->status.agc, s->status.agc_gain, s->status.awb, s->status.awb_gain, s->status.binning,
                s->status.bpc, s->status.brightness, s->status.colorbar, s->status.contrast, s->status.dcw,
                s->status.denoise, s->status.framesize, s->status.gainceiling, s->status.hmirror, s->status.lenc,
                s->status.quality, s->status.raw_gma, s->status.saturation, s->status.scale, s->status.sharpness,
                s->status.special_effect, s->status.vflip, s->status.wb_mode, s->status.wpc);
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Camera config: " + std::string(camconfig));
}


void CCamera::setCameraFrequency(int _frequency)
{
    if (camera_config.xclk_freq_hz == (_frequency * 1000000)) // If frequency is matching, return without any action
        return;
    
    if (_frequency >= 5 && _frequency <= 20)
        camera_config.xclk_freq_hz = _frequency * 1000000;
    else
        camera_config.xclk_freq_hz = 2000000;

    initCam();
    printCamInfo();
}


void CCamera::setImageWidthHeightFromResolution(framesize_t _resol)
{
    if (_resol == FRAMESIZE_QVGA)
    {
        image_height = 240;
        image_width = 320;
    }
    else if (_resol == FRAMESIZE_VGA)
    {
        image_height = 480;
        image_width = 640;
    }
    else if (_resol == FRAMESIZE_SVGA)
    {
        image_height = 600;
        image_width = 800;
    }
    else if (_resol == FRAMESIZE_XGA)
    {
        image_height = 768;
        image_width = 1024;
    }
    else if (_resol == FRAMESIZE_HD)
    {
        image_height = 720;
        image_width = 1280;
    }
    else if (_resol == FRAMESIZE_SXGA)
    {
        image_height = 1024;
        image_width = 1280;
    }
    else if (_resol == FRAMESIZE_UXGA)
    {
        image_height = 1200;
        image_width = 1600;
    }
}


void CCamera::setSizeQuality(int _qual, framesize_t _resol, int _zoomMode, 
                                int _zoomOffsetX, int _zoomOffsetY)
{
    if (!getcameraInitSuccessful())
        return;

    // Set resolution
    camParameter.actualResolution = _resol;
    setImageWidthHeightFromResolution(camParameter.actualResolution);

    // Set zoom / framesize
    setZoom(_zoomMode, _zoomOffsetX, _zoomOffsetY);

    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setSizeQuality: Failed to get control structure");
        return;
    }

    // Set quality
    camParameter.actualQuality = std::min(63, std::max(8, _qual)); // Limit quality from 8..63 (values lower than 8 tent to be unstable)
    s->set_quality(s, camParameter.actualQuality);
}


/*
* resolution = 0 \\ 1600 x 1200
* resolution = 1 \\  800 x  600
*/
void CCamera::setCamWindow(sensor_t *_s, int _resolution, int _xOffset, int _yOffset, int _xLength, int _yLength)
{
    if (_s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setCamWindow: No valid control structure");
        return;
    }
    _s->set_res_raw(_s, _resolution, 0, 0, 0, _xOffset, _yOffset, _xLength, _yLength, _xLength, _yLength, false, false);
}


void CCamera::setZoom(int _zoomMode, int _zoomOffsetX, int _zoomOffsetY)
{
    camParameter.zoomMode = _zoomMode;
    camParameter.zoomOffsetX = _zoomOffsetX;
    camParameter.zoomOffsetY = _zoomOffsetY;

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setZoom: Failed to get control structure");
        return;
    }

    if (camParameter.zoomMode > 0) { // zoomMode = 0 -> zoom off
        int resolMode = camParameter.zoomMode - 1;
        int x = camParameter.zoomOffsetX;
        int y = camParameter.zoomOffsetY;

        // Maintain correct mode, only mode 0 and 1 available
        resolMode = std::min(resolMode, 1);

        // Force mode 0 if image size is larger than 800 x 600
        if (image_width >= 800 || image_height >= 600)
            resolMode = 0;

        // Max values depending on mode
        int maxX = 1600 - image_width;
        int maxY = 1200 - image_height;

        if (resolMode == 1) {
            maxX = 800 - image_width;
            maxY = 600 - image_height;
        }

        // Maintain max x,y values
        x = std::min(x, maxX);
        y = std::min(y, maxY);
        
        setCamWindow(s, resolMode, x, y, image_width, image_height);
    }
    else {
        s->set_framesize(s, camParameter.actualResolution);
    }
}


bool CCamera::setImageManipulation(int _brightness, int _contrast, int _saturation, int _sharpness, int _exposureControlMode, 
                                   int _autoExposureLevel, int _manualExposureValue, int _gainControlMode, 
                                   int _manualGainValue, int _specialEffect, bool _mirror, bool _flip)
{
    if (!getcameraInitSuccessful())
        return false;

    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setImageManipulation: Failed to get control structure");
        return false;
    }

    camParameter.brightness = _brightness;
    camParameter.contrast = _contrast;
    camParameter.saturation = _saturation;
    camParameter.sharpness = _sharpness;
    camParameter.exposureControlMode = _exposureControlMode;
    camParameter.autoExposureLevel = _autoExposureLevel;
    camParameter.manualExposureValue = _manualExposureValue;
    camParameter.gainControlMode = _gainControlMode;
    camParameter.manualGainValue = _manualGainValue;
    camParameter.specialEffect = _specialEffect;
    camParameter.mirrorImage = _mirror;
    camParameter.flipImage = _flip;

    // Basic image manipulation
    s->set_saturation(s, std::min(2, std::max(-2, camParameter.saturation)));   // [-2 .. 2]
    s->set_contrast(s, std::min(2, std::max(-2, camParameter.contrast )));       // [-2 .. 2]
    s->set_brightness(s, std::min(2, std::max(-2, camParameter.brightness)));   // [-2 .. 2] (IMPORTANT: Apply brightness after saturation and conrast)
    s->set_sharpness(s, 0); // Default: Sharpness not supported, use owm implementation
    
    // Set special effect (0: None, 1: Negative, 2: Grayscale, 3: Reddish, 4: Greenish, 5: Blueish, 6: Sepia)
    if (camParameter.specialEffect >= 0 && camParameter.specialEffect <= 6)
        s->set_special_effect(s, camParameter.specialEffect); // [0 .. 6]
    // Set sepcial effect: 7: Grayscale + Negative in combination
    // Potential bug in camera firmware -> Workaround: Do grayscale on camera + negative on MCU
    else if (camParameter.specialEffect == 7)
        s->set_special_effect(s, 2); // 2: Grayscale
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setImageManipulation: Selected special effect unknown");
        return false;
    }

    // Auto exposure control
    s->set_exposure_ctrl(s, camParameter.exposureControlMode > 0 ? 1 : 0); // Enable auto exposure control
    
    if (s->status.aec) { // Auto exposure control --> Use exposure level correction
        s->set_ae_level(s, std:: min(2, std::max(-2, camParameter.autoExposureLevel))); // Adjust auto exposure level [-2 .. 2]
        s->set_aec2(s, camParameter.exposureControlMode == 2 ? 1 : 0); // Switch to alternative auto exposure control alogrithm
    }
    else { // Manual exposure control -> Use exposure value
        s->set_aec_value(s, std::min(1200, std::max(0, camParameter.manualExposureValue))); // Set manual exposure value [0 .. 1200]
    }

    // Gain control
    s->set_gain_ctrl(s, camParameter.gainControlMode == 1 ? 1 : 0); // Enable auto gain control
    if (s->status.agc) // Auto gain control --> Use GAINCEILING parameter (max 2X)
        s->set_gainceiling(s, GAINCEILING_2X); // GAINCEILING_2X 4X 8X 16X 32X 64X 128X -> Limit gain
    else // Manual gain control --> Use manual gain value [0 .. 30]
        s->set_agc_gain(s, std::min(30, std::max(0, camParameter.manualGainValue)));

    // White balance control
    s->set_whitebal(s, 1); // Enable auto white balance control
    s->set_awb_gain(s, 1); // Enable auto white balance gain control
    s->set_wb_mode(s, 0); // Set white balance mode to Auto

    // Image orientation
    s->set_hmirror(s, camParameter.mirrorImage ? 1 : 0);
    s->set_vflip(s, camParameter.flipImage ? 1 : 0);

    camera_sensor_info_t *sensor_info = esp_camera_sensor_get_info(&(s->id));
    if (sensor_info == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setImageManipulation: Failed to get info structure");
        return false;
    }

    if (sensor_info->model == CAMERA_OV2640) {
        // Sharpness implementation (not officially supported)
        if (camParameter.sharpness > -4) { // Sharpness == -4 -> Auto sharpness
            ov2640_set_sharpness(s, std::min(3, std::max(-3, std::min(camParameter.sharpness, 3)))); // -3 .. 3
        } 
        else {
            ov2640_enable_auto_sharpness(s);    
        }

        // Enable brightness, contrast, saturation and optional special effects
        /* Workaround - bug in cam library - enable bits are set without using bitwise OR logic -> only latest enable setting is used */
        /* Library version: https://github.com/espressif/esp32-camera/commit/5c8349f4cf169c8a61283e0da9b8cff10994d3f3 */
        /* Reference: https://esp32.com/viewtopic.php?f=19&t=14376#p93178 */
        //s->set_reg(s, OV2640_IRA_BPADDR, 0xFF, 0x02); // Optional feature - hue setting: Select byte 2 in register 0x7C to set hue value
        //s->set_reg(s, OV2640_IRA_BPDATA, 0xFF, 0x80); // Optional feature - hue setting: Hue value 0 - 255

        int registerValue = 0x07; // Set bit 0, 1, 2 to enable saturation, contrast, brightness and hue control
        
        // Bitwise OR of special effect enable bits
        if (camParameter.specialEffect == 1) { // Sepcial effect: 1: negative
            registerValue |= 0x40;
        }
        // Sepcial effect: 2: grayscale, 3: reddish, 4: greenish, 5: blueish, 6: sepia
        else if (camParameter.specialEffect >= 2 && camParameter.specialEffect <= 6) {
            registerValue |= 0x18;
        }
        // Sepcial effect: 7: Grayscale + Negative in combination
        else if (camParameter.specialEffect == 7) {
            //registerValue |= 0x58;    // Flags which should perform both together on camera
            registerValue |= 0x18;      // Potential bug in camera firmware -> Workaround: Do grayscale on camera + negative on MCU
                                        // Disadvantage: effect in combination not visible in other camera consumers like live stream / REST API
        }

        // Maintain DSP bank byte 0 register to keep brightness, contrast, saturation and special effect settings
        s->set_reg(s, 0xFF, 0x01, 0); // Select DSP bank
        s->set_reg(s, OV2640_IRA_BPADDR, 0xFF, 0x00); // Select byte 0 on DSP bank
        s->set_reg(s, OV2640_IRA_BPDATA, 0xFF, registerValue); // Write value
    }
    else {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "setImageManipulation: Camera model not fully supported. "
                            "Sharpness, brightness, contrast, saturation and special effects not properly set"); 
    }

    return true;
}


bool CCamera::setMirrorFlip(bool _mirror, bool _flip)
{
    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setMirrorFlip: Failed to get control structure");
        return false;
    }

    camParameter.mirrorImage = _mirror;
    camParameter.flipImage = _flip;

    s->set_hmirror(s, camParameter.mirrorImage ? 1 : 0);
    s->set_vflip(s, camParameter.flipImage ? 1 : 0);

    return true;
}


void CCamera::setFlashIntensity(int _flashIntensity)
{
    _flashIntensity = std::min(_flashIntensity, 100);
    _flashIntensity = std::max(_flashIntensity, 0);
    camParameter.flashIntensity = ((_flashIntensity * LEDC_RESOLUTION) / 100);
    ESP_LOGD(TAG, "Set flashIntensity to %d of %d", camParameter.flashIntensity, LEDC_RESOLUTION); // @TODO: LOGD
}


/* Set flash time in milliseconds */
void CCamera::setFlashTime(int _flashTime)
{
    camParameter.flashTime = std::max(_flashTime, 0);
    ESP_LOGD(TAG, "Set flashTime to %d", camParameter.flashTime); // @TODO: LOGD
}


/* Get flash time in milliseconds */
int CCamera::getFlashTime()
{
    return camParameter.flashTime;
}


esp_err_t CCamera::captureToBasisImage(CImageBasis *_Image)
{
    if (!getcameraInitSuccessful())
        return ESP_FAIL;

    if (camParameter.flashTime > 0) {    // Switch on for defined time if a flashTime is set
        setStatusLED(true);
        setFlashlight(true);
        vTaskDelay(camParameter.flashTime / portTICK_PERIOD_MS);
    }

	#ifdef DEBUG_DETAIL_ON
	    LogFile.WriteHeapInfo("captureToBasisImage - After LightOn");
	#endif

    camera_fb_t * fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);        
    fb = esp_camera_fb_get();

    if (camParameter.flashTime > 0) {    // Switch off if flashlight was on
        setStatusLED(false);  
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToBasisImage: Failed to get camera framebuffer");
        return ESP_FAIL;
    }

    if (demoMode) { // Use images stored on SD-Card instead of camera image
        /* Replace Framebuffer with image from SD-Card */
        loadNextDemoImage(fb);
    }

    if (_Image != NULL) {
        STBIObjectPSRAM.name="rawImage";
        STBIObjectPSRAM.usePreallocated = true;
        STBIObjectPSRAM.PreallocatedMemory = _Image->RGBImageGet();
        STBIObjectPSRAM.PreallocatedMemorySize = _Image->getMemsize();

        if(!_Image->LoadFromMemoryPreallocated(fb->buf, fb->len))
            return ESP_FAIL;

        // Special effect: grayscale + negative in combination: Not functional due to potential bug in camera firmware
        // Workaround: Do grayscale on camera + negative on MCU
        // Disadvantage: effect in combination not visible in other camera consumers like live stream / REST API
        if (camParameter.specialEffect == 7)
            _Image->Negative();
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToBasisImage: rawImage not allocated");
    }
    esp_camera_fb_return(fb);        

    return ESP_OK;    
}


esp_err_t CCamera::captureToFile(std::string _nm)
{
    if (!getcameraInitSuccessful())
        return ESP_FAIL;
    
    std::string ftype;

    if (camParameter.flashTime > 0) {    // Switch on for defined time if a flashTime is set
        setStatusLED(true);
        setFlashlight(true);
        vTaskDelay(camParameter.flashTime / portTICK_PERIOD_MS);
    }

    camera_fb_t * fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (camParameter.flashTime > 0) {    // Switch off if flashlight was on
        setStatusLED(false);    
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to get camera framebuffer");
        return ESP_FAIL;
    }

    #ifdef DEBUG_DETAIL_ON    
        ESP_LOGD(TAG, "w %d, h %d, size %d", fb->width, fb->height, fb->len);
    #endif

    _nm = FormatFileName(_nm);

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "Save Camera to: %s", _nm.c_str());
    #endif

    ftype = toUpper(getFileType(_nm));

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "Filetype: %s", ftype.c_str());
    #endif

    uint8_t * buf = NULL;
    size_t buf_len = 0;   
    bool converted = false; 

    if (ftype.compare("BMP") == 0) {
        frame2bmp(fb, &buf, &buf_len);
        converted = true;
    }
    else if (ftype.compare("JPG") == 0) {
        if (fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, camParameter.actualQuality, &buf, &buf_len);
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

    FILE * fp = fopen(_nm.c_str(), "wb");
    if (fp == NULL) { // If an error occurs during the file creation
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to open file " + _nm);
    }
    else {
        /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
        // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
        setvbuf(fp, NULL, _IOFBF, 512);

        fwrite(buf, sizeof(uint8_t), buf_len, fp); 
        fclose(fp);
    }   

    if (converted)
        free(buf);

    return ESP_OK;    
}


static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len)
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


esp_err_t CCamera::captureToHTTP(httpd_req_t *_req)
{
    if (!getcameraInitSuccessful())
        return ESP_FAIL;
    
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();

    if (camParameter.flashTime > 0) {
        setStatusLED(true);
        setFlashlight(true);
        vTaskDelay(camParameter.flashTime / portTICK_PERIOD_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (camParameter.flashTime > 0) {    // Switch off if flashlight was on
        setStatusLED(false); 
        setFlashlight(false);
    }

    if (fb == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToFile: Failed to get camera framebuffer");
        httpd_resp_send_500(_req);
        return ESP_FAIL;
    }
  
    res = httpd_resp_set_type(_req, "image/jpeg");
    if (res == ESP_OK) {
        res = httpd_resp_set_hdr(_req, "Content-Disposition", "inline; filename=raw.jpg");
    }

    if (res == ESP_OK) {
        if (demoMode) { // Use images stored on SD-Card instead of camera image
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Demo mode active");
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
                res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
                httpd_resp_send_chunk(_req, NULL, 0);
                fb_len = jchunk.len;
            }
        }
    }
    esp_camera_fb_return(fb);
    
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %dKB %dms", (int)(fb_len/1024), (int)((fr_end - fr_start)/1000));

    return res;
}


esp_err_t CCamera::captureToStream(httpd_req_t *_req, bool _flashlightOn)
{
    if (!getcameraInitSuccessful())
        return ESP_FAIL;
    
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start;
    char * part_buf[64];

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Live stream started");

    if (_flashlightOn) {
        setStatusLED(true);
        setFlashlight(true);
    }

    //httpd_resp_set_hdr(_req, "Access-Control-Allow-Origin", "*");  //stream is blocking web interface, only serving to local

    httpd_resp_set_type(_req, _STREAM_CONTENT_TYPE);
    httpd_resp_send_chunk(_req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    while(1) {
        fr_start = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        fb = esp_camera_fb_get();
        if (fb == NULL) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "captureToStream: Failed to get camera framebuffer");
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
        ESP_LOGD(TAG, "JPG: %dKB %dms", (int)(fb_len/1024), (int)((fr_end - fr_start)/1000));

        if (res != ESP_OK) { // Exit loop, e.g. also when closing the webpage
            break;
        }

        int64_t fr_delta_ms = (fr_end - fr_start) / 1000;
        if (CAM_LIVESTREAM_REFRESHRATE > fr_delta_ms) {
            const TickType_t xDelay = (CAM_LIVESTREAM_REFRESHRATE - fr_delta_ms)  / portTICK_PERIOD_MS;
            ESP_LOGD(TAG, "Stream: sleep for: %ldms", (long) xDelay*10);
            vTaskDelay(xDelay);        
        }
    }

    setStatusLED(false);
    setFlashlight(false);

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Live stream stopped");

    return res;
}


void CCamera::setFlashlight(bool _status)
{
    GpioHandler* gpioHandler = gpio_handler_get();
    if ((gpioHandler != NULL) && (gpioHandler->isEnabled())) {
        ESP_LOGD(TAG, "GPIO handler enabled: Trigger flashlight by GPIO handler");
        gpioHandler->flashLightEnable(_status);
    }
    else {
    #ifdef GPIO_FLASHLIGHT_DEFAULT_USE_LEDC
        if (_status) {
            ESP_LOGD(TAG, "Default flashlight turn on with PWM %d", camParameter.flashIntensity);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, camParameter.flashIntensity));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }
        else {
            ESP_LOGD(TAG, "Default flashlight turn off PWM");
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }
    #else
        // Init the GPIO
        esp_rom_gpio_pad_select_gpio(FLASH_GPIO);
        // Set the GPIO as a push/pull output 
        gpio_set_direction(GPIO_FLASHLIGHT_DEFAULT, GPIO_MODE_OUTPUT);  

        if (_status)  
            gpio_set_level(GPIO_FLASHLIGHT_DEFAULT, 1);
        else
            gpio_set_level(GPIO_FLASHLIGHT_DEFAULT, 0);
    #endif
    }
}


void CCamera::setStatusLED(bool _status)
{
	if (xHandle_task_StatusLED == NULL) { // Only if status LED is not used by higher prior status
        // Init the GPIO
        esp_rom_gpio_pad_select_gpio(GPIO_STATUS_LED_ONBOARD);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(GPIO_STATUS_LED_ONBOARD, GPIO_MODE_OUTPUT);  

        if (!_status)  
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 1);
        else
            gpio_set_level(GPIO_STATUS_LED_ONBOARD, 0);   
    }
}


framesize_t CCamera::textToFramesize(const char * _size)
{
    if (strcmp(_size, "QVGA") == 0)
        return FRAMESIZE_QVGA;     // 320x240
    else if (strcmp(_size, "VGA") == 0)
        return FRAMESIZE_VGA;      // 640x480
    else if (strcmp(_size, "SVGA") == 0)
        return FRAMESIZE_SVGA;     // 800x600
    else if (strcmp(_size, "XGA") == 0)
        return FRAMESIZE_XGA;      // 1024x768
    else if (strcmp(_size, "SXGA") == 0)
        return FRAMESIZE_SXGA;     // 1280x1024
    else if (strcmp(_size, "UXGA") == 0)
        return FRAMESIZE_UXGA;     // 1600x1200  

    return camParameter.actualResolution;
}


bool CCamera::getcameraInitSuccessful() 
{
    return cameraInitSuccessful;
}


CameraParameter CCamera::getCameraParameter()
{
    return camParameter;
}


void CCamera::enableDemoMode()
{
    demoFiles.clear();

    char line[50];

    FILE *fd = fopen("/sdcard/demo/files.txt", "r");
    if (fd == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Can not start Demo mode, the folder '/sdcard/demo/' does not contain the needed files");
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "See details on https://jomjol.github.io/AI-on-the-edge-device-docs/Demo-Mode");
        return;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        line[strlen(line) - 1] = '\0';
        demoFiles.push_back(line);
    }
    
    fclose(fd);

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Using demo images (" + std::to_string(demoFiles.size()) + 
            " files) instead of real camera image");

    /*// Print all file to log
    for (auto file : demoFiles) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, file);
    }*/

    demoMode = true;
}


void CCamera::disableDemoMode()
{
    demoMode = false;
    demoFiles.clear();
}


bool CCamera::loadNextDemoImage(camera_fb_t *_fb) {
    char filename[50];
    long fileSize;

    snprintf(filename, sizeof(filename), "/sdcard/demo/%s", demoFiles[getFlowCycleCounter() % demoFiles.size()].c_str());

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Using " + std::string(filename) + " as demo image");

    /* Inject saved image */

    fileSize = getFileSize(filename);
    if (fileSize > DEMO_IMAGE_SIZE) {
        char buf[100];
        snprintf(buf, sizeof(buf), "Demo image (%d bytes) is larger than provided buffer (%d bytes)",
                (int)fileSize, DEMO_IMAGE_SIZE);
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, std::string(buf));
        return false;
    }

    FILE * fp = fopen(filename, "rb");
    if (fp == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to read file: " + std::string(filename));
        return false;
    }

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(fp, NULL, _IOFBF, 512);

    _fb->len = fread(_fb->buf, 1, fileSize, fp);
    fclose(fp);

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "read " + std::to_string(_fb->len) + " bytes");

    return true;
}


/* Free only user allocated memory without deinit of cam driver */
void CCamera::freeMemoryOnly()
{
    demoFiles.clear();
}


CCamera::~CCamera()
{
    deinitCam();
    demoFiles.clear();
}
