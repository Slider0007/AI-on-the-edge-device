#ifndef ESP_SYSTEM_H
#define ESP_SYSTEM_H

#include <string>

#include "sdmmc_cmd.h"

typedef enum {
    SPIRAMCategory_4MB = 0,
    SPIRAMCategory_8MB = 1,
    SPIRAMCategory_16MB = 2,
    SPIRAMCategory_32MB = 3,
    SPIRAMCategory_MAX = 4
}SPIRAMCategory_t;

std::string getBoardType(void);
std::string getChipModel(void);
int getChipCoreCount(void);
std::string getChipRevision(void);
void printDeviceInfo(void);

std::string getIDFVersion(void);
int getConfigVersion(void);

#ifdef SOC_TEMP_SENSOR_SUPPORTED
void initSOCTemperatureSensor();
#endif
float getSOCTemperature();

bool setCPUFrequency(void);

std::string getESPHeapInfo(void);
size_t getESPHeapSizeTotalFree(void);
size_t getESPHeapSizeInternalFree(void);
size_t getESPHeapSizeInternalLargestFree(void);
size_t getESPHeapSizeInternalMinFree(void);
size_t getESPHeapSizeSPIRAMFree(void);
size_t getESPHeapSizeSPIRAMLargestFree(void);
size_t getESPHeapSizeSPIRAMMinFree(void);

void setSPIRAMCategory(SPIRAMCategory_t category);
SPIRAMCategory_t getSPIRAMCategory();

/* Error bit fields
   One bit per error
   Make sure it matches https://jomjol.github.io/AI-on-the-edge-device-docs/Error-Codes */
enum SystemStatusFlag_t {          // One bit per error
    // First Byte
    SYSTEM_STATUS_PSRAM_BAD         = 1 << 0, //  1, Critical Error
    SYSTEM_STATUS_HEAP_TOO_SMALL    = 1 << 1, //  2, Critical Error
    SYSTEM_STATUS_CAM_BAD           = 1 << 2, //  4, Critical Error
    SYSTEM_STATUS_SDCARD_CHECK_BAD  = 1 << 3, //  8, Critical Error
    SYSTEM_STATUS_FOLDER_CHECK_BAD  = 1 << 4, //  16, Critical Error

    // Second Byte
    SYSTEM_STATUS_CAM_FB_BAD        = 1 << (0+8), //  8, Flow still might work
    SYSTEM_STATUS_NTP_BAD           = 1 << (1+8), //  9, Flow will work but time will be wrong
};

void setSystemStatusFlag(SystemStatusFlag_t flag);
void clearSystemStatusFlag(SystemStatusFlag_t flag);
int getSystemStatus(void);
bool isSetSystemStatusFlag(SystemStatusFlag_t flag);

std::string getResetReason(void);

void checkIsPlannedReboot();
bool getIsPlannedReboot();

void saveSDCardInfo(sdmmc_card_t* card);
std::string parseSDCardManufacturerID(int);
std::string getSDCardManufacturer(void);
std::string getSDCardName(void);
int getSDCardPartitionSize(void);
int getSDCardFreePartitionSpace(void);
int getSDCardPartitionAllocationSize(void);
int getSDCardCapacity(void);
int getSDCardSectorSize(void);

#endif //ESP_SYSTEM_H
