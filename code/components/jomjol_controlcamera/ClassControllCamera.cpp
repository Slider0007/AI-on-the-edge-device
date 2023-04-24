#include "ClassControllCamera.h"
#include "ClassLogFile.h"

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "Helper.h"
#include "statusled.h"
#include "CImageBasis.h"

#include "server_ota.h"
#include "server_GPIO.h"

#include "../../include/defines.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#include "driver/ledc.h"
#include "MainFlowControl.h"

#if (ESP_IDF_VERSION_MAJOR >= 5)
#include "soc/periph_defs.h"
#include "esp_private/periph_ctrl.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_periph.h"
#include "soc/io_mux_reg.h"
#include "esp_rom_gpio.h"
#define gpio_pad_select_gpio esp_rom_gpio_pad_select_gpio
#define gpio_matrix_in(a,b,c) esp_rom_gpio_connect_in_signal(a,b,c)
#define gpio_matrix_out(a,b,c,d) esp_rom_gpio_connect_out_signal(a,b,c,d)
#define ets_delay_us(a) esp_rom_delay_us(a)
#endif


static const char *TAG = "CAM"; 


CCamera Camera;

/* Camera live stream */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,             // Orginal value
//    .xclk_freq_hz =    5000000,         // Test to get rid of the image errors !!!! Hangs in version 9.2 !!!!
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
//    .frame_size = FRAMESIZE_UXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM, /*!< The location where the frame buffer will be allocated */
    .grab_mode = CAMERA_GRAB_LATEST,      // only from new esp32cam version
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
    #ifdef DEBUG_DETAIL_ON    
        ESP_LOGD(TAG, "CreateClassCamera");
    #endif

    brightness = 0;
    contrast = 0;
    saturation = 0;
    isFixedExposure = false;
    flashduration = 5000;
    led_intensity = 4095;
    demoImage = NULL;

    #ifdef USE_PWM_LEDFLASH
        ledc_init();   
    #endif
}


esp_err_t CCamera::InitCam()
{
    ESP_LOGD(TAG, "Init Camera");
    ActualQuality = camera_config.jpeg_quality;
    ActualResolution = camera_config.frame_size;
    //initialize the camera
    esp_camera_deinit(); // De-init in case it was already initialized
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    CameraInitSuccessful = true;
    return ESP_OK;
}


bool CCamera::testCamera(void) 
{
    bool success;
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb)  {
        success = true;
    }
    else {
        success = false;
    }  
    esp_camera_fb_return(fb);

    return success;
}


bool CCamera::SetBrightnessContrastSaturation(int _brightness, int _contrast, int _saturation)
{
    _brightness = min(2, max(-2, _brightness));
    _contrast = min(2, max(-2, _contrast));
    _saturation = min(2, max(-2, _saturation));

    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        s->set_saturation(s, _saturation);
        s->set_contrast(s, _contrast);
        s->set_brightness(s, _brightness);

        /* Workaround - bug in cam library - enable bits are set without using bitwise OR logic -> only latest enable setting is used */
        /* Library version: https://github.com/espressif/esp32-camera/commit/5c8349f4cf169c8a61283e0da9b8cff10994d3f3 */
        /* Reference: https://esp32.com/viewtopic.php?f=19&t=14376#p93178 */
        /* The memory structure is as follows for 
        byte_0 = enable_bits
            byte_0->bit0 = enable saturation and hue --> OK
            byte_0->bit1 = enable saturation --> OK
            byte_0->bit2 = enable brightness and contrast --> OK
            byte_0->bit3 = enable green -> blue spitial effect (Antique and blunish and greenish and readdish and b&w) enable
            byte_0->bit4 = anable gray -> read spitial effect (Antique and blunish and greenish and readdish and b&w) enable
            byte_0->bit5 = remove (UV) in YUV color system
            byte_0->bit6 = enable negative
            byte_0->bit7 = remove (Y) in YUV color system
        byte_1 = saturation1 0-255 --> ?
        byte_2 = hue 0-255 --> OK
        byte_3 = saturation2 0-255 --> OK
        byte_4 = reenter saturation2 in documents --> ?
        byte_5 = spital effect green -> blue 0-255 --> ?
        byte_6 = spital effect gray -> read 0-255 --> ?
        byte_7 = contrast lower byte 0-255 --> OK
        byte_8 = contrast higher byte 0-255 --> OK
        byte_9 = brightness 0-255 --> OK
        byte_10= if byte_10==4 contrast effective --> ?
        */

        //s->set_reg(s, 0x7C, 0xFF, 2); // Optional feature - hue setting: Select byte 2 in register 0x7C to set hue value
        //s->set_reg(s, 0x7D, 0xFF, 0); // Optional feature - hue setting: Hue value 0 - 255
        s->set_reg(s, 0xFF, 0x01, 0); // Select DSP bank
        s->set_reg(s, 0x7C, 0xFF, 0); // Select byte 0 in register 0x7C
        s->set_reg(s, 0x7D, 7, 7); // Set bit 0, 1, 2 in register 0x7D to enable saturation, contrast, brightness and hue control
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SetBrightnessContrastSaturation: Failed to get control structure");
    }

    if (((_brightness != brightness) || (_contrast != contrast) || (_saturation != saturation)) && isFixedExposure)
        EnableAutoExposure(flashduration);

    brightness = _brightness;
    contrast = _contrast;
    saturation = _saturation;

    ESP_LOGD(TAG, "brightness %d, contrast: %d, saturation %d", brightness, contrast, saturation);

    return true;
}


