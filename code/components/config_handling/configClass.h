#ifndef CONFIGCLASS_H
#define CONFIGCLASS_H

#include <string>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <cJSON.h>

#include "cfgDataStruct.h"


/* Function calls
 *
 * 1. Load Config From File (once after boot)
 *    readConfigFile()
 *      > parseJsonFromFile()
 *        > parseConfig()
 *          > migrateConfiguration()
 *        > serializeConfig()
 *        > writeConfigFile()
 *
 * 2. REST API Set
 *    setConfigRequest()
 *      > parseConfig()
 *      > serializeConfig()
 *      > writeConfigFile()
 *      > REST API Response
 *
 * 3. REST API Get
 *    getConfigRequest()
 *      > serializeConfig()
 *      > REST API Response
 *
 * 4. Unity Tests
 *    parseJsonFromFile(..., true)
 *      > parseConfig(..., true)
 *      > serializeConfig(true)
 *      > no writeConfigFile()
 */

class ConfigClass
{
  private:
    static ConfigClass cfgClass; // Config class init here instead of global variable + extern declaration
    CfgData cfgDataTemp;         // Keeps last parameter modifications, but not in yet used by process (gets promoted to active config by
                                 // reinitConfig())
    CfgData cfgData;             // Keep parameter configuration (in use by process)

    SemaphoreHandle_t cfgMutex = nullptr;
    cJSON *cJsonObject = NULL;
    uint8_t *cJsonObjectBuffer = NULL;
    char *jsonBuffer = NULL;
    char *httpBuffer = NULL;

    bool parseJsonFromFile(const char *jsonStr, bool unityTest = false);

    // Parse JSON to internal struct
    esp_err_t parseConfig(bool init = false, bool unityTest = false);

    // Serialize internal struct to JSON string
    esp_err_t serializeConfig(bool unityTest = false);

    esp_err_t writeConfigFile();

    bool loadDataFromNVS(std::string key, std::string &value);
    bool saveDataToNVS(std::string key, std::string value);

    void validatePath(std::string &path, bool withFile = false);
    void validateStructure(std::string &structureName);

  public:
    ConfigClass();
    ~ConfigClass();

    void clearCfgData(void);
    void clearCfgDataTemp(void);

    void readConfigFile(bool unityTest = false, std::string unityTestData = "{}");
    void reinitConfig(void) { cfgData = cfgDataTemp; };
    bool persistConfig(void);

    static ConfigClass *getInstance(void) { return &cfgClass; }
    const CfgData *get(void) const { return &cfgData; };

    esp_err_t getConfigRequest(httpd_req_t *req);
    esp_err_t setConfigRequest(httpd_req_t *req, bool triggerReload = false);

    // Only for migration and internal parameter modification purpose
    void initCfgTmp(void)
    {
        clearCfgDataTemp();
        cfgDataTemp = {};
    };
    CfgData *cfgTmp(void) { return &cfgDataTemp; };
    bool saveMigDataToNVS(std::string key, std::string value) { return saveDataToNVS(key, value); };

    // Only for testing purpose --> unity test
    CfgData *get(void) { return &cfgData; };
    char *getJsonBuffer(void) { return jsonBuffer; };
};

void registerConfigFileUri(httpd_handle_t server);


//-------------------------------------------------------------------------------------
// Mutex Guard
//-------------------------------------------------------------------------------------
class CfgMutexGuard
{
    SemaphoreHandle_t mMutex;
    bool mAcquired;

  public:
    explicit CfgMutexGuard(SemaphoreHandle_t mutex, TickType_t timeout = portMAX_DELAY) : mMutex(mutex), mAcquired(false)
    {
        if (mMutex && xSemaphoreTake(mMutex, timeout) == pdTRUE) {
            mAcquired = true;
        }
    }

    bool isAcquired() const { return mAcquired; }

    ~CfgMutexGuard()
    {
        if (mAcquired) {
            xSemaphoreGive(mMutex);
        }
    }
};

#endif // CONFIGCLASS_H
