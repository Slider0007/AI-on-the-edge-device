#include "configMigration.h"
#include "../../include/defines.h"

#include <fstream>

#include "configClass.h"
#include "configIni.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "ClassControlCamera.h"


static const char *TAG = "CFGMIGRATION";


void migrateConfiguration(void)
{
    // ********************************************************************************
    // Legacy: Support config.ini as parameter management
    // Config.ini paramter (config version 1 or 2) gets read out and gets migrated to new structure (version 3 and newer)
    // ********************************************************************************
    migrateConfigurationIni();

    // ********************************************************************************
    // New JSON based config handling
    // Structure located in cfgDataStruct.h
    // ********************************************************************************
    bool migrated = false;
    ConfigClass::getInstance()->set()->sectionConfig.desiredConfigVersion = 3; // Set to new version whenever to data structure was modified

    // Process every version iteration beginning from actual version
    // Version 3 and newer is handled in internal struct (peristant to config.json)
    for (int configFileVersion = 3; configFileVersion < ConfigClass::getInstance()->get()->sectionConfig.desiredConfigVersion; configFileVersion++) {
           //*************************************************************************************************
            // Migrate from version 3 to version 4
            // Description ....
            //*************************************************************************************************
            if (configFileVersion == 3) {
                // Update config version
                // ---------------------
                ConfigClass::getInstance()->set()->sectionConfig.version = configFileVersion + 1;
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "cfgData: Migrate v" + std::to_string(configFileVersion) +
                                                        " > v" + std::to_string(configFileVersion+1));
                migrated = true;
            }
    }
}


