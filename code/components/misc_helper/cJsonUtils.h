#ifndef CJSONUTILS_H
#define CJSONUTILS_H

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <climits>
#include <string>
#include <algorithm>
#include <initializer_list>

#include <esp_err.h>
#include <cJSON.h>

// #include "cfgDataStruct.h"
#include "ClassLogFile.h"


static const char *TAG_JSONUTILS = "JSONUTILS";

//-------------------------------------------------------------------------------------
// CJSON HELPER FUNCTIONS
//-------------------------------------------------------------------------------------
namespace cJsonUtils
{

// Helper function for building static error logs
inline void logJsonAddError(const char *keyName, const char *reason)
{
    char errBuf[128];
    snprintf(errBuf, sizeof(errBuf), "Element '%s': %s", keyName ? keyName : "Unknown", reason);
    LogFile.writeToFile(ESP_LOG_ERROR, TAG_JSONUTILS, errBuf);
}


//-----------------------------
// JSON DESERIALIZATION HELPER
//-----------------------------

// Helper: Construct key path string for error messages
inline std::string buildKeyPath(std::initializer_list<const char *> keys)
{
    std::string path;
    for (const char *key : keys) {
        if (!path.empty()) {
            path += ".";
        }
        path += key ? key : "NULL";
    }
    return path;
}

// Helper: Get nested item helper
inline cJSON *getNestedItem(cJSON *root, std::initializer_list<const char *> keys)
{
    cJSON *curr = root;
    for (const char *key : keys) {
        if (!curr) {
            return nullptr;
        }
        curr = cJSON_GetObjectItem(curr, key);
    }
    return curr;
}

// Parse numeric field, clamped to [minVal, maxVal]
template <typename T, typename U = T>
inline bool parseIntClamped(cJSON *root, std::initializer_list<const char *> keys, T &target, U minVal, U maxVal)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsNumber(node)) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing int path");
        return false;
    }

    target = static_cast<T>(std::clamp(node->valueint, static_cast<int>(minVal), static_cast<int>(maxVal)));
    return true;
}

// Parse numeric field, minimum-only (no upper bound)
template <typename T> inline bool parseIntClampedMin(cJSON *root, std::initializer_list<const char *> keys, T &target, T minVal)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsNumber(node)) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing int path");
        return false;
    }
    target = static_cast<T>(std::max(node->valueint, static_cast<int>(minVal)));
    return true;
}

// Parse numeric field, no clamping
template <typename T> inline bool parseInt(cJSON *root, std::initializer_list<const char *> keys, T &target)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsNumber(node)) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing int path");
        return false;
    }
    target = static_cast<T>(node->valueint);
    return true;
}

// Parse string-encoded float field, clamped to [minVal, maxVal]
inline bool parseFloatClamped(cJSON *root, std::initializer_list<const char *> keys, float &target, float minVal, float maxVal)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsString(node) || !node->valuestring) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing float path");
        return false;
    }

    const char *str = node->valuestring;
    char *endPtr = nullptr;
    errno = 0;
    float parsed = strtof(str, &endPtr);

    // Reject: empty string, no digits consumed, or trailing garbage after the number
    if (endPtr == str || *endPtr != '\0' || errno == ERANGE) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing float path");
        return false;
    }

    target = std::clamp(parsed, minVal, maxVal);
    return true;
}

// Parse string field
inline bool parseString(cJSON *root, std::initializer_list<const char *> keys, std::string &target)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsString(node) || !node->valuestring) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing string path");
        return false;
    }
    target = node->valuestring;
    return true;
}

// Parse string field with validity predicate applied
template <typename Pred>
inline bool parseStringValidated(cJSON *root, std::initializer_list<const char *> keys, std::string &target, Pred pred)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsString(node) || !node->valuestring) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG_JSONUTILS, std::string("Failed parsing string path '") + buildKeyPath(keys) + "'");
        return false;
    }
    if (!pred(node->valuestring)) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing string path");
        return false;
    }
    target = node->valuestring;
    return true;
}

