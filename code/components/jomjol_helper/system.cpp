#include "system.h"

#include "esp_pm.h"
#include "esp_chip_info.h"
#include "esp_vfs_fat.h"

#include "configFile.h"
#include "Helper.h"
#include "ClassLogFile.h"
#include "../../include/defines.h"

static const char* TAG = "SYSTEM";

unsigned int systemStatus = 0;

sdmmc_cid_t SDCardCid;
sdmmc_csd_t SDCardCsd;


std::string getChipModel(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    switch((int)chipInfo.model) {
        case 0 : return (char*)"ESP8266";
        case (int)esp_chip_model_t::CHIP_ESP32 : return std::string("ESP32");
        case (int)esp_chip_model_t::CHIP_ESP32S2 : return std::string("ESP32-S2");
        case (int)esp_chip_model_t::CHIP_ESP32S3 : return std::string("ESP32-S3");
        case (int)esp_chip_model_t::CHIP_ESP32C3 : return std::string("ESP32-C3");
        case 6 : return std::string("ESP32-H4");
        case 12 : return std::string("ESP32-C2");
        case 13 : return std::string("ESP32-C6");
        //case (int)esp_chip_model_t::CHIP_ESP32H4 : return std::string("ESP32-H4");
        //case (int)esp_chip_model_t::CHIP_ESP32C2 : return std::string("ESP32-C2");
        //case (int)esp_chip_model_t::CHIP_ESP32C6 : return std::string("ESP32-C6");
        //case (int)esp_chip_model_t::CHIP_ESP32H2 : return std::string("ESP32-H2");
        case 16 : return std::string("ESP32-H2");
        //case (int)esp_chip_model_t::CHIP_POSIX_LINUX : return std::string("CHIP_POSIX_LINUX");
    }
    return std::string("Chip unknown");
}


int getChipCoreCount(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    return (int)chipInfo.cores;
}


std::string getChipRevision(void)
{
	esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
	return to_stringWithPrecision(chipInfo.revision / 100.0, 2);
}


void printDeviceInfo(void)
{
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Device info: Model: " + getChipModel() + 
                                           ", Cores: " + std::to_string(chipInfo.cores) + 
                                           ", Revision: " + getChipRevision());
}


/*std::string get_device_info()
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    std::string espInfoResultStr = "";
    char aMsgBuf[40];

    espInfoResultStr += "Device Info:";
    espInfoResultStr += "---------------\n";
    espInfoResultStr += "Chip Model: " + std::string(GetChipModel()) +"\n";
    sprintf(aMsgBuf,"Chip Revision: %d\n", chip_info.revision);
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf,"CPU Cores: %d\n", chip_info.cores);
    espInfoResultStr += std::string(aMsgBuf);
    sprintf(aMsgBuf,"Flash Memory: %dMB\n", spi_flash_get_chip_size()/(1024*1024));
    espInfoResultStr += std::string(aMsgBuf);
    if(chip_info.features & CHIP_FEATURE_WIFI_BGN)
        //espInfoResultStr += "Base MAC: " + std::string(getMac()) +"\n";
        espInfoResultStr += "ESP-IDF version: " + std::string(esp_get_idf_version()) +"\n";
    if((chip_info.features & CHIP_FEATURE_WIFI_BGN) || (chip_info.features & CHIP_FEATURE_BT) ||
       (chip_info.features & CHIP_FEATURE_BLE) || (chip_info.features & CHIP_FEATURE_EMB_FLASH))
    {
        espInfoResultStr += "Characteristics:\n";
        if(chip_info.features & CHIP_FEATURE_WIFI_BGN)
            espInfoResultStr += "    WiFi 2.4GHz\n";
        if(chip_info.features & CHIP_FEATURE_BT)
            espInfoResultStr += "    Bluetooth Classic\n";
        if(chip_info.features & CHIP_FEATURE_BLE)
            espInfoResultStr += "    Bluetooth Low Energy\n";
        if(chip_info.features & CHIP_FEATURE_EMB_FLASH)
            espInfoResultStr += "    Embedded Flash memory\n";
        else
           espInfoResultStr += "    External Flash memory\n";
    }

    #ifdef USE_HIMEM_IF_AVAILABLE
        sprintf(aMsgBuf,"spiram size %u\n", esp_psram_get_size());
        espInfoResultStr += std::string(aMsgBuf);
        sprintf(aMsgBuf,"himem free %u\n", esp_himem_get_free_size());
        espInfoResultStr += std::string(aMsgBuf);
        sprintf(aMsgBuf,"himem phys %u\n", esp_himem_get_phys_size());
        espInfoResultStr += std::string(aMsgBuf);
        sprintf(aMsgBuf,"himem reserved %u\n", esp_himem_reserved_area_size());
        espInfoResultStr += std::string(aMsgBuf);
    #endif
    
    return espInfoResultStr; 
}*/


