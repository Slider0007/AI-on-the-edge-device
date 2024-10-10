#ifndef CONFIGCLASS_H
#define CONFIGCLASS_H

#include <string>
#include <vector>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cJSON.h>

#include "cfgDataStruct.h"
#include "CFindTemplate.h"


/* Function calls
* 1. Restore Config : readConfigFile()       > parseConfig > serializeConfig > writeConfigFile
* 2. REST API Set   : setConfigRequest()     > parseConfig > serializeConfig > REST API Response
* 3. REST API Get   : getConfigRequest()                   > serializeConfig > REST API Response
* 4. Cfg Migration  : migrateConfiguration()               > serializeConfig > writeConfigFile
*/

class ConfigClass
{
  private:
    static ConfigClass cfgClass; // Config class init here instead of global variable + extern declaration
    CfgData cfgDataTemp; // Keeps last parameter modifications, but not in use by process (gets promoted to active config by reinitConfig())
    CfgData cfgData; // Keep active parameter configuration in use by process

    cJSON *cJsonObject = NULL;
    uint8_t *cJsonObjectBuffer = NULL;
    char *jsonBuffer = NULL;
    char *httpBuffer = NULL;

    esp_err_t parseConfig(httpd_req_t *req = NULL, bool init = false, bool unityTest = false);
    esp_err_t serializeConfig(bool unityTest = false);
    esp_err_t writeConfigFile(void);

    bool loadDataFromNVS(std::string key, std::string &value);
    bool saveDataToNVS(std::string key, std::string value);

    void validatePath(std::string& path, bool withFile = false);
    void validateStructure(std::string& structureName);

  public:
    ConfigClass();
    ~ConfigClass();

    void clearCfgData(void);
    void clearCfgDataTemp(void);

    void readConfigFile(bool unityTest = false, std::string unityTestData = "{}");
    void reinitConfig(void) { cfgData = cfgDataTemp; };
    void persistConfig(void) { serializeConfig(); writeConfigFile(); };

    static ConfigClass *getInstance(void) { return &cfgClass; }
    const CfgData *get(void) const { return &cfgData; };

    esp_err_t getConfigRequest(httpd_req_t *req);
    esp_err_t setConfigRequest(httpd_req_t *req);

    // Only for migration and internal parameter modification purpose
    void initCfgTmp(void) { cfgDataTemp = {}; };
    CfgData *cfgTmp(void) { return &cfgDataTemp; };
    bool saveMigDataToNVS(std::string key, std::string value) { return saveDataToNVS(key, value); };

    // Only for testing purpose --> unity test
    CfgData *get(void) { return &cfgData; };
    char *getJsonBuffer(void) { return jsonBuffer; };
};

bool loadDataFromNVS(std::string key, std::string &value);
bool saveDataToNVS(std::string key, std::string value);

void registerConfigFileUri(httpd_handle_t server);

#endif // CONFIGCLASS_H