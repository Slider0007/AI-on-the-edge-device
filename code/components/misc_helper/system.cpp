#include "system.h"
#include "../../include/defines.h"

#include "esp_pm.h"
#include "esp_chip_info.h"
#include "hal/efuse_hal.h"
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"

#ifdef SOC_TEMP_SENSOR_SUPPORTED
#include "driver/temperature_sensor.h"
#endif // SOC_TEMP_SENSOR_SUPPORTED

#include "configClass.h"
#include "helper.h"
#include "ClassLogFile.h"


static const char *TAG = "SYSTEM";

static unsigned int systemStatus = 0;
static bool isPlannedReboot = false;
static SPIRAMCategory_t SPIRAMCategory = SPIRAMCategory_4MB;

static sdmmc_cid_t SDCardCid;
static sdmmc_csd_t SDCardCsd;


std::string getBoardType(void)
{
    return std::string(BOARD_TYPE_NAME);
}


std::string getChipModel(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    switch ((int)chipInfo.model) {
        case (int)esp_chip_model_t::CHIP_ESP32:
            return std::string("ESP32");
        case (int)esp_chip_model_t::CHIP_ESP32S2:
            return std::string("ESP32S2");
        case (int)esp_chip_model_t::CHIP_ESP32C3:
            return std::string("ESP32C3");
        case (int)esp_chip_model_t::CHIP_ESP32S3:
            return std::string("ESP32S3");
        case (int)esp_chip_model_t::CHIP_ESP32C2:
            return std::string("ESP32C2");
        // case (int)esp_chip_model_t::CHIP_ESP32C6 : return std::string("ESP32C6");
        case (int)esp_chip_model_t::CHIP_ESP32H2:
            return std::string("ESP32H2");
    }

    return std::string("Chip model unknown");
}


int getChipCoreCount(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    return (int)chipInfo.cores;
}


std::string getChipRevision(void)
{
    return std::to_string(efuse_hal_get_major_chip_version()) + "." + std::to_string(efuse_hal_get_minor_chip_version());
}


void printDeviceInfo(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                        "Device info: Board: " + getBoardType() + ", SOC: " + getChipModel() +
                            ", Cores: " + std::to_string(chipInfo.cores) + ", Revision: " + getChipRevision());
}


/////////////////////////////////////////////////////////////////////////////////////////////
std::string getIDFVersion(void)
{
    return std::string(esp_get_idf_version());
}


