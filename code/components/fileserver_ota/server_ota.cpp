#include "server_ota.h"
#include "../../include/defines.h"

#include <string>
#include <functional>

#include <freertos/task.h>

#include <sys/stat.h>
#include <esp_task_wdt.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_app_format.h>
#include "miniz.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif // ENABLE_MQTT

#include "webserver.h"
#include "MainFlowControl.h"
#include "gpioControl.h"
#include "ClassControlCamera.h"
#include "network_main.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "statusled.h"


static const char *TAG = "SERVER_OTA";

static std::string fileNameUpdate; // Filename of update


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


static void infiniteLoop(void)
{
    int i = 0;
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "When a new firmware is available on the server, press the reset button to download it");
    while (1) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Waiting for a new firmware (" + std::to_string(++i) + ")");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// OTA Partition State Check is only needed if sdkconfig flag CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is set
// Rollback functionality is not yet implemented in this firmware
void checkOtaPartitionState(void)
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


static UnzipOtaResult unzipOta(const std::string &inputZipFile, const std::string &rootFolder)
{
    mz_zip_archive zipArchive = {};

    if (!mz_zip_reader_init_file(&zipArchive, inputZipFile.c_str(), 0)) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOta: Failed to init");
        return {UnzipOtaStatus::Failed, {}};
    }

    const mz_uint numberOfFiles = mz_zip_reader_get_num_files(&zipArchive);

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Files to be extracted: " + std::to_string(numberOfFiles));

    std::string firmwarePath;

    for (mz_uint i = 0; i < numberOfFiles; ++i) {
        mz_zip_archive_file_stat fileStat;

        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOta: Failed to get file stat | File index: " + std::to_string(i));

            mz_zip_reader_end(&zipArchive);
            return {UnzipOtaStatus::Failed, {}};
        }

        if (fileStat.m_is_directory) {
            continue;
        }

        const std::string archiveFilename(fileStat.m_filename);

        if (!isSafePath(archiveFilename)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOta: Unzip rejected | Unsafe path: " + archiveFilename);

            mz_zip_reader_end(&zipArchive);
            return {UnzipOtaStatus::Failed, {}};
        }

        const std::string archiveFilenameUpper = toUpper(archiveFilename);

        std::string destinationPath;

        if (archiveFilenameUpper == "FIRMWARE.BIN") {
            // Always extract firmware.bin into the dedicated firmware directory
            destinationPath = rootFolder + "firmware/" + archiveFilename;
            firmwarePath = destinationPath;
        }
        else if (archiveFilenameUpper == "BOOTLOADER.BIN" || archiveFilenameUpper == "PARTITIONS.BIN" ||
                 archiveFilenameUpper == "README.MD" || archiveFilenameUpper == "META.JSON") {
            // Not required for OTA / handled elsewhere.
            continue;
        }
        else {
            // Preserve the directory structure from the ZIP.
            destinationPath = rootFolder + archiveFilename;
        }

        ESP_LOGI(TAG, "Unzip file: %s", destinationPath.c_str());

        constexpr const char *TEMP_SUFFIX = "_0xge";
        const std::string tempPath = destinationPath + TEMP_SUFFIX;

        makeDir(getDirectory(destinationPath));
        deleteFile(tempPath);

        if (!mz_zip_reader_extract_to_file(&zipArchive, i, tempPath.c_str(), 0)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOta: Failed to unzip | File: " + archiveFilename);
            deleteFile(tempPath);
            mz_zip_reader_end(&zipArchive);
            return {UnzipOtaStatus::Failed, {}};
        }

        deleteFile(destinationPath);

        if (!renameFile(tempPath, destinationPath)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "unzipOta: Failed to rename | File: " + tempPath + " -> " + destinationPath);
            deleteFile(tempPath);
            mz_zip_reader_end(&zipArchive);
            return {UnzipOtaStatus::Failed, {}};
        }
    }

    mz_zip_reader_end(&zipArchive);

    return {UnzipOtaStatus::Success, firmwarePath};
}


