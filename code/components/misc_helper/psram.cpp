#include "psram.h"

#include "esp_heap_caps.h"
#include <cJSON.h>

#include "ClassLogFile.h"


static const char *TAG = "PSRAM";

struct strSTBI STBIObjectPSRAM = {};

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


// cJSON custom hooks
// *****************
static void *mallocCjson(size_t size)
{
    if (tActiveArena != nullptr && tActiveArena->active) {
        size_t alignedSize = (size + 3) & ~3; // Ensure 4-byte boundary alignment
        if (tActiveArena->offset + alignedSize > tActiveArena->capacity) {
            ESP_LOGE("cJSON", "PSRAM Arena Exhausted! Needed: %zu, Capacity: %zu", alignedSize, tActiveArena->capacity);
            return nullptr;
        }
        void *ptr = &tActiveArena->buffer[tActiveArena->offset];
        tActiveArena->offset += alignedSize;
        return ptr;
    }
    return malloc(size); // Fallback to standard heap for non-arena tasks
}


static void freeCjson(void *ptr)
{
    if (tActiveArena != nullptr && tActiveArena->active) {
        return; // Arena deallocations are cleared in bulk when offset resets
    }
    free(ptr); // Standard heap free for non-arena tasks
}


void initCjsonHooks(void)
{
    cJSON_Hooks hooks;
    hooks.malloc_fn = mallocCjson;
    hooks.free_fn = freeCjson;
    cJSON_InitHooks(&hooks);
}