void migrateConfigurationIni(void)
{
    if (!fileExists(CONFIG_FILE_LEGACY)) { // No migration from config.ini needed
        return;
    }

    const std::string sectionConfigFile= "[ConfigFile]";
    bool configSectionFound = false;
    int actualConfigFileVersion = 0;

    bool migrated = false;
    bool persistConfig = false;
    std::string section = "";

    // Read config file
    std::ifstream ifs(CONFIG_FILE_LEGACY);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    std::vector<std::string> configLines = splitStringAtNewline(content); // Split config file in array of lines

    // Read config file version
    for (int i = 0; i < configLines.size(); i++) {
        if (configLines[i] == sectionConfigFile) {
            configSectionFound = true;

            std::vector<std::string> splitted = splitString(configLines[i+1]);
            if (toUpper(splitted[0]) == "VERSION") {
                actualConfigFileVersion = stoi(splitted[1]);
            }
            break;
        }
    }

    // If no [Config] section is available, add section and set config version to zero
    if (!configSectionFound) {
        configLines.insert(configLines.begin(), ""); // 3rd line
        configLines.insert(configLines.begin(), "Version = 0"); //2nd line
	    configLines.insert(configLines.begin(), sectionConfigFile); // 1st line
        migrated = true;
    }

    // Process every version iteration beginning from actual version
    // Up to version 2 config handled in config.ini.
    // Newer version of config is handled in cfgData struct and config.json (only persistence)
	for (int configFileVersion = actualConfigFileVersion; configFileVersion < 3; configFileVersion++) {
        // Process each line of config
        for (int i = 0; i < configLines.size(); i++) {
            if (configLines[i].find("[") != std::string::npos) { // Detect start of new section
                section = configLines[i];
                replaceString(section, ";", "", false); // Remove possible semicolon (just for the string comparison)
            }

           //*************************************************************************************************
            // Migrate from version 2 to version 3
            // Migrate config.ini to internal struct which gets persistant to config.json
            //*************************************************************************************************
            if (configFileVersion == 2) {
                if (section == sectionConfigFile) {
                    // Update config version
                    // ---------------------
                    ConfigClass::getInstance()->set()->sectionConfig.version = configFileVersion + 1;
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config.ini: Migrate v" + std::to_string(configFileVersion) +
                                " > v" + std::to_string(configFileVersion+1) + " => Config will be handled in firmware + config.json");

                    // Rename marker files to new naming scheme
                    renameFile("/sdcard/config/ref0.jpg", "/sdcard/config/marker1.jpg");
                    renameFile("/sdcard/config/ref1.jpg", "/sdcard/config/marker2.jpg");

                    // Rename file to harmonize model name syntax
                    renameFile("/sdcard/config/dig-class100-0168_s2_q.tflite", "/sdcard/config/dig-class100_0168_s2_q.tflite");

                    // Create model subfolder in /sdcard/config and move all models to subfolder
                    makeDir("/sdcard/config/models");
                    moveAllFilesWithFiletype("/sdcard/config", "/sdcard/config/models", "tfl");
                    moveAllFilesWithFiletype("/sdcard/config", "/sdcard/config/models", "tflite");

                    // Create backup subfolder to archive legacy config files (config.ini, wlan.ini)
                    makeDir("/sdcard/config/backup");

                    // Migrate wlan.ini --> handled in config.json
                    migrateWlanIni();

                    migrated = true;
                    persistConfig = true;
                    i += 2; // Skip lines
                }

                // Migrate parameter
                // ---------------------
                std::vector<std::string> splitted;

                if (section == "[TakeImage]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) ==  "RAWIMAGESLOCATION" || (toUpper(splitted[0]) == ";RAWIMAGESLOCATION")) && (splitted[0].size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.debug.rawImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionTakeImage.debug.saveRawImages = true;
                    }
                    if ((toUpper(splitted[0]) == "RAWIMAGESRETENTION" || (toUpper(splitted[0]) == ";RAWIMAGESRETENTION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.debug.rawImagesRetention = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "FLASHTIME") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.flashlight.flashTime = (int)(stof(splitted[1])*1000); // Flashtime in ms
                    }
                    if ((toUpper(splitted[0]) == "FLASHINTENSITY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.flashlight.flashIntensity = std::max(0, std::min(stoi(splitted[1]), 100));
                    }
                    if ((toUpper(splitted[0]) == "CAMERAFREQUENCY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.cameraFrequency = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "IMAGEQUALITY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.imageQuality = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "IMAGESIZE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.imageSize = splitted[1].c_str();
                    }
                    if ((toUpper(splitted[0]) == "BRIGHTNESS") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.brightness = stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "CONTRAST") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.contrast = stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SATURATION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.saturation = stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SHARPNESS") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.sharpness = stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "EXPOSURECONTROLMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.exposureControlMode = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "AUTOEXPOSURELEVEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.autoExposureLevel = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "MANUALEXPOSUREVALUE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.manualExposureValue = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "GAINCONTROLMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.gainControlMode = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "MANUALGAINVALUE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.manualGainValue = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SPECIALEFFECT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.specialEffect = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "MIRRORIMAGE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionTakeImage.camera.mirrorImage = true;
                        else
                            ConfigClass::getInstance()->set()->sectionTakeImage.camera.mirrorImage = false;
                    }
                    if ((toUpper(splitted[0]) == "FLIPIMAGE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionTakeImage.camera.flipImage = true;
                        else
                            ConfigClass::getInstance()->set()->sectionTakeImage.camera.flipImage = false;
                    }
                    if ((toUpper(splitted[0]) == "ZOOMMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.zoomMode = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "ZOOMOFFSETX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.zoomOffsetX = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "ZOOMOFFSETY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionTakeImage.camera.zoomOffsetY = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "DEMO") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionOperationMode.useDemoImages = true;
                        else
                            ConfigClass::getInstance()->set()->sectionOperationMode.useDemoImages = false;
                    }
                    if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionTakeImage.debug.saveAllFiles = true;
                        else
                            ConfigClass::getInstance()->set()->sectionTakeImage.debug.saveAllFiles = false;
                    }
                }

                if (section == "[Alignment]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "ALIGNMENTALGO") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "HIGHACCURACY")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_HIGH_ACCURACY;
                        else if (toUpper(splitted[1]) == "FAST")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_FAST;
                        else if (toUpper(splitted[1]) == "ROTATION")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_ROTATION_ONLY;
                        else if (toUpper(splitted[1]) == "OFF")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_OFF;
                        else
                            ConfigClass::getInstance()->set()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_DEFAULT;
                    }
                    if ((toUpper(splitted[0]) == "SEARCHFIELDX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionImageAlignment.searchField.x = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SEARCHFIELDY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionImageAlignment.searchField.y = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "INITIALROTATE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionImageAlignment.initialRotation = std::stof(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "FLIPIMAGESIZE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.flipImageSize = true;
                        else
                            ConfigClass::getInstance()->set()->sectionImageAlignment.flipImageSize = false;
                    }
                    /*if ((toUpper(splitted[0]) == "ANTIALIASING") && (splitted.size() > 1)) { //@TODO
                        if (toUpper(splitted[1]) == "TRUE")
                            useAntialiasing = true;
                        else
                            useAntialiasing = false;
                    }*/
                    if ((toUpper(splitted[0]) == "SAVEDEBUGINFO") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.debug.saveDebugInfo = true;
                        else
                            ConfigClass::getInstance()->set()->sectionImageAlignment.debug.saveDebugInfo = false;
                    }
                    if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionImageAlignment.debug.saveAllFiles = true;
                        else
                            ConfigClass::getInstance()->set()->sectionImageAlignment.debug.saveAllFiles = false;
                    }
                    static int idx = 0;
                    if (splitted.size() == 3 && idx < 2) {
                        ConfigClass::getInstance()->set()->sectionImageAlignment.marker[idx].x = std::stoi(splitted[1]);
                        ConfigClass::getInstance()->set()->sectionImageAlignment.marker[idx].y = std::stoi(splitted[2]);
                        idx++;
                    }
                }

                if (section == "[Digits]") {
                    if (configLines[i].starts_with(";"))
                        ConfigClass::getInstance()->set()->sectionDigit.enabled = false;
                    else
                        ConfigClass::getInstance()->set()->sectionDigit.enabled = true;

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "MODEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionDigit.model = splitted[1].substr(8, std::string::npos);
                    }
                    if ((toUpper(splitted[0]) == "CNNGOODTHRESHOLD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionDigit.cnnGoodThreshold = std::stof(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "ROIIMAGESLOCATION"  || (toUpper(splitted[0]) == ";ROIIMAGESLOCATION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionDigit.debug.roiImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionDigit.debug.saveRoiImages = true;
                    }
                    if ((toUpper(splitted[0]) == "ROIIMAGESRETENTION"  || (toUpper(splitted[0]) == ";ROIIMAGESRETENTION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionDigit.debug.roiImagesRetention = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1))
                    {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionDigit.debug.saveAllFiles = true;
                        else
                            ConfigClass::getInstance()->set()->sectionDigit.debug.saveAllFiles = false;
                    }
                    // Sequence related config migration is not supported. Sequences need to be configured from scratch.
                    /*if (splitted.size() >= 5) {
                        // Migration not implemented
                    }*/
                }

                if (section == "[Analog]") {
                    if (configLines[i].starts_with(";"))
                        ConfigClass::getInstance()->set()->sectionAnalog.enabled = false;
                    else
                        ConfigClass::getInstance()->set()->sectionAnalog.enabled = true;

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "MODEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionAnalog.model = splitted[1].substr(8, std::string::npos);
                    }
                    if ((toUpper(splitted[0]) == "ROIIMAGESLOCATION"  || (toUpper(splitted[0]) == ";ROIIMAGESLOCATION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionAnalog.debug.roiImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionAnalog.debug.saveRoiImages = true;
                    }
                    if ((toUpper(splitted[0]) == "ROIIMAGESRETENTION" || (toUpper(splitted[0]) == ";ROIIMAGESRETENTION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionAnalog.debug.roiImagesRetention = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1))
                    {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionAnalog.debug.saveAllFiles = true;
                        else
                            ConfigClass::getInstance()->set()->sectionAnalog.debug.saveAllFiles = false;
                    }
                    // Sequence related config migration is not supported. Sequences need to be configured from scratch.
                    /*if (splitted.size() >= 5) {
                        // Migration not implemented
                    }*/
                }

                if (section == "[PostProcessing]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    // Sequence related config migration is not supported. Sequences need to be configured from scratch.
                    /*if ((toUpper(splitted[0]) == "FALLBACKVALUEUSE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            // Migration not implemented
                        else
                            // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "FALLBACKVALUEAGESTARTUP") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "CHECKDIGITINCREASECONSISTENCY") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "ALLOWNEGATIVERATES") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "DECIMALSHIFT") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "ANALOGDIGITALTRANSITIONSTART") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "MAXRATETYPE") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "MAXRATEVALUE") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "EXTENDEDRESOLUTION") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if ((toUpper(splitted[0]) == "IGNORELEADINGNAN") && (splitted.size() > 1)) {
                        // Migration not implemented
                    }*/
                    if ((toUpper(splitted[0]) == "SAVEDEBUGINFO") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionPostProcessing.debug.saveDebugInfo = true;
                        else
                            ConfigClass::getInstance()->set()->sectionPostProcessing.debug.saveDebugInfo = true;
                    }
                }

                if (section == "[MQTT]") {
                    if (configLines[i].starts_with(";"))
                        ConfigClass::getInstance()->set()->sectionMqtt.enabled = false;
                    else
                        ConfigClass::getInstance()->set()->sectionMqtt.enabled = true;

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.uri =
                            (splitted[1] == "mqtt://IP-ADDRESS:1883"  || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "MAINTOPIC") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.mainTopic = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "CLIENTID") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.clientID = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "USER" || toUpper(splitted[0]) == ";USER") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.username = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionMqtt.authMode = AUTH_BASIC;
                    }
                    if ((toUpper(splitted[0]) == "PASSWORD" || toUpper(splitted[0]) == ";PASSWORD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.password = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionMqtt.authMode = AUTH_BASIC;
                    }
                    if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionMqtt.authMode = AUTH_TLS;
                    }
                    if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.tls.caCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.tls.clientCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.tls.clientKey = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "RETAINPROCESSDATA") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionMqtt.retainProcessData = true;
                        else
                            ConfigClass::getInstance()->set()->sectionMqtt.retainProcessData = false;
                    }
                    if ((toUpper(splitted[0]) == "PROCESSDATANOTATION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.processDataNotation = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "HOMEASSISTANTDISCOVERY") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.discoveryEnabled = true;
                        else
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.discoveryEnabled = false;
                    }
                    if ((toUpper(splitted[0]) == "HADISCOVERYPREFIX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.discoveryPrefix = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "HASTATUSTOPIC") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.statusTopic = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "HARETAINDISCOVERY") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.retainDiscovery = true;
                        else
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.retainDiscovery = false;
                    }
                    if ((toUpper(splitted[0]) == "HAMETERTYPE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "WATER_M3") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = WATER_M3;
                        }
                        else if (toUpper(splitted[1]) == "WATER_L") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = WATER_L;
                        }
                        else if (toUpper(splitted[1]) == "WATER_FT3") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = WATER_FT3;
                        }
                        else if (toUpper(splitted[1]) == "WATER_GAL") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = WATER_GAL;
                        }
                        else if (toUpper(splitted[1]) == "GAS_M3") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = GAS_M3;
                        }
                        else if (toUpper(splitted[1]) == "GAS_FT3") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = GAS_FT3;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_WH") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = ENERGY_WH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_KWH") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = ENERGY_KWH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_MWH") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = ENERGY_MWH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_GJ") {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = ENERGY_GJ;
                        }
                        else {
                            ConfigClass::getInstance()->set()->sectionMqtt.homeAssistant.meterType = TYPE_NONE;
                        }
                    }
                }

                if (section == "[InfluxDB]") {
                    if (configLines[i].starts_with(";"))
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.enabled = false;
                    else
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.enabled = true;

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.uri =
                            (splitted[1] == "http://IP-ADDRESS:PORT"  || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if (((toUpper(splitted[0]) == "DATABASE")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.database = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "USER" || toUpper(splitted[0]) == ";USER") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.username = (splitted[1] == "undefined" ? "" : splitted[1]);
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionInfluxDBv1.authMode = AUTH_BASIC;
                    }
                    if ((toUpper(splitted[0]) == "PASSWORD" || toUpper(splitted[0]) == ";PASSWORD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.password = (splitted[1] == "undefined" ? "" : splitted[1]);
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionInfluxDBv1.authMode = AUTH_BASIC;
                    }
                    if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionInfluxDBv1.authMode = AUTH_TLS;
                    }
                    if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.tls.caCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.tls.clientCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv1.tls.clientKey = splitted[1];
                    }
                    // Sequence related config migration is not supported. Sequences need to be configured from scratch.
                    /*if (((toUpper(splitted[0]) == "MEASUREMENT")) && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if (((toUpper(splitted[0]) == "FIELD")) && (splitted.size() > 1)) {
                        // Migration not implemented
                    }*/
                }

                if (section == "[InfluxDBv2]") {
                    if (configLines[i].starts_with(";"))
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.enabled = false;
                    else
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.enabled = true;

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.uri =
                            (splitted[1] == "http://IP-ADDRESS:PORT"  || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if (((toUpper(splitted[0]) == "BUCKET")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.bucket = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "ORG" || (toUpper(splitted[0]) == ";ORG")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.organization = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "TOKEN" || (toUpper(splitted[0]) == ";TOKEN")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.token = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionInfluxDBv2.authMode = AUTH_TLS;
                        else
                            ConfigClass::getInstance()->set()->sectionInfluxDBv2.authMode = AUTH_BASIC;
                    }
                    if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.tls.caCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.tls.clientCert = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionInfluxDBv2.tls.clientKey = splitted[1];
                    }
                    // Sequence related config migration is not supported. Sequences need to be configured from scratch.
                    /*if (((toUpper(splitted[0]) == "MEASUREMENT")) && (splitted.size() > 1)) {
                        // Migration not implemented
                    }
                    if (((toUpper(splitted[0]) == "FIELD")) && (splitted.size() > 1)) {
                        // Migration not implemented
                    }*/
                }

                if (section == "[AutoTimer]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "AUTOSTART") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionOperationMode.opMode = OPMODE_AUTO;
                        else
                            ConfigClass::getInstance()->set()->sectionOperationMode.opMode = OPMODE_MANUAL;
                    }
                    if ((toUpper(splitted[0]) == "INTERVAL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionOperationMode.automaticProcessInterval = std::stof(splitted[1]);
                    }
                }

                if (section == "[DataLogging]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "DATALOGACTIVE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->set()->sectionLog.data.enabled = true;
                        else
                            ConfigClass::getInstance()->set()->sectionLog.data.enabled = false;
                    }
                    if ((toUpper(splitted[0]) == "DATAFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionLog.data.dataFilesRetention = std::stoi(splitted[1]);
                    }
                }

                if (section == "[Debug]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "LOGLEVEL") && (splitted.size() > 1)) {
                        if ((toUpper(splitted[1]) == "TRUE") || (toUpper(splitted[1]) == "2")) {
                            ConfigClass::getInstance()->set()->sectionLog.debug.logLevel = ESP_LOG_WARN;
                        }
                        else if ((toUpper(splitted[1]) == "FALSE") || (toUpper(splitted[1]) == "0") || (toUpper(splitted[1]) == "1")) {
                            ConfigClass::getInstance()->set()->sectionLog.debug.logLevel = ESP_LOG_ERROR;
                        }
                        else if (toUpper(splitted[1]) == "3") {
                            ConfigClass::getInstance()->set()->sectionLog.debug.logLevel = ESP_LOG_INFO;
                        }
                        else if (toUpper(splitted[1]) == "4") {
                            ConfigClass::getInstance()->set()->sectionLog.debug.logLevel = ESP_LOG_DEBUG;
                        }
                        else {
                            ConfigClass::getInstance()->set()->sectionLog.debug.logLevel = ESP_LOG_ERROR;
                        }
                    }
                    if ((toUpper(splitted[0]) == "LOGFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionLog.debug.logFilesRetention = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "DEBUGFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionLog.debug.debugFilesRetention = std::stoi(splitted[1]);
                    }
                }

                if (section == "[System]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "TIMESERVER") || (toUpper(splitted[0]) == ";TIMESERVER")) {
                        ConfigClass::getInstance()->set()->sectionNetwork.time.ntp.timeServer = splitted[1];
                    }
                    if (toUpper(splitted[0]) == "TIMEZONE") {
                        ConfigClass::getInstance()->set()->sectionNetwork.time.timeZone = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "HOSTNAME") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->set()->sectionNetwork.wlan.hostname = splitted[1];
                    }
                    if ((toUpper(splitted[0]) == "RSSITHRESHOLD") || (toUpper(splitted[0]) == ";RSSITHRESHOLD")) {
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->set()->sectionNetwork.wlan.wlanRoaming.enabled = true;

                        ConfigClass::getInstance()->set()->sectionNetwork.wlan.wlanRoaming.rssiThreshold = atoi(splitted[1].c_str());
                    }
                    if ((toUpper(splitted[0]) == "CPUFREQUENCY")) {
                        ConfigClass::getInstance()->set()->sectionSystem.cpuFrequency = atoi(splitted[1].c_str());
                    }
                    if ((toUpper(splitted[0]) == "SETUPMODE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "FALSE") {
                            ConfigClass::getInstance()->set()->sectionOperationMode.opMode = OPMODE_AUTO;
                        }
                        else if (toUpper(splitted[1]) == "TRUE") {
                            ConfigClass::getInstance()->set()->sectionOperationMode.opMode = OPMODE_SETUP;
                        }
                    }
                }
            }

            /* Notes:
            * The migration has some simplifications:
            *  - Case sensitiveness must be like in the config.ini
            *  - No whitespace after a semicollon
            *  - Only one whitespace before/after the equal sign
            */

            //*************************************************************************************************
            // Migrate from version 1 to version 2
            // Migrate GPIO section due to PR#154 (complete refactoring of GPIO) which is part of v17.x
            //*************************************************************************************************
            if (configFileVersion == 1) {
                // Update config version
                // ---------------------
                if (section == sectionConfigFile) {
                    if(replaceString(configLines[i], "Version = " + std::to_string(configFileVersion),
                                                     "Version = " + std::to_string(configFileVersion+1))) {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config.ini: Migrate v" + std::to_string(configFileVersion) +
                                                                                " > v" + std::to_string(configFileVersion+1));
                        migrated = true;
                    }
                }

                // Migrate parameter
                // ---------------------
                if (section == "[GPIO]") {
                    // Erase complete section content due to major change in parameter usage
                    // Section will be filled again by WebUI after save config initially
                    if (configLines[i].find("[GPIO]") == std::string::npos && !configLines[i].empty()) {
                        configLines.erase(configLines.begin()+i);
                        i--; // One element removed, check same position again
                    }
                }
            }

            //*************************************************************************************************
            // Migrate from version 0 to version 1
            // Version 0: All config file versions before 17.x
            //*************************************************************************************************
            else if (configFileVersion == 0) {
                // Update config version
                // ---------------------
                if (section == sectionConfigFile) {
                    if(replaceString(configLines[i], "Version = " + std::to_string(configFileVersion),
                                                     "Version = " + std::to_string(configFileVersion+1))) {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config.ini: Migrate v" + std::to_string(configFileVersion) +
                                                                                " > v" + std::to_string(configFileVersion+1));
                        migrated = true;
                    }
                }

                // Migrate parameter
                // ---------------------
                if (section == "[MakeImage]") {
                    migrated = migrated | replaceString(configLines[i], "[MakeImage]", "[TakeImage]"); // Rename the section itself
                }

                if (section == "[MakeImage]" || section == "[TakeImage]") {
                    migrated = migrated | replaceString(configLines[i], "LogImageLocation", "RawImagesLocation");
                    migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "RawImagesRetention");

                    migrated = migrated | replaceString(configLines[i], "WaitBeforeTakingPicture", "FlashTime"); // Rename
                    migrated = migrated | replaceString(configLines[i], "LEDIntensity", "FlashIntensity"); // Rename

                    migrated = migrated | replaceString(configLines[i], "FixedExposure", "UNUSED"); // Mark as unused

                    migrated = migrated | replaceString(configLines[i], ";Demo = true", ";Demo = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";Demo", "Demo"); // Enable it
                }

                if (section == "[Alignment]") {
                    if (isInString(configLines[i], "AlignmentAlgo") && isInString(configLines[i], ";")) { // It is the parameter "AlignmentAlgo" and it is commented out
                        migrated = migrated | replaceString(configLines[i], "highAccuracy", "default"); // Set it to its default value and enable it
                        migrated = migrated | replaceString(configLines[i], "fast", "default"); // Set it to its default value and enable it
                        migrated = migrated | replaceString(configLines[i], "off", "default"); // Set it to its default value and enable it
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    migrated = migrated | replaceString(configLines[i], ";FlipImageSize = true", ";FlipImageSize = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";FlipImageSize", "FlipImageSize"); // Enable it

                    migrated = migrated | replaceString(configLines[i], "InitialMirror", "UNUSED"); // Mark as unused
                }

                if (section == "[Digits]") {
                    if (isInString(configLines[i], "CNNGoodThreshold")) { // It is the parameter "CNNGoodThreshold"
                        migrated = migrated | replaceString(configLines[i], "0.5", "0.0");
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }
                    migrated = migrated | replaceString(configLines[i], "LogImageLocation", "ROIImagesLocation");
                    migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "ROIImagesRetention");
                }

                if (section == "[Analog]") {
                    migrated = migrated | replaceString(configLines[i], "LogImageLocation", "ROIImagesLocation");
                    migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "ROIImagesRetention");
                    migrated = migrated | replaceString(configLines[i], "CNNGoodThreshold", ";UNUSED_PARAMETER"); // This parameter is no longer used
                    migrated = migrated | replaceString(configLines[i], "ExtendedResolution", ";UNUSED_PARAMETER"); // This parameter is no longer used
                }

                if (section == "[PostProcessing]") {
                    migrated = migrated | replaceString(configLines[i], ";PreValueUse = true", ";PreValueUse = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";PreValueUse", "PreValueUse"); // Enable it
                    migrated = migrated | replaceString(configLines[i], "PreValueUse", "FallbackValueUse"); // Rename it

                    migrated = migrated | replaceString(configLines[i], ";PreValueAgeStartup", "PreValueAgeStartup"); // Enable it
                    migrated = migrated | replaceString(configLines[i], "PreValueAgeStartup", "FallbackValueAgeStartup");

                    migrated = migrated | replaceString(configLines[i], ";CheckDigitIncreaseConsistency = true", ";CheckDigitIncreaseConsistency = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";CheckDigitIncreaseConsistency", "CheckDigitIncreaseConsistency"); // Enable it

                    if (isInString(configLines[i], "DecimalShift") && isInString(configLines[i], ";")) { // It is the parameter "DecimalShift" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* AllowNegativeRates has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "AllowNegativeRates") && isInString(configLines[i], ";")) { // It is the parameter "AllowNegativeRates" and it is commented out
                        migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "AnalogDigitalTransitionStart") && isInString(configLines[i], ";")) { // It is the parameter "AnalogDigitalTransitionStart" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* MaxRateType has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "MaxRateType")) { // It is the parameter "MaxRateType"
                        if (isInString(configLines[i], ";")) { // if disabled
                            migrated = migrated | replaceString(configLines[i], "= Off", "= RatePerMin"); // Convert it to its default value
                            migrated = migrated | replaceString(configLines[i], "= RateChange", "= RatePerMin"); // Convert it to its default value
                            migrated = migrated | replaceString(configLines[i], "= AbsoluteChange", "= RatePerMin"); // Convert it to its default value
                            migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                        }
                        else {
                            migrated = migrated | replaceString(configLines[i], "= Off", "= RateOff"); // Convert it to its new name
                            migrated = migrated | replaceString(configLines[i], "= RateChange", "= RatePerMin"); // Convert it to its new name
                            migrated = migrated | replaceString(configLines[i], "= AbsoluteChange", "= RatePerInterval"); // Convert it to its new name
                        }
                    }

                    if (isInString(configLines[i], "MaxRateValue") && isInString(configLines[i], ";")) { // It is the parameter "MaxRateValue" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* ExtendedResolution has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "ExtendedResolution") && isInString(configLines[i], ";")) { // It is the parameter "ExtendedResolution" and it is commented out
                        migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* IgnoreLeadingNaN has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "IgnoreLeadingNaN") && isInString(configLines[i], ";")) { // It is the parameter "IgnoreLeadingNaN" and it is commented out
                        migrated = migrated | replaceString(configLines[i], "true", "false"); // Set it to its default value
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }
                }

                if (section == "[MQTT]") {
                    migrated = migrated | replaceString(configLines[i], ";Uri", "Uri"); // Enable it
                    migrated = migrated | replaceString(configLines[i], ";MainTopic", "MainTopic"); // Enable it
                    migrated = migrated | replaceString(configLines[i], ";ClientID", "ClientID"); // Enable it

                    if (isInString(configLines[i], "CACert") && !isInString(configLines[i], "TLSCACert")) {
                        migrated = migrated | replaceString(configLines[i], "CACert =", "TLSCACert ="); // Rename it
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "ClientCert") && !isInString(configLines[i], "TLSClientCert")) {
                        migrated = migrated | replaceString(configLines[i], "ClientCert =", "TLSClientCert ="); // Rename it
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "ClientKey") && !isInString(configLines[i], "TLSClientKey")) {
                        migrated = migrated | replaceString(configLines[i], "ClientKey =", "TLSClientKey ="); // Rename it
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    migrated = migrated | replaceString(configLines[i], "SetRetainFlag", "RetainMessages"); // First rename it, enable it with its default value
                    migrated = migrated | replaceString(configLines[i], ";RetainMessages = true", ";RetainMessages = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";RetainMessages", "RetainMessages"); // Enable it
                    migrated = migrated | replaceString(configLines[i], "RetainMessages", "RetainProcessData"); // Rename it

                    migrated = migrated | replaceString(configLines[i], ";HomeassistantDiscovery = true", ";HomeassistantDiscovery = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";HomeassistantDiscovery", "HomeassistantDiscovery"); // Enable it

                    if (isInString(configLines[i], "MeterType") && !isInString(configLines[i], "HAMeterType")) {
                        migrated = migrated | replaceString(configLines[i], "MeterType =", "HAMeterType ="); // Rename it
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                        migrated = migrated | replaceString(configLines[i], "HAMeterType = other", "HAMeterType = water_m3"); // Enable it
                    }

                    if (configLines[i].rfind("Topic", 0) != std::string::npos) { // only if string starts with "Topic" (Was the naming in very old version)
                        migrated = migrated | replaceString(configLines[i], "Topic", "MainTopic");
                    }
                }

                if (section == "[InfluxDB]") {
                    if (isInString(configLines[i], "Uri") && isInString(configLines[i], ";")) { // It is the parameter "Uri" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "Database") && isInString(configLines[i], ";")) { // It is the parameter "Database" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* Measurement has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Measurement") && isInString(configLines[i], ";")) { // It is the parameter "Measurement" and is it disabled
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* Fieldname has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Fieldname")) { // It is the parameter "Fieldname"
                        migrated = migrated | replaceString(configLines[i], "Fieldname", "Field"); // Rename it to Field
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* Field has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Field") && isInString(configLines[i], ";")) { // It is the parameter "Field" and is it disabled
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }
                }

                if (section == "[InfluxDBv2]") {
                    if (isInString(configLines[i], "Uri") && isInString(configLines[i], ";")) { // It is the parameter "Uri" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "Database") && isInString(configLines[i], ";")) { // It is the parameter "Database" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }
                    if (isInString(configLines[i], "Database")) { // It is the parameter "Database"
                        migrated = migrated | replaceString(configLines[i], "Database", "Bucket"); // Rename it to Bucket
                    }

                    /* Measurement has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Measurement") && isInString(configLines[i], ";")) { // It is the parameter "Measurement" and is it disabled
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* Fieldname has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Fieldname")) { // It is the parameter "Fieldname"
                        migrated = migrated | replaceString(configLines[i], "Fieldname", "Field"); // Rename it to Field
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    /* Field has a <NUMBER> as prefix! */
                    if (isInString(configLines[i], "Field") && isInString(configLines[i], ";")) { // It is the parameter "Field" and is it disabled
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }
                }

                if (section == "[DataLogging]") {
                    /* DataLogActive is true by default! */
                    migrated = migrated | replaceString(configLines[i], ";DataLogActive = false", ";DataLogActive = true"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";DataLogActive", "DataLogActive"); // Enable it

                    migrated = migrated | replaceString(configLines[i], "DataLogRetentionInDays", "DataFilesRetention");
                }

                if (section == "[AutoTimer]") {
                    migrated = migrated | replaceString(configLines[i], ";AutoStart = true", ";AutoStart = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";AutoStart", "AutoStart"); // Enable it

                    migrated = migrated | replaceString(configLines[i], "Intervall", "Interval");
                }

                if (section == "[Debug]") {
                    migrated = migrated | replaceString(configLines[i], "Logfile ", "LogLevel "); // Whitespace needed so it does not match `LogfileRetentionInDays`
                    /* LogLevel (resp. LogFile) was originally a boolean, but we switched it to an int
                    * For both cases (true/false), we set it to level 2 (WARNING) */
                    migrated = migrated | replaceString(configLines[i], "LogLevel = true", "LogLevel = 2");
                    migrated = migrated | replaceString(configLines[i], "LogLevel = false", "LogLevel = 2");

                    migrated = migrated | replaceString(configLines[i], "LogfileRetentionInDays", "LogfilesRetention");
                }

                if (section == "[System]") {
                    if (isInString(configLines[i], "TimeServer = undefined") && isInString(configLines[i], ";"))
                    { // It is the parameter "TimeServer" and is it disabled
                        migrated = migrated | replaceString(configLines[i], "undefined", "pool.ntp.org");
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "TimeZone") && isInString(configLines[i], ";")) { // It is the parameter "TimeZone" and it is commented out
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    if (isInString(configLines[i], "Hostname") && isInString(configLines[i], ";")) { // It is the parameter "Hostname" and is it disabled
                        migrated = migrated | replaceString(configLines[i], "undefined", "watermeter");
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    migrated = migrated | replaceString(configLines[i], "RSSIThreashold", "RSSIThreshold");

                    if (isInString(configLines[i], "CPUFrequency") && isInString(configLines[i], ";")) { // It is the parameter "CPUFrequency" and is it disabled
                        migrated = migrated | replaceString(configLines[i], "240", "160");
                        migrated = migrated | replaceString(configLines[i], ";", ""); // Enable it
                    }

                    migrated = migrated | replaceString(configLines[i], "AutoAdjustSummertime", ";UNUSED_PARAMETER"); // This parameter is no longer used

                    migrated = migrated | replaceString(configLines[i], ";SetupMode = true", ";SetupMode = false"); // Set it to its default value
                    migrated = migrated | replaceString(configLines[i], ";SetupMode", "SetupMode"); // Enable it
                }
            }
        }
    }

    if (migrated) { // At least one replacement happened
        if (persistConfig)
            ConfigClass::getInstance()->persistConfig();

        if (!renameFile(CONFIG_FILE_LEGACY, CONFIG_FILE_BACKUP_LEGACY)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create backup of config.ini file");
            return;
        }
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Config file migrated. Saved backup to " + std::string(CONFIG_FILE_BACKUP_LEGACY));
    }
}


void migrateWlanIni()
{
    if (!fileExists(CONFIG_WIFI_FILE_LEGACY)) { // No migration from wlan.ini needed
        return;
    }

    std::string line = "";
    std::string tmp = "";
    std::vector<std::string> splitted;

    FILE* pFile = fopen(std::string(CONFIG_WIFI_FILE_LEGACY).c_str(), "r");
    if (pFile == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unable to open file (read)");
        return;
    }

    char zw[256];
    if (fgets(zw, sizeof(zw), pFile) == NULL) {
        line = "";
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "File opened, but empty or content not readable");
        fclose(pFile);
        return;
    }
    else {
        line = std::string(zw);
    }

    while ((line.size() > 0) || !(feof(pFile))) {
        if (line[0] != ';') {   // Skip lines which starts with ';'
            splitted = splitStringWLAN(line, "=");
            splitted[0] = trim(splitted[0], " ");

            if ((splitted.size() > 1) && (toUpper(splitted[0]) == "SSID")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.ssid = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "PASSWORD")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.password = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "HOSTNAME")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.hostname = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "IP")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.ipv4.ipAddress = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "GATEWAY")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.ipv4.gatewayAddress = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "NETMASK")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.ipv4.subnetMask = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "DNS")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->set()->sectionNetwork.wlan.ipv4.dnsServer = tmp;
            }
        }

        // read next line
        if (fgets(zw, sizeof(zw), pFile) == NULL) {
            line = "";
        }
        else {
            line = std::string(zw);
        }
    }
    fclose(pFile);

    if (!renameFile(CONFIG_WIFI_FILE_LEGACY, CONFIG_WIFI_FILE_BACKUP_LEGACY)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create backup of wlan.ini file");
    }
}


