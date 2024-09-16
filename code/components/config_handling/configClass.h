#ifndef CFGCLASS_H
#define CFGCLASS_H

#include <string>
#include <vector>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cJSON.h>

#include "cfgDataStruct.h"
#include "CFindTemplate.h"


class ConfigClass
{
  private:
    // Config class init here instead of global variable + extern declaration
    static ConfigClass cfgClass;
    CfgData cfgDataInternal;
    CfgData cfgData;

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

    void readConfigFile(bool unityTest = false, std::string unityTestData = "{}");
    void reinitConfig(void) { cfgData = cfgDataInternal; };
    void persistConfig() { serializeConfig(); writeConfigFile(); };

    static ConfigClass *getInstance() { return &cfgClass; }
    const CfgData *get() const { return &cfgData; };
    CfgData *set() { return &cfgDataInternal; };

    esp_err_t getConfigRequest(httpd_req_t *req);
    esp_err_t setConfigRequest(httpd_req_t *req);

    /* Use only for testing purpose --> unity test */
    CfgData *get() { return &cfgData; };
    char *getJsonBuffer(void) { return jsonBuffer; };
    /***********************************************/
};

void registerConfigFileUri(httpd_handle_t server);

#endif // CONFIGCLASS_H