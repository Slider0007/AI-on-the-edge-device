#include "configClass.h"
#include "../../include/defines.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <lwip/sockets.h>
#include <arpa/inet.h>

#include <esp_log.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "webserver.h"
#include "MainFlowControl.h"
#include "psram.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "gpioControl.h"
#include "time_sntp.h"


static const char *TAG = "CONFIG";

ConfigClass ConfigClass::cfgClass;

extern const gpio_num_t gpio_spare[];
extern const char *gpio_spare_usage[];


bool isValidIpAddress(const char *ipAddress)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) == 1;
}


void ConfigClass::clearCfgDataTemp()
{
    cfgDataTemp.sectionNumberSequences.sequence.clear();
    cfgDataTemp.sectionNumberSequences.sequence.shrink_to_fit();
    for (auto &seq : cfgDataTemp.sectionDigit.sequence) {
        seq.roi.clear();
        seq.roi.shrink_to_fit();
    }
    cfgDataTemp.sectionDigit.sequence.clear();
    cfgDataTemp.sectionDigit.sequence.shrink_to_fit();
    for (auto &seq : cfgDataTemp.sectionAnalog.sequence) {
        seq.roi.clear();
        seq.roi.shrink_to_fit();
    }
    cfgDataTemp.sectionAnalog.sequence.clear();
    cfgDataTemp.sectionAnalog.sequence.shrink_to_fit();
    cfgDataTemp.sectionPostProcessing.sequence.clear();
    cfgDataTemp.sectionPostProcessing.sequence.shrink_to_fit();
    cfgDataTemp.sectionInfluxDBv1.sequence.clear();
    cfgDataTemp.sectionInfluxDBv1.sequence.shrink_to_fit();
    cfgDataTemp.sectionInfluxDBv2.sequence.clear();
    cfgDataTemp.sectionInfluxDBv2.sequence.shrink_to_fit();
    cfgDataTemp.sectionGpio.gpioPin.clear();
    cfgDataTemp.sectionGpio.gpioPin.shrink_to_fit();
}


void ConfigClass::clearCfgData()
{
    cfgData.sectionNumberSequences.sequence.clear();
    cfgData.sectionNumberSequences.sequence.shrink_to_fit();
    for (auto &seq : cfgData.sectionDigit.sequence) {
        seq.roi.clear();
        seq.roi.shrink_to_fit();
    }
    cfgData.sectionDigit.sequence.clear();
    cfgData.sectionDigit.sequence.shrink_to_fit();
    for (auto &seq : cfgData.sectionAnalog.sequence) {
        seq.roi.clear();
        seq.roi.shrink_to_fit();
    }
    cfgData.sectionAnalog.sequence.clear();
    cfgData.sectionAnalog.sequence.shrink_to_fit();
    cfgData.sectionPostProcessing.sequence.clear();
    cfgData.sectionPostProcessing.sequence.shrink_to_fit();
    cfgData.sectionInfluxDBv1.sequence.clear();
    cfgData.sectionInfluxDBv1.sequence.shrink_to_fit();
    cfgData.sectionInfluxDBv2.sequence.clear();
    cfgData.sectionInfluxDBv2.sequence.shrink_to_fit();
    cfgData.sectionGpio.gpioPin.clear();
    cfgData.sectionGpio.gpioPin.shrink_to_fit();
}


ConfigClass::ConfigClass()
{
    // Use preallocted buffer to avoid fragmentation and reduce internal RAM usage using SPIRAM
    cJsonObjectBuffer = (uint8_t *)heap_caps_calloc(1, CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    jsonBuffer = (char *)heap_caps_calloc(1, CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}


ConfigClass::~ConfigClass()
{
    clearCfgDataTemp();
    clearCfgData();

    delete cJsonObjectBuffer;
    cJsonObjectBuffer = NULL;
    delete jsonBuffer;
    jsonBuffer = NULL;
    delete httpBuffer;
    httpBuffer = NULL;
}


void ConfigClass::readConfigFile(bool unityTest, std::string unityTestData)
{
    std::stringstream streamBuffer;

    if (unityTest) { // Unity test
        clearCfgDataTemp(); // Clear internal struct
        streamBuffer.str(unityTestData); // Inject test data
    }
    else { // Read data from file
        std::ifstream file(CONFIG_PERSISTENCE_FILE);

        if (file.good() && file.is_open()) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "readConfigFile: Config file found");
            streamBuffer << file.rdbuf();
            file.close();
        }
        else {
            if (file.is_open()) {
               file.close();
            }
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "readConfigFile: Failed to open config file");
        }
    }

    // Check for empty content -> either empty file or no / bad file
    if (streamBuffer.rdbuf()->in_avail() == 0) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "readConfigFile: No persistent config found");
        streamBuffer.str("{}"); // Ensure any content
    }

    // Modify hook to use SPIRAM for cJSON object
    cJSON_Hooks hooks;
    hooks.malloc_fn = malloc_psram_heap_cjson;
    hooks.free_fn = free_psram_heap_cjson;
    cJSON_InitHooks(&hooks);
    cJSONObjectPSRAM.preallocatedMemory = cJsonObjectBuffer;
    cJSONObjectPSRAM.preallocatedMemorySize = CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE;
    cJSONObjectPSRAM.usedMemory = 0;

    // Parse content to cJSON object structure
    cJsonObject = cJSON_Parse(streamBuffer.str().c_str());
    if (cJsonObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "parseConfig: Failed to parse JSON data | Fallback: Use default config");
        // Continue to try to restore WLAN config from NVS, otherwise Access Point is getting started to reconfigure.
    }

    // Parse config out of cJSON object structure
    parseConfig(NULL, true, unityTest);
}