/////////////////////////////////////////////////////////////////////////////////////////////
std::string getIDFVersion(void)
{
    return std::string(esp_get_idf_version());
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CPU Temp
extern "C" uint8_t temprature_sens_read();
float temperatureRead()
{
    return (temprature_sens_read() - 32) / 1.8;
}


bool setCPUFrequency(void)
{
    ConfigFile configFile = ConfigFile(CONFIG_FILE); 
    std::string cpuFrequency = "160";
    esp_pm_config_esp32_t pm_config; 

    if (!configFile.ConfigFileExists()){
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "No config file - exit setCpuFrequency");
        return false;
    }

    std::vector<std::string> splitted;
    std::string line = "";
    bool disabledLine = false;
    bool eof = false;


    /* Load config from config file */
    while ((!configFile.GetNextParagraph(line, disabledLine, eof) || 
            (line.compare("[System]") != 0)) && !eof) {}
    if (eof) {
        return false;
    }

    if (disabledLine) {
        return false;
    }

    while (configFile.getNextLine(&line, disabledLine, eof) && 
            !configFile.isNewParagraph(line)) {
        splitted = ZerlegeZeile(line);

        if (toUpper(splitted[0]) == "CPUFREQUENCY") {
            cpuFrequency = splitted[1];
            break;
        }
    }

    if (esp_pm_get_configuration(&pm_config) != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: Failed to read CPU frequency");
        return false;
    }

    if (cpuFrequency == "160") { // 160 is the default
        // No change needed
    }
    else if (cpuFrequency == "240") {
        pm_config.max_freq_mhz = 240;
        pm_config.min_freq_mhz = pm_config.max_freq_mhz;
        if (esp_pm_configure(&pm_config) != ESP_OK) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: Failed to set requested CPU frequency");
            return false;
        }
    }
    else {
		LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "setCpuFrequency: CPU frequency not supported: " + cpuFrequency);
        return false;
    }

    if (esp_pm_get_configuration(&pm_config) == ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "CPU frequency: " + std::to_string(pm_config.max_freq_mhz) + " MHz");
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
std::string getESPHeapInfo(){
	std::string espInfoResultStr = "";
	char aMsgBuf[80];

	size_t aFreeHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT);

	size_t aFreeSPIHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aFreeInternalHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

	size_t aHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aHeapIntLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

	size_t aMinFreeHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
	size_t aMinFreeInternalHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);


	sprintf(aMsgBuf,"Heap Total: %ld", (long) aFreeHeapSize);
	espInfoResultStr += std::string(aMsgBuf);

	sprintf(aMsgBuf," | SPI Free: %ld", (long) aFreeSPIHeapSize);
	espInfoResultStr += std::string(aMsgBuf);
	sprintf(aMsgBuf," | SPI Large Block:  %ld", (long) aHeapLargestFreeBlockSize);
	espInfoResultStr += std::string(aMsgBuf);
	sprintf(aMsgBuf," | SPI Min Free: %ld", (long) aMinFreeHeapSize);
	espInfoResultStr += std::string(aMsgBuf);

	sprintf(aMsgBuf," | Int Free: %ld", (long) (aFreeInternalHeapSize));
	espInfoResultStr += std::string(aMsgBuf);
	sprintf(aMsgBuf," | Int Large Block:  %ld", (long) aHeapIntLargestFreeBlockSize);
	espInfoResultStr += std::string(aMsgBuf);
	sprintf(aMsgBuf," | Int Min Free: %ld", (long) (aMinFreeInternalHeapSize));
	espInfoResultStr += std::string(aMsgBuf);
	
	return 	espInfoResultStr;
}