// OTA update: 3rd step
static bool otaUpdateFirmware(const std::string &filename)
{
    esp_ota_handle_t otaHandle = 0;
    esp_err_t retVal = ESP_OK;
    bool success = false;

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Flashing firmware...");

    if (configured != running) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                            "Configured boot partition (0x" + intToHexString(configured->address) + ") differs from running partition (0x" +
                                intToHexString(running->address) + ")");
    }

    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, (unsigned int)running->address);

    const esp_partition_t *updatePartition = esp_ota_get_next_update_partition(NULL);
    if (!updatePartition) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Target OTA partition not found");
        return false;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", updatePartition->subtype, (unsigned int)updatePartition->address);

    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Failed to open file: " + filename);
        return false;
    }

    struct stat st;
    if (fstat(fileno(file), &st) != 0 || st.st_size < 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Binary file size eval failed or invalid size on: " + filename);
        fclose(file);
        return false;
    }
    const size_t totalFileSize = (size_t)st.st_size;

    if (totalFileSize == 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Binary file is empty (0 bytes)");
        fclose(file);
        return false;
    }

    char *otaDataBuffer = (char *)malloc(SERVER_OTA_SCRATCH_BUFSIZE);
    if (!otaDataBuffer) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Heap allocation failed for buffer");
        fclose(file);
        return false;
    }

    constexpr size_t headerRequiredSize = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    static_assert(SERVER_OTA_SCRATCH_BUFSIZE >= headerRequiredSize,
                  "SERVER_OTA_SCRATCH_BUFSIZE must be large enough to hold the full image header "
                  "in a single read (see otaUpdateFirmware header-validity check)");


    bool imageHeaderValid = false;
    size_t binaryFileLength = 0;
    size_t bytesRead = fread(otaDataBuffer, 1, SERVER_OTA_SCRATCH_BUFSIZE, file);

    while (bytesRead > 0) {
        if (!imageHeaderValid) {
            if (bytesRead >= headerRequiredSize) {
                esp_app_desc_t newAppInfo;
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
                if (lastInvalidApp && esp_ota_get_partition_description(lastInvalidApp, &invalidAppInfo) == ESP_OK) {
                    ESP_LOGI(TAG, "Last invalid firmware version: %s", invalidAppInfo.version);
                    if (memcmp(invalidAppInfo.version, newAppInfo.version, sizeof(newAppInfo.version)) == 0) {
                        LogFile.writeToFile(ESP_LOG_WARN, TAG, "New version matches previously invalid version. Aborting.");
                        fclose(file);
                        free(otaDataBuffer);
                        infiniteLoop();
                    }
                }
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

                imageHeaderValid = true;

                retVal = esp_ota_begin(updatePartition, totalFileSize, &otaHandle);
                if (retVal != ESP_OK) {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_begin failed: " + intToHexString(retVal));
                    goto cleanup;
                }
                ESP_LOGI(TAG, "esp_ota_begin succeeded");
            }
            else {
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: File header too small: " + std::to_string(bytesRead));
                goto cleanup;
            }
        }

        retVal = esp_ota_write(otaHandle, (const void *)otaDataBuffer, bytesRead);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_write failed: " + intToHexString(retVal));
            goto cleanup;
        }

        binaryFileLength += bytesRead;
        bytesRead = fread(otaDataBuffer, 1, SERVER_OTA_SCRATCH_BUFSIZE, file);

        if (bytesRead == 0 && ferror(file)) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG,
                                "otaUpdateFirmware: fread() failed after " + std::to_string(binaryFileLength) +
                                    " bytes (I/O error, not EOF)");
            goto cleanup;
        }
    }

    ESP_LOGI(TAG, "Total written image length: %u", (unsigned int)binaryFileLength);

    retVal = esp_ota_end(otaHandle);
    if (retVal != ESP_OK) {
        if (retVal == ESP_ERR_OTA_VALIDATE_FAILED) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: Image validation failed (corrupt image)");
        }
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_end failed: " + intToHexString(retVal));
        otaHandle = 0; // esp_ota_end already cleaned handle
        goto cleanup;
    }
    otaHandle = 0; // Successfully ended

    retVal = esp_ota_set_boot_partition(updatePartition);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "otaUpdateFirmware: esp_ota_set_boot_partition failed: " + intToHexString(retVal));
        goto cleanup;
    }

    // Clear Core Dump partition on success
    {
        const esp_partition_t *coredumpPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP,
                                                                            "coredump");
        if (coredumpPartition) {
            esp_partition_erase_range(coredumpPartition, 0, coredumpPartition->size);
        }
    }

    success = true;

cleanup:
    if (file) {
        fclose(file);
    }
    if (otaDataBuffer) {
        free(otaDataBuffer);
    }
    if (!success && otaHandle != 0) {
        esp_ota_abort(otaHandle);
    }

    return success;
}


// OTA update: 2nd step
static void taskOtaUpdate(void *pvParameter)
{
    setStatusLed(AP_OR_OTA, 1, true);

    const std::string fileType = toUpper(getFileType(fileNameUpdate));
    std::string firmwarePath;

    if (fileType == "ZIP") {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Processing ZIP file...");
        const UnzipOtaResult result = unzipOta(fileNameUpdate, "/sdcard/");

        if (result.status == UnzipOtaStatus::Failed) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to unzip files. Rebooting...");
        }
        else {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Unzip successful");
            firmwarePath = result.firmwarePath;
        }
    }
    else if (fileType == "BIN") {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Processing BIN file...");
        firmwarePath = fileNameUpdate;
    }
    else {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Only ZIP or BIN files are supported. Skip update request. Rebooting...");
    }

    // Process firmware.bin
    if (!firmwarePath.empty()) {
        if (otaUpdateFirmware(firmwarePath)) {
            deleteAllFilesInDirectory("/sdcard/firmware");
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Rebooting to finalize update process...");
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to update firmware. Rebooting...");
        }
    }

    doRebootOTA();
}


