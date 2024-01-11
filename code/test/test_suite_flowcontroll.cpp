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

#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"

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


bool Init_NVS_SDCard()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
////////////////////////////////////////////////

    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:

    #ifdef __SD_USE_ONE_LINE_MODE__
        slot_config.width = 1;
    #endif

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    #ifndef __SD_USE_ONE_LINE_MODE__
        gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
        gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    #endif
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 12,                         // anstatt 5 (2022-09-21)
        .allocation_unit_size = 16 * 1024
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return false;
    }

    return true;
}


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
    // Init NVS & SD card
    // ********************************************
    Init_NVS_SDCard();


    // Set log level to DEBUG
    // Be aware: Output is limited to max defined log level in sdkconfig
    // ********************************************
    esp_log_level_set("*", ESP_LOG_DEBUG); // set all components to DEBUG level


    // Check for updates before start testing
    // It is possbile to update thr firmware also by placing 'firmware.bin' to '/sdcard/firmware' and
    // file 'update.txt' with content '/sdcard/firmware/firmware.bin' to sd card root folder.
    // Note: OTA Status check only necessary if OTA rollback feature is enabled
    // ********************************************
    #ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    CheckOTAStatus();
    #endif
    CheckUpdate();


    // Create dedicated testing task (heap size can be configured - large enough to handle a lot of testing cases)
    // ********************************************
    xTaskCreate(&task_UnityTesting, "task_UnityTesting", 12 * 1024, NULL, tskIDLE_PRIORITY+2, NULL);
}