size_t getESPHeapSizeTotal()
{
   return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}


size_t getESPHeapSizeInternal() 
{
	return heap_caps_get_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeInternalLargestFree() 
{
	return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeInternalMinFree() 
{
	return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
}


size_t getESPHeapSizeSPIRAM()
{
	return heap_caps_get_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_SPIRAM);
}


size_t getESPHeapSizeSPIRAMLargestFree() 
{
	return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


size_t getESPHeapSizeSPIRAMMinFree() 
{
	return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
}


#ifdef USE_HIMEM_IF_AVAILABLE
size_t getESPHimemTotal()
{ 
	return esp_himem_get_phys_size();
}

size_t getESPHimemFree()
{ 
	return esp_himem_get_free_size();
}

size_t getESPHimemReservedArea()
{ 
	return esp_himem_reserved_area_size();
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
void setSystemStatusFlag(SystemStatusFlag_t flag)
{
	systemStatus = systemStatus | flag; // set bit

	char buf[20];
	snprintf(buf, sizeof(buf), "0x%08X", getSystemStatus());
    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "System status code: " + std::string(buf));
}


void clearSystemStatusFlag(SystemStatusFlag_t flag)
{
	systemStatus = systemStatus | ~flag; // clear bit

	char buf[20];
	snprintf(buf, sizeof(buf), "0x%08X", getSystemStatus());
    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "System status code: " + std::string(buf));
}


int getSystemStatus(void)
{
    return systemStatus;
}


bool isSetSystemStatusFlag(SystemStatusFlag_t flag)
{
	//ESP_LOGE(TAG, "Flag (0x%08X) is set (0x%08X): %d", flag, systemStatus , ((systemStatus & flag) == flag));

	if ((systemStatus & flag) == flag) {
		return true;
	}
	else {
		return false;
	}
}


std::string getResetReason(void)
{
	std::string reasonText;

	switch(esp_reset_reason()) {
		case ESP_RST_POWERON: reasonText = "Power-on event (or reset button)"; break;    //!< Reset due to power-on event
		case ESP_RST_EXT: reasonText = "External pin"; break;        //!< Reset by external pin (not applicable for ESP32)
		case ESP_RST_SW: reasonText = "Via esp_restart"; break;         //!< Software reset via esp_restart
		case ESP_RST_PANIC: reasonText = "Exception/panic"; break;      //!< Software reset due to exception/panic
		case ESP_RST_INT_WDT: reasonText = "Interrupt watchdog"; break;    //!< Reset (software or hardware) due to interrupt watchdog
		case ESP_RST_TASK_WDT: reasonText = "Task watchdog"; break;   //!< Reset due to task watchdog
		case ESP_RST_WDT: reasonText = "Other watchdogs"; break;        //!< Reset due to other watchdogs
		case ESP_RST_DEEPSLEEP: reasonText = "Exiting deep sleep mode"; break;  //!< Reset after exiting deep sleep mode
		case ESP_RST_BROWNOUT: reasonText = "Brownout"; break;   //!< Brownout reset (software or hardware)
		case ESP_RST_SDIO: reasonText = "SDIO"; break;       //!< Reset over SDIO

		case ESP_RST_UNKNOWN:   //!< Reset reason can not be determined
		default: 
			reasonText = "Unknown";
	}
    return reasonText;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/* Source: https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/tree/lsmmc.c */
/* SD Card Manufacturer Database */
struct SDCard_Manufacturer_database {
	std::string type;
	int id;
	std::string manufacturer;
};


/* Source: https://git.kernel.org/pub/scm/utils/mmc/mmc-utils.git/tree/lsmmc.c */
/* SD Card Manufacturer Database */
struct SDCard_Manufacturer_database database[] = {
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
		.id = 0x08,
		.manufacturer = "Silicon Power",
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
		.manufacturer = "Unknown",
	}
};


/* Parse SD Card Manufacturer Database */
std::string SDCardParseManufacturerIDs(int id) 
{
	unsigned int id_cnt = sizeof(database) / sizeof(struct SDCard_Manufacturer_database);
	std::string ret_val = "";

	for (int i = 0; i < id_cnt; i++) {
		if (database[i].id == id) {
			return database[i].manufacturer;
		}
		else {
			ret_val = "ID unknown (not in DB)";
		}
	}
	return ret_val;
}


std::string getSDCardPartitionSize(){
	FATFS *fs;
    uint32_t fre_clust, tot_sect;

    /* Get volume information and free clusters of drive 0 */
    f_getfree("0:", (DWORD *)&fre_clust, &fs);
    tot_sect = ((fs->n_fatent - 2) * fs->csize) /1024 /(1024/SDCardCsd.sector_size);	//corrected by SD Card sector size (usually 512 bytes) and convert to MB

	//ESP_LOGD(TAG, "%d MB total drive space (Sector size [bytes]: %d)", (int)tot_sect, (int)fs->ssize);

	return std::to_string(tot_sect);
}


std::string getSDCardFreePartitionSpace(){
	FATFS *fs;
    uint32_t fre_clust, fre_sect;
  
    /* Get volume information and free clusters of drive 0 */
    f_getfree("0:", (DWORD *)&fre_clust, &fs);
    fre_sect = (fre_clust * fs->csize) / 1024 /(1024/SDCardCsd.sector_size);	//corrected by SD Card sector size (usually 512 bytes) and convert to MB

    //ESP_LOGD(TAG, "%d MB free drive space (Sector size [bytes]: %d)", (int)fre_sect, (int)fs->ssize);

	return std::to_string(fre_sect);
}


std::string getSDCardPartitionAllocationSize(){
	FATFS *fs;
    uint32_t fre_clust, allocation_size;
  
    /* Get volume information and free clusters of drive 0 */
    f_getfree("0:", (DWORD *)&fre_clust, &fs);
    allocation_size = fs->ssize;

    //ESP_LOGD(TAG, "SD Card Partition Allocation Size: %d bytes", allocation_size);

	return std::to_string(allocation_size);
}


void SaveSDCardInfo(sdmmc_card_t* card) {
	SDCardCid = card->cid;
    SDCardCsd = card->csd;
}


std::string getSDCardManufacturer(){
	std::string SDCardManufacturer = SDCardParseManufacturerIDs(SDCardCid.mfg_id);
	//ESP_LOGD(TAG, "SD Card Manufacturer: %s", SDCardManufacturer.c_str());
	
	return (SDCardManufacturer + " (ID: " + std::to_string(SDCardCid.mfg_id) + ")");
}


std::string getSDCardName(){
	char *SDCardName = SDCardCid.name;
	//ESP_LOGD(TAG, "SD Card Name: %s", SDCardName); 

	return std::string(SDCardName);
}


std::string getSDCardCapacity(){
	int SDCardCapacity = SDCardCsd.capacity / (1024/SDCardCsd.sector_size) / 1024;  // total sectors * sector size  --> Byte to MB (1024*1024)
	//ESP_LOGD(TAG, "SD Card Capacity: %s", std::to_string(SDCardCapacity).c_str()); 

	return std::to_string(SDCardCapacity);
}


std::string getSDCardSectorSize(){
	int SDCardSectorSize = SDCardCsd.sector_size;
	//ESP_LOGD(TAG, "SD Card Sector Size: %s bytes", std::to_string(SDCardSectorSize).c_str()); 

	return std::to_string(SDCardSectorSize);
}