esp_err_t ConfigClass::setConfigRequest(httpd_req_t *req)
{
    int remaining = req->content_len; // Content length of the request gives the size of the file being uploaded
    int received = 0;
    jsonBuffer[0] = '\0'; // Reset buffer content before usage

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");

    char *httpBuffer = (char *) ((struct HttpServerData *)req->user_ctx)->scratch;
    while (remaining > 0) {
        // Receive the file part by part into a buffer
        if ((received = httpd_req_recv(req, httpBuffer, std::min(remaining, WEBSERVER_SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; // Retry if timeout occurred
            }

            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setConfig: Config reception failed");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E92: Config reception failed");
            return ESP_FAIL;
        }

        // Concat content
        std::strncat(jsonBuffer, httpBuffer, received);

        // Keep track of remaining size of the file left to be uploaded
        remaining -= received;
    }

    // Modify hook to use SPIRAM for cJSON object
    cJSON_Hooks hooks;
    hooks.malloc_fn = malloc_psram_heap_cjson;
    hooks.free_fn = free_psram_heap_cjson;
    cJSON_InitHooks(&hooks);
    cJSONObjectPSRAM.preallocatedMemory = cJsonObjectBuffer;
    cJSONObjectPSRAM.preallocatedMemorySize = CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE;
    cJSONObjectPSRAM.usedMemory = 0;

    // Parse content to cJSON object structure
    cJsonObject = cJSON_Parse(jsonBuffer);
    if (cJsonObject == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E91: Failed to parse JSON data, e.g. malformed notation");
        return ESP_FAIL;
    }

    // Parse config out of cJSON object structure
    return parseConfig(req);
}


esp_err_t ConfigClass::parseConfig(httpd_req_t *req, bool init, bool unityTest)
{
    // Config Verison
    // ***************************
    cJSON *objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "config"), "version");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionConfig.version = objEl->valueint;

    if (init) { // Reload data from backup during initial boot
        cJSON *objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "config"), "lastmodified");
        if (cJSON_IsString(objEl))
            cfgDataTemp.sectionConfig.lastModified = objEl->valuestring;
    }
    else { // Update timestamp whenever content gets updated
        cfgDataTemp.sectionConfig.lastModified = getCurrentTimeString(TIME_FORMAT_OUTPUT);
    }


    // Operation Mode
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "operationmode"), "opmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionOperationMode.opMode = std::clamp(objEl->valueint, -1, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "operationmode"), "automaticprocessinterval");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionOperationMode.automaticProcessInterval = std::max(std::stof(objEl->valuestring), (float)0.1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "operationmode"), "usedemoimages");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionOperationMode.useDemoImages = objEl->valueint;


    // TakeImage
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "flashlight"), "flashtime");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.flashlight.flashTime = std::max(objEl->valueint, 100); // milliseconds

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "flashlight"), "flashintensity");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.flashlight.flashIntensity = std::clamp(objEl->valueint, 0, 100);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "camerafrequency");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.cameraFrequency = std::clamp(objEl->valueint, 5, 20);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "imagequality");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.imageQuality = std::clamp(objEl->valueint, 8, 63);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "imagesize");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionTakeImage.camera.imageSize = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "brightness");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.brightness = std::clamp(objEl->valueint, -2, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "contrast");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.contrast = std::clamp(objEl->valueint, -2, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "saturation");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.saturation = std::clamp(objEl->valueint, -2, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "sharpness");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.sharpness = std::clamp(objEl->valueint, -4, 3);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "exposurecontrolmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.exposureControlMode = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "autoexposurelevel");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.autoExposureLevel = std::clamp(objEl->valueint, -2, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "manualexposurevalue");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.manualExposureValue = std::clamp(objEl->valueint, 0, 1200);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "gaincontrolmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.gainControlMode = std::clamp(objEl->valueint, 0, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "manualgainvalue");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.manualGainValue = std::clamp(objEl->valueint, 0, 5);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "specialeffect");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.specialEffect = std::clamp(objEl->valueint, 0, 7);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "mirrorimage");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionTakeImage.camera.mirrorImage = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "flipimage");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionTakeImage.camera.flipImage = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "zoommode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.zoomMode = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "zoomoffsetx");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.zoomOffsetX = std::clamp(objEl->valueint, 0, 960);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "zoomoffsety");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.camera.zoomOffsetY = std::clamp(objEl->valueint, 0, 720);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "debug"), "saverawimages");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionTakeImage.debug.saveRawImages = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "debug"), "rawimageslocation");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionTakeImage.debug.rawImagesLocation = objEl->valuestring;
        validatePath(cfgDataTemp.sectionTakeImage.debug.rawImagesLocation);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "debug"), "rawimagesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionTakeImage.debug.rawImagesRetention = std::max(objEl->valueint, 0);


    // Image Alignment
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "alignmentalgo");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionImageAlignment.alignmentAlgo = std::clamp(objEl->valueint, 0, 4);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "searchfield"), "x");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionImageAlignment.searchField.x = std::max(objEl->valueint, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "searchfield"), "y");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionImageAlignment.searchField.y = std::max(objEl->valueint, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "imagerotation");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionImageAlignment.imageRotation = std::clamp(std::stof(objEl->valuestring), (float)-180.0, (float)180.0);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "flipimagesize");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionImageAlignment.flipImageSize = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "marker");
    for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
        cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);
        cJSON *arrEl;

        arrEl = cJSON_GetObjectItem(objArrEl, "x");
        if (cJSON_IsNumber(arrEl))
            cfgDataTemp.sectionImageAlignment.marker[i].x = std::max(arrEl->valueint, 1);

        arrEl = cJSON_GetObjectItem(objArrEl, "y");
        if (cJSON_IsNumber(arrEl))
            cfgDataTemp.sectionImageAlignment.marker[i].y = std::max(arrEl->valueint, 1);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "imagealignment"), "debug"), "savedebuginfo");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionImageAlignment.debug.saveDebugInfo = objEl->valueint;


    // Number Sequences
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "numbersequences"), "sequence");

    if (cJSON_GetArraySize(objEl) > 0) {
        // Restore backup --> Add sequences
        if (init) {
            for (int j = 0; j < cJSON_GetArraySize(objEl); j++) {
                cJSON *objArrEl = cJSON_GetArrayItem(objEl, j);

                std::string sequenceNameTemp = "";
                cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
                if (cJSON_IsString(sequenceArrEl)) {
                    sequenceNameTemp = sequenceArrEl->valuestring;
                }

                sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequenceid");
                if (cJSON_IsNumber(sequenceArrEl)) {
                    SequenceList sequenceEl;
                    sequenceEl.sequenceId = sequenceArrEl->valueint;
                    sequenceEl.sequenceName = sequenceNameTemp;
                    cfgDataTemp.sectionNumberSequences.sequence.push_back(sequenceEl);
                    RoiPerSequence sequenceRoiEl;
                    sequenceRoiEl.sequenceId = sequenceArrEl->valueint;
                    sequenceRoiEl.sequenceName = sequenceNameTemp;
                    cfgDataTemp.sectionDigit.sequence.push_back(sequenceRoiEl);
                    cfgDataTemp.sectionAnalog.sequence.push_back(sequenceRoiEl);
                    PostProcessingPerSequence sequencePostProcEl;
                    sequencePostProcEl.sequenceId = sequenceArrEl->valueint;
                    sequencePostProcEl.sequenceName = sequenceNameTemp;
                    cfgDataTemp.sectionPostProcessing.sequence.push_back(sequencePostProcEl);
                    InfluxDBPerSequence sequenceInfluxDBEl;
                    sequenceInfluxDBEl.sequenceId = sequenceArrEl->valueint;
                    sequenceInfluxDBEl.sequenceName = sequenceNameTemp;
                    cfgDataTemp.sectionInfluxDBv1.sequence.push_back(sequenceInfluxDBEl);
                    cfgDataTemp.sectionInfluxDBv2.sequence.push_back(sequenceInfluxDBEl);
                }
            }
        }
        else {
            // Remove deleted sequences
            for (int i = 0; i < cfgDataTemp.sectionNumberSequences.sequence.size(); i++) {
                bool existing = false;
                for (int j = 0; j < cJSON_GetArraySize(objEl); j++) {
                    cJSON *objArrEl = cJSON_GetArrayItem(objEl, j);
                    cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequenceid");
                    if (cJSON_IsNumber(sequenceArrEl)) {
                        if (cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId == sequenceArrEl->valueint) {
                            existing = true;
                            break;
                        }
                    }
                    else {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Sequence ID malformed");
                        existing = true;
                        break;
                    }
                }
                if (!existing) {
                    cfgDataTemp.sectionNumberSequences.sequence.erase(cfgDataTemp.sectionNumberSequences.sequence.begin() + i);
                    //cfgDataTemp.sectionNumberSequences.sequence.shrink_to_fit();
                    cfgDataTemp.sectionDigit.sequence.erase(cfgDataTemp.sectionDigit.sequence.begin() + i);
                    //cfgDataTemp.sectionDigit.sequence.shrink_to_fit();
                    cfgDataTemp.sectionAnalog.sequence.erase(cfgDataTemp.sectionAnalog.sequence.begin() + i);
                    //cfgDataTemp.sectionAnalog.sequence.shrink_to_fit();
                    cfgDataTemp.sectionPostProcessing.sequence.erase(cfgDataTemp.sectionPostProcessing.sequence.begin() + i);
                    //cfgDataTemp.sectionPostProcessing.sequence.shrink_to_fit();
                    cfgDataTemp.sectionInfluxDBv1.sequence.erase(cfgDataTemp.sectionInfluxDBv1.sequence.begin() + i);
                    //cfgDataTemp.sectionInfluxDBv1.sequence.shrink_to_fit();
                    cfgDataTemp.sectionInfluxDBv2.sequence.erase(cfgDataTemp.sectionInfluxDBv2.sequence.begin() + i);
                    //cfgDataTemp.sectionInfluxDBv2.sequence.shrink_to_fit();
                    i--;
                }
            }

            // Add sequences / Update existing sequences
            for (int j = 0; j < cJSON_GetArraySize(objEl); j++) {
                cJSON *objArrEl = cJSON_GetArrayItem(objEl, j);

                std::string sequenceNameTemp = "";
                cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
                if (cJSON_IsString(sequenceArrEl))
                    sequenceNameTemp = sequenceArrEl->valuestring;

                sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequenceid");
                if (cJSON_IsNumber(sequenceArrEl)) {
                    if (sequenceArrEl->valueint == -1) { // Indication for new sequence --> Add sequence (Increment ID)
                        SequenceList sequenceEl;
                        if (cfgDataTemp.sectionNumberSequences.sequence.empty()) {
                            sequenceEl.sequenceId = 0;
                        }
                        else {
                            sequenceEl.sequenceId = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceId + 1;
                        }
                        sequenceEl.sequenceName = sequenceNameTemp;
                        cfgDataTemp.sectionNumberSequences.sequence.push_back(sequenceEl);
                        RoiPerSequence sequenceRoiEl;
                        sequenceRoiEl.sequenceId = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceId ;
                        sequenceRoiEl.sequenceName = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceName;
                        cfgDataTemp.sectionDigit.sequence.push_back(sequenceRoiEl);
                        cfgDataTemp.sectionAnalog.sequence.push_back(sequenceRoiEl);
                        PostProcessingPerSequence sequencePostProcEl;
                        sequencePostProcEl.sequenceId = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceId;
                        sequencePostProcEl.sequenceName = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceName;
                        cfgDataTemp.sectionPostProcessing.sequence.push_back(sequencePostProcEl);
                        InfluxDBPerSequence sequenceInfluxDBEl;
                        sequenceInfluxDBEl.sequenceId = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceId;
                        sequenceInfluxDBEl.sequenceName = cfgDataTemp.sectionNumberSequences.sequence.back().sequenceName;
                        cfgDataTemp.sectionInfluxDBv1.sequence.push_back(sequenceInfluxDBEl);
                        cfgDataTemp.sectionInfluxDBv2.sequence.push_back(sequenceInfluxDBEl);
                    }
                    else if (sequenceArrEl->valueint > -1) { // Update existing sequence
                        for (int i = 0; i < cfgDataTemp.sectionNumberSequences.sequence.size(); i++) {
                            if (sequenceArrEl->valueint == cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId) {
                                cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName = sequenceNameTemp;
                                cfgDataTemp.sectionDigit.sequence[i].sequenceId = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId;
                                cfgDataTemp.sectionDigit.sequence[i].sequenceName = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName;
                                cfgDataTemp.sectionAnalog.sequence[i].sequenceId = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId;
                                cfgDataTemp.sectionAnalog.sequence[i].sequenceName = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName;
                                cfgDataTemp.sectionPostProcessing.sequence[i].sequenceId = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId;
                                cfgDataTemp.sectionPostProcessing.sequence[i].sequenceName = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName;
                                cfgDataTemp.sectionInfluxDBv1.sequence[i].sequenceId = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId;
                                cfgDataTemp.sectionInfluxDBv1.sequence[i].sequenceName = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName;
                                cfgDataTemp.sectionInfluxDBv2.sequence[i].sequenceId = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId;
                                cfgDataTemp.sectionInfluxDBv2.sequence[i].sequenceName = cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName;
                                break;
                            }
                        }
                    }
                }
                else {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Sequence ID malformed");
                    continue;
                }
            }
        }

        // Sort sequences
        std::sort(cfgDataTemp.sectionNumberSequences.sequence.begin(), cfgDataTemp.sectionNumberSequences.sequence.end(),
                [](const SequenceList &x, const SequenceList &y) { return x.sequenceId < y.sequenceId; });
        std::sort(cfgDataTemp.sectionDigit.sequence.begin(), cfgDataTemp.sectionDigit.sequence.end(),
                [](const RoiPerSequence &x, const RoiPerSequence &y) { return x.sequenceId < y.sequenceId; });
        std::sort(cfgDataTemp.sectionAnalog.sequence.begin(), cfgDataTemp.sectionAnalog.sequence.end(),
                [](const RoiPerSequence &x, const RoiPerSequence &y) { return x.sequenceId < y.sequenceId; });
        std::sort(cfgDataTemp.sectionPostProcessing.sequence.begin(), cfgDataTemp.sectionPostProcessing.sequence.end(),
                [](const PostProcessingPerSequence &x, const PostProcessingPerSequence &y) { return x.sequenceId < y.sequenceId; });
        std::sort(cfgDataTemp.sectionInfluxDBv1.sequence.begin(), cfgDataTemp.sectionInfluxDBv1.sequence.end(),
                [](const InfluxDBPerSequence &x, const InfluxDBPerSequence &y) { return x.sequenceId < y.sequenceId; });
        std::sort(cfgDataTemp.sectionInfluxDBv2.sequence.begin(), cfgDataTemp.sectionInfluxDBv2.sequence.end(),
                [](const InfluxDBPerSequence &x, const InfluxDBPerSequence &y) { return x.sequenceId < y.sequenceId; });
    }
    else if (cfgDataTemp.sectionNumberSequences.sequence.size() == 0) {
        // Make sure, at least one sequence is available
        cfgDataTemp.sectionNumberSequences.sequence.push_back({0, "main"});
        RoiPerSequence sequenceRoiEl;
        sequenceRoiEl.sequenceId = 0;
        sequenceRoiEl.sequenceName = "main";
        cfgDataTemp.sectionDigit.sequence.push_back(sequenceRoiEl);
        cfgDataTemp.sectionAnalog.sequence.push_back(sequenceRoiEl);
        PostProcessingPerSequence sequencePostProcEl;
        sequencePostProcEl.sequenceId = 0;
        sequencePostProcEl.sequenceName = "main";
        cfgDataTemp.sectionPostProcessing.sequence.push_back(sequencePostProcEl);
        InfluxDBPerSequence sequenceInfluxDBEl;
        sequenceInfluxDBEl.sequenceId = 0;
        sequenceInfluxDBEl.sequenceName = "main";
        cfgDataTemp.sectionInfluxDBv1.sequence.push_back(sequenceInfluxDBEl);
        cfgDataTemp.sectionInfluxDBv2.sequence.push_back(sequenceInfluxDBEl);
    }


    // Digit
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionDigit.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "model");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionDigit.model = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "cnngoodthreshold");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionDigit.cnnGoodThreshold = std::clamp(std::stof(objEl->valuestring), (float)0.00, (float)1.00);

    // Update sequences
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "sequence");
    if (cJSON_GetArraySize(objEl) > 0) {
        for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
            cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);

            std::string sequenceNameTemp = "";
            cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
            if (cJSON_IsString(sequenceArrEl))
                sequenceNameTemp = sequenceArrEl->valuestring;

            RoiPerSequence *sequenceEl = NULL;
            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequenceid");
            if (cJSON_IsNumber(sequenceArrEl)) {
                for (auto &seqEl : cfgDataTemp.sectionDigit.sequence) {
                    if (sequenceArrEl->valueint == -1) { // Update new sequence
                        if (seqEl.sequenceName == sequenceNameTemp) {
                            sequenceEl = &seqEl; // Get sequence config structure
                            break;
                        }
                    }
                    else if (sequenceArrEl->valueint == seqEl.sequenceId) { // Update existing sequence
                        sequenceEl = &seqEl; // Get sequence config structure
                        break;
                    }
                }
            }
            if (sequenceEl == NULL)
                continue;


            sequenceEl->roi.clear();
            sequenceEl->roi.shrink_to_fit();
            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "roi");
            for (int j = 0; j < cJSON_GetArraySize(sequenceArrEl); j++) {
                cJSON *roiArrEl = cJSON_GetArrayItem(sequenceArrEl, j);
                cJSON *roiEl;
                RoiElement roiElTemp;

                roiElTemp.roiName = sequenceEl->sequenceName + "_dig" + std::to_string(j+1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "x");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.x = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "y");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.y = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "dx");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.dx = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "dy");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.dy = std::max(roiEl->valueint, 1);

                sequenceEl->roi.push_back(roiElTemp);
            }
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "debug"), "saveroiimages");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionDigit.debug.saveRoiImages = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "debug"), "roiimageslocation");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionDigit.debug.roiImagesLocation = objEl->valuestring;
        validatePath(cfgDataTemp.sectionDigit.debug.roiImagesLocation);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "digit"), "debug"), "roiimagesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionDigit.debug.roiImagesRetention = std::max(objEl->valueint, 0);


    // Analog
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionAnalog.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "model");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionAnalog.model = objEl->valuestring;


    // Update sequences
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "sequence");
    if (cJSON_GetArraySize(objEl) > 0) {
        for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
            cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);

            std::string sequenceNameTemp = "";
            cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
            if (cJSON_IsString(sequenceArrEl))
                sequenceNameTemp = sequenceArrEl->valuestring;

            RoiPerSequence *sequenceEl = NULL;
            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequenceid");
            if (cJSON_IsNumber(sequenceArrEl)) {
                for (auto &seqEl : cfgDataTemp.sectionAnalog.sequence) {
                    if (sequenceArrEl->valueint == -1) { // Update new sequence
                        if (seqEl.sequenceName == sequenceNameTemp) {
                            sequenceEl = &seqEl; // Get sequence config structure
                            break;
                        }
                    }
                    else if (sequenceArrEl->valueint == seqEl.sequenceId) { // Update existing sequence
                        sequenceEl = &seqEl; // Get sequence config structure
                        break;
                    }
                }
            }
            if (sequenceEl == NULL)
                continue;

            sequenceEl->roi.clear();
            sequenceEl->roi.shrink_to_fit();
            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "roi");
            for (int j = 0; j < cJSON_GetArraySize(sequenceArrEl); j++) {
                cJSON *roiArrEl = cJSON_GetArrayItem(sequenceArrEl, j);
                cJSON *roiEl;
                RoiElement roiElTemp;

                roiElTemp.roiName = sequenceEl->sequenceName + "_ana" + std::to_string(j+1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "x");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.x = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "y");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.y = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "dx");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.dx = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "dy");
                if (cJSON_IsNumber(roiEl))
                    roiElTemp.dy = std::max(roiEl->valueint, 1);

                roiEl = cJSON_GetObjectItem(roiArrEl, "ccw");
                if (cJSON_IsBool(roiEl))
                    roiElTemp.ccw = roiEl->valueint;

                sequenceEl->roi.push_back(roiElTemp);
            }
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "debug"), "saveroiimages");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionAnalog.debug.saveRoiImages = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "debug"), "roiimageslocation");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionAnalog.debug.roiImagesLocation = objEl->valuestring;
        validatePath(cfgDataTemp.sectionAnalog.debug.roiImagesLocation);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "analog"), "debug"), "roiimagesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionAnalog.debug.roiImagesRetention = std::max(objEl->valueint, 0);


    // Post-Processing
    // ***************************
    // Disable post-processing not yet implemented // @TODO FEATURE
    /*objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "postprocessing"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionPostProcessing.enabled = objEl->valueint;*/

    // Update sequences
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "postprocessing"), "sequence");
    if (cJSON_GetArraySize(objEl) > 0) {
        for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
            cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);

            PostProcessingPerSequence *sequenceEl = NULL;
            cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
            if (cJSON_IsString(sequenceArrEl)) {
                for (auto &seqEl : cfgDataTemp.sectionPostProcessing.sequence) {
                    if (sequenceArrEl->valuestring == seqEl.sequenceName) { // Update existing sequence
                        sequenceEl = &seqEl; // Get sequence config structure
                        break;
                    }
                }
            }
            if (sequenceEl == NULL)
                continue;

            // Disable post-processing per sequence not yet implemented // @TODO FEATURE
            /*sequenceArrEl = cJSON_GetObjectItem(objArrEl, "enabled");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->enabled = sequenceArrEl->valueint;*/

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "decimalshift");
            if (cJSON_IsNumber(sequenceArrEl))
                sequenceEl->decimalShift = std::clamp(sequenceArrEl->valueint, -9, 9);

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "analogdigitsyncvalue");
            if (cJSON_IsString(sequenceArrEl))
                sequenceEl->analogDigitSyncValue = std::clamp(std::stof(sequenceArrEl->valuestring), (float)6.0, (float)9.9);

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "extendedresolution");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->extendedResolution = sequenceArrEl->valueint;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "ignoreleadingnan");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->ignoreLeadingNaN = sequenceArrEl->valueint;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "checkdigitincreaseconsistency");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->checkDigitIncreaseConsistency = sequenceArrEl->valueint;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "maxratechecktype");
            if (cJSON_IsNumber(sequenceArrEl))
                sequenceEl->maxRateCheckType = std::clamp(sequenceArrEl->valueint, 0, 2);

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "maxrate");
            if (cJSON_IsString(sequenceArrEl))
                sequenceEl->maxRate = std::max(std::stof(sequenceArrEl->valuestring), (float)0.001);

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "allownegativerate");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->allowNegativeRate = sequenceArrEl->valueint;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "usefallbackvalue");
            if (cJSON_IsBool(sequenceArrEl))
                sequenceEl->useFallbackValue = sequenceArrEl->valueint;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "fallbackvalueagestartup");
            if (cJSON_IsNumber(sequenceArrEl))
                sequenceEl->fallbackValueAgeStartup = std::max(sequenceArrEl->valueint, 0);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "postprocessing"), "debug"), "savedebuginfo");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionPostProcessing.debug.saveDebugInfo = objEl->valueint;


    // MQTT
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionMqtt.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "uri");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionMqtt.uri = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "maintopic");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.mainTopic = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.mainTopic);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "clientid");
    if (cJSON_IsString(objEl) && strlen(objEl->valuestring) <= 23) // MQTT v3.1 spec limits to max 23 character
        cfgDataTemp.sectionMqtt.clientID = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "authmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionMqtt.authMode = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "username");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionMqtt.username = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "password");
    if (cJSON_IsString(objEl) && strcmp(objEl->valuestring, "******") != 0) {
        cfgDataTemp.sectionMqtt.password = objEl->valuestring;
        saveDataToNVS("mqtt_pw", cfgDataTemp.sectionMqtt.password);
    }
    else {
        if (!unityTest) {
            loadDataFromNVS("mqtt_pw", cfgDataTemp.sectionMqtt.password);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "tls"), "cacert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.tls.caCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.tls.caCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "tls"), "clientcert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.tls.clientCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.tls.clientCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "tls"), "clientkey");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.tls.clientKey = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.tls.clientKey);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "processdatanotation");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionMqtt.processDataNotation = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "retainprocessdata");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionMqtt.retainProcessData = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "homeassistant"), "discoveryenabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionMqtt.homeAssistant.discoveryEnabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "homeassistant"), "discoveryprefix");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.homeAssistant.discoveryPrefix = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.homeAssistant.discoveryPrefix);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "homeassistant"), "statustopic");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionMqtt.homeAssistant.statusTopic = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionMqtt.homeAssistant.statusTopic);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "homeassistant"), "metertype");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionMqtt.homeAssistant.meterType = std::clamp(objEl->valueint, 0, 10);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "mqtt"), "homeassistant"), "retaindiscovery");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionMqtt.homeAssistant.retainDiscovery = objEl->valueint;


    // InfluxDB v1.x
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionInfluxDBv1.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "uri");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv1.uri = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "database");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv1.database = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "authmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionInfluxDBv1.authMode = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "username");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv1.username = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "password");
    if (cJSON_IsString(objEl) && strcmp(objEl->valuestring, "******") != 0) {
        cfgDataTemp.sectionInfluxDBv1.password = objEl->valuestring;
        saveDataToNVS("influxdbv1_pw", cfgDataTemp.sectionInfluxDBv1.password);
    }
    else {
        if (!unityTest) {
            loadDataFromNVS("influxdbv1_pw", cfgDataTemp.sectionInfluxDBv1.password);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "tls"), "cacert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv1.tls.caCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv1.tls.caCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "tls"), "clientcert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv1.tls.clientCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv1.tls.clientCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "tls"), "clientkey");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv1.tls.clientKey = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv1.tls.clientKey);
    }

    // Update sequences
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv1"), "sequence");
    if (cJSON_GetArraySize(objEl) > 0) {
        for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
            cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);

            InfluxDBPerSequence *sequenceEl = NULL;
            cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
            if (cJSON_IsString(sequenceArrEl)) {
                for (auto &seqEl : cfgDataTemp.sectionInfluxDBv1.sequence) {
                    if (sequenceArrEl->valuestring == seqEl.sequenceName) { // Update existing sequence
                        sequenceEl = &seqEl; // Get sequence config structure
                        break;
                    }
                }
            }
            if (sequenceEl == NULL)
                continue;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "measurementname");
            if (cJSON_IsString(sequenceArrEl)) {
                sequenceEl->measurementName = sequenceArrEl->valuestring;
                validateStructure(sequenceEl->measurementName);
            }

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "fieldkey1");
            if (cJSON_IsString(sequenceArrEl)) {
                sequenceEl->fieldKey1 = sequenceArrEl->valuestring;
                validateStructure(sequenceEl->fieldKey1);
            }
        }
    }


    // InfluxDB v2.x
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionInfluxDBv2.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "uri");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv2.uri = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "bucket");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv2.bucket = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "organization");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionInfluxDBv2.organization = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "authmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionInfluxDBv2.authMode = std::clamp(objEl->valueint, 0, 2);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "token");
    if (cJSON_IsString(objEl) && strcmp(objEl->valuestring, "******") != 0) {
        cfgDataTemp.sectionInfluxDBv2.token = objEl->valuestring;
        saveDataToNVS("influxdbv2_pw", cfgDataTemp.sectionInfluxDBv2.token);
    }
    else {
        if (!unityTest) {
            loadDataFromNVS("influxdbv2_pw", cfgDataTemp.sectionInfluxDBv2.token);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "tls"), "cacert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv2.tls.caCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv2.tls.caCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "tls"), "clientcert");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv2.tls.clientCert = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv2.tls.clientCert);
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "tls"), "clientkey");
    if (cJSON_IsString(objEl)) {
        cfgDataTemp.sectionInfluxDBv2.tls.clientKey = objEl->valuestring;
        validateStructure(cfgDataTemp.sectionInfluxDBv2.tls.clientKey);
    }


    // Update sequences
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "influxdbv2"), "sequence");
    if (cJSON_GetArraySize(objEl) > 0) {
        for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
            cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);

            InfluxDBPerSequence *sequenceEl = NULL;
            cJSON *sequenceArrEl = cJSON_GetObjectItem(objArrEl, "sequencename");
            if (cJSON_IsString(sequenceArrEl)) {
                for (auto &seqEl : cfgDataTemp.sectionInfluxDBv2.sequence) {
                    if (sequenceArrEl->valuestring == seqEl.sequenceName) { // Update existing sequence
                        sequenceEl = &seqEl; // Get sequence config structure
                        break;
                    }
                }
            }
            if (sequenceEl == NULL)
                continue;

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "measurementname");
            if (cJSON_IsString(sequenceArrEl)) {
                sequenceEl->measurementName = sequenceArrEl->valuestring;
                validateStructure(sequenceEl->measurementName);
            }

            sequenceArrEl = cJSON_GetObjectItem(objArrEl, "fieldkey1");
            if (cJSON_IsString(sequenceArrEl)) {
                sequenceEl->fieldKey1 = sequenceArrEl->valuestring;
                validateStructure(sequenceEl->fieldKey1);
            }
        }
    }


    // GPIO
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "gpio"), "customizationenabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionGpio.customizationEnabled = objEl->valueint;

    // Restore backup --> Add sequences
    if (init) {
        for (int i = 0; i < GPIO_SPARE_PIN_COUNT; i++) {
            if (gpio_spare[i] == -1)
                continue;

            bool elementVerified = false;
            for (auto it = std::begin(cfgDataTemp.sectionGpio.gpioPin); it != std::end(cfgDataTemp.sectionGpio.gpioPin); ++it) {
                if (gpio_spare[i] == (gpio_num_t)it->gpioNumber) {
                    elementVerified = true;
                }
            }

            if (!elementVerified) {
                GpioElement gpioEl;
                gpioEl.gpioNumber = (int)gpio_spare[i];
                gpioEl.gpioUsage = gpio_spare_usage[i];
                if (std::string(gpio_spare_usage[i]).substr(0, 10) == "flashlight")
                    gpioEl.pinMode = "flashlight-default";

                cfgDataTemp.sectionGpio.gpioPin.push_back(gpioEl);
            }
        }
    }

    // Sort gpio pins
    std::sort(cfgDataTemp.sectionGpio.gpioPin.begin(), cfgDataTemp.sectionGpio.gpioPin.end(),
              [](const GpioElement &x, const GpioElement &y) { return x.gpioNumber < y.gpioNumber; });

    // Gather gpio data
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "gpio"), "gpiopin");
    for (int i = 0; i < cJSON_GetArraySize(objEl); i++) {
        cJSON *objArrEl = cJSON_GetArrayItem(objEl, i);
        cJSON *arrEl;
        GpioElement *gpioElTemp = NULL;

        arrEl = cJSON_GetObjectItem(objArrEl, "gpionumber");
        if (cJSON_IsNumber(arrEl)) {
            for (auto &gpioEl : cfgDataTemp.sectionGpio.gpioPin) {
                if (gpioEl.gpioNumber == arrEl->valueint) {
                    gpioElTemp = &gpioEl; // Get pin config structure
                }
            }
        }
        if (gpioElTemp == NULL)
            continue;

        for (int i = 0; i < GPIO_SPARE_PIN_COUNT; i++) {
            if (gpio_spare[i] == gpioElTemp->gpioNumber)
                gpioElTemp->gpioUsage = gpio_spare_usage[i];
        }

        arrEl = cJSON_GetObjectItem(objArrEl, "pinenabled");
        if (cJSON_IsBool(arrEl))
            gpioElTemp->pinEnabled = arrEl->valueint;

        arrEl = cJSON_GetObjectItem(objArrEl, "pinname");
        if (cJSON_IsString(arrEl))
            gpioElTemp->pinName = arrEl->valuestring;

        arrEl = cJSON_GetObjectItem(objArrEl, "pinmode");
        if (cJSON_IsString(arrEl))
            gpioElTemp->pinMode = arrEl->valuestring;

        arrEl = cJSON_GetObjectItem(objArrEl, "capturemode");
        if (cJSON_IsString(arrEl))
            gpioElTemp->captureMode = arrEl->valuestring;

        arrEl = cJSON_GetObjectItem(objArrEl, "inputdebouncetime");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->inputDebounceTime = std::clamp(arrEl->valueint, 0, 5000); // Milliseconds

        arrEl = cJSON_GetObjectItem(objArrEl, "pwmfrequency");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->PwmFrequency = std::clamp(arrEl->valueint, 5, 1000000); // Hertz

        arrEl = cJSON_GetObjectItem(objArrEl, "exposetomqtt");
        if (cJSON_IsBool(arrEl))
            gpioElTemp->exposeToMqtt = arrEl->valueint;

        arrEl = cJSON_GetObjectItem(objArrEl, "exposetorest");
        if (cJSON_IsBool(arrEl))
            gpioElTemp->exposeToRest = arrEl->valueint;

        arrEl = cJSON_GetObjectItem(cJSON_GetObjectItem(objArrEl, "smartled"), "type");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->smartLed.type = std::clamp(arrEl->valueint, 0, 5);

        arrEl = cJSON_GetObjectItem(cJSON_GetObjectItem(objArrEl, "smartled"), "quantity");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->smartLed.quantity = std::max(arrEl->valueint, 1);

        arrEl = cJSON_GetObjectItem(cJSON_GetObjectItem(objArrEl, "smartled"), "colorredchannel");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->smartLed.colorRedChannel = std::clamp(arrEl->valueint, 0, 255);

        arrEl = cJSON_GetObjectItem(cJSON_GetObjectItem(objArrEl, "smartled"), "colorgreenchannel");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->smartLed.colorGreenChannel = std::clamp(arrEl->valueint, 0, 255);

        arrEl = cJSON_GetObjectItem(cJSON_GetObjectItem(objArrEl, "smartled"), "colorbluechannel");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->smartLed.colorBlueChannel = std::clamp(arrEl->valueint, 0, 255);

        arrEl = cJSON_GetObjectItem(objArrEl, "intensitycorrectionfactor");
        if (cJSON_IsNumber(arrEl))
            gpioElTemp->intensityCorrectionFactor = std::clamp(arrEl->valueint, 1, 100);
    }


    // Logging
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "log"), "debug"), "loglevel");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionLog.debug.logLevel = std::clamp(objEl->valueint, 1, 4);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "log"), "debug"), "logfilesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionLog.debug.logFilesRetention = std::max(objEl->valueint, 0);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "log"), "debug"), "debugfilesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionLog.debug.debugFilesRetention = std::max(objEl->valueint, 0);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "log"), "data"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionLog.data.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "log"), "data"), "datafilesretention");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionLog.data.dataFilesRetention = std::max(objEl->valueint, 0);


    // Network
    //@TODO FEATURE. WLAN operation modes not yet implemented
    /*objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "opmode");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionNetwork.wlan.enabled = objEl->valueint;*/

    bool ssidEmpty = false;
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ssid");
    if (cJSON_IsString(objEl) && strlen(objEl->valuestring) > 0) {
        cfgDataTemp.sectionNetwork.wlan.ssid = objEl->valuestring;
        cfgDataTemp.sectionNetwork.wlan.ssid = trim(cfgDataTemp.sectionNetwork.wlan.ssid); // Remove leading / trailing whitespaces
        saveDataToNVS("wlan_ssid", cfgDataTemp.sectionNetwork.wlan.ssid);
    }
    else {
        if (init) {
            ssidEmpty = true; // If SSID is empty or no config provided, try to use saved data from NVS for SSID and password
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "parseConfig: No SSID config, try to use SSID and password from NVS");
        }

        if (!unityTest) {
            loadDataFromNVS("wlan_ssid", cfgDataTemp.sectionNetwork.wlan.ssid);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "password");
    if (cJSON_IsString(objEl) && strcmp(objEl->valuestring, "******") != 0  && !ssidEmpty) {
        cfgDataTemp.sectionNetwork.wlan.password = objEl->valuestring;
        saveDataToNVS("wlan_pw", cfgDataTemp.sectionNetwork.wlan.password);
    }
    else {
        if (!unityTest) {
            loadDataFromNVS("wlan_pw", cfgDataTemp.sectionNetwork.wlan.password);
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "hostname");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionNetwork.wlan.hostname = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ipv4"), "networkconfig");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionNetwork.wlan.ipv4.networkConfig = std::clamp(objEl->valueint, 0, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ipv4"), "ipaddress");
    if (cJSON_IsString(objEl) && isValidIpAddress(objEl->valuestring))
        cfgDataTemp.sectionNetwork.wlan.ipv4.ipAddress = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ipv4"), "subnetmask");
    if (cJSON_IsString(objEl) && isValidIpAddress(objEl->valuestring))
        cfgDataTemp.sectionNetwork.wlan.ipv4.subnetMask = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ipv4"), "gatewayaddress");
    if (cJSON_IsString(objEl) && isValidIpAddress(objEl->valuestring))
        cfgDataTemp.sectionNetwork.wlan.ipv4.gatewayAddress = objEl->valuestring;

    // Static IP config selected, but IP config invalid --> Fallback to DHCP
    if (cfgDataTemp.sectionNetwork.wlan.ipv4.networkConfig == NETWORK_CONFIG_STATIC) {
        if (!isValidIpAddress(cfgDataTemp.sectionNetwork.wlan.ipv4.ipAddress.c_str()) ||
            !isValidIpAddress(cfgDataTemp.sectionNetwork.wlan.ipv4.subnetMask.c_str()) ||
            !isValidIpAddress(cfgDataTemp.sectionNetwork.wlan.ipv4.gatewayAddress.c_str())) {
                cfgDataTemp.sectionNetwork.wlan.ipv4.networkConfig = NETWORK_CONFIG_DHCP;
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "parseConfig: Static network config invalid. Use DHCP as fallback");
        }
    }

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "ipv4"), "dnsserver");
    if (cJSON_IsString(objEl) && isValidIpAddress(objEl->valuestring))
        cfgDataTemp.sectionNetwork.wlan.ipv4.dnsServer = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "wlanroaming"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionNetwork.wlan.wlanRoaming.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "wlanroaming"), "rssithreshold");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionNetwork.wlan.wlanRoaming.rssiThreshold = std::clamp(objEl->valueint, -100, 0);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "time"), "ntp"), "timesyncenabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionNetwork.time.ntp.timeSyncEnabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "time"), "ntp"), "timeserver");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionNetwork.time.ntp.timeServer = objEl->valuestring;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "time"), "ntp"), "processstartinterlock");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionNetwork.time.ntp.processStartInterlock = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "time"), "timezone");
    if (cJSON_IsString(objEl))
        cfgDataTemp.sectionNetwork.time.timeZone = objEl->valuestring;


    // System
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "system"), "cpufrequency");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionSystem.cpuFrequency = std::clamp(objEl->valueint, 160, 240);


    // WebUI
    // ***************************
    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "webui"), "autorefresh"), "overviewpage"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionWebUi.AutoRefresh.overviewPage.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "webui"), "autorefresh"), "overviewpage"), "refreshtime");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionWebUi.AutoRefresh.overviewPage.refreshTime = std::max(objEl->valueint, 1);

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "webui"), "autorefresh"), "datagraphpage"), "enabled");
    if (cJSON_IsBool(objEl))
        cfgDataTemp.sectionWebUi.AutoRefresh.dataGraphPage.enabled = objEl->valueint;

    objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "webui"), "autorefresh"), "datagraphpage"), "refreshtime");
    if (cJSON_IsNumber(objEl))
        cfgDataTemp.sectionWebUi.AutoRefresh.dataGraphPage.refreshTime = std::max(objEl->valueint, 1);

    cJSON_InitHooks(NULL); // Reset cJSON hooks to default (cJSON_Delete -> not needed)

    // Init active config struct with latest configuration data
    if (init) {
        cfgData = cfgDataTemp;
    }

    // Serialize config to provide feedback and/or store to JSON file
    if (serializeConfig(unityTest) != ESP_OK) {
        return ESP_FAIL;
    }

    // Response actual config to HTTP POST request
    if (req) {
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_send(req, jsonBuffer, strlen(jsonBuffer));
    }

    // Only for testing purpose
    if (!unityTest) {
        return writeConfigFile();
    }

    return ESP_OK;
}


