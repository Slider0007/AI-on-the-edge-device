#include "configMigration.h"
#include "../../include/defines.h"

#include <fstream>
#include <algorithm>

#include "configClass.h"
#include "helper.h"
#include "ClassLogFile.h"
#include "ClassControlCamera.h"


static const char *TAG = "CFGMIGRATION";


void migrateConfiguration(void)
{
    // ********************************************************************************
    // Legacy: Support config.ini / wlan.ini migration
    // Firmware version: v15.0 - v16.x, Config version: 0 - 2
    // ********************************************************************************
    migrateConfigIni();


    // ********************************************************************************
    // Config based on JSON notation
    // Firmware version: v17.0 and newer, Config version: 3 and newer
    // ********************************************************************************
    bool migrated = false;
    ConfigClass::getInstance()->cfgTmp()->sectionConfig.desiredConfigVersion = 3; // Set to new version whenever to data structure was modified

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
                ConfigClass::getInstance()->cfgTmp()->sectionConfig.version = configFileVersion + 1;
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "cfgData: Migrate v" + std::to_string(configFileVersion) +
                                                        " > v" + std::to_string(configFileVersion+1));
                migrated = true;

                // Update parameter
                // ---------------------

            }
    }

    // Migration detected
    if (migrated) {
        deleteFile(CONFIG_PERSISTENCE_FILE_BACKUP);

        if (!renameFile(CONFIG_PERSISTENCE_FILE, CONFIG_PERSISTENCE_FILE_BACKUP)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create backup of config.json file");
            return;
        }

        ConfigClass::getInstance()->persistConfig();
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Config file migrated. Saved backup to " + std::string(CONFIG_PERSISTENCE_FILE_BACKUP));
    }
}


