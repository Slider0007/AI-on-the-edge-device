#include "psram.h"
#include "ClassLogFile.h"


static const char *TAG = "PSRAM";

struct strSTBI STBIObjectPSRAM = {};
struct strcJSON cJSONObjectPSRAM = {};

void *malloc_psram_heap(std::string name, size_t size, uint32_t caps)
{
    void *ptr;

    ptr = heap_caps_malloc(size, caps);
    if (ptr != NULL) {
#ifdef DEBUG_DETAIL_ON
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
#endif // DEBUG_DETAIL_ON
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

    return ptr;
}


void *remalloc_psram_heap(std::string name, void *p, size_t size, uint32_t caps)
{
    void *ptr;

    ptr = heap_caps_realloc(p, size, caps);
    if (ptr != NULL) {
#ifdef DEBUG_DETAIL_ON
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
#endif // DEBUG_DETAIL_ON
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

    return ptr;
}


void *malloc_psram_heap_STBI(std::string name, size_t size, uint32_t caps)
{
    void *ptr;

    if (STBIObjectPSRAM.usePreallocated && STBIObjectPSRAM.PreallocatedMemorySize == size && STBIObjectPSRAM.PreallocatedMemory != NULL) {
        ptr = STBIObjectPSRAM.PreallocatedMemory;
#ifdef DEBUG_DETAIL_ON
        name += ": Use preallocated memory (" + STBIObjectPSRAM.name + ")";
#endif // DEBUG_DETAIL_ON
        STBIObjectPSRAM.usePreallocated = false;
    }
    else {
        ptr = heap_caps_malloc(size, caps);
    }


    if (ptr != NULL) {
#ifdef DEBUG_DETAIL_ON
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
#endif // DEBUG_DETAIL_ON
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

    return ptr;
}


void *calloc_psram_heap(std::string name, size_t n, size_t size, uint32_t caps)
{
    void *ptr;

    ptr = heap_caps_calloc(n, size, caps);
    if (ptr != NULL) {
#ifdef DEBUG_DETAIL_ON
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
#endif // DEBUG_DETAIL_ON
    }
    else {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Free memory");
    }

    return ptr;
}


void free_psram_heap(std::string name, void *ptr)
{
#ifdef DEBUG_DETAIL_ON
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, name + ": Free memory");
#endif // DEBUG_DETAIL_ON
    heap_caps_free(ptr);
}


void *malloc_psram_heap_cjson(size_t size)
{
    cJSONObjectPSRAM.usedMemory += size;
    if (cJSONObjectPSRAM.preallocatedMemory != NULL && cJSONObjectPSRAM.preallocatedMemorySize >= cJSONObjectPSRAM.usedMemory) {
        return (uint8_t *)cJSONObjectPSRAM.preallocatedMemory + cJSONObjectPSRAM.usedMemory - size;
    }
    else {
#ifdef DEBUG_DETAIL_ON
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "cJSON: Use default region");
#endif // DEBUG_DETAIL_ON
        cJSONObjectPSRAM.useDefaultAllocation = true;
        return heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
    }
}


void free_psram_heap_cjson(void *ptr)
{
    if (!cJSONObjectPSRAM.useDefaultAllocation) {
        cJSONObjectPSRAM.usedMemory = 0;
    }
    else {
        cJSONObjectPSRAM.useDefaultAllocation = false;
        heap_caps_free(ptr);
    }
}