void CCamera::SetQualitySize(int qual, framesize_t resol)
{
    qual = min(63, max(8, qual)); // Limit quality from 8..63 (values lower than 8 tent to be unstable)
    
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, qual);    
        s->set_framesize(s, resol);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SetQualitySize: Failed to get control structure");
    }

    ActualResolution = resol;
    ActualQuality = qual;

    if (resol == FRAMESIZE_QVGA)
    {
        image_height = 240;
        image_width = 320;             
    }
    else if (resol == FRAMESIZE_VGA)
    {
        image_height = 480;
        image_width = 640;             
    }
}


void CCamera::SetLEDIntensity(int _intensity)
{
    _intensity = min(_intensity, 100);
    _intensity = max(_intensity, 0);
    led_intensity = ((_intensity * LEDC_RESOLUTION) / 100);
    ESP_LOGD(TAG, "Set led_intensity to %d of %d", led_intensity, LEDC_RESOLUTION); // TODO: LOGD
}


bool CCamera::EnableAutoExposure(int _flashduration)
{
    flashduration = _flashduration; // save last flashduration internally
    ESP_LOGD(TAG, "EnableAutoExposure");
    
    if (flashduration > 0) {
        LEDOnOff(true);
        LightOnOff(true);
        vTaskDelay(flashduration / portTICK_PERIOD_MS);
    }

    camera_fb_t * fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (flashduration > 0) {
        LEDOnOff(false);  
        LightOnOff(false);
    }    

    if (!fb) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "EnableAutoExposure: Capture Failed. "
                                                "Check camera module and/or proper electrical connection");
        //doReboot();
        return false;
    }
    esp_camera_fb_return(fb);

    sensor_t * s = esp_camera_sensor_get(); 
    if (s) {
        s->set_gain_ctrl(s, 0);
        s->set_exposure_ctrl(s, 0);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "EnableAutoExposure: Failed to get control structure to set gain+exposure");
    }

    isFixedExposure = true;
    return true;
}