int getConfigVersion(void)
{
    return ConfigClass::getInstance()->get()->sectionConfig.version;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// SOC temperature sensor
#if defined(SOC_TEMP_SENSOR_SUPPORTED)
static float socTemperature = -1;

void taskSocTemp(void *pvParameter)
{
    temperature_sensor_handle_t socTempSensor = NULL;
    temperature_sensor_config_t socTempSensorConfig = {};
    socTempSensorConfig.range_min = 20;
    socTempSensorConfig.range_max = 100;

    temperature_sensor_install(&socTempSensorConfig, &socTempSensor);
    temperature_sensor_enable(socTempSensor);

    while (1) {
        if (temperature_sensor_get_celsius(socTempSensor, &socTemperature) != ESP_OK) {
            socTemperature = -1;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


void initSOCTemperatureSensor()
{
    // Create a dedicated task to ensure access temperature ressource only from a single source
    BaseType_t xReturned = xTaskCreate(&taskSocTemp, "taskSocTemp", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

    if (xReturned != pdPASS) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create taskSocTemp");
    }
}


float getSOCTemperature()
{
    return socTemperature;
}
#elif defined(CONFIG_IDF_TARGET_ESP32) // Unofficial support of vanilla ESP32. Value might be unreliable
extern "C" uint8_t temprature_sens_read();

float getSOCTemperature()
{
    return (temprature_sens_read() - 32) / 1.8;
}
#else
#warning "SOC temperature sensor not supported"
float getSOCTemperature()
{
    return -1.0;
}
#endif // SOC_TEMP_SENSOR_SUPPORTED


bool setCPUFrequency(void)
{
    esp_pm_config_t pm_config;

    if (esp_pm_get_configuration(&pm_config) != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: Failed to read CPU frequency");
        return false;
    }

    int cpuFrequency = ConfigClass::getInstance()->get()->sectionSystem.cpuFrequency;
    if (cpuFrequency == 160) { // 160 is the default
        // No change needed
    }
    else if (cpuFrequency == 240) {
        pm_config.max_freq_mhz = 240;
        pm_config.min_freq_mhz = pm_config.max_freq_mhz;
        if (esp_pm_configure(&pm_config) != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: Failed to set requested CPU frequency");
            return false;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: CPU frequency not supported: " + cpuFrequency);
        return false;
    }

    if (esp_pm_get_configuration(&pm_config) == ESP_OK) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "CPU frequency: " + std::to_string(pm_config.max_freq_mhz) + " MHz");
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
std::string getESPHeapInfo()
{
    std::string espInfoResultStr = "";
    char aMsgBuf[80];

    size_t aFreeHeapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    size_t aFreeSPIHeapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    size_t aFreeInternalHeapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    size_t aHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    size_t aHeapIntLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    size_t aMinFreeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    size_t aMinFreeInternalHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);


    sprintf(aMsgBuf, "Heap Total: %ld", (long)aFreeHeapSize);
    espInfoResultStr += std::string(aMsgBuf);

    sprintf(aMsgBuf, " | SPI Free: %ld", (long)aFreeSPIHeapSize);
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf, " | SPI Large Block:  %ld", (long)aHeapLargestFreeBlockSize);
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf, " | SPI Min Free: %ld", (long)aMinFreeHeapSize);
    espInfoResultStr += std::string(aMsgBuf);

    sprintf(aMsgBuf, " | Int Free: %ld", (long)(aFreeInternalHeapSize));
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf, " | Int Large Block:  %ld", (long)aHeapIntLargestFreeBlockSize);
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf, " | Int Min Free: %ld", (long)(aMinFreeInternalHeapSize));
    espInfoResultStr += std::string(aMsgBuf);

    return espInfoResultStr;
}


size_t getESPHeapSizeTotalFree()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}


size_t getESPHeapSizeInternalFree()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeInternalLargestFree()
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeInternalMinFree()
{
    return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeSPIRAMFree()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


size_t getESPHeapSizeSPIRAMLargestFree()
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


size_t getESPHeapSizeSPIRAMMinFree()
{
    return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


void setSPIRAMCategory(SPIRAMCategory_t category)
{
    SPIRAMCategory = category;
}


SPIRAMCategory_t getSPIRAMCategory()
{
    return SPIRAMCategory;
}


/////////////////////////////////////////////////////////////////////////////////////////////
void setSystemStatusFlag(SystemStatusFlag_t flag)
{
    systemStatus = systemStatus | flag; // set bit

    char buf[20];
    snprintf(buf, sizeof(buf), "0x%08X", getSystemStatus());
    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "System status code: " + std::string(buf));
}


void clearSystemStatusFlag(SystemStatusFlag_t flag)
{
    systemStatus = systemStatus | ~flag; // clear bit

    char buf[20];
    snprintf(buf, sizeof(buf), "0x%08X", getSystemStatus());
    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "System status code: " + std::string(buf));
}


int getSystemStatus(void)
{
    return systemStatus;
}


bool isSetSystemStatusFlag(SystemStatusFlag_t flag)
{
    // ESP_LOGE(TAG, "Flag (0x%08X) is set (0x%08X): %d", flag, systemStatus , ((systemStatus & flag) == flag));

    if ((systemStatus & flag) == flag) {
        return true;
    }
    else {
        return false;
    }
}