void migrateConfigIni(void)
{
    // No migration from config.ini needed
    if (!fileExists(CONFIG_FILE_LEGACY)) {
        return;
    }

    const std::string sectionConfigFile= "[ConfigFile]";
    bool configSectionFound = false;
    int actualConfigFileVersion = 0;

    bool migrated = false;
    bool migratedToJson = false;
    std::string section = "";
    static int sequenceID = 0;

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
                actualConfigFileVersion = std::stoi(splitted[1]);
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
                if (configFileVersion < 2) {
                    replaceString(section, ";", "", false); // Remove possible semicolon (just for the string comparison)
                }
            }

           //*************************************************************************************************
            // Migrate from version 2 to version 3
            // Migrate config.ini to internal struct which gets persistant to config.json
            //*************************************************************************************************
            if (configFileVersion == 2) {
                std::vector<std::string> splitted;

                if (section == sectionConfigFile) {
                    // Update config version
                    // ---------------------
                    ConfigClass::getInstance()->cfgTmp()->sectionConfig.version = configFileVersion + 1;
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config.ini: Migrate v" + std::to_string(configFileVersion) +
                                " > v" + std::to_string(configFileVersion+1) + " => Config will be handled in firmware + config.json");

                    // Remove unused files
                    deleteFile("/sdcard/bootloader.bin");
                    deleteFile("/sdcard/partitions.bin");
                    deleteFile("/sdcard/readme.md");
                    deleteFile("/sdcard/config/config.bak");
                    deleteFile("/sdcard/config/prevalue.ini");
                    deleteFile("/sdcard/config/align.txt");
                    deleteFile("/sdcard/config/ref0_org.jpg");
                    deleteFile("/sdcard/config/ref1_org.jpg");
                    deleteAllFilesInDirectory("/sdcard/img_tmp");

                    // Rename marker files to new naming scheme
                    renameFile("/sdcard/config/ref0.jpg", "/sdcard/config/marker1.jpg");
                    renameFile("/sdcard/config/ref1.jpg", "/sdcard/config/marker2.jpg");

                    // Create model subfolder in /sdcard/config and move all models to subfolder
                    makeDir("/sdcard/config/models");
                    moveAllFilesWithFiletype("/sdcard/config", "/sdcard/config/models", "tfl");
                    moveAllFilesWithFiletype("/sdcard/config", "/sdcard/config/models", "tflite");

                    // Create backup subfolder to archive legacy config files (config.ini, wlan.ini)
                    makeDir("/sdcard/config/backup");

                    // Migrate wlan.ini --> handled in config.json
                    migrateWlanIni();

                    migratedToJson = true;
                    i += 2; // Skip lines
                }
                else if (section == "[TakeImage]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) ==  "RAWIMAGESLOCATION" || toUpper(splitted[0]) == ";RAWIMAGESLOCATION") && (splitted[0].size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.debug.rawImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.debug.saveRawImages = true;
                    }
                    else if ((toUpper(splitted[0]) == "RAWIMAGESRETENTION" || toUpper(splitted[0]) == ";RAWIMAGESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.debug.rawImagesRetention = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "FLASHTIME") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.flashlight.flashTime = (int)(stof(splitted[1])*1000); // Flashtime in ms
                    }
                    else if ((toUpper(splitted[0]) == "FLASHINTENSITY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.flashlight.flashIntensity = std::max(0, std::min(std::stoi(splitted[1]), 100));
                    }
                    else if ((toUpper(splitted[0]) == "CAMERAFREQUENCY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.cameraFrequency = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "IMAGEQUALITY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.imageQuality = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "IMAGESIZE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.imageSize = splitted[1].c_str();
                    }
                    else if ((toUpper(splitted[0]) == "BRIGHTNESS") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.brightness = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "CONTRAST") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.contrast = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SATURATION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.saturation = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SHARPNESS") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.sharpness = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "EXPOSURECONTROLMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.exposureControlMode = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "AUTOEXPOSURELEVEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.autoExposureLevel = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "MANUALEXPOSUREVALUE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.manualExposureValue = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "GAINCONTROLMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.gainControlMode = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "MANUALGAINVALUE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.manualGainValue = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SPECIALEFFECT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.specialEffect = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "MIRRORIMAGE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.mirrorImage = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "FLIPIMAGE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.flipImage = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "ZOOMMODE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomMode = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "ZOOMOFFSETX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomOffsetX = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "ZOOMOFFSETY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomOffsetY = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "DEMO") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionOperationMode.useDemoImages = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.debug.saveAllFiles = (toUpper(splitted[1]) == "TRUE");
                    }
                }
                else if (section == "[Alignment]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    static int idx = 0;
                    if ((toUpper(splitted[0]) == "ALIGNMENTALGO") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "HIGHACCURACY")
                            ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_HIGH_ACCURACY;
                        else if (toUpper(splitted[1]) == "FAST")
                            ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_FAST;
                        else if (toUpper(splitted[1]) == "ROTATION")
                            ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_ROTATION_ONLY;
                        else if (toUpper(splitted[1]) == "OFF")
                            ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_OFF;
                        else
                            ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.alignmentAlgo = ALIGNALGO_DEFAULT;
                    }
                    else if ((toUpper(splitted[0]) == "SEARCHFIELDX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.searchField.x = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SEARCHFIELDY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.searchField.y = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "INITIALROTATE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.imageRotation = std::stof(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "FLIPIMAGESIZE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.flipImageSize = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "SAVEDEBUGINFO") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.debug.saveDebugInfo = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.debug.saveAllFiles = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if (splitted.size() == 3 && idx < 2) {
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.marker[idx].x = std::stoi(splitted[1]);
                        ConfigClass::getInstance()->cfgTmp()->sectionImageAlignment.marker[idx].y = std::stoi(splitted[2]);
                        idx++;
                    }
                }
                else if (section == "[Digits]" || section == ";[Digits]") {
                    ConfigClass::getInstance()->cfgTmp()->sectionDigit.enabled = !section.starts_with(";");

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "MODEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionDigit.model = splitted[1].substr(8, std::string::npos);
                    }
                    else if ((toUpper(splitted[0]) == "CNNGOODTHRESHOLD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionDigit.cnnGoodThreshold = std::stof(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "ROIIMAGESLOCATION"  || (toUpper(splitted[0]) == ";ROIIMAGESLOCATION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionDigit.debug.roiImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionDigit.debug.saveRoiImages = true;
                    }
                    else if ((toUpper(splitted[0]) == "ROIIMAGESRETENTION"  || (toUpper(splitted[0]) == ";ROIIMAGESRETENTION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionDigit.debug.roiImagesRetention = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1)) {
                            ConfigClass::getInstance()->cfgTmp()->sectionDigit.debug.saveAllFiles = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if (splitted.size() >= 5) {
                        RoiElement roiEl;
                        roiEl.x = std::stoi(splitted[1]);
                        roiEl.y = std::stoi(splitted[2]);
                        roiEl.dx = std::stoi(splitted[3]);
                        roiEl.dy = std::stoi(splitted[4]);

                        SequenceList sequenceListEl;
                        sequenceListEl.sequenceId = sequenceID;
                        sequenceListEl.sequenceName = toLower(splitted[0].substr(0, splitted[0].find_first_of(".")));

                        bool existing = false;
                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionDigit.sequence) {
                            if (seqEl.sequenceName == sequenceListEl.sequenceName) {
                                seqEl.roi.push_back(roiEl);
                                sequenceID = seqEl.sequenceId + 1;
                                existing = true;
                                break;
                            }
                        }

                        if (!existing) {
                            ConfigClass::getInstance()->cfgTmp()->sectionNumberSequences.sequence.push_back(sequenceListEl);
                            RoiPerSequence sequenceRoiEl;
                            sequenceRoiEl.sequenceId = sequenceListEl.sequenceId;
                            sequenceRoiEl.sequenceName = sequenceListEl.sequenceName;
                            ConfigClass::getInstance()->cfgTmp()->sectionDigit.sequence.push_back(sequenceRoiEl);
                            ConfigClass::getInstance()->cfgTmp()->sectionAnalog.sequence.push_back(sequenceRoiEl);
                            PostProcessingPerSequence sequencePostProcEl;
                            sequencePostProcEl.sequenceId = sequenceListEl.sequenceId;
                            sequencePostProcEl.sequenceName = sequenceListEl.sequenceName;
                            ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.sequence.push_back(sequencePostProcEl);
                            InfluxDBPerSequence sequenceInfluxDBEl;
                            sequenceInfluxDBEl.sequenceId = sequenceListEl.sequenceId;
                            sequenceInfluxDBEl.sequenceName = sequenceListEl.sequenceName;
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.sequence.push_back(sequenceInfluxDBEl);
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.sequence.push_back(sequenceInfluxDBEl);

                            ConfigClass::getInstance()->cfgTmp()->sectionDigit.sequence.back().roi.push_back(roiEl);

                            sequenceID++;
                        }
                    }
                }
                else if (section == "[Analog]" || section == ";[Analog]") {
                    ConfigClass::getInstance()->cfgTmp()->sectionAnalog.enabled = !section.starts_with(";");

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "MODEL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionAnalog.model = splitted[1].substr(8, std::string::npos);
                    }
                    else if ((toUpper(splitted[0]) == "ROIIMAGESLOCATION"  || (toUpper(splitted[0]) == ";ROIIMAGESLOCATION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionAnalog.debug.roiImagesLocation = splitted[1];
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionAnalog.debug.saveRoiImages = true;
                    }
                    else if ((toUpper(splitted[0]) == "ROIIMAGESRETENTION" || (toUpper(splitted[0]) == ";ROIIMAGESRETENTION")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionAnalog.debug.roiImagesRetention = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1))
                    {
                        ConfigClass::getInstance()->cfgTmp()->sectionAnalog.debug.saveAllFiles = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if (splitted.size() >= 5) {
                        RoiElement roiEl;
                        roiEl.x = std::stoi(splitted[1]);
                        roiEl.y = std::stoi(splitted[2]);
                        roiEl.dx = std::stoi(splitted[3]);
                        roiEl.dy = std::stoi(splitted[4]);
                        roiEl.ccw = (toUpper(splitted[1]) == "TRUE");

                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionAnalog.sequence) {
                            if (seqEl.sequenceName == toLower(splitted[0].substr(0, splitted[0].find_first_of(".")))) {
                                seqEl.roi.push_back(roiEl);
                                break;
                            }
                        }
                    }
                }
                else if (section == "[PostProcessing]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    // Global parameter used for all sequences and now they are parameter per sequence
                    if (toUpper(splitted[0]) == "FALLBACKVALUEUSE" && (splitted.size() > 1)) {
                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.sequence) {
                            seqEl.useFallbackValue = (toUpper(splitted[1]) == "TRUE");
                        }
                    }
                    else if (toUpper(splitted[0]) == "FALLBACKVALUEAGESTARTUP" && (splitted.size() > 1)) {
                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.sequence) {
                            seqEl.fallbackValueAgeStartup = std::stoi(splitted[1]);
                        }
                    }
                    else if (toUpper(splitted[0]) == "CHECKDIGITINCREASECONSISTENCY" && (splitted.size() > 1)) {
                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.sequence) {
                                seqEl.checkDigitIncreaseConsistency = (toUpper(splitted[1]) == "TRUE");
                        }
                    }
                    // Parameter per sequence
                    else if (splitted[0].find_first_of(".") != std::string::npos) {
                        std::string parameter = toUpper(splitted[0].substr(splitted[0].find_first_of(".")+1));

                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.sequence) {
                            if (seqEl.sequenceName == toLower(splitted[0].substr(0, splitted[0].find_first_of(".")))) {
                                if (parameter == "ALLOWNEGATIVERATES" && (splitted.size() > 1)) {
                                    seqEl.allowNegativeRate = (toUpper(splitted[1]) == "TRUE");
                                }
                                else if (parameter == "DECIMALSHIFT" && (splitted.size() > 1)) {
                                    seqEl.decimalShift = std::stoi(splitted[1]);
                                }
                                else if (parameter == "ANALOGDIGITALTRANSITIONSTART" && (splitted.size() > 1)) {
                                    seqEl.analogDigitSyncValue = std::stof(splitted[1]);
                                }
                                else if (parameter == "MAXRATETYPE" && (splitted.size() > 1)) {
                                    if (toUpper(splitted[1]) == "RATEPERMIN") {
                                        seqEl.maxRateCheckType = RATE_PER_MIN;
                                    }
                                    else if (toUpper(splitted[1]) == "RATEPERINTERVAL") {
                                        seqEl.maxRateCheckType = RATE_PER_INTERVAL;
                                    }
                                    else if (toUpper(splitted[1]) == "RATEOFF") {
                                        seqEl.maxRateCheckType = RATE_CHECK_OFF;
                                    }
                                    else {
                                        seqEl.maxRateCheckType = RATE_PER_MIN;
                                    }
                                }
                                else if (parameter == "MAXRATEVALUE" && (splitted.size() > 1)) {
                                    seqEl.maxRate = std::stof(splitted[1]);
                                }
                                else if (parameter == "EXTENDEDRESOLUTION" && (splitted.size() > 1)) {
                                    seqEl.extendedResolution = (toUpper(splitted[1]) == "TRUE");
                                }
                                else if (parameter == "IGNORELEADINGNAN" && (splitted.size() > 1)) {
                                    seqEl.ignoreLeadingNaN = (toUpper(splitted[1]) == "TRUE");
                                }
                                break;
                            }
                        }
                    }
                    else if ((toUpper(splitted[0]) == "SAVEDEBUGINFO") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionPostProcessing.debug.saveDebugInfo = (toUpper(splitted[1]) == "TRUE");
                    }
                }
                else if (section == "[MQTT]" || section == ";[MQTT]") {
                    ConfigClass::getInstance()->cfgTmp()->sectionMqtt.enabled = !section.starts_with(";");

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.uri =
                            (splitted[1] == "mqtt://IP-ADDRESS:1883" || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "MAINTOPIC") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.mainTopic = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "CLIENTID") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.clientID = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "USER" || toUpper(splitted[0]) == ";USER") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.username = (splitted[1] == "undefined" ? "" : splitted[1]);
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.authMode = AUTH_BASIC;
                    }
                    else if ((toUpper(splitted[0]) == "PASSWORD" || toUpper(splitted[0]) == ";PASSWORD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.password = (splitted[1] == "undefined" ? "" : splitted[1]);
                        ConfigClass::getInstance()->saveMigDataToNVS("mqtt_pw", ConfigClass::getInstance()->cfgTmp()->sectionMqtt.password);

                        if (!splitted[0].starts_with(";")) {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.authMode = AUTH_BASIC;
                        }
                    }
                    else if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.authMode = AUTH_TLS;
                    }
                    else if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.tls.caCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.tls.clientCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.tls.clientKey =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "RETAINPROCESSDATA") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.retainProcessData = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "PROCESSDATANOTATION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.processDataNotation = std::stoi(splitted[1]);
                    }
                    if ((toUpper(splitted[0]) == "HOMEASSISTANTDISCOVERY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.discoveryEnabled = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "HADISCOVERYPREFIX") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.discoveryPrefix = splitted[1];
                    }
                    else if ((toUpper(splitted[0]) == "HASTATUSTOPIC") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.statusTopic = splitted[1];
                    }
                    else if ((toUpper(splitted[0]) == "HARETAINDISCOVERY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.retainDiscovery = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "HAMETERTYPE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "WATER_M3") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = WATER_M3;
                        }
                        else if (toUpper(splitted[1]) == "WATER_L") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = WATER_L;
                        }
                        else if (toUpper(splitted[1]) == "WATER_FT3") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = WATER_FT3;
                        }
                        else if (toUpper(splitted[1]) == "WATER_GAL") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = WATER_GAL;
                        }
                        else if (toUpper(splitted[1]) == "GAS_M3") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = GAS_M3;
                        }
                        else if (toUpper(splitted[1]) == "GAS_FT3") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = GAS_FT3;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_WH") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = ENERGY_WH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_KWH") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = ENERGY_KWH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_MWH") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = ENERGY_MWH;
                        }
                        else if (toUpper(splitted[1]) == "ENERGY_GJ") {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = ENERGY_GJ;
                        }
                        else {
                            ConfigClass::getInstance()->cfgTmp()->sectionMqtt.homeAssistant.meterType = TYPE_NONE;
                        }
                    }
                }
                else if (section == "[InfluxDB]" || section == ";[InfluxDB]") {
                    ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.enabled = !section.starts_with(";");

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.uri =
                            (splitted[1] == "http://IP-ADDRESS:PORT" || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if (((toUpper(splitted[0]) == "DATABASE")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.database = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "USER" || toUpper(splitted[0]) == ";USER") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.username = (splitted[1] == "undefined" ? "" : splitted[1]);
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.authMode = AUTH_BASIC;
                    }
                    else if ((toUpper(splitted[0]) == "PASSWORD" || toUpper(splitted[0]) == ";PASSWORD") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.password = (splitted[1] == "undefined" ? "" : splitted[1]);
                        ConfigClass::getInstance()->saveMigDataToNVS("influxdbv1_pw", ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.password);

                        if (!splitted[0].starts_with(";")) {
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.authMode = AUTH_BASIC;
                        }
                    }
                    else if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE") {
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.authMode = AUTH_TLS;
                        }
                    }
                    else if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.tls.caCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.tls.clientCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.tls.clientKey =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if (splitted[0].find_first_of(".") != std::string::npos) {
                        std::string parameter = toUpper(splitted[0].substr(splitted[0].find_first_of(".")+1));

                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv1.sequence) {
                            if (seqEl.sequenceName == toLower(splitted[0].substr(0, splitted[0].find_first_of(".")))) {
                                if (parameter == "MEASUREMENT" && (splitted.size() > 1)) {
                                    seqEl.measurementName = (splitted[1] == "undefined" ? "" : splitted[1]);
                                }
                                else if (parameter == "FIELD" && (splitted.size() > 1)) {
                                    seqEl.fieldKey1 = (splitted[1] == "undefined" ? "" : splitted[1]);
                                }
                                break;
                            }
                        }
                    }
                }
                else if (section == "[InfluxDBv2]" || section == ";[InfluxDBv2]") {
                    ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.enabled = !section.starts_with(";");

                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.uri =
                            (splitted[1] == "http://IP-ADDRESS:PORT" || splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if (((toUpper(splitted[0]) == "BUCKET")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.bucket = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "ORG" || (toUpper(splitted[0]) == ";ORG")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.organization = (splitted[1] == "undefined" ? "" : splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "TOKEN" || (toUpper(splitted[0]) == ";TOKEN")) && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.token = (splitted[1] == "undefined" ? "" : splitted[1]);
                        ConfigClass::getInstance()->saveMigDataToNVS("influxdbv2_pw", ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.token);
                    }
                    else if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE") {
                            ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.authMode = AUTH_TLS;
                        }
                    }
                    else if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.tls.caCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.tls.clientCert =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.tls.clientKey =
                            (splitted[1] == "undefined" ? "" : splitted[1].substr(splitted[1].find_last_of("/")+1));
                    }
                    else if (splitted[0].find_first_of(".") != std::string::npos) {
                        std::string parameter = toUpper(splitted[0].substr(splitted[0].find_first_of(".")+1));

                        for (auto &seqEl : ConfigClass::getInstance()->cfgTmp()->sectionInfluxDBv2.sequence) {
                            if (seqEl.sequenceName == toLower(splitted[0].substr(0, splitted[0].find_first_of(".")))) {
                                if (parameter == "MEASUREMENT" && (splitted.size() > 1)) {
                                    seqEl.measurementName = (splitted[1] == "undefined" ? "" : splitted[1]);
                                }
                                else if (parameter == "FIELD" && (splitted.size() > 1)) {
                                    seqEl.fieldKey1 = (splitted[1] == "undefined" ? "" : splitted[1]);
                                }
                                break;
                            }
                        }
                    }
                }
                else if (section == "[GPIO]" || section == ";[GPIO]" ) {
                    // GPIO config migration is not implemented (depends on hardware) --> Needs to be configured from scratch
                }
                else if (section == "[AutoTimer]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "AUTOSTART") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "TRUE")
                            ConfigClass::getInstance()->cfgTmp()->sectionOperationMode.opMode = OPMODE_AUTO;
                        else
                            ConfigClass::getInstance()->cfgTmp()->sectionOperationMode.opMode = OPMODE_MANUAL;
                    }
                    else if ((toUpper(splitted[0]) == "INTERVAL") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionOperationMode.automaticProcessInterval = std::stof(splitted[1]);
                    }
                }
                else if (section == "[DataLogging]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "DATALOGACTIVE") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionLog.data.enabled = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if ((toUpper(splitted[0]) == "DATAFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionLog.data.dataFilesRetention = std::stoi(splitted[1]);
                    }
                }
                else if (section == "[Debug]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "LOGLEVEL") && (splitted.size() > 1)) {
                        if ((toUpper(splitted[1]) == "TRUE") || (toUpper(splitted[1]) == "2")) {
                            ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logLevel = ESP_LOG_WARN;
                        }
                        else if ((toUpper(splitted[1]) == "FALSE") || (toUpper(splitted[1]) == "0") || (toUpper(splitted[1]) == "1")) {
                            ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logLevel = ESP_LOG_ERROR;
                        }
                        else if (toUpper(splitted[1]) == "3") {
                            ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logLevel = ESP_LOG_INFO;
                        }
                        else if (toUpper(splitted[1]) == "4") {
                            ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logLevel = ESP_LOG_DEBUG;
                        }
                        else {
                            ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logLevel = ESP_LOG_ERROR;
                        }
                    }
                    else if ((toUpper(splitted[0]) == "LOGFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.logFilesRetention = std::stoi(splitted[1]);
                    }
                    else if ((toUpper(splitted[0]) == "DEBUGFILESRETENTION") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionLog.debug.debugFilesRetention = std::stoi(splitted[1]);
                    }
                }
                else if (section == "[System]") {
                    splitted = splitString(configLines[i+1]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if ((toUpper(splitted[0]) == "TIMESERVER") || (toUpper(splitted[0]) == ";TIMESERVER")) {
                        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.time.ntp.timeServer = splitted[1];
                    }
                    else if (toUpper(splitted[0]) == "TIMEZONE") {
                        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.time.timeZone = splitted[1];
                    }
                    else if ((toUpper(splitted[0]) == "HOSTNAME") && (splitted.size() > 1)) {
                        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.hostname = splitted[1];
                    }
                    else if ((toUpper(splitted[0]) == "RSSITHRESHOLD") || (toUpper(splitted[0]) == ";RSSITHRESHOLD")) {
                        if (!splitted[0].starts_with(";"))
                            ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.wlanRoaming.enabled = true;

                        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.wlanRoaming.rssiThreshold = atoi(splitted[1].c_str());
                    }
                    else if ((toUpper(splitted[0]) == "CPUFREQUENCY")) {
                        ConfigClass::getInstance()->cfgTmp()->sectionSystem.cpuFrequency = atoi(splitted[1].c_str());
                    }
                    else if ((toUpper(splitted[0]) == "SETUPMODE") && (splitted.size() > 1)) {
                        if (toUpper(splitted[1]) == "FALSE") {
                            ConfigClass::getInstance()->cfgTmp()->sectionOperationMode.opMode = OPMODE_AUTO;
                        }
                    }
                }
                else if (section == "[WebUI]") {
                    splitted = splitString(configLines[i]);

                    if (splitted[0] == "") // Skip empty lines
                        continue;

                    if (toUpper(splitted[0]) == "OVERVIEWAUTOREFRESH") {
                        ConfigClass::getInstance()->cfgTmp()->sectionWebUi.AutoRefresh.overviewPage.enabled = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if (toUpper(splitted[0]) == "OVERVIEWAUTOREFRESHTIME") {
                        ConfigClass::getInstance()->cfgTmp()->sectionWebUi.AutoRefresh.overviewPage.refreshTime = atoi(splitted[1].c_str());
                    }
                    else if (toUpper(splitted[0]) == "DATAGRAPHAUTOREFRESH") {
                        ConfigClass::getInstance()->cfgTmp()->sectionWebUi.AutoRefresh.dataGraphPage.enabled = (toUpper(splitted[1]) == "TRUE");
                    }
                    else if (toUpper(splitted[0]) == "DATAGRAPHAUTOREFRESHTIME") {
                        ConfigClass::getInstance()->cfgTmp()->sectionWebUi.AutoRefresh.dataGraphPage.refreshTime = atoi(splitted[1].c_str());
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

    // At least one replacement happened
    if (migrated || migratedToJson) {
        if (migratedToJson) {
            ConfigClass::getInstance()->persistConfig();
            ConfigClass::getInstance()->initCfgTmp();
        }

        deleteFile(CONFIG_FILE_BACKUP_LEGACY);

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
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ssid = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "PASSWORD")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password = tmp;
                ConfigClass::getInstance()->saveMigDataToNVS("wlan_pw", ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password);
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "HOSTNAME")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.hostname = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "IP")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.ipAddress = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "GATEWAY")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.gatewayAddress = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "NETMASK")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.subnetMask = tmp;
            }
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "DNS")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.dnsServer = tmp;
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

    if (!ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.ipAddress.empty() &&
        !ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.subnetMask.empty() &&
        !ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.gatewayAddress.empty())
    {
        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ipv4.networkConfig = NETWORK_CONFIG_STATIC;
    }

    deleteFile(CONFIG_WIFI_FILE_BACKUP_LEGACY);

    if (!renameFile(CONFIG_WIFI_FILE_LEGACY, CONFIG_WIFI_FILE_BACKUP_LEGACY)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create backup of wlan.ini file");
    }
}