esp_err_t CCamera::CaptureToBasisImage(CImageBasis *_Image, int _flashduration)
{
	#ifdef DEBUG_DETAIL_ON
	    LogFile.WriteHeapInfo("CaptureToBasisImage - Start");
	#endif

    flashduration = _flashduration; // save last flashduration internally

    if (flashduration > 0) {    // Switch on for defined time if a flashduration is set
        LEDOnOff(true);
        LightOnOff(true);
        vTaskDelay(flashduration / portTICK_PERIOD_MS);
    }

	#ifdef DEBUG_DETAIL_ON
	    LogFile.WriteHeapInfo("CaptureToBasisImage - After LightOn");
	#endif

    camera_fb_t * fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);        
    fb = esp_camera_fb_get();

    if (flashduration > 0) {    // Switch off if flashlight was on
        LEDOnOff(false);  
        LightOnOff(false);
    }

    if (!fb) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "is not working anymore (CaptureToBasisImage) - most probably caused "
                                                "by a hardware problem (instablility, ...). System will reboot.");
        return ESP_FAIL;
    }

    if (demoMode) { // Use images stored on SD-Card instead of camera image
        /* Replace Framebuffer with image from SD-Card */
        loadNextDemoImage(fb);
    }

    CImageBasis* _zwImage = new CImageBasis("zwImage");
    if (_zwImage) {
        _zwImage->LoadFromMemory(fb->buf, fb->len);
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CaptureToBasisImage: Can't allocate _zwImage");
    }
    esp_camera_fb_return(fb);        
  
    #ifdef DEBUG_DETAIL_ON
        LogFile.WriteHeapInfo("CaptureToBasisImage - After LoadFromMemory");
    #endif

    stbi_uc* p_target;
    stbi_uc* p_source;    
    int channels = 3;
    int width = image_width;
    int height = image_height;

    #ifdef DEBUG_DETAIL_ON
        std::string _zw = "Targetimage: " + std::to_string((int) _Image->rgb_image) + " Size: " + std::to_string(_Image->width) + ", " + std::to_string(_Image->height);
        _zw = _zw + " _zwImage: " + std::to_string((int) _zwImage->rgb_image)  + " Size: " + std::to_string(_zwImage->width) + ", " + std::to_string(_zwImage->height);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, _zw);
    #endif

    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
        {
            p_target = _Image->rgb_image + (channels * (y * width + x));
            p_source = _zwImage->rgb_image + (channels * (y * width + x));
            p_target[0] = p_source[0];
            p_target[1] = p_source[1];
            p_target[2] = p_source[2];
        }

    delete _zwImage;

    #ifdef DEBUG_DETAIL_ON
        LogFile.WriteHeapInfo("CaptureToBasisImage - Done");
    #endif

    return ESP_OK;    
}


esp_err_t CCamera::CaptureToFile(std::string nm, int _flashduration)
{
    string ftype;
    flashduration = _flashduration; // save last flashduration internally

    if (flashduration > 0) {    // Switch on for defined time if a flashduration is set
        LEDOnOff(true);
        LightOnOff(true);
        vTaskDelay(flashduration / portTICK_PERIOD_MS);
    }

    camera_fb_t * fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (flashduration > 0) {    // Switch off if flashlight was on
        LEDOnOff(false);    
        LightOnOff(false);
    }

    if (!fb) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CaptureToFile: Capture Failed. "
                                                "Check camera module and/or proper electrical connection");
        return ESP_FAIL;
    }

    #ifdef DEBUG_DETAIL_ON    
        ESP_LOGD(TAG, "w %d, h %d, size %d", fb->width, fb->height, fb->len);
    #endif

    nm = FormatFileName(nm);

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "Save Camera to: %s", nm.c_str());
    #endif

    ftype = toUpper(getFileType(nm));

    #ifdef DEBUG_DETAIL_ON
        ESP_LOGD(TAG, "Filetype: %s", ftype.c_str());
    #endif

    uint8_t * buf = NULL;
    size_t buf_len = 0;   
    bool converted = false; 

    if (ftype.compare("BMP") == 0)
    {
        frame2bmp(fb, &buf, &buf_len);
        converted = true;
    }
    if (ftype.compare("JPG") == 0)
    {
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, ActualQuality, &buf, &buf_len);
            converted = true;
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
            }
        } else {
            buf_len = fb->len;
            buf = fb->buf;
        }
    }

    esp_camera_fb_return(fb);

    FILE * fp = fopen(nm.c_str(), "wb");
    if (fp == NULL) { // If an error occurs during the file creation
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CaptureToFile: Failed to open file " + nm);
    }
    else {
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

    if(!index) {
        j->len = 0;
    }

    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
        return 0;
    }

    j->len += len;

    return len;
}