std::string getResetReason(void)
{
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
            return "Power-on event (or reset button)"; // Reset due to power-on event
        case ESP_RST_EXT:
            return "External pin"; // Reset by external pin (not applicable for ESP32)
        case ESP_RST_SW:
            return "Software restart (via esp_restart)"; // Software reset via esp_restart
        case ESP_RST_PANIC:
            return "Exception or panic"; // Software reset due to panic/exception
        case ESP_RST_INT_WDT:
            return "Interrupt watchdog"; // Reset due to interrupt watchdog
        case ESP_RST_TASK_WDT:
            return "Task watchdog"; // Reset due to task watchdog
        case ESP_RST_WDT:
            return "Other watchdogs"; // Reset due to other watchdogs
        case ESP_RST_DEEPSLEEP:
            return "Wakeup from deep sleep"; // Reset after exiting deep sleep mode
        case ESP_RST_BROWNOUT:
            return "Brownout (voltage drop of power supply)"; // Brownout reset
        case ESP_RST_SDIO:
            return "Reset via SDIO"; // Reset over SDIO
        case ESP_RST_UNKNOWN:
        default:
            return "Unknown reset reason"; // Fallback
    }
}


void checkIsPlannedReboot()
{
    FILE *file = fopen("/sdcard/reboot.txt", "r");
    if (!file) {
        isPlannedReboot = false;
        return;
    }

    fclose(file);

    if (!deleteFile("/sdcard/reboot.txt")) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Failed to delete reboot file");
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Planned reboot");
    isPlannedReboot = true;
}


bool getIsPlannedReboot()
{
    return isPlannedReboot;
}


void saveSDCardInfo(sdmmc_card_t *card)
{
    SDCardCid = card->cid;
    SDCardCsd = card->csd;
}


// SD Card Manufacturer Database
// Source: https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/tree/lsmmc.c
struct SDCardManufacturerDatabase {
    std::string type;
    int id;
    std::string manufacturer;
};


static const SDCardManufacturerDatabase database[] = {
    {
        .type = "sd",
        .id = 0x01,
        .manufacturer = "Panasonic",
    },
    {
        .type = "sd",
        .id = 0x02,
        .manufacturer = "Toshiba/Kingston/Viking",
    },
    {
        .type = "sd",
        .id = 0x03,
        .manufacturer = "SanDisk",
    },
    {
        .type = "sd",
        .id = 0x05,
        .manufacturer = "Lenovo",
    },
    {
        .type = "sd",
        .id = 0x08,
        .manufacturer = "Silicon Power",
    },
    {
        .type = "sd",
        .id = 0x09,
        .manufacturer = "ATP",
    },
    {
        .type = "sd",
        .id = 0x18,
        .manufacturer = "Infineon",
    },
    {
        .type = "sd",
        .id = 0x1b,
        .manufacturer = "Transcend/Samsung",
    },
    {
        .type = "sd",
        .id = 0x1c,
        .manufacturer = "Transcend",
    },
    {
        .type = "sd",
        .id = 0x1d,
        .manufacturer = "Corsair/AData",
    },
    {
        .type = "sd",
        .id = 0x1e,
        .manufacturer = "Transcend",
    },
    {
        .type = "sd",
        .id = 0x1f,
        .manufacturer = "Kingston",
    },
    {
        .type = "sd",
        .id = 0x27,
        .manufacturer = "Delkin/Phison",
    },
    {
        .type = "sd",
        .id = 0x28,
        .manufacturer = "Lexar",
    },
    {
        .type = "sd",
        .id = 0x30,
        .manufacturer = "SanDisk",
    },
    {
        .type = "sd",
        .id = 0x31,
        .manufacturer = "Silicon Power",
    },
    {
        .type = "sd",
        .id = 0x33,
        .manufacturer = "STMicroelectronics",
    },
    {
        .type = "sd",
        .id = 0x41,
        .manufacturer = "Kingston",
    },
    {
        .type = "sd",
        .id = 0x6f,
        .manufacturer = "STMicroelectronics",
    },
    {
        .type = "sd",
        .id = 0x74,
        .manufacturer = "Transcend",
    },
    {
        .type = "sd",
        .id = 0x76,
        .manufacturer = "Patriot",
    },
    {
        .type = "sd",
        .id = 0x82,
        .manufacturer = "Gobe/Sony",
    },
    {
        .type = "sd",
        .id = 0x89,
        .manufacturer = "Netac",
    },
    {
        .type = "sd",
        .id = 0x9f,
        .manufacturer = "Kingston/Kodak/Silicon Power",
    },
    {
        .type = "sd",
        .id = 0xad,
        .manufacturer = "Amazon Basics/Lexar/OV",
    },
    {
        .type = "sd",
        .id = 0xdf,
        .manufacturer = "Lenovo",
    },
    {
        .type = "sd",
        .id = 0xfe,
        .manufacturer = "Bekit/Cloudisk/HP/Reletech",
    } //
};