std::vector<std::string> splitString(std::string input, std::string delimiter)
{
	std::vector<std::string> output;
    // Line contains a password, use the equal sign as the only delimiter and only split on first occurrence
	if ((input.find("password") != std::string::npos) || (input.find("Token") != std::string::npos)) {
		size_t pos = input.find("=");
		output.push_back(trim(input.substr(0, pos), ""));
		output.push_back(trim(input.substr(pos +1, std::string::npos), ""));
	}
	else { // Legacy Mode
		input = trim(input, delimiter);	// trim to avoid delimiter deletion at the of string (z.B. == in string)
		size_t pos = findDelimiterPos(input, delimiter);
		std::string token;
		while (pos != std::string::npos) {
			token = input.substr(0, pos);
			token = trim(token, delimiter);
			output.push_back(token);
			input.erase(0, pos + 1);
			input = trim(input, delimiter);
			pos = findDelimiterPos(input, delimiter);
		}
		output.push_back(input);
	}

	return output;
}


std::vector<std::string> splitStringWLAN(std::string input, std::string _delimiter)
{
	std::vector<std::string> output;
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
		output.push_back(token);
		input.erase(0, pos + 1);
		input = trim(input, delimiter);
	}
	output.push_back(input);

	return output;
}


bool replaceString(std::string &s, std::string const &toReplace, std::string const &replaceWith)
{
    return replaceString(s, toReplace, replaceWith, true);
}


bool replaceString(std::string &s, std::string const &toReplace, std::string const &replaceWith, bool logIt)
{
    std::size_t pos = s.find(toReplace);

    if (pos == std::string::npos) { // Not found
        return false;
    }

    std::string old = s;
    s.replace(pos, toReplace.length(), replaceWith);
    if (logIt) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config.ini: Migrate '" + old + "' > '" + s + "'");
    }
    return true;
}
