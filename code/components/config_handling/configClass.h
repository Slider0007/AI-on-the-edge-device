#ifndef CONFIGCLASS_H
#define CONFIGCLASS_H

#include <string>
#include <algorithm>

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
    void parseSectionConfig(bool init);
    void parseSectionOperationMode();
    void parseSectionTakeImage();
    void parseSectionImageAlignment();
    void parseSectionSequences(bool init);
    template <typename SectionType> void parseSectionRoi(const char *sectionKey, SectionType &section, const char *roiSuffix);
    void parseSectionPostProcessing();
    void parseSectionMqtt(bool unityTest);
    void parseSectionInfluxDBv1(bool unityTest);
    void parseSectionInfluxDBv2(bool unityTest);
    void parseSectionWebhook(bool unityTest);
    void parseSectionGpio(bool init);
    void parseSectionLogging();
    void parseSectionNetwork(bool init, bool unityTest);
    void parseSectionSystem();
    void parseSectionWebUi(bool unityTest);

    void parseSecretParameter(cJSON *root, std::initializer_list<const char *> path, std::string &out, const char *nvsKey, bool unityTest);
    void parseTlsParameters(cJSON *tlsObj, TLSParams &tls);

    // Serialize internal struct to JSON string
    esp_err_t serializeConfig(bool unityTest = false);

    esp_err_t writeConfigFile();

    bool loadDataFromNVS(const std::string &key, const std::string &value);
    bool saveDataToNVS(const std::string &key, const std::string &value);

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


//-------------------------------------------------------------------------------------
// CONFIG CLASS HELPER FUNCTIONS
//-------------------------------------------------------------------------------------
namespace configClassHelper
{
// Find a sequence: id == -1 looks up by name (new sequence being filled in), otherwise matches by id
template <typename Vec> typename Vec::value_type *findSequenceByIdOrName(Vec &vec, int id, const std::string &name)
{
    for (auto &el : vec) {
        if (id == -1) {
            if (el.sequenceName == name) {
                return &el;
            }
        }
        else if (id == el.sequenceId) {
            return &el;
        }
    }
    return nullptr;
}

// Find a sequence: Match by name only
template <typename Vec> typename Vec::value_type *findSequenceByName(Vec &vec, const std::string &name)
{
    for (auto &el : vec) {
        if (el.sequenceName == name) {
            return &el;
        }
    }
    return nullptr;
}

// Sync sequence vector accross sections
template <typename... Vecs> void syncSequenceVectors(const std::vector<SequenceList> &master, Vecs &...vecs)
{
    auto removeStale = [&](auto &vec) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [&](const auto &el) {
                                     return std::none_of(master.begin(), master.end(),
                                                         [&](const SequenceList &m) { return m.sequenceId == el.sequenceId; });
                                 }),
                  vec.end());
    };
    (removeStale(vecs), ...);

    auto addOrUpdate = [&](auto &vec) {
        for (const auto &m : master) {
            auto it = std::find_if(vec.begin(), vec.end(), [&](const auto &el) { return el.sequenceId == m.sequenceId; });
            if (it != vec.end()) {
                it->sequenceName = m.sequenceName;
            }
            else {
                typename std::decay_t<decltype(vec)>::value_type newEl{};
                newEl.sequenceId = m.sequenceId;
                newEl.sequenceName = m.sequenceName;
                vec.push_back(newEl);
            }
        }
    };
    (addOrUpdate(vecs), ...);

    auto sortById = [](auto &vec) {
        using ElType = typename std::decay_t<decltype(vec)>::value_type;
        std::sort(vec.begin(), vec.end(), [](const ElType &x, const ElType &y) { return x.sequenceId < y.sequenceId; });
    };
    (sortById(vecs), ...);
}


// Validate path formatting: normalizing slashes and leading/trailing separators
inline void validatePath(std::string &path, bool withFile = false)
{
    if (path.empty()) {
        return;
    }

    // Replace backslashes
    std::replace(path.begin(), path.end(), '\\', '/');

    if (!withFile) {
        // Remove trailing slash
        if (path.back() == '/') {
            path.pop_back();
        }
    }

    // Ensure leading slash
    if (!path.empty() && path.front() != '/') {
        path.insert(path.begin(), '/');
    }
}


// Validate structure formatting: removing leading/trailing slashes
inline void validateStructure(std::string &structureName)
{
    if (structureName.empty()) {
        return;
    }

    // Replace backslashes
    std::replace(structureName.begin(), structureName.end(), '\\', '/');

    // Remove leading slash
    if (structureName.front() == '/') {
        structureName.erase(structureName.begin());
    }

    // Remove trailing slash
    if (!structureName.empty() && structureName.back() == '/') {
        structureName.pop_back();
    }
}

} // namespace configClassHelper

#endif // CONFIGCLASS_H