std::string parseSDCardManufacturerID(int id)
{
    const size_t idCnt = sizeof(database) / sizeof(struct SDCardManufacturerDatabase);

    for (size_t i = 0; i < idCnt; i++) {
        if (database[i].id == id) {
            return database[i].manufacturer;
        }
    }

    return "Unknown";
}


std::string getSDCardManufacturer()
{
    std::string SDCardManufacturer = parseSDCardManufacturerID(SDCardCid.mfg_id);
    // ESP_LOGD(TAG, "SD Card Manufacturer: %s", SDCardManufacturer.c_str());

    return SDCardManufacturer + " (ID: " + std::to_string(SDCardCid.mfg_id) + ")";
}


std::string getSDCardName()
{
    // ESP_LOGD(TAG, "SD Card Name: %s", SDCardCid.name);

    size_t len = 0;
    while (len < 8 && SDCardCid.name[len] != '\0') {
        ++len;
    }

    return std::string(SDCardCid.name, len);
}


int getSDCardPartitionSize()
{
    FATFS *fs;
    uint32_t freeClusters;

    // Get volume information and free clusters of drive 0
    if (f_getfree("0:", (DWORD *)&freeClusters, &fs) != FR_OK || fs == nullptr) {
        return -1; // Error case
    }

    // Corrected by SD Card sector size (usually 512 bytes) and convert to MB
    const uint32_t totalSectors = ((fs->n_fatent - 2) * fs->csize) / 1024 / (1024 / SDCardCsd.sector_size);

    // ESP_LOGD(TAG, "%d MB total drive space (Sector size [bytes]: %d)", (int)totalSectors, (int)fs->ssize);

    return totalSectors;
}


int getSDCardFreePartitionSpace()
{
    FATFS *fs;
    uint32_t freeClusters;

    // Get volume information and free clusters of drive 0
    if (f_getfree("0:", (DWORD *)&freeClusters, &fs) != FR_OK || fs == nullptr) {
        return -1; // Error case
    }

    // Corrected by SD Card sector size (usually 512 bytes) and convert to MB
    const uint32_t freeSectors = (freeClusters * fs->csize) / 1024 / (1024 / SDCardCsd.sector_size);

    // ESP_LOGD(TAG, "%d MB free drive space (Sector size [bytes]: %d)", (int)freeSectors, (int)fs->ssize);

    return freeSectors;
}


int getSDCardPartitionAllocationSize()
{
    FATFS *fs;
    uint32_t freeClusters;

    // Get volume information and free clusters of drive 0
    if (f_getfree("0:", (DWORD *)&freeClusters, &fs) != FR_OK || fs == nullptr) {
        return -1; // Error case
    }

    // ESP_LOGD(TAG, "SD Card Partition Allocation Size: %d bytes", fs->ssize);

    return fs->ssize;
}


int getSDCardCapacity()
{
    // Total sectors * sector size  --> Byte to MB (1024*1024)
    const int sdCardCapacity = SDCardCsd.capacity / (1024 / SDCardCsd.sector_size) / 1024;

    // ESP_LOGD(TAG, "SD Card Capacity: %d", sdCardCapacity);

    return sdCardCapacity;
}


int getSDCardSectorSize()
{
    // ESP_LOGD(TAG, "SD Card Sector Size: %d bytes", SDCardCsd.sector_size);

    return SDCardCsd.sector_size;
}
