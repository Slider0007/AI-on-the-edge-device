#include "psram.h"
#include "ClassLogFile.h"


static const char* TAG = "PSRAM";

struct strSTBI STBIObjectPSRAM = {};

void *malloc_psram_heap(std::string name, size_t size, uint32_t caps)
{
	void *ptr;

	ptr = heap_caps_malloc(size, caps);
    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

	return ptr;
}


void *remalloc_psram_heap(std::string name, void* p, size_t size, uint32_t caps)
{
	void *ptr;

	ptr = heap_caps_realloc(p, size, caps);
    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

	return ptr;
}


void *malloc_psram_heap_STBI(std::string name, size_t size, uint32_t caps)
{
	void *ptr;

	if (STBIObjectPSRAM.usePreallocated && STBIObjectPSRAM.PreallocatedMemorySize == size &&
        STBIObjectPSRAM.PreallocatedMemory != NULL)
    {
        ptr = STBIObjectPSRAM.PreallocatedMemory;
        name += ": Use preallocated memory (" + STBIObjectPSRAM.name + ")";
        STBIObjectPSRAM.usePreallocated = false;
    }
    else {
        ptr = heap_caps_malloc(size, caps);
    }


    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

	return ptr;
}


void *calloc_psram_heap(std::string name, size_t n, size_t size, uint32_t caps)
{
	void *ptr;

	ptr = heap_caps_calloc(n, size, caps);
    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, name + ": Allocated: " + std::to_string(size));
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, name + ": Failed to allocate " + std::to_string(size));
    }

	return ptr;
}


void free_psram_heap(std::string name, void *ptr)
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, name + ": Free memory");
    heap_caps_free(ptr);
}