std::vector<std::string> splitString(std::string input, std::string delimiter)
{
	std::vector<std::string> Output;
	/* The input can have multiple formats:
	 *  - key = value
     *  - key = value1 value2 value3 ...
     *  - key value1 value2 value3 ...
	 *
	 * Examples:
	 *  - ImageSize = VGA
	 *  - IO0 = input disabled 10 false false
	 *  - main.dig1 28 144 55 100 false
	 *
	 * This causes issues eg. if a password key has a whitespace or equal sign in its value.
	 * As a workaround and to not break any legacy usage, we enforce to only use the
	 * equal sign, if the key is "password"
	*/
    // Line contains a password, use the equal sign as the only delimiter and only split on first occurrence
	if ((input.find("password") != std::string::npos) || (input.find("Token") != std::string::npos)) {
		size_t pos = input.find("=");
		Output.push_back(trim(input.substr(0, pos), ""));
		Output.push_back(trim(input.substr(pos +1, std::string::npos), ""));
	}
	else { // Legacy Mode
		input = trim(input, delimiter);	// trim to avoid delimiter deletion at the of string (z.B. == in string)
		size_t pos = findDelimiterPos(input, delimiter);
		std::string token;
		while (pos != std::string::npos) {
			token = input.substr(0, pos);
			token = trim(token, delimiter);
			Output.push_back(token);
			input.erase(0, pos + 1);
			input = trim(input, delimiter);
			pos = findDelimiterPos(input, delimiter);
		}
		Output.push_back(input);
	}

	return Output;
}


std::vector<std::string> splitStringWLAN(std::string input, std::string _delimiter)
{
	std::vector<std::string> Output;
	std::string delimiter = " =,";
    if (_delimiter.length() > 0) {
        delimiter = _delimiter;
    }

	input = trim(input, delimiter);
	size_t pos = findDelimiterPos(input, delimiter);
	std::string token;
    if (pos != std::string::npos) { // splitted only up to first equal sign !!! Special case for WLAN.ini
		token = input.substr(0, pos);
		token = trim(token, delimiter);
		Output.push_back(token);
		input.erase(0, pos + 1);
		input = trim(input, delimiter);
	}
	Output.push_back(input);

	return Output;
}
