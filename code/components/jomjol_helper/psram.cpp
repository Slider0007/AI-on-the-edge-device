#include "ClassLogFile.h"
#include "esp_heap_caps.h"

static const char* TAG = "PSRAM";

using namespace std;


void *malloc_psram_heap(std::string name, size_t size, uint32_t caps) {
	void *ptr;

	ptr = heap_caps_malloc(size, caps);
    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, to_string(size) + " bytes allocated: " + name);
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to allocate " + to_string(size) + " bytes: " + name);
    }

	return ptr;
}


void *calloc_psram_heap(std::string name, size_t n, size_t size, uint32_t caps) {
	void *ptr;

	ptr = heap_caps_calloc(n, size, caps);
    if (ptr != NULL) {
	    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, to_string(size) + " bytes allocated: " + name);
	}
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to allocate " + to_string(size) + " bytes: " + name);
    }

	return ptr;
}


void free_psram_heap(std::string name, void *ptr) {
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Free memory: " + name);
    heap_caps_free(ptr);
}