esp_err_t ConfigClass::serializeConfig(bool unityTest)
{
    // Modify hook to use SPIRAM for cJSON object
    cJSON_Hooks hooks;
    hooks.malloc_fn = malloc_psram_heap_cjson;
    hooks.free_fn = free_psram_heap_cjson;
    cJSON_InitHooks(&hooks);
    cJSONObjectPSRAM.preallocatedMemory = cJsonObjectBuffer;
    cJSONObjectPSRAM.preallocatedMemorySize = CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE;
    cJSONObjectPSRAM.usedMemory = 0;

    cJsonObject = cJSON_CreateObject();
    if (cJsonObject == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "serializeConfig: Error while creating JSON object");
        return ESP_FAIL;
    }

    esp_err_t retVal = ESP_OK;

    // Config Version
    // ***************************
    cJSON *config;
    if (!cJSON_AddItemToObject(cJsonObject, "config", config = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(config, "version", cfgDataTemp.sectionConfig.version) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(config, "lastmodified", cfgDataTemp.sectionConfig.lastModified.c_str()) == NULL)
        retVal = ESP_FAIL;


    // Operation Mode
    // ***************************
    cJSON *operationmode;
    if (!cJSON_AddItemToObject(cJsonObject, "operationmode", operationmode = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(operationmode, "opmode", cfgDataTemp.sectionOperationMode.opMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(operationmode, "automaticprocessinterval",
                                to_stringWithPrecision(cfgDataTemp.sectionOperationMode.automaticProcessInterval, 1).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(operationmode, "usedemoimages", cfgDataTemp.sectionOperationMode.useDemoImages) == NULL)
        retVal = ESP_FAIL;


    // Take Image
    // ***************************
    cJSON *takeImage, *flashlight, *camera, *takeImageDebug;
    if (!cJSON_AddItemToObject(cJsonObject, "takeimage", takeImage = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(takeImage, "flashlight", flashlight = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(flashlight, "flashtime", cfgDataTemp.sectionTakeImage.flashlight.flashTime) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(flashlight, "flashintensity", cfgDataTemp.sectionTakeImage.flashlight.flashIntensity) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(takeImage, "camera", camera = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "camerafrequency", cfgDataTemp.sectionTakeImage.camera.cameraFrequency) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "imagequality", cfgDataTemp.sectionTakeImage.camera.imageQuality) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(camera, "imagesize", cfgDataTemp.sectionTakeImage.camera.imageSize.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "brightness", cfgDataTemp.sectionTakeImage.camera.brightness) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "contrast", cfgDataTemp.sectionTakeImage.camera.contrast) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "saturation", cfgDataTemp.sectionTakeImage.camera.saturation) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "sharpness", cfgDataTemp.sectionTakeImage.camera.sharpness) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "exposurecontrolmode", cfgDataTemp.sectionTakeImage.camera.exposureControlMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "autoexposurelevel", cfgDataTemp.sectionTakeImage.camera.autoExposureLevel) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "manualexposurevalue", cfgDataTemp.sectionTakeImage.camera.manualExposureValue) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "gaincontrolmode", cfgDataTemp.sectionTakeImage.camera.gainControlMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "manualgainvalue", cfgDataTemp.sectionTakeImage.camera.manualGainValue) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "specialeffect", cfgDataTemp.sectionTakeImage.camera.specialEffect) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(camera, "mirrorimage", cfgDataTemp.sectionTakeImage.camera.mirrorImage) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(camera, "flipimage", cfgDataTemp.sectionTakeImage.camera.flipImage) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "zoommode", cfgDataTemp.sectionTakeImage.camera.zoomMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "zoomoffsetx", cfgDataTemp.sectionTakeImage.camera.zoomOffsetX) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(camera, "zoomoffsety", cfgDataTemp.sectionTakeImage.camera.zoomOffsetY) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(takeImage, "debug", takeImageDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(takeImageDebug, "saverawimages", cfgDataTemp.sectionTakeImage.debug.saveRawImages) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(takeImageDebug, "rawimageslocation", cfgDataTemp.sectionTakeImage.debug.rawImagesLocation.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(takeImageDebug, "rawimagesretention", cfgDataTemp.sectionTakeImage.debug.rawImagesRetention) == NULL)
        retVal = ESP_FAIL;


    // Image Alignment
    // ***************************
    cJSON *imageAlignment, *searchField, *marker, *markerEl, *imageAlignmentDebug;
    if (!cJSON_AddItemToObject(cJsonObject, "imagealignment", imageAlignment = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(imageAlignment, "alignmentalgo", cfgDataTemp.sectionImageAlignment.alignmentAlgo) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(imageAlignment, "searchfield", searchField = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(searchField, "x", cfgDataTemp.sectionImageAlignment.searchField.x) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(searchField, "y", cfgDataTemp.sectionImageAlignment.searchField.y) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(imageAlignment, "imagerotation", to_stringWithPrecision(cfgDataTemp.sectionImageAlignment.imageRotation, 1).c_str()) ==
        NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(imageAlignment, "flipimagesize", cfgDataTemp.sectionImageAlignment.flipImageSize) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(imageAlignment, "marker", marker = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < 2; ++i) {
        cJSON_AddItemToArray(marker, markerEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(markerEl, "x", cfgDataTemp.sectionImageAlignment.marker[i].x) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(markerEl, "y", cfgDataTemp.sectionImageAlignment.marker[i].y) == NULL)
            retVal = ESP_FAIL;
    }
    if (!cJSON_AddItemToObject(imageAlignment, "debug", imageAlignmentDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(imageAlignmentDebug, "savedebuginfo", cfgDataTemp.sectionImageAlignment.debug.saveDebugInfo) == NULL)
        retVal = ESP_FAIL;


    // Number Sequences
    // ***************************
    cJSON *numbersequences, *sequences, *sequence;
    if (!cJSON_AddItemToObject(cJsonObject, "numbersequences", numbersequences = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(numbersequences, "sequence", sequences = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionNumberSequences.sequence.size(); ++i) {
        cJSON_AddItemToArray(sequences, sequence = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(sequence, "sequenceid", cfgDataTemp.sectionNumberSequences.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(sequence, "sequencename", cfgDataTemp.sectionNumberSequences.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
    }


    // Digit
    // ***************************
    cJSON *digit, *digitSequence, *digitSequenceEl, *digitRoi, *digitRoiEl, *digitDebug;
    if (!cJSON_AddItemToObject(cJsonObject, "digit", digit = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(digit, "enabled", cfgDataTemp.sectionDigit.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(digit, "model", cfgDataTemp.sectionDigit.model.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(digit, "cnngoodthreshold", to_stringWithPrecision(cfgDataTemp.sectionDigit.cnnGoodThreshold, 2).c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(digit, "sequence", digitSequence = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionDigit.sequence.size(); i++) {
        cJSON_AddItemToArray(digitSequence, digitSequenceEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(digitSequenceEl, "sequenceid", cfgDataTemp.sectionDigit.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(digitSequenceEl, "sequencename", cfgDataTemp.sectionDigit.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (!cJSON_AddItemToObject(digitSequenceEl, "roi", digitRoi = cJSON_CreateArray()))
            retVal = ESP_FAIL;
        for (int j = 0; j < cfgDataTemp.sectionDigit.sequence[i].roi.size(); j++) {
            cJSON_AddItemToArray(digitRoi, digitRoiEl = cJSON_CreateObject());
            if (cJSON_AddNumberToObject(digitRoiEl, "x", cfgDataTemp.sectionDigit.sequence[i].roi[j].x) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(digitRoiEl, "y", cfgDataTemp.sectionDigit.sequence[i].roi[j].y) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(digitRoiEl, "dx", cfgDataTemp.sectionDigit.sequence[i].roi[j].dx) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(digitRoiEl, "dy", cfgDataTemp.sectionDigit.sequence[i].roi[j].dy) == NULL)
                retVal = ESP_FAIL;
        }
    }
    if (!cJSON_AddItemToObject(digit, "debug", digitDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(digitDebug, "saveroiimages", cfgDataTemp.sectionDigit.debug.saveRoiImages) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(digitDebug, "roiimageslocation", cfgDataTemp.sectionDigit.debug.roiImagesLocation.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(digitDebug, "roiimagesretention", cfgDataTemp.sectionDigit.debug.roiImagesRetention) == NULL)
        retVal = ESP_FAIL;


    // Analog
    // ***************************
    cJSON *analog, *analogSequence, *analogSequenceEl, *analogRoi, *analogRoiEl, *analogDebug;
    if (!cJSON_AddItemToObject(cJsonObject, "analog", analog = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(analog, "enabled", cfgDataTemp.sectionAnalog.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(analog, "model", cfgDataTemp.sectionAnalog.model.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(analog, "sequence", analogSequence = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionAnalog.sequence.size(); i++) {
        cJSON_AddItemToArray(analogSequence, analogSequenceEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(analogSequenceEl, "sequenceid", cfgDataTemp.sectionAnalog.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(analogSequenceEl, "sequencename", cfgDataTemp.sectionAnalog.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (!cJSON_AddItemToObject(analogSequenceEl, "roi", analogRoi = cJSON_CreateArray()))
            retVal = ESP_FAIL;
        for (int j = 0; j < cfgDataTemp.sectionAnalog.sequence[i].roi.size(); j++) {
            cJSON_AddItemToArray(analogRoi, analogRoiEl = cJSON_CreateObject());
            if (cJSON_AddNumberToObject(analogRoiEl, "x", cfgDataTemp.sectionAnalog.sequence[i].roi[j].x) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(analogRoiEl, "y", cfgDataTemp.sectionAnalog.sequence[i].roi[j].y) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(analogRoiEl, "dx", cfgDataTemp.sectionAnalog.sequence[i].roi[j].dx) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddNumberToObject(analogRoiEl, "dy", cfgDataTemp.sectionAnalog.sequence[i].roi[j].dy) == NULL)
                retVal = ESP_FAIL;
            if (cJSON_AddBoolToObject(analogRoiEl, "ccw", cfgDataTemp.sectionAnalog.sequence[i].roi[j].ccw) == NULL)
                retVal = ESP_FAIL;
        }
    }
    if (!cJSON_AddItemToObject(analog, "debug", analogDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(analogDebug, "saveroiimages", cfgDataTemp.sectionAnalog.debug.saveRoiImages) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(analogDebug, "roiimageslocation", cfgDataTemp.sectionAnalog.debug.roiImagesLocation.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(analogDebug, "roiimagesretention", cfgDataTemp.sectionAnalog.debug.roiImagesRetention) == NULL)
        retVal = ESP_FAIL;


    // Post-Processing
    // ***************************
    cJSON *postprocessing, *postprocessingSequence, *postprocessingSequenceEl, *postprocessingDebug;
    if (!cJSON_AddItemToObject(cJsonObject, "postprocessing", postprocessing = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    // Disable post-processing not yet implemented // @TODO FEATURE
    /*if (cJSON_AddBoolToObject(postprocessing, "enabled", cfgDataTemp.sectionPostProcessing.enabled) == NULL)
        retVal = ESP_FAIL;*/
    if (!cJSON_AddItemToObject(postprocessing, "sequence", postprocessingSequence = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionPostProcessing.sequence.size(); ++i) {
        cJSON_AddItemToArray(postprocessingSequence, postprocessingSequenceEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(postprocessingSequenceEl, "sequenceid",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(postprocessingSequenceEl, "sequencename",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
        // Disable post-processing per sequence not yet implemented // @TODO FEATURE
        /*if (cJSON_AddBoolToObject(postprocessingSequenceEl, "enabled",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].enabled) == NULL)
            retVal = ESP_FAIL;*/
        if (cJSON_AddNumberToObject(postprocessingSequenceEl, "decimalshift",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].decimalShift) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(postprocessingSequenceEl, "analogdigitsyncvalue",
                                    to_stringWithPrecision(cfgDataTemp.sectionPostProcessing.sequence[i].analogDigitSyncValue, 1).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(postprocessingSequenceEl, "extendedresolution",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].extendedResolution) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(postprocessingSequenceEl, "ignoreleadingnan",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].ignoreLeadingNaN) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(postprocessingSequenceEl, "checkdigitincreaseconsistency",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].checkDigitIncreaseConsistency) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(postprocessingSequenceEl, "maxratechecktype",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].maxRateCheckType) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(postprocessingSequenceEl, "maxrate",
                                    to_stringWithPrecision(cfgDataTemp.sectionPostProcessing.sequence[i].maxRate, 3).c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(postprocessingSequenceEl, "allownegativerate",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].allowNegativeRate) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(postprocessingSequenceEl, "usefallbackvalue",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].useFallbackValue) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(postprocessingSequenceEl, "fallbackvalueagestartup",
                                    cfgDataTemp.sectionPostProcessing.sequence[i].fallbackValueAgeStartup) == NULL)
            retVal = ESP_FAIL;
    }
    if (!cJSON_AddItemToObject(postprocessing, "debug", postprocessingDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(postprocessingDebug, "savedebuginfo", cfgDataTemp.sectionPostProcessing.debug.saveDebugInfo) == NULL)
        retVal = ESP_FAIL;


    // MQTT
    // ***************************
    cJSON *mqtt, *mqttTls, *mqttHomeAssistant;
    if (!cJSON_AddItemToObject(cJsonObject, "mqtt", mqtt = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(mqtt, "enabled", cfgDataTemp.sectionMqtt.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqtt, "uri", cfgDataTemp.sectionMqtt.uri.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqtt, "maintopic", cfgDataTemp.sectionMqtt.mainTopic.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqtt, "clientid", cfgDataTemp.sectionMqtt.clientID.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(mqtt, "authmode", cfgDataTemp.sectionMqtt.authMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqtt, "username", cfgDataTemp.sectionMqtt.username.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqtt, "password", cfgDataTemp.sectionMqtt.password.empty() ? "" : "******") == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(mqtt, "tls", mqttTls = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqttTls, "cacert", cfgDataTemp.sectionMqtt.tls.caCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqttTls, "clientcert", cfgDataTemp.sectionMqtt.tls.clientCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqttTls, "clientkey", cfgDataTemp.sectionMqtt.tls.clientKey.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(mqtt, "processdatanotation", cfgDataTemp.sectionMqtt.processDataNotation) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(mqtt, "retainprocessdata", cfgDataTemp.sectionMqtt.retainProcessData) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(mqtt, "homeassistant", mqttHomeAssistant = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(mqttHomeAssistant, "discoveryenabled", cfgDataTemp.sectionMqtt.homeAssistant.discoveryEnabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqttHomeAssistant, "discoveryprefix", cfgDataTemp.sectionMqtt.homeAssistant.discoveryPrefix.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(mqttHomeAssistant, "statustopic", cfgDataTemp.sectionMqtt.homeAssistant.statusTopic.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(mqttHomeAssistant, "metertype", cfgDataTemp.sectionMqtt.homeAssistant.meterType) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(mqttHomeAssistant, "retaindiscovery", cfgDataTemp.sectionMqtt.homeAssistant.retainDiscovery) == NULL)
        retVal = ESP_FAIL;


    // InfluxDB v1.x
    // ***************************
    cJSON *influxdbv1, *influxdbv1Tls, *influxdbv1Sequence, *influxdbv1SequenceEl;
    if (!cJSON_AddItemToObject(cJsonObject, "influxdbv1", influxdbv1 = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(influxdbv1, "enabled", cfgDataTemp.sectionInfluxDBv1.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1, "uri", cfgDataTemp.sectionInfluxDBv1.uri.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1, "database", cfgDataTemp.sectionInfluxDBv1.database.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(influxdbv1, "authmode", cfgDataTemp.sectionInfluxDBv1.authMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1, "username", cfgDataTemp.sectionInfluxDBv1.username.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1, "password", cfgDataTemp.sectionInfluxDBv1.password.empty() ? "" : "******") == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(influxdbv1, "tls", influxdbv1Tls = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1Tls, "cacert", cfgDataTemp.sectionInfluxDBv1.tls.caCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1Tls, "clientcert", cfgDataTemp.sectionInfluxDBv1.tls.clientCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv1Tls, "clientkey", cfgDataTemp.sectionInfluxDBv1.tls.clientKey.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(influxdbv1, "sequence", influxdbv1Sequence = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionInfluxDBv1.sequence.size(); ++i) {
        cJSON_AddItemToArray(influxdbv1Sequence, influxdbv1SequenceEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(influxdbv1SequenceEl, "sequenceid", cfgDataTemp.sectionInfluxDBv1.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv1SequenceEl, "sequencename", cfgDataTemp.sectionInfluxDBv1.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv1SequenceEl, "measurementname", cfgDataTemp.sectionInfluxDBv1.sequence[i].measurementName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv1SequenceEl, "fieldkey1", cfgDataTemp.sectionInfluxDBv1.sequence[i].fieldKey1.c_str()) == NULL)
            retVal = ESP_FAIL;
    }


    // InfluxDB v2.x
    // ***************************
    cJSON *influxdbv2, *influxdbv2Tls, *influxdbv2Sequence, *influxdbv2SequenceEl;
    if (!cJSON_AddItemToObject(cJsonObject, "influxdbv2", influxdbv2 = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(influxdbv2, "enabled", cfgDataTemp.sectionInfluxDBv2.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2, "uri", cfgDataTemp.sectionInfluxDBv2.uri.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2, "bucket", cfgDataTemp.sectionInfluxDBv2.bucket.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2, "organization", cfgDataTemp.sectionInfluxDBv2.organization.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(influxdbv2, "authmode", cfgDataTemp.sectionInfluxDBv2.authMode) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2, "token", cfgDataTemp.sectionInfluxDBv2.token.empty() ? "" : "******") == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(influxdbv2, "tls", influxdbv2Tls = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2Tls, "cacert", cfgDataTemp.sectionInfluxDBv2.tls.caCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2Tls, "clientcert", cfgDataTemp.sectionInfluxDBv2.tls.clientCert.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(influxdbv2Tls, "clientkey", cfgDataTemp.sectionInfluxDBv2.tls.clientKey.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(influxdbv2, "sequence", influxdbv2Sequence = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionInfluxDBv2.sequence.size(); ++i) {
        cJSON_AddItemToArray(influxdbv2Sequence, influxdbv2SequenceEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(influxdbv2SequenceEl, "sequenceid", cfgDataTemp.sectionInfluxDBv2.sequence[i].sequenceId) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv2SequenceEl, "sequencename", cfgDataTemp.sectionInfluxDBv2.sequence[i].sequenceName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv2SequenceEl, "measurementname", cfgDataTemp.sectionInfluxDBv2.sequence[i].measurementName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(influxdbv2SequenceEl, "fieldkey1", cfgDataTemp.sectionInfluxDBv2.sequence[i].fieldKey1.c_str()) == NULL)
            retVal = ESP_FAIL;
    }


    // GPIO
    // ***************************
    cJSON *gpio, *gpiopin, *gpiopinEl, *gpiopinSmartled;
    if (!cJSON_AddItemToObject(cJsonObject, "gpio", gpio = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(gpio, "customizationenabled", cfgDataTemp.sectionGpio.customizationEnabled) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(gpio, "gpiopin", gpiopin = cJSON_CreateArray()))
        retVal = ESP_FAIL;
    for (int i = 0; i < cfgDataTemp.sectionGpio.gpioPin.size(); ++i) {
        cJSON_AddItemToArray(gpiopin, gpiopinEl = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(gpiopinEl, "gpionumber", cfgDataTemp.sectionGpio.gpioPin[i].gpioNumber) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(gpiopinEl, "gpiousage", cfgDataTemp.sectionGpio.gpioPin[i].gpioUsage.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(gpiopinEl, "pinenabled", cfgDataTemp.sectionGpio.gpioPin[i].pinEnabled) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(gpiopinEl, "pinname", cfgDataTemp.sectionGpio.gpioPin[i].pinName.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(gpiopinEl, "pinmode", cfgDataTemp.sectionGpio.gpioPin[i].pinMode.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddStringToObject(gpiopinEl, "capturemode", cfgDataTemp.sectionGpio.gpioPin[i].captureMode.c_str()) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinEl, "inputdebouncetime", cfgDataTemp.sectionGpio.gpioPin[i].inputDebounceTime) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinEl, "pwmfrequency", cfgDataTemp.sectionGpio.gpioPin[i].PwmFrequency) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(gpiopinEl, "exposetomqtt", cfgDataTemp.sectionGpio.gpioPin[i].exposeToMqtt) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddBoolToObject(gpiopinEl, "exposetorest", cfgDataTemp.sectionGpio.gpioPin[i].exposeToRest) == NULL)
            retVal = ESP_FAIL;
        cJSON_AddItemToObject(gpiopinEl, "smartled", gpiopinSmartled = cJSON_CreateObject());
        if (cJSON_AddNumberToObject(gpiopinSmartled, "type", cfgDataTemp.sectionGpio.gpioPin[i].smartLed.type) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinSmartled, "quantity", cfgDataTemp.sectionGpio.gpioPin[i].smartLed.quantity) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinSmartled, "colorredchannel", cfgDataTemp.sectionGpio.gpioPin[i].smartLed.colorRedChannel) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinSmartled, "colorgreenchannel", cfgDataTemp.sectionGpio.gpioPin[i].smartLed.colorGreenChannel) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinSmartled, "colorbluechannel", cfgDataTemp.sectionGpio.gpioPin[i].smartLed.colorBlueChannel) == NULL)
            retVal = ESP_FAIL;
        if (cJSON_AddNumberToObject(gpiopinEl, "intensitycorrectionfactor", cfgDataTemp.sectionGpio.gpioPin[i].intensityCorrectionFactor) == NULL)
            retVal = ESP_FAIL;
    }


    // Logging
    // ***************************
    cJSON *log, *logDebug, *logDatda;
    if (!cJSON_AddItemToObject(cJsonObject, "log", log = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(log, "debug", logDebug = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(logDebug, "loglevel", cfgDataTemp.sectionLog.debug.logLevel) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(logDebug, "logfilesretention", cfgDataTemp.sectionLog.debug.logFilesRetention) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(logDebug, "debugfilesretention", cfgDataTemp.sectionLog.debug.debugFilesRetention) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(log, "data", logDatda = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(logDatda, "enabled", cfgDataTemp.sectionLog.data.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(logDatda, "datafilesretention", cfgDataTemp.sectionLog.data.dataFilesRetention) == NULL)
        retVal = ESP_FAIL;


    // Network
    // ***************************
    cJSON *network, *networkIpv4, *networkWlan, *networkWlanRoaming, *networkTime, *networkTimeNtp;
    if (!cJSON_AddItemToObject(cJsonObject, "network", network = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(network, "wlan", networkWlan = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    /*if (cJSON_AddNumberToObject(networkWlan, "opmode", cfgDataTemp.sectionNetwork.wlan.opmode) == NULL) //@TODO FEATURE. Not yet implemented
        retVal = ESP_FAIL;*/
    if (cJSON_AddStringToObject(networkWlan, "ssid", cfgDataTemp.sectionNetwork.wlan.ssid.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkWlan, "password", cfgDataTemp.sectionNetwork.wlan.password.empty() ? "" : "******") == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkWlan, "hostname", cfgDataTemp.sectionNetwork.wlan.hostname.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(networkWlan, "ipv4", networkIpv4 = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(networkIpv4, "networkconfig", cfgDataTemp.sectionNetwork.wlan.ipv4.networkConfig) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkIpv4, "ipaddress", cfgDataTemp.sectionNetwork.wlan.ipv4.ipAddress.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkIpv4, "subnetmask", cfgDataTemp.sectionNetwork.wlan.ipv4.subnetMask.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkIpv4, "gatewayaddress", cfgDataTemp.sectionNetwork.wlan.ipv4.gatewayAddress.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkIpv4, "dnsserver", cfgDataTemp.sectionNetwork.wlan.ipv4.dnsServer.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(networkWlan, "wlanroaming", networkWlanRoaming = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(networkWlanRoaming, "enabled", cfgDataTemp.sectionNetwork.wlan.wlanRoaming.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(networkWlanRoaming, "rssithreshold", cfgDataTemp.sectionNetwork.wlan.wlanRoaming.rssiThreshold) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(network, "time", networkTime = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkTime, "timezone", cfgDataTemp.sectionNetwork.time.timeZone.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(networkTime, "ntp", networkTimeNtp = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(networkTimeNtp, "timesyncenabled", cfgDataTemp.sectionNetwork.time.ntp.timeSyncEnabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddStringToObject(networkTimeNtp, "timeserver", cfgDataTemp.sectionNetwork.time.ntp.timeServer.c_str()) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(networkTimeNtp, "processstartinterlock", cfgDataTemp.sectionNetwork.time.ntp.processStartInterlock) == NULL)
        retVal = ESP_FAIL;


    // System
    // ***************************
    cJSON *system;
    if (!cJSON_AddItemToObject(cJsonObject, "system", system = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(system, "cpufrequency", cfgDataTemp.sectionSystem.cpuFrequency) == NULL)
        retVal = ESP_FAIL;


    // WebUI
    // ***************************
    cJSON *webui, *webuiAutorefresh, *webuiAutorefreshOverview, *webuiAutorefreshDataGraph;
    if (!cJSON_AddItemToObject(cJsonObject, "webui", webui = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(webui, "autorefresh", webuiAutorefresh = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(webuiAutorefresh, "overviewpage", webuiAutorefreshOverview = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(webuiAutorefreshOverview, "enabled", cfgDataTemp.sectionWebUi.AutoRefresh.overviewPage.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(webuiAutorefreshOverview, "refreshtime", cfgDataTemp.sectionWebUi.AutoRefresh.overviewPage.refreshTime) == NULL)
        retVal = ESP_FAIL;
    if (!cJSON_AddItemToObject(webuiAutorefresh, "datagraphpage", webuiAutorefreshDataGraph = cJSON_CreateObject()))
        retVal = ESP_FAIL;
    if (cJSON_AddBoolToObject(webuiAutorefreshDataGraph, "enabled", cfgDataTemp.sectionWebUi.AutoRefresh.dataGraphPage.enabled) == NULL)
        retVal = ESP_FAIL;
    if (cJSON_AddNumberToObject(webuiAutorefreshDataGraph, "refreshtime", cfgDataTemp.sectionWebUi.AutoRefresh.dataGraphPage.refreshTime) == NULL)
        retVal = ESP_FAIL;

    jsonBuffer[0] = '\0'; // Reset content
    // Print to preallocted buffer
    if (!cJSON_PrintPreallocated(cJsonObject, jsonBuffer, CONFIG_HANDLING_PREALLOCATED_BUFFER_SIZE, unityTest ? 0 : 1))
        retVal = ESP_FAIL;

    cJSON_InitHooks(NULL); // Reset cJSON hooks to default (cJSON_Delete -> not needed)

    return retVal;
}


esp_err_t ConfigClass::getConfigRequest(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");

    if (serializeConfig() == ESP_OK) {
        httpd_resp_send(req, jsonBuffer, strlen(jsonBuffer));
        return ESP_OK;
    }
    else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "E93: Failed to serialize JSON data");
        return ESP_FAIL;
    }
}


esp_err_t ConfigClass::writeConfigFile()
{
    std::ofstream file(CONFIG_PERSISTENCE_FILE);

    if (!file.good() || !file.is_open()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "writeConfigFile: Failed to write JSON file");
        return ESP_FAIL;
    }

    file << jsonBuffer;
    file.close();

    return ESP_OK;
}


bool ConfigClass::loadDataFromNVS(std::string key, std::string &value)
{
    if (key.empty() || key.length() > 15) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: Key: " + key + ": empty / too long (max. 15)");
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t nvshandle;

    err = nvs_open("cfg_data", NVS_READONLY, &nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_open | error: " + intToHexString(err));
        return false;
    }
    else if (err != ESP_OK && (err == ESP_ERR_NVS_NOT_FOUND || err == ESP_ERR_NVS_INVALID_HANDLE)) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "loadDataFromNVS: nvs_open | No data in NVS namespace 'cfg_data'");
        return false;
    }

    // Get string length
    size_t requiredSize = 0;
    err = nvs_get_str(nvshandle, key.c_str(), NULL, &requiredSize);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_get_str | Key: " + key + " length | error: " + intToHexString(err));
        nvs_close(nvshandle);
        return false;
    }

    if (requiredSize > 0) {
        char cValue[requiredSize+1];
        err = nvs_get_str(nvshandle, key.c_str(), cValue, &requiredSize);
        if (err != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "loadDataFromNVS: nvs_get_str | Key: " + key + " | error: " + intToHexString(err));
            nvs_close(nvshandle);
            return false;
        }
        value = std::string(cValue);
    }

    nvs_close(nvshandle);
    return true;
}


bool ConfigClass::saveDataToNVS(std::string key, std::string value)
{
    if (key.empty() || key.length() > 15) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: Key: " + key + ": empty / too long (max. 15)");
        return false;
    }

    esp_err_t err = ESP_OK;
    nvs_handle_t nvshandle;

    err = nvs_open("cfg_data", NVS_READWRITE, &nvshandle);
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: nvs_open | error : " + intToHexString(err));
        return false;
    }

    err = nvs_set_str(nvshandle, key.c_str(), value.c_str());
    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: Key: " + key + " | nvs_set_str | error: " + intToHexString(err));
        nvs_close(nvshandle);
        return false;
    }

    err = nvs_commit(nvshandle);
    nvs_close(nvshandle);

    if (err != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "saveDataToNVS: nvs_commit | error: " + intToHexString(err));
        return false;
    }

    return true;
}


void ConfigClass::validatePath(std::string& path, bool withFile)
{
	if (path.empty())
		return;

	// Replace backslashes
	std::replace(path.begin(), path.end(), '\\', '/');

	if (!withFile) {
		// Remove slash or backslash at the end of the string
		if(path.back() == '/')
			path.pop_back();
	}

	// Add slash at the beginning
	if(path[0] != '/')
		path = "/" + path;
}


void ConfigClass::validateStructure(std::string& structureName)
{
	if (structureName.empty())
		return;

	// Replace backslashes
	std::replace(structureName.begin(), structureName.end(), '\\', '/');

	// Remove slash at the begin of the string
	if(structureName[0] == '/')
		structureName.erase(structureName.begin());

	// Remove slash at the end of the string
	if(structureName.back() == '/')
		structureName.pop_back();
}


esp_err_t handlerGetConfigRequest(httpd_req_t *req)
{
    const char *APIName = "config:v1"; // API name and version
    char _query[200];
    char _valuechar[30];
    std::string task;

    if (httpd_req_get_url_query_str(req, _query, sizeof(_query)) == ESP_OK) {
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            task = std::string(_valuechar);
        }
    }

    if (task.compare("api_name") == 0) { // Response API version
        httpd_resp_sendstr(req, APIName);
        return ESP_OK;
    }
    else if (task.compare("reload") == 0) { // Load configuration and reinit process
        return triggerReloadConfig(req);
    }

    return ConfigClass::getInstance()->getConfigRequest(req); // Response with configuration
}


esp_err_t handlerSetConfigRequest(httpd_req_t *req)
{
    return ConfigClass::getInstance()->setConfigRequest(req);
}


void registerConfigFileUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = {};

    camuri.uri = "/config";
    camuri.handler = handlerGetConfigRequest;
    camuri.method = HTTP_GET;
    camuri.user_ctx = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);

    camuri.uri = "/config";
    camuri.handler = handlerSetConfigRequest;
    camuri.method = HTTP_POST,
    camuri.user_ctx = httpServerData; // Pass server data as context
    httpd_register_uri_handler(server, &camuri);
}
