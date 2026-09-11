#include "configMigration.h"
#include "../../include/defines.h"

#include "configClass.h"
#include "helper.h"
#include "ClassLogFile.h"


static const char *TAG = "CFGMIG";


// ********************************************************************************
// Configuration migration
// Firmware version: >= v17.0 (config version: >= 3)
// ********************************************************************************
void migrateConfiguration(cJSON *cJsonObject)
{
    int migratedVersion = 0;

    // Version validation
    if (ConfigClass::getInstance()->cfgTmp()->sectionConfig.version < 3) {
        // Set to latest version and reset last modified
        migratedVersion = ConfigClass::getInstance()->cfgTmp()->sectionConfig.version;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.version = ConfigClass::getInstance()->get()->sectionConfig.version;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.lastModified = "";

        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                            "Migration for v" + std::to_string(migratedVersion) + " is not supported. Check configuration manually.");

        return;
    }

    //*************************************************************************************************
    // Migrate from version 5 to version 6
    // Date: August 2025
    // Description: Implement function to set time manually (#275)
    //*************************************************************************************************
    if (ConfigClass::getInstance()->cfgTmp()->sectionConfig.version == 5) {
        // Update config version
        // ---------------------
        migratedVersion = ConfigClass::getInstance()->cfgTmp()->sectionConfig.version;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.version += 1;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.lastModified = ""; // Reset last modified
        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                            "cfgData: Migrate v" + std::to_string(migratedVersion) + " > v" +
                                std::to_string(ConfigClass::getInstance()->cfgTmp()->sectionConfig.version));

        // Update parameter
        // ---------------------
        const cJSON *objEl = cJSON_GetObjectItem(
            cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "time"), "ntp"), "processstartinterlock");
        if (cJSON_IsBool(objEl)) {
            ConfigClass::getInstance()->cfgTmp()->sectionNetwork.time.processStartInterlock = objEl->valueint;
        }
    }

    //*************************************************************************************************
    // Migrate from version 4 to version 5
    // Date: July 2025
    // Description: Support Waveshare ESP32S3-ETH board with ethernet interface (#274)
    //*************************************************************************************************
    if (ConfigClass::getInstance()->cfgTmp()->sectionConfig.version == 4) {
        // Update config version
        // ---------------------
        migratedVersion = ConfigClass::getInstance()->cfgTmp()->sectionConfig.version;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.version += 1;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.lastModified = ""; // Reset last modified
        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                            "cfgData: Migrate v" + std::to_string(migratedVersion) + " > v" +
                                std::to_string(ConfigClass::getInstance()->cfgTmp()->sectionConfig.version));

        // Update parameter
        // ---------------------
        const cJSON *objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "network"), "wlan"), "hostname");
        if (cJSON_IsString(objEl)) {
            ConfigClass::getInstance()->cfgTmp()->sectionNetwork.hostname = objEl->valuestring;
        }
    }

    //*************************************************************************************************
    // Migrate from version 3 to version 4
    // Date: November 2024
    // Description: Support camera model OV5640 + Enhanced digital zoom (#189)
    //*************************************************************************************************
    if (ConfigClass::getInstance()->cfgTmp()->sectionConfig.version == 3) {
        // Update config version
        // ---------------------
        migratedVersion = ConfigClass::getInstance()->cfgTmp()->sectionConfig.version;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.version += 1;
        ConfigClass::getInstance()->cfgTmp()->sectionConfig.lastModified = ""; // Reset last modified
        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                            "cfgData: Migrate v" + std::to_string(migratedVersion) + " > v" +
                                std::to_string(ConfigClass::getInstance()->cfgTmp()->sectionConfig.version));

        // Update parameter
        // ---------------------
        const cJSON *objEl = cJSON_GetObjectItem(cJSON_GetObjectItem(cJSON_GetObjectItem(cJsonObject, "takeimage"), "camera"), "zoommode");
        if (cJSON_IsNumber(objEl)) {
            if (objEl->valueint == 0) {                                                          // Disabled
                ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomFactor = 1000; // 1.0x
            }
            else if (objEl->valueint == 1) {                                                     // Crop (1600 x 1200 -> 640 x 480)
                ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomFactor = 2500; // 2.5x
            }
            else if (objEl->valueint == 2) {                                                     // Scale & Crop (800 x 600 -> 640 x 480)
                ConfigClass::getInstance()->cfgTmp()->sectionTakeImage.camera.zoomFactor = 1250; // 1.25x
            }
        }
    }

    // Backup config file (run only if config migration performed)
    if (migratedVersion >= 3) {
        std::string ConfigBackupFile = std::string(CONFIG_PERSISTENCE_FILE_BACKUP) + "_v" + std::to_string(migratedVersion);
        deleteFile(ConfigBackupFile);

        if (!renameFile(CONFIG_PERSISTENCE_FILE, ConfigBackupFile)) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Config migrated. Failed to create backup file: " + std::string(ConfigBackupFile));
            return;
        }

        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Config migrated. Backup file: " + std::string(ConfigBackupFile));
    }
}
