
#include "esp_heap_caps.h"
#include <string>

struct strSTBI {
    std::string name = "";
    bool usePreallocated = false;
    uint8_t* PreallocatedMemory = NULL;
    int PreallocatedMemorySize = 0;
    int NeededAllocationSize = 0;
};
extern struct strSTBI STBIObjectPSRAM;


void *malloc_psram_heap(std::string name, size_t size, uint32_t caps);
void *malloc_psram_heap_STBI(std::string name, size_t size, uint32_t caps);
void *remalloc_psram_heap(std::string name, size_t size, void* p, uint32_t caps);
void *calloc_psram_heap(std::string name, size_t n, size_t size, uint32_t caps);

void free_psram_heap(std::string name, void *ptr);
