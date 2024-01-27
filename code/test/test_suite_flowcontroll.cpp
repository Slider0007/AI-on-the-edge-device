#include "../../include/defines.h"

#include <unity.h>

// Use isnan and isinf from <cmath> instead of a internal unity definition
#ifdef isnan
    #undef isnan
#endif
#ifdef isinf
    #undef isinf
#endif
#include <cmath>

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include "server_ota.h"

//*****************************************************************************
// Include files with functions to test
//*****************************************************************************
#include "components/jomjol-flowcontroll/test_cnnflowcontroll.cpp"
#include "components/jomjol-flowcontroll/test_flow_postrocess_helper.cpp"
#include "components/jomjol-flowcontroll/test_flowpostprocessing.cpp"
#include "components/jomjol-flowcontroll/test_flow_pp_negative.cpp"
#include "components/jomjol-flowcontroll/test_PointerEvalAnalogToDigitNew.cpp"
#include "components/jomjol-flowcontroll/test_getReadoutRawString.cpp"


esp_err_t initSDCard();


/**
 * @brief Startup the test. Like a test-suite 
 * all test methods must be called here
 */
void task_UnityTesting(void *pvParameter)
{
    vTaskDelay( 5000 / portTICK_PERIOD_MS ); // 5s delay to ensure established serial connection
    
    UNITY_BEGIN();
        printf("BEGIN TESTING -------------------------------------------------------------\n");
        RUN_TEST(test_getReadoutRawString);
        printf("---------------------------------------------------------------------------\n");
        
        RUN_TEST(test_EvalAnalogNumber);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_EvalDigitNumber);
        printf("---------------------------------------------------------------------------\n");
        
        RUN_TEST(testNegative_Issues);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(testNegative);
        printf("---------------------------------------------------------------------------\n");

        RUN_TEST(test_analogToDigit_Standard);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_analogToDigit_Transition);
        printf("---------------------------------------------------------------------------\n");

        RUN_TEST(test_doFlowPP);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_doFlowPP1);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_doFlowPP2);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_doFlowPP3);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_doFlowPP4);
        printf("---------------------------------------------------------------------------\n");
        RUN_TEST(test_doFlowPP5);
    UNITY_END();

    while(1);
}


/**
 * @brief Main task
 */
extern "C" void app_main()
{
    // Init SD card
    // ********************************************
    if (ESP_OK != initSDCard()) {
        ESP_LOGE(TAG, "Device init aborted");
        return; // Stop here, SD card is needed for proper operation
    }

    // Check for updates before start testing
    // It is possbile to update thr firmware also by placing 'firmware.bin' to '/sdcard/firmware' and
    // file 'update.txt' with content '/sdcard/firmware/firmware.bin' to sd card root folder.
    // Note: OTA Status check only necessary if OTA rollback feature is enabled
    // ********************************************
    #ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    CheckOTAPartitionState();
    #endif
    CheckOTAUpdate();

    // Set log level to DEBUG
    // Be aware: Output is limited to max defined log level in sdkconfig
    // ********************************************
    esp_log_level_set("*", ESP_LOG_DEBUG); // set all components to DEBUG level

    // Create dedicated testing task (heap size can be configured - large enough to handle a lot of testing cases)
    // ********************************************
    xTaskCreate(&task_UnityTesting, "task_UnityTesting", 12 * 1024, NULL, tskIDLE_PRIORITY+2, NULL);
}


esp_err_t initSDCard()
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG,"Initializing SD card: Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // Pullup SD card D3 pin to ensure SD init using MMC mode
    // Additionally, an external pullup is needed
    gpio_set_pull_mode(GPIO_SDCARD_D3, GPIO_PULLUP_ONLY);

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    #ifdef SOC_SDMMC_USE_GPIO_MATRIX
        slot_config.clk = GPIO_SDCARD_CLK;
        slot_config.cmd = GPIO_SDCARD_CMD;
        slot_config.d0 = GPIO_SDCARD_D0;
    #endif

    #ifdef BOARD_SDCARD_SDMMC_BUS_WIDTH_1
        slot_config.width = 1;
    #else
        #ifdef SOC_SDMMC_USE_GPIO_MATRIX
            slot_config.d1 = GPIO_SDCARD_D1;
            slot_config.d2 = GPIO_SDCARD_D2;
            slot_config.d3 = GPIO_SDCARD_D3;
        #endif
        slot_config.width = 4;
    #endif

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 12,                         // previously -> 2022-09-21: 5, 2023-01-02: 7 
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = 0
    };

    sdmmc_card_t* card;

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount FAT filesystem on SD card. Check SD card filesystem (only FAT supported) or try another card");
        } 
        else if (ret == 263) { // Error code: 0x107 --> usually: SD not found
            ESP_LOGE(TAG, "SD card init failed. Check if SD card is properly inserted into SD card slot or try another card");
        }
        else {
            ESP_LOGE(TAG, "SD card init failed. Check error code or try another card");
        }
        return ret;
    }

    return ret;
}