// Parse boolean field
inline bool parseBool(cJSON *root, std::initializer_list<const char *> keys, bool &target)
{
    cJSON *node = getNestedItem(root, keys);

    // Key doesn't exist -> valid optional case
    if (node == NULL) {
        return false;
    }

    if (!cJSON_IsBool(node)) {
        logJsonAddError(buildKeyPath(keys).c_str(), "Failed parsing bool path");
        return false;
    }
    target = cJSON_IsTrue(node);
    return true;
}

//-----------------------------
// JSON SERIALIZATION HELPER
//-----------------------------

// Create a key - value pair : Bool
inline void addElementHelper(cJSON *parent, const char *name, const bool value, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return;
    }
    if (!parent) {
        logJsonAddError(name, "Adding key failed: Parent object is NULL");
        retVal = ESP_FAIL;
        return;
    }
    if (!cJSON_AddBoolToObject(parent, name, value)) {
        logJsonAddError(name, "cJSON allocation failure");
        retVal = ESP_FAIL;
    }
}


// Create a key-value pair: Numbers & Enums
template <typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
inline void addElementHelper(cJSON *parent, const char *name, const T value, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return;
    }
    if (!parent) {
        logJsonAddError(name, "Adding key failed: Parent object is NULL");
        retVal = ESP_FAIL;
        return;
    }
    if (!cJSON_AddNumberToObject(parent, name, static_cast<double>(value))) {
        logJsonAddError(name, "cJSON allocation failure");
        retVal = ESP_FAIL;
    }
}


// Create a key-value pair: C-Strings
inline void addElementHelper(cJSON *parent, const char *name, const char *const value, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return;
    }
    if (!parent) {
        logJsonAddError(name, "Adding key failed: Parent object is NULL");
        retVal = ESP_FAIL;
        return;
    }
    if (!cJSON_AddStringToObject(parent, name, value)) {
        logJsonAddError(name, "cJSON allocation failure");
        retVal = ESP_FAIL;
    }
}


// Create a key-value pair: std::string
inline void addElementHelper(cJSON *parent, const char *name, const std::string &value, esp_err_t &retVal)
{
    addElementHelper(parent, name, value.c_str(), retVal);
}


// Create and attach a sub-object
static cJSON *addObjectHelper(cJSON *parent, const char *name, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return NULL;
    }

    const char *keyName = name ? name : "UNKNOWN";

    if (parent == NULL) {
        logJsonAddError(keyName, "Failed to create object: Parent object is NULL");
        retVal = ESP_FAIL;
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL || !cJSON_AddItemToObject(parent, name, obj)) {
        logJsonAddError(keyName, "Failed to create or attach object");
        if (obj) {
            cJSON_Delete(obj);
        }
        retVal = ESP_FAIL;
        return NULL;
    }
    return obj;
}

// Create and attach a sub-array
static cJSON *addArrayHelper(cJSON *parent, const char *name, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return NULL;
    }

    const char *keyName = name ? name : "UNKNOWN";

    if (parent == NULL) {
        logJsonAddError(keyName, "Failed to create array: parent object is NULL");
        retVal = ESP_FAIL;
        return NULL;
    }

    cJSON *arr = cJSON_CreateArray();
    if (arr == NULL || !cJSON_AddItemToObject(parent, name, arr)) {
        logJsonAddError(keyName, "Failed to create or attach array");
        if (arr) {
            cJSON_Delete(arr);
        }
        retVal = ESP_FAIL;
        return NULL;
    }
    return arr;
}

// Append a new object element to a parent array
static cJSON *addArrayObjectHelper(cJSON *array, esp_err_t &retVal)
{
    if (retVal != ESP_OK) {
        return NULL;
    }

    if (array == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG_JSONUTILS, "Failed to append element: Parent array is NULL");
        retVal = ESP_FAIL;
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL || !cJSON_AddItemToArray(array, obj)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG_JSONUTILS, "Failed to create or append object into array");
        if (obj) {
            cJSON_Delete(obj);
        }
        retVal = ESP_FAIL;
        return NULL;
    }
    return obj;
}

} // namespace cJsonUtils

#endif // CJSONUTILS_H