// OTA update: 1st step
void checkOtaStaged()
{
    FILE *pfile = fopen("/sdcard/update.txt", "r");
    if (!pfile) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No pending update");
        return;
    }

    char zw[256] = {0};
    fgets(zw, sizeof(zw), pfile);
    fileNameUpdate = std::string(zw);
    fclose(pfile);
    deleteFile("/sdcard/update.txt"); // Delete after processing

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Preparing update process | Staged file: " + fileNameUpdate);
    xTaskCreate(&taskOtaUpdate, "taskOTAUpdate", 16384, NULL, tskIDLE_PRIORITY + 5, NULL);

    while (1) { // wait until reboot is performed
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


static esp_err_t handler_ota_update(httpd_req_t *req)
{
    const char *apiName = "ota:v2"; // API name and version
    char query[196] = {};
    char filenameParam[128] = {};
    char taskParam[32] = {};

    // Default usage message when handler gets called without parameters or with empty query
    const std::string restUsageInfo = "Handler usage:<br>"
                                      "1. Delete all files in firmware directory:<br>"
                                      "- /ota?task=emptyfirmwaredir<br>"
                                      "2. Process firmware or neural network update file from SD card:<br>"
                                      "- /ota?task=update&file=firmware.bin (Supported: ZIP, BIN, TFL, TFLITE)";

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "task", taskParam, sizeof(taskParam));
        httpd_query_key_value(query, "file", filenameParam, sizeof(filenameParam));
    }

    const std::string task = taskParam;

    // Return HTML usage message if no task parameter is provided
    if (task.empty()) {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_sendstr(req, restUsageInfo.c_str());
        return ESP_OK;
    }
    // Task: Return API name
    else if (task == "api_name") {
        httpd_resp_sendstr(req, apiName);
        return ESP_OK;
    }
    // Task: Delete firmware directory
    else if (task == "emptyfirmwaredir") {
        deleteAllFilesInDirectory("/sdcard/firmware");
        httpd_resp_sendstr(req, "Directory /firmware deleted");
        return ESP_OK;
    }
    // Task: Update firmware
    else if (task == "update") {
        const std::string baseDir = "/sdcard/firmware/";
        const std::string sanitizedFile = getFileName(filenameParam);
        const std::string file = baseDir + sanitizedFile;

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "OTA update triggered | File: " + file);

        // Path safety & existence checks
        struct stat st;
        if (sanitizedFile.empty() || stat(file.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File not found or invalid path");
            return ESP_FAIL;
        }

        const std::string fileType = getFileType(file);
        if (fileType == "ZIP" || fileType == "BIN") {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "ZIP/BIN file: Reboot required to update");

            FILE *pFile = fopen("/sdcard/update.txt", "w");
            if (!pFile) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open update.txt");
                return ESP_FAIL;
            }

            const size_t len = file.length();
            const size_t written = fwrite(file.data(), 1, len, pFile);
            const int closeRes = fclose(pFile);

            if (written != len || closeRes != 0) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed writing update.txt");
                return ESP_FAIL;
            }

            // Send response before triggering reboot to guarantee delivery
            httpd_resp_sendstr(req, "reboot: Device reboots to process file.");

            // Defer reboot slightly to allow webserver TCP socket flushing
            vTaskDelay(pdMS_TO_TICKS(500));
            doReboot();
            return ESP_OK;
        }

        if (fileType == "TFLITE" || fileType == "TFL") {
            const std::string destFile = "/sdcard/config/models/" + sanitizedFile;

            deleteFile(destFile);

            if (!copyFile(file, destFile)) {
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to copy neural network file");
                return ESP_FAIL;
            }

            deleteFile(file);
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "TFLITE/TFL file: Update completed");
            httpd_resp_sendstr(req, "Neural network file updated. No reboot required.");
            return ESP_OK;
        }

        const std::string errMsg = "Unsupported file type for file '" + sanitizedFile + "'. Allowed: ZIP, BIN, TFL, TFLITE";
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, errMsg.c_str());
        return ESP_FAIL;
    }

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "OTA handler: Unknown or missing task");
    return ESP_FAIL;
}


static void forceReboot()
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 1,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Bitmask of all cores
        .trigger_panic = true,
    };
    esp_task_wdt_init(&twdt_config);
    esp_task_wdt_add(NULL);

    while (true) {
        // Intentionally stall for TWDT trigger
    }
}


static void taskReboot(void *DeleteMainFlow)
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

    cameraCtrl.setFlashlight(false);
    forceStatusLedOff();
    esp_camera_deinit();

    destroyGpioHandler();

    httpd_stop(server);

    vTaskDelay(pdMS_TO_TICKS(3000));
    deinitNetwork();

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart(); // Reset type: CPU reset (Reset both CPUs)

    vTaskDelay(pdMS_TO_TICKS(5000));
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
    forceStatusLedOff();
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
    camuri.user_ctx = httpServerData;
    httpd_register_uri_handler(server, &camuri);

    camuri.method = HTTP_GET;
    camuri.uri = "/reboot";
    camuri.handler = HTTP_AUTH_BASIC(handler_reboot);
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
