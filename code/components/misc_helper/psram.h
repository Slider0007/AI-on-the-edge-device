#ifndef COMPONENTS_HELPER_PSRAM_H_
#define COMPONENTS_HELPER_PSRAM_H_

#include <cstdint>
#include <string>
#include <unistd.h>

// SPIRAM profile in IDLE (date: 31.08.2026) / Freenove ESP32-S3 N16R8
/*
Showing data for heap: 0x3c160000
Block 0x3c160900 data, size: 36 bytes, Free: No
Block 0x3c160928 data, size: 36 bytes, Free: No
Block 0x3c160950 data, size: 36 bytes, Free: No
Block 0x3c160978 data, size: 36 bytes, Free: No
Block 0x3c1609a0 data, size: 32768 bytes, Free: No      --> CONFIG
Block 0x3c1689a4 data, size: 32768 bytes, Free: No      --> CONFIG
Block 0x3c1709a8 data, size: 59392 bytes, Free: No      --> FAT
Block 0x3c17f1ac data, size: 48 bytes, Free: No
Block 0x3c17f1e0 data, size: 36 bytes, Free: No
Block 0x3c17f208 data, size: 24 bytes, Free: No
Block 0x3c17f224 data, size: 56 bytes, Free: No
Block 0x3c17f260 data, size: 36 bytes, Free: No
Block 0x3c17f288 data, size: 68 bytes, Free: No
Block 0x3c17f2d0 data, size: 84 bytes, Free: No
Block 0x3c17f328 data, size: 84 bytes, Free: No
Block 0x3c17f380 data, size: 84 bytes, Free: No
Block 0x3c17f3d8 data, size: 12 bytes, Free: No
Block 0x3c17f3e8 data, size: 32 bytes, Free: No
Block 0x3c17f40c data, size: 12 bytes, Free: No
Block 0x3c17f41c data, size: 100 bytes, Free: No
Block 0x3c17f484 data, size: 33792 bytes, Free: No      --> HTTP
Block 0x3c187888 data, size: 720 bytes, Free: No
Block 0x3c187b5c data, size: 16 bytes, Free: Yes
Block 0x3c187b70 data, size: 56 bytes, Free: No
Block 0x3c187bac data, size: 16 bytes, Free: Yes
Block 0x3c187bc0 data, size: 61440 bytes, Free: No      --> CAMERA
Block 0x3c196bc4 data, size: 168 bytes, Free: No
Block 0x3c196c70 data, size: 24 bytes, Free: No
Block 0x3c196c8c data, size: 36864 bytes, Free: No      --> WIFI
Block 0x3c19fc90 data, size: 933888 bytes, Free: No     --> RAWIMAGE (shared)
Block 0x3c283c94 data, size: 192512 bytes, Free: No     --> ALG_ROI
Block 0x3c2b2c98 data, size: 2880 bytes, Free: No       --> REF0
Block 0x3c2b37dc data, size: 4864 bytes, Free: No       --> REF1
Block 0x3c2b4ae0 data, size: 229376 bytes, Free: No     --> MODEL 1
Block 0x3c2ecae4 data, size: 1920 bytes, Free: No       --> ROIs DIGIT
Block 0x3c2ed268 data, size: 3840 bytes, Free: No       ..
Block 0x3c2ee16c data, size: 1920 bytes, Free: No       ..
Block 0x3c2ee8f0 data, size: 3840 bytes, Free: No       ..
Block 0x3c2ef7f4 data, size: 1920 bytes, Free: No       ..
Block 0x3c2eff78 data, size: 3840 bytes, Free: No       ..
Block 0x3c2f0e7c data, size: 1920 bytes, Free: No       ..
Block 0x3c2f1600 data, size: 3840 bytes, Free: No       ..
Block 0x3c2f2504 data, size: 1920 bytes, Free: No       ..
Block 0x3c2f2c88 data, size: 3840 bytes, Free: No       ..
Block 0x3c2f3b8c data, size: 135168 bytes, Free: No     --> MODEL 2
Block 0x3c314b90 data, size: 3072 bytes, Free: No       --> ROIs ANALOG
Block 0x3c315794 data, size: 21504 bytes, Free: No      ..
Block 0x3c31ab98 data, size: 3072 bytes, Free: No       ..
Block 0x3c31b79c data, size: 21504 bytes, Free: No      ..
Block 0x3c320ba0 data, size: 3072 bytes, Free: No       ..
Block 0x3c3217a4 data, size: 21504 bytes, Free: No      ..
Block 0x3c326ba8 data, size: 3072 bytes, Free: No       ..
Block 0x3c3277ac data, size: 21504 bytes, Free: No      ..
Block 0x3c32cbb0 data, size: 256 bytes, Free: No
Block 0x3c32ccb4 data, size: 6501192 bytes, Free: Yes
*/

struct strSTBI {
    std::string name = "";
    bool usePreallocated = false;
    uint8_t *PreallocatedMemory = NULL;
    int PreallocatedMemorySize = 0;
    int NeededAllocationSize = 0;
};
extern struct strSTBI STBIObjectPSRAM;


void *malloc_psram_heap(std::string name, size_t size, uint32_t caps);
void *malloc_psram_heap_STBI(std::string name, size_t size, uint32_t caps);
void *remalloc_psram_heap(std::string name, void *p, size_t size, uint32_t caps);
void *calloc_psram_heap(std::string name, size_t n, size_t size, uint32_t caps);

void free_psram_heap(std::string name, void *ptr);


// cJSON - PSRAM arena
// ****************************************
void initCjsonHooks(void);

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    bool active;
} taskArena_t;


class cJsonObjectArena
{
  public:
    cJsonObjectArena(uint8_t *buffer, size_t capacity);
    ~cJsonObjectArena();

    cJsonObjectArena(const cJsonObjectArena &) = delete;
    cJsonObjectArena &operator=(const cJsonObjectArena &) = delete;

  private:
    taskArena_t arenaState;
};

#endif // PSRAM_H_