esp_err_t CCamera::CaptureToHTTP(httpd_req_t *req, int _flashduration)
{
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();
    flashduration = _flashduration; // save last flashduration internally

    if (flashduration > 0) {
        LEDOnOff(true);
        LightOnOff(true);
        vTaskDelay(flashduration / portTICK_PERIOD_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();

    if (flashduration > 0) {    // Switch off if flashlight was on
        LEDOnOff(false); 
        LightOnOff(false);
    }

    if (!fb) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CaptureToFile: Capture Failed. "
                                        "Check camera module and/or proper electrical connection");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
  
    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=raw.jpg");
    }

    if(res == ESP_OK){
        if (demoMode) { // Use images stored on SD-Card instead of camera image
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Using Demo image!");
            /* Replace Framebuffer with image from SD-Card */
            loadNextDemoImage(fb);

            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        }
        else {
            if(fb->format == PIXFORMAT_JPEG){
                fb_len = fb->len;
                res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
            } else {
                jpg_chunking_t jchunk = {req, 0};
                res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
                httpd_resp_send_chunk(req, NULL, 0);
                fb_len = jchunk.len;
            }
        }
    }
    esp_camera_fb_return(fb);
    
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %dKB %dms", (int)(fb_len/1024), (int)((fr_end - fr_start)/1000));

    return res;
}


esp_err_t CCamera::CaptureToStream(httpd_req_t *req, bool FlashlightOn)
{
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start;
    char * part_buf[64];

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Live stream started");

    if (FlashlightOn) {
        LEDOnOff(true);
        LightOnOff(true);
    }

    //httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");  //stream is blocking web interface, only serving to local

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    while(1)
    {
        fr_start = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        fb = esp_camera_fb_get();
        if (!fb) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "CaptureToStream: Camera framebuffer not available");
            break;
        }
        fb_len = fb->len;
   
        if (res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, sizeof(part_buf), _STREAM_PART, fb_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb_len);
        }
        if (res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        
        esp_camera_fb_return(fb);

        int64_t fr_end = esp_timer_get_time();
        ESP_LOGD(TAG, "JPG: %dKB %dms", (int)(fb_len/1024), (int)((fr_end - fr_start)/1000));

        if (res != ESP_OK){ // Exit loop, e.g. also when closing the webpage
            break;
        }

        int64_t fr_delta_ms = (fr_end - fr_start) / 1000;
        if (CAM_LIVESTREAM_REFRESHRATE > fr_delta_ms) {
            const TickType_t xDelay = (CAM_LIVESTREAM_REFRESHRATE - fr_delta_ms)  / portTICK_PERIOD_MS;
            ESP_LOGD(TAG, "Stream: sleep for: %ldms", (long) xDelay*10);
            vTaskDelay(xDelay);        
        }
    }

    LEDOnOff(false);
    LightOnOff(false);

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Live stream stopped");

    return res;
}


void CCamera::LightOnOff(bool status)
{
    GpioHandler* gpioHandler = gpio_handler_get();
    if ((gpioHandler != NULL) && (gpioHandler->isEnabled())) {
        ESP_LOGD(TAG, "Use gpioHandler to trigger flashlight");
        gpioHandler->flashLightEnable(status);
    }  
    else {
    #ifdef USE_PWM_LEDFLASH
        if (status) {
            ESP_LOGD(TAG, "Internal Flash-LED turn on with PWM %d", led_intensity);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, led_intensity));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }
        else {
            ESP_LOGD(TAG, "Internal Flash-LED turn off PWM");
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }
    #else
        // Init the GPIO
        gpio_pad_select_gpio(FLASH_GPIO);
        // Set the GPIO as a push/pull output 
        gpio_set_direction(FLASH_GPIO, GPIO_MODE_OUTPUT);  

        if (status)  
            gpio_set_level(FLASH_GPIO, 1);
        else
            gpio_set_level(FLASH_GPIO, 0);
    #endif
    }
}


void CCamera::LEDOnOff(bool status)
{
	if (xHandle_task_StatusLED == NULL) {
        // Init the GPIO
        gpio_pad_select_gpio(BLINK_GPIO);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);  

        if (!status)  
            gpio_set_level(BLINK_GPIO, 1);
        else
            gpio_set_level(BLINK_GPIO, 0);   
    }
}


void CCamera::GetCameraParameter(httpd_req_t *req, int &qual, framesize_t &resol)
{
    char _query[100];
    char _qual[10];
    char _size[10];

    resol = ActualResolution;
    qual = ActualQuality;


    if (httpd_req_get_url_query_str(req, _query, 100) == ESP_OK)
    {
        ESP_LOGD(TAG, "Query: %s", _query);
        if (httpd_query_key_value(_query, "size", _size, 10) == ESP_OK)
        {
            #ifdef DEBUG_DETAIL_ON   
            ESP_LOGD(TAG, "Size: %s", _size);
            #endif
            if (strcmp(_size, "QVGA") == 0)
                resol = FRAMESIZE_QVGA;       // 320x240
            else if (strcmp(_size, "VGA") == 0)
                resol = FRAMESIZE_VGA;      // 640x480
            else if (strcmp(_size, "SVGA") == 0)
                resol = FRAMESIZE_SVGA;     // 800x600
            else if (strcmp(_size, "XGA") == 0)
                resol = FRAMESIZE_XGA;      // 1024x768
            else if (strcmp(_size, "SXGA") == 0)
                resol = FRAMESIZE_SXGA;     // 1280x1024
            else if (strcmp(_size, "UXGA") == 0)
                 resol = FRAMESIZE_UXGA;     // 1600x1200   
        }
        if (httpd_query_key_value(_query, "quality", _qual, 10) == ESP_OK)
        {
            #ifdef DEBUG_DETAIL_ON   
            ESP_LOGD(TAG, "Quality: %s", _qual);
            #endif
            qual = atoi(_qual);
                
            if (qual > 63)      // Limit to max. 63
                qual = 63;
            else if (qual < 8)  // Limit to min. 8
                qual = 8;
        }
    }
}


framesize_t CCamera::TextToFramesize(const char * _size)
{
    if (strcmp(_size, "QVGA") == 0)
        return FRAMESIZE_QVGA;       // 320x240
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

    return ActualResolution;
}


bool CCamera::getCameraInitSuccessful() 
{
    return CameraInitSuccessful;
}


std::vector<std::string> demoFiles;

void CCamera::EnableDemoMode()
{
    demoFiles.clear();
    free(demoImage);

    char line[50];

    FILE *fd = fopen("/sdcard/demo/files.txt", "r");
    if (!fd) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Can not start Demo mode, the folder '/sdcard/demo/' does not contain the needed files!");
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "See Details on https://jomjol.github.io/AI-on-the-edge-device-docs/Demo-Mode!");
        return;
    }

    demoImage = (uint8_t*)malloc(DEMO_IMAGE_SIZE);
    if (demoImage == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Unable to acquire required memory for demo image!");
        return;
    }

    while (fgets(line, sizeof(line), fd) != NULL) {
        line[strlen(line) - 1] = '\0';
        demoFiles.push_back(line);
    }
    
    fclose(fd);

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Using Demo mode (" + std::to_string(demoFiles.size()) + 
            " files) instead of real camera image!");

    for (auto file : demoFiles) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, file);
    }

    demoMode = true;
}


void CCamera::DisableDemoMode()
{
    demoMode = false;
    demoFiles.clear();
    free(demoImage);
}


bool CCamera::loadNextDemoImage(camera_fb_t *fb) {
    char filename[50];
    int readBytes;
    long fileSize;

    snprintf(filename, sizeof(filename), "/sdcard/demo/%s", demoFiles[getCountFlowRounds() % demoFiles.size()].c_str());

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Using " + std::string(filename) + " as demo image");

    /* Inject saved image */

    FILE * fp = fopen(filename, "rb");
    if (!fp) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to read file: " + std::string(filename) +"!");
        return false;
    }

    fileSize = GetFileSize(filename);
    if (fileSize > DEMO_IMAGE_SIZE) {
        char buf[100];
        snprintf(buf, sizeof(buf), "Demo Image (%d bytes) is larger than provided buffer (%d bytes)!",
                (int)fileSize, DEMO_IMAGE_SIZE);
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, std::string(buf));
        return false;
    }

    readBytes = fread(demoImage, 1, DEMO_IMAGE_SIZE, fp);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "read " + std::to_string(readBytes) + " bytes");
    fclose(fp);

    fb->buf = demoImage; // Update pointer
    fb->len = readBytes;
    // ToDo do we also need to set height, width, format and timestamp?

    return true;
}


long CCamera::GetFileSize(std::string filename)
{
    struct stat stat_buf;
    long rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


CCamera::~CCamera()
{
    demoFiles.clear();
    free(demoImage);
}