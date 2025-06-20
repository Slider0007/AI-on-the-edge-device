#include "server_ota.h"
#include "../../include/defines.h"

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <sys/stat.h>

/* TODO Rethink the usage of the int watchdog. It is no longer to be used, see
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/migration-guides/release-5.x/5.0/system.html?highlight=esp_int_wdt */
#include "esp_private/esp_int_wdt.h"
#include <esp_task_wdt.h>

#include <esp_ota_ops.h>
#include "esp_system.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "miniz.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif // ENABLE_MQTT

#include "webserver.h"
#include "MainFlowControl.h"
#include "gpioControl.h"
#include "ClassControlCamera.h"
#include "connect_wlan.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "statusled.h"


static const char *TAG = "SERVER_OTA";


static std::string fileNameUpdate;                               // Filename of update
static char otaDataBuffer[SERVER_OTA_SCRATCH_BUFSIZE + 1] = {0}; // OTA buffer


#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
static void infiniteLoop(void)
{
    int i = 0;
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "When a new firmware is available on the server, press the reset button to download it");
    while (1) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Waiting for a new firmware (" + std::to_string(++i) + ")");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE


// OTA update: 3rd step
static bool otaUpdateFirmware(std::string fn)
{
    esp_err_t retVal;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t otaHandle = 0;
    const esp_partition_t *updatePartition = NULL;

    ESP_LOGI(TAG, "Starting firmware update");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "Configured OTA boot partition at offset " + std::to_string(configured->address) +
                                ", but running from offset " + std::to_string(running->address));
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "This can happen if either the OTA boot data or preferred boot image become somehow corrupted.");
    }

    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, (unsigned int)running->address);

    updatePartition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", updatePartition->subtype, (unsigned int)updatePartition->address);
    // assert(updatePartition != NULL);

    FILE *file = fopen(fn.c_str(), "rb"); // previously only "r
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: File open failed: " + fn);
        return false;
    }

    int binaryFileLength = 0;
    // deal with all receive packet
    bool imageHeaderValid = false;

    int bytesRead = fread(otaDataBuffer, 1, SERVER_OTA_SCRATCH_BUFSIZE, file);

    while (bytesRead > 0) {
        if (imageHeaderValid == false) {
            esp_app_desc_t newAppInfo;
            if (bytesRead > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                // check current version with downloading
                memcpy(&newAppInfo, &otaDataBuffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)],
                       sizeof(esp_app_desc_t));
                ESP_LOGI(TAG, "New firmware version: %s", newAppInfo.version);

                esp_app_desc_t runningAppInfo;
                if (esp_ota_get_partition_description(running, &runningAppInfo) == ESP_OK) {
                    ESP_LOGI(TAG, "Running firmware version: %s", runningAppInfo.version);
                }

#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
                const esp_partition_t *lastInvalidApp = esp_ota_get_last_invalid_partition();
                esp_app_desc_t invalidAppInfo;
                if (esp_ota_get_partition_description(lastInvalidApp, &invalidAppInfo) == ESP_OK) {
                    ESP_LOGI(TAG, "Last invalid firmware version: %s", invalidAppInfo.version);
                }

                // check current version with last invalid partition
                if (lastInvalidApp != NULL) {
                    if (memcmp(invalidAppInfo.version, newAppInfo.version, sizeof(newAppInfo.version)) == 0) {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "New version is the same as invalid version");
                        LogFile.writeToFile(ESP_LOG_WARN, TAG,
                                            "Previously, there was an attempt to launch the firmware with " +
                                                std::string(invalidAppInfo.version) + " version, but it failed");
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "The firmware has been rolled back to the previous version");
                        infiniteLoop();
                    }
                }

                /*
                if (memcmp(newAppInfo.version, runningAppInfo.version, sizeof(newAppInfo.version)) == 0) {
                    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Current running version is the same as a new. We will not continue the update");
                    infiniteLoop();
                }
                */
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

                imageHeaderValid = true;

                retVal = esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &otaHandle);
                if (retVal != ESP_OK) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_begin failed. Error: " + intToHexString(retVal));
                    return false;
                }
                ESP_LOGI(TAG, "esp_ota_begin succeeded");
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "otaUpdateFirmware: Update file size too small. File size: " + std::to_string(bytesRead));
                return false;
            }
        }

        retVal = esp_ota_write(otaHandle, (const void *)otaDataBuffer, bytesRead);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_write failed. Error: " + intToHexString(retVal));
            return false;
        }

        binaryFileLength += bytesRead;
        bytesRead = fread(otaDataBuffer, 1, SERVER_OTA_SCRATCH_BUFSIZE, file);
    }
    fclose(file);

    ESP_LOGI(TAG, "Total written image length: %d", binaryFileLength);

    retVal = esp_ota_end(otaHandle);
    if (retVal != ESP_OK) {
        if (retVal == ESP_ERR_OTA_VALIDATE_FAILED) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Image validation failed, image is corrupted");
        }
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_end failed. Error: " + intToHexString(retVal));
        return false;
    }

    retVal = esp_ota_set_boot_partition(updatePartition);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_set_boot_partition failed. Error: " + intToHexString(retVal));
        return false;
    }

    // Clear core dump partition content after successful firmware update (clean start)
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");
    if (partition != NULL) {
        esp_partition_erase_range(partition, 0, partition->size);
    }

    return true;
}


// OTA update: 2nd step
void taskOtaUpdate(void *pvParameter)
{
    setStatusLed(AP_OR_OTA, 1, true); // Signaling an OTA update

    std::string filetype = toUpper(getFileType(fileNameUpdate));
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "File name: " + fileNameUpdate + " | File type: " + filetype);

    if (filetype == "ZIP") {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Processing ZIP file: " + fileNameUpdate);
        std::string retVal = unzipOTA(fileNameUpdate, "/sdcard/");
        if (retVal.length() > 0) {
            if (retVal == "ERROR") {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to unzip files. Update process failed");
            }
            else {
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "Found firmware.bin");
                if (!otaUpdateFirmware(retVal)) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to update firmware. Update process failed");
                }
            }
        }
        else {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Files unzipped");
        }
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Reboot to finalize update process");
        doRebootOTA();
    }
    else if (filetype == "BIN") {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Processing BIN file: " + fileNameUpdate);
        if (!otaUpdateFirmware(fileNameUpdate)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Firmware update failed");
        }

        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Reboot to finalize update process");
        doRebootOTA();
    }
    else {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "taskOtaUpdate: Only ZIP or BIN files are supported");
    }
}


// OTA update: 1st step
void checkOTAUpdate()
{
    FILE *pfile = fopen("/sdcard/update.txt", "r");
    if (!pfile) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No pending update");
        return;
    }

    char zw[256];
    fgets(zw, sizeof(zw), pfile);
    fileNameUpdate = std::string(zw);
    fclose(pfile);
    deleteFile("/sdcard/update.txt"); // Delete after processing

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Prepare update process | File: " + fileNameUpdate);
    xTaskCreate(&taskOtaUpdate, "taskOTAUpdate", configMINIMAL_STACK_SIZE * 35, NULL, tskIDLE_PRIORITY + 1, NULL);

    while (1) { // wait until reboot is performed
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


//****************************************************************************
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
static void printSha256(const uint8_t *imageHash, const char *label)
{
    char hashPrint[HASH_LEN * 2 + 1];
    hashPrint[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hashPrint[i * 2], "%02x", imageHash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hashPrint);
}


static bool diagnostic(void)
{
    return true;
}


// OTA Partition State Check is only needed if sdkconfig flag CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is set
// Rollback functionality is not yet implemented in this firmware
void checkOTAPartitionState(void)
{
    ESP_LOGI(TAG, "Check OTA partition state");

    uint8_t sha256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address = ESP_PARTITION_TABLE_OFFSET;
    partition.size = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha256);
    printSha256(sha256, "SHA-256 for the partition table");

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha256);
    printSha256(sha256, "SHA-256 for bootloader");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha256);
    printSha256(sha256, "SHA-256 for current firmware");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t otaState;
    if (esp_ota_get_state_partition(running, &otaState) == ESP_OK) {
        if (otaState == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function
            if (diagnostic()) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Diagnostics failed! Start rollback to the previous version");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
//****************************************************************************


esp_err_t handler_ota_update(httpd_req_t *req)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "handler_ota_update");
    char query[200];
    char filename[100];
    char valuechar[30];
    std::string fn = "/sdcard/firmware/";
    std::string task = "";
    bool deleteFileRequest = false;

    if (httpd_req_get_url_query_str(req, query, 200) == ESP_OK) {
        if (httpd_query_key_value(query, "task", valuechar, sizeof(valuechar)) == ESP_OK) {
            task = std::string(valuechar);
        }
        if (httpd_query_key_value(query, "file", filename, sizeof(filename)) == ESP_OK) {
            fn.append(filename);
        }
        if (httpd_query_key_value(query, "delete", filename, sizeof(filename)) == ESP_OK) {
            fn.append(filename);
            deleteFileRequest = true;
        }
    }

    if (task.compare("emptyfirmwaredir") == 0) {
        deleteAllFilesInDirectory("/sdcard/firmware");
        httpd_resp_sendstr(req, "Directory /sdcard/firmware deleted");
        return ESP_OK;
    }
    else if (task.compare("update") == 0) {
        std::string filetype = toUpper(getFileType(fn));
        if ((filetype == "TFLITE") || (filetype == "TFL")) {
            std::string out = "/sdcard/config/models/" + getFileFullFileName(fn);
            deleteFile(out);
            copyFile(fn, out);
            deleteFile(fn);

            LogFile.writeToFile(ESP_LOG_INFO, TAG, "TFLITE/TFL file: Update completed");
            httpd_resp_sendstr(req, "Neural network file updated. No reboot required");
            return ESP_OK;
        }
        else if ((filetype == "ZIP") || (filetype == "BIN")) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "ZIP/BIN file: Reboot required to update");

            FILE *pfile = fopen("/sdcard/update.txt", "w");
            fwrite(fn.c_str(), fn.length(), 1, pfile);
            fclose(pfile);

            httpd_resp_sendstr(req, "reboot"); // String needs to be started with "reboot" -> Trigger reboot from WebUI
            return ESP_OK;
        }

        std::string zw = "task=update: No valid file (.zip, .bin, .tfl, .tlite)";
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, zw);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, zw.c_str());
        return ESP_FAIL;
    }

    if (deleteFileRequest) {
        if (!deleteFile(fn)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Deletion failed. File does not exist: " + fn);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File deletion failed");
            return ESP_FAIL;
        }

        std::string zw = "File deleted: " + fn;
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, zw);
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;
    }

    std::string zw = "No valid task/action: OTA handler called without any parameter";
    LogFile.writeToFile(ESP_LOG_ERROR, TAG, zw);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, zw.c_str());
    return ESP_FAIL;
}


std::string unzipOTA(std::string inputZipFile, std::string rootFolder)
{
    mz_zip_archive zipArchive;
    std::string retVal; // return string "ERROR" -> FAILURE | return string != "ERROR" -> firmware filename

    // Open archive
    memset(&zipArchive, 0, sizeof(zipArchive));
    if (!mz_zip_reader_init_file(&zipArchive, inputZipFile.c_str(), 0)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOTA: mz_zip_reader_init_file() failed");
        return "ERROR";
    }

    // Get and print information about each file in the archive.
    int numberoffiles = (int)mz_zip_reader_get_num_files(&zipArchive);
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Files to be extracted: " + std::to_string(numberoffiles));

    for (int i = 0; i < numberoffiles; i++) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to get file stat for file index: " + std::to_string(i));
            continue;
        }

        std::string archiveFilename(fileStat.m_filename);

        if (!fileStat.m_is_directory) {
            // Extract file to heap
            // IMPORTANT NOTE -> miniz v3.x crashes here with ESP32S3, more details --> miniz changelog
            size_t uncompSize = 0;
            void *p = mz_zip_reader_extract_file_to_heap(&zipArchive, archiveFilename.c_str(), &uncompSize, 0);
            if (!p) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOTA: mz_zip_reader_extract_file_to_heap() failed | file: " + archiveFilename);
                mz_zip_reader_end(&zipArchive);
                return "ERROR";
            }

            // Save content to file
            std::string archiveFilenameTemp = archiveFilename;
            ESP_LOGD(TAG, "archive filename: %s", archiveFilenameTemp.c_str());

            const std::string archiveFilenameUpper = toUpper(archiveFilenameTemp);
            if (archiveFilenameUpper == "FIRMWARE.BIN") {
                // Redirect firmware.bin to /sdcard/firmware/
                archiveFilenameTemp = rootFolder + "firmware/" + archiveFilenameTemp;
                retVal = archiveFilenameTemp; // Return file for further processing
            }
            else if (archiveFilenameUpper == "BOOTLOADER.BIN" || archiveFilenameUpper == "PARTITIONS.BIN" ||
                     archiveFilenameUpper == "README.MD" || archiveFilenameUpper == "META.JSON") {
                // Skip not required binary files, readme.md from OTA package and meta.json from backup file
                continue;
            }
            else {
                // Other files use path structure of zip file
                archiveFilenameTemp = rootFolder + archiveFilenameTemp;
            }

            ESP_LOGI(TAG, "Unzip file: %s", archiveFilenameTemp.c_str());

            // Add suffix to ensure not directly overwriting original file
            constexpr const char *TEMP_SUFFIX = "_0xge";
            std::string archiveFilenameTempSuffix = archiveFilenameTemp + TEMP_SUFFIX;

            // Create directory if not yet existing
            makeDir(getDirectory(archiveFilenameTemp));

            // Ensure that temp file is surly deleted before writing data
            deleteFile(archiveFilenameTempSuffix);

            FILE *fpTargetFile = fopen(archiveFilenameTempSuffix.c_str(), "wb");
            if (!fpTargetFile) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to open file for writing: " + archiveFilenameTempSuffix);
                mz_free(p);
                retVal = "ERROR";
                break;
            }
            size_t writtenbytes = fwrite(p, 1, uncompSize, fpTargetFile);
            fclose(fpTargetFile);
            mz_free(p);

            bool isokay = true;

            if (writtenbytes != uncompSize) {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                    "unzipOTA: Failed to write file (written size differ from extracted size). File: " + archiveFilename +
                                        " | Extracted size: " + std::to_string(uncompSize));
                isokay = false;
            }
            else {
                deleteFile(archiveFilenameTemp); // Make sure, file is not existing. Note: It is possible that no file exists
                if (!renameFile(archiveFilenameTempSuffix, archiveFilenameTemp)) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                        "unzipOTA: Failed to rename file: " + archiveFilenameTempSuffix + " -> " + archiveFilenameTemp);
                    isokay = false;
                }
            }

            if (isokay) {
                LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "unzipOTA: File successful: " + archiveFilename);
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOTA: File failed: " + archiveFilename);
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOTA: Please repeat update to ensure proper functionality");
                retVal = "ERROR";
                break;
            }
        }
    }
    // Close the archive, freeing any resources it was using
    mz_zip_reader_end(&zipArchive);

    return retVal;
}


void forceReboot()
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 1,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Bitmask of all cores
        .trigger_panic = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));

    esp_task_wdt_add(NULL);
    while (true)
        ;
}


void taskReboot(void *DeleteMainFlow)
{
    // Write a reboot, to identify a reboot by purpose
    FILE *pfile = fopen("/sdcard/reboot.txt", "w");
    if (pfile) {
        std::string zw = "reboot";
        fwrite(zw.c_str(), strlen(zw.c_str()), 1, pfile);
        fclose(pfile);
    }

    // Kill main task if executed in extra task, if not don't kill parent task to force reboot
    if ((bool)DeleteMainFlow) {
        deleteMainFlowTask();
    }

/* Stop service tasks */
#ifdef ENABLE_MQTT
    deinitMqttClient(true);
#endif // ENABLE_MQTT

    gpio_handler_destroy();

    cameraCtrl.setFlashlight(false);
    setStatusLedOff();
    esp_camera_deinit();

    httpd_stop(server);

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    deinitWifi();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart(); // Reset type: CPU reset (Reset both CPUs)

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    forceReboot(); // Reset type: System reset (Triggered by watchdog), if esp_restart stalls (WDT needs to be activated)

    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Reboot failed");
    vTaskDelete(NULL); // Delete this task if it comes to this point
}


void doReboot()
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Reboot triggered by software");
    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Reboot in 5 seconds");

    BaseType_t xReturned = xTaskCreate(&taskReboot, "taskReboot", configMINIMAL_STACK_SIZE * 4, (void *)true, 10, NULL);
    if (xReturned != pdPASS) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "taskReboot not created -> force reboot without killing flow");
        taskReboot((void *)false);
    }
}


void doRebootOTA()
{
    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Reboot in 5sec");

    cameraCtrl.setFlashlight(false);
    setStatusLedOff();
    cameraCtrl.deinitCam();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart(); // Reset type: CPU reset (Reset both CPUs)

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    forceReboot(); // Reset type: System reset (Triggered by watchdog), if esp_restart stalls (WDT needs to be activated)
}


esp_err_t handler_reboot(httpd_req_t *req)
{
    LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "handler_reboot");

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Reboot initiated");

    doReboot();

    return ESP_OK;
}


void registerOtaRebootUri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = {};
    camuri.method = HTTP_GET;
    camuri.uri = "/ota";
    camuri.handler = HTTP_AUTH_BASIC(handler_ota_update);
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);

    camuri.method = HTTP_GET;
    camuri.uri = "/reboot";
    camuri.handler = HTTP_AUTH_BASIC(handler_reboot);
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
