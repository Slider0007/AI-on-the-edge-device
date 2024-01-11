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
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_app_format.h"

#include "MainFlowControl.h"
#include "server_help.h"
#include "server_GPIO.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif //ENABLE_MQTT

#include "ClassControllCamera.h"
#include "connect_wlan.h"
#include "ClassLogFile.h"
#include "Helper.h"
#include "statusled.h"


static const char *TAG = "SERVER_OTA";

std::string file_name_update;
// An ota data write buffer ready to write to the flash
static char ota_write_data[SERVER_OTA_SCRATCH_BUFSIZE + 1] = { 0 };


static void infinite_loop(void)
{
    int i = 0;
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "When a new firmware is available on the server, press the reset button to download it");
    while(1) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Waiting for a new firmware (" + std::to_string(++i) + ")");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


// OTA update: 3rd step
static bool ota_update_task(std::string fn)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA update");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {        
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Configured OTA boot partition at offset " + std::to_string(configured->address) + 
                ", but running from offset " + std::to_string(running->address));
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "(This can happen if either the OTA boot data or preferred boot image become somehow corrupted.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, (unsigned int)running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, (unsigned int)update_partition->address);
    //assert(update_partition != NULL);

    int binary_file_length = 0;

    //deal with all receive packet 
    bool image_header_was_checked = false;

    int data_read;     

    FILE* f = fopen(fn.c_str(), "rb");     // previously only "r

    if (f == NULL) { // File does not exist
        return false;
    }

    data_read = fread(ota_write_data, 1, SERVER_OTA_SCRATCH_BUFSIZE, f);

    while (data_read > 0) {
        if (data_read < 0) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Error: SSL data read error");
            return false;
        }
        else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "New version is the same as invalid version");
                            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Previously, there was an attempt to launch the firmware with " + 
                                    std::string(invalid_app_info.version) + " version, but it failed");
                            LogFile.WriteToFile(ESP_LOG_WARN, TAG, "The firmware has been rolled back to the previous version");
                            infinite_loop();
                        }
                    }

                    /*
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Current running version is the same as a new. We will not continue the update");
                        infinite_loop();
                    }
                    */
                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "esp_ota_begin failed (" + std::string(esp_err_to_name(err)) + ")");
                        return false;
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "received package is not fit len");
                    return false;
                }
            }            
            err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                return false;
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        }
        else if (data_read == 0) {
           //
           // * As esp_http_client_read never returns negative error code, we rely on
           // * `errno` to check for underlying transport connectivity closure if any
           //
            if (errno == ECONNRESET || errno == ENOTCONN) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Connection closed, errno = " + std::to_string(errno));
                break;
            }
        }
        data_read = fread(ota_write_data, 1, SERVER_OTA_SCRATCH_BUFSIZE, f);
    }
    fclose(f);  

    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Image validation failed, image is corrupted");
        }
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "esp_ota_end failed (" + std::string(esp_err_to_name(err)) + ")");
        return false;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "esp_ota_set_boot_partition failed (" + std::string(esp_err_to_name(err)) + ")");
    }

    return true ;
}


// OTA update: 2nd step
void task_do_update(void *pvParameter)
{
  	StatusLED(AP_OR_OTA, 1, true);  // Signaling an OTA update  

    std::string filetype = toUpper(getFileType(file_name_update));
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "File name: " + file_name_update + " | File type: " + filetype);

    if (filetype == "ZIP") {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Processing ZIP file: " + file_name_update);
        std::string out, outbin, retfirmware;

        out = "/sdcard/html";
        outbin = "/sdcard/firmware";

        retfirmware = unzip_new(file_name_update, out + "/", outbin + "/", "/sdcard/", false);
    	LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Files unzipped");

        if (retfirmware.length() > 0) {
        	LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Found firmware.bin");
            ota_update_task(retfirmware);
        }

        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger reboot due to firmware update");
        doRebootOTA();
    }
    else if (filetype == "BIN") {
       	LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Processing BIN file: " + file_name_update);
        ota_update_task(file_name_update);
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Trigger reboot due to firmware update");
        doRebootOTA();
    }
    else {
    	LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Only ZIP or BIN files are supported for firmware update during startup");
    }
}


// OTA update: 1st step
void CheckUpdate()
{
 	FILE *pfile;
    if ((pfile = fopen("/sdcard/update.txt", "r")) == NULL) {
		LogFile.WriteToFile(ESP_LOG_INFO, TAG, "No pending update");
        return;
	}

	char zw[256] = "";
	fgets(zw, sizeof(zw), pfile);
    file_name_update = std::string(zw);
    if (fgets(zw, sizeof(zw), pfile)) {
        if (std::string(zw) == "init") {
       		LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Inital Setup triggered");
        }
	}

    fclose(pfile);
    DeleteFile("/sdcard/update.txt");   // Prevent Boot Loop!!!

	LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Prepare update process | File: " + file_name_update);
    xTaskCreate(&task_do_update, "task_do_update", configMINIMAL_STACK_SIZE * 35, NULL, tskIDLE_PRIORITY+1, NULL);

    while(1) { // wait until reboot within task_do_update
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


//****************************************************************************
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}


static bool diagnostic(void)
{
    return true;
}


// OTA status check is only needed if sdkconfig flag CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is set
void CheckOTAStatus(void)
{
    ESP_LOGI(TAG, "Check OTAStatus");

    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function
            if (diagnostic()) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Diagnostics failed! Start rollback to the previous version");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}
#endif
//****************************************************************************


esp_err_t handler_ota_update(httpd_req_t *req)
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "handler_ota_update");
    char _query[200];
    char _filename[100];
    char _valuechar[30];    
    std::string fn = "/sdcard/firmware/";
    std::string _task = "";
    bool _file_del = false;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (httpd_req_get_url_query_str(req, _query, 200) == ESP_OK) {
        //ESP_LOGD(TAG, "Query: %s", _query);    
        if (httpd_query_key_value(_query, "task", _valuechar, sizeof(_valuechar)) == ESP_OK) {
            //ESP_LOGD(TAG, "task is found: %s", _valuechar);
            _task = std::string(_valuechar);
        }

        if (httpd_query_key_value(_query, "file", _filename, sizeof(_filename)) == ESP_OK) {
            fn.append(_filename);
            //ESP_LOGD(TAG, "File: %s", fn.c_str());
        }

        if (httpd_query_key_value(_query, "delete", _filename, sizeof(_filename)) == ESP_OK) {
            fn.append(_filename);
            _file_del = true;
            //ESP_LOGD(TAG, "Delete Default File: %s", fn.c_str());
        }
    }

    if (_task.compare("emptyfirmwaredir") == 0) {
        delete_all_in_directory("/sdcard/firmware");
        httpd_resp_sendstr(req, "Directory /sdcard/firmware deleted"); 
        return ESP_OK;
    }

    else if (_task.compare("update") == 0) {
        std::string filetype = toUpper(getFileType(fn));
        if ((filetype == "TFLITE") || (filetype == "TFL")) {
            std::string out = "/sdcard/config/" + getFileFullFileName(fn);
            DeleteFile(out);
            CopyFile(fn, out);
            DeleteFile(fn);

            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "TFLITE/TFL file: Update completed");
            httpd_resp_sendstr(req, "Neural network file updated. No reboot required");
            return ESP_OK;
        }

        else if ((filetype == "ZIP") || (filetype == "BIN")) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "ZIP/BIN file: Reboot required to update");

            FILE *pfile = fopen("/sdcard/update.txt", "w");
            fwrite(fn.c_str(), fn.length(), 1, pfile);
            fclose(pfile);

            httpd_resp_sendstr(req, "reboot"); // String needs to be started with "reboot" -> Trigger reboot from WebUI
            return ESP_OK;  
        }

        std::string zw = "task=update: No valid file (.zip, .bin, .tfl, .tlite)";
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, zw);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, zw.c_str()); 
        return ESP_FAIL;        
    }

    else if (_task.compare("unziphtml") == 0) {
        //ESP_LOGD(TAG, "Task unziphtml");
        std::string in, out, zw;

        in = "/sdcard/firmware/html.zip";
        out = "/sdcard/html";

        delete_all_in_directory(out);

        unzip(in, out + "/");

        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Web interface: Update completed");
        httpd_resp_sendstr(req, "Web interface: Update completed. No reboot required");
        return ESP_OK;        
    }

    if (_file_del) {
        if(!DeleteFile(fn)) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Deletetion failed. File does not exist: " + fn);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File deletion failed"); 
            return ESP_FAIL;
        }
        
        std::string zw = "File deleted: " + fn;
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, zw);
        httpd_resp_sendstr(req, zw.c_str());
        return ESP_OK;
    }

    std::string zw = "No valid task/action: OTA handler called without any parameter";
    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, zw);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, zw.c_str()); 
    return ESP_FAIL;
}


void hard_restart() 
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 1,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of all cores
        .trigger_panic = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));

    esp_task_wdt_add(NULL);
    while(true);
}


void task_reboot(void *DeleteMainFlow)
{
    // write a reboot, to identify a reboot by purpouse
    FILE* pfile = fopen("/sdcard/reboot.txt", "w");
    std::string _s_zw= "reboot";
    fwrite(_s_zw.c_str(), strlen(_s_zw.c_str()), 1, pfile);
    fclose(pfile);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    if ((bool)DeleteMainFlow) {
        DeleteMainFlowTask();  // Kill autoflow task if executed in extra task, if not don't kill parent task
    }

    /* Stop service tasks */
    #ifdef ENABLE_MQTT
        MQTTdestroy_client(true);
    #endif //ENABLE_MQTT

    gpio_handler_destroy();

    Camera.LightOnOff(false);
    StatusLEDOff();
    esp_camera_deinit();

    WIFIDestroy();

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    esp_restart();      // Reset type: CPU reset (Reset both CPUs)

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    hard_restart();     // Reset type: System reset (Triggered by watchdog), if esp_restart stalls (WDT needs to be activated)

    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Reboot failed");
    vTaskDelete(NULL); //Delete this task if it comes to this point
}


void doReboot()
{
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Reboot triggered by Software (5s)");
    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Reboot in 5sec");

    BaseType_t xReturned = xTaskCreate(&task_reboot, "task_reboot", configMINIMAL_STACK_SIZE * 4, (void*) true, 10, NULL);
    if( xReturned != pdPASS )
    {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "task_reboot not created -> force reboot without killing flow");
        task_reboot((void*) false);
    }
    vTaskDelay(15000 / portTICK_PERIOD_MS); // Prevent serving web client fetch response until system is shuting down
}


void doRebootOTA()
{
    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Reboot in 5sec");

    Camera.LightOnOff(false);
    StatusLEDOff();
    Camera.DeinitCam();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();      // Reset type: CPU reset (Reset both CPUs)

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    hard_restart();     // Reset type: System reset (Triggered by watchdog), if esp_restart stalls (WDT needs to be activated)
}


esp_err_t handler_reboot(httpd_req_t *req)
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "handler_reboot");
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "System will restart within 5 sec");

    std::string response = 
        "<html><head><script>"
            "function m(h) {"
                "document.getElementById('t').innerHTML=h;"
                "setInterval(function (){h +='.'; document.getElementById('t').innerHTML=h;"
                "fetch('reboot_page.html',{mode: 'no-cors'}).then(r=>{parent.location.href=('index.html');})}, 1000);"
            "}</script></head></html><body style='font-family: arial'><h3 id=t></h3>"
            "<script>m('Rebooting!<br>The page will automatically reload in around 25..60s.<br><br>');</script>"
            "</body></html>";

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response.c_str(), strlen(response.c_str()));
    
    doReboot();

    return ESP_OK;
}


void register_server_ota_sdcard_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");
    
    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;
    camuri.uri       = "/ota";
    camuri.handler   = handler_ota_update;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

    camuri.method    = HTTP_GET;
    camuri.uri       = "/reboot";
    camuri.handler   = handler_reboot;
    camuri.user_ctx  = NULL;    
    httpd_register_uri_handler(server, &camuri);

}
