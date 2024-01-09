#ifndef COMPONENTS_HELPER_PSRAM_H_
#define COMPONENTS_HELPER_PSRAM_H_

#include <string>

#include "esp_heap_caps.h"


/* SPIRAM profile in IDLE (date: 19.08.2023)*/
/* Showing data for heap: 0x3f802fa8
Block 0x3f803824 data, size: 55880 bytes, Free: No      --> WIFI
Block 0x3f811270 data, size: 12 bytes, Free: No
Block 0x3f811280 data, size: 56 bytes, Free: No
Block 0x3f8112bc data, size: 16 bytes, Free: Yes
Block 0x3f8112d0 data, size: 61440 bytes, Free: No      --> CAMERA
Block 0x3f8202d4 data, size: 921604 bytes, Free: No     --> RAWIMAGE (shared)
Block 0x3f9012dc data, size: 128004 bytes, Free: No     --> ALG_ROI
Block 0x3f9206e4 data, size: 2816 bytes, Free: No       --> REF0
Block 0x3f9211e8 data, size: 3964 bytes, Free: No       --> REF1
Block 0x3f922168 data, size: 226968 bytes, Free: No     --> MODEL 1
Block 0x3f959804 data, size: 1920 bytes, Free: No       --> ROIs DIGIT
Block 0x3f959f88 data, size: 10904 bytes, Free: No      ..
Block 0x3f95ca24 data, size: 1920 bytes, Free: No       ..
Block 0x3f95d1a8 data, size: 10904 bytes, Free: No      ..
Block 0x3f95fc44 data, size: 1920 bytes, Free: No       ..
Block 0x3f9603c8 data, size: 10904 bytes, Free: No      ..
Block 0x3f962e64 data, size: 1920 bytes, Free: No       ..
Block 0x3f9635e8 data, size: 10904 bytes, Free: No      ..
Block 0x3f966084 data, size: 133656 bytes, Free: No     --> MODEL 2
Block 0x3f986aa0 data, size: 3072 bytes, Free: No       --> ROIs ANALOG
Block 0x3f9876a4 data, size: 50700 bytes, Free: No      ..
Block 0x3f993cb4 data, size: 3072 bytes, Free: No       ..
Block 0x3f9948b8 data, size: 50700 bytes, Free: No      ..
Block 0x3f9a0ec8 data, size: 3072 bytes, Free: No       ..
Block 0x3f9a1acc data, size: 50700 bytes, Free: No      ..
Block 0x3f9ae0dc data, size: 3072 bytes, Free: No       ..
Block 0x3f9aece0 data, size: 50700 bytes, Free: No      ..
Block 0x3f9bb2f0 data, size: 2379020 bytes, Free: Yes
*/

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

#endif //PSRAM_H_ 