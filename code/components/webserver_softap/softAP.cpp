#include "softAP.h"
#include "../../include/defines.h"

#ifdef ENABLE_SOFTAP
#include <string>
#include <unistd.h>
#include <sys/param.h>

#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>

#include "configClass.h"
#include "server_help.h"
#include "helper.h"
#include "statusled.h"
#include "server_ota.h"


static const char *TAG = "WIFI_AP";

static bool credentialsSet = false;
static bool SDCardContentExisting = false;


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}


void wifiInitAP(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitCfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifiConfig = { };
    strcpy((char*)wifiConfig.ap.ssid, (const char*) AP_ESP_WIFI_SSID);
    strcpy((char*)wifiConfig.ap.password, (const char*) AP_ESP_WIFI_PASS);
    wifiConfig.ap.channel = AP_ESP_WIFI_CHANNEL;
    wifiConfig.ap.max_connection = AP_MAX_STA_CONN;
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(AP_ESP_WIFI_PASS) == 0) {
        wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "started with SSID \"%s\", password: \"%s\", channel: %d. Connect to AP and open http://192.168.4.1",
             AP_ESP_WIFI_SSID, AP_ESP_WIFI_PASS, AP_ESP_WIFI_CHANNEL);
}


esp_err_t main_handler_AP(httpd_req_t *req)
{
    std::string message = "<h1>AI-on-the-edge - BASIC SETUP</h1><p>This is an access point to setup ";
    message += "the minimum required files and information on the device and the SD-card.<br><br>";
    message += "The initial setup is performed in 3 steps:<br>1. Set WLAN credentials<br>";
    message += "2. Upload ZIP package to flash SD card content<br>3. Reboot<br>";
    httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));

    if (!credentialsSet) {
        message = "<h3>1. Set WLAN credentials</h3><p>";
        message += "<table>";
        message += "<tr><td>SSID</td><td><input type=\"text\" name=\"ssid\" id=\"ssid\"></td><td>   ";
        message += "Enter the SSID name of WLAN network</td></tr>";
        message += "<tr><td>Password</td><td><input type=\"text\" name=\"password\" id=\"password\"></td><td>   ";
        message += "Enter the WLAN network password (ATTENTION: The password will be transmitted unencrypted!)</td><tr>";
        message += "</table><p>";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));

        message = "<button class=\"button\" type=\"button\" onclick=\"wr()\">Submit</button>";
        message += "<script language=\"JavaScript\">async function wr(){";
        message += "api = \"/config?\"+\"ssid=\"+document.getElementById(\"ssid\").value+\"&pwd=\"+document.getElementById(\"password\").value;";
        message += "fetch(api);await new Promise(resolve => setTimeout(resolve, 1000));location.reload();}</script>";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));
        credentialsSet = true;
        return ESP_OK;
    }

    if (!SDCardContentExisting) {
        message = "<h3>2. Upload ZIP package to flash SD card content</h3><p>";
        message += "After initial flashing of the firmware the the device sd-card is still empty.<br>";
        message += "Please upload \"AI-on-the-edge-device__{Board Type}__*.zip\", which installs the SD card content.<p>";
        message += "<input id=\"newfile\" type=\"file\"><br><br>";
        message += "<button class=\"button\" style=\"width:300px\" id=\"doUpdate\" type=\"button\" onclick=\"upload()\">Upload File</button><p>";
        message += "The upload might take up to 60s. After a succesfull upload the page will be reloaded.";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));

        message = "<script language=\"JavaScript\">";
        message += "function upload() {";
        message += "let xhttp = new XMLHttpRequest();";
        message += "xhttp.onreadystatechange = function() {if (xhttp.readyState == 4) {if (xhttp.status == 200) {location.reload();}}};";
        message += "let filePath = document.getElementById(\"newfile\").value.split(/[\\\\/]/).pop();";
        message += "let file = document.getElementById(\"newfile\").files[0];";
        message += "if (!file.name.includes(\"AI-on-the-edge-device__\")){if (!confirm(\"The zip file name should contain 'AI-on-the-edge-device__'. ";
        message += "Are you sure that you have downloaded the correct file?\"))return;};";
        message += "let upload_path = \"/upload/firmware/\" + filePath; xhttp.open(\"POST\", upload_path, true); xhttp.send(file);";
        message += "document.getElementById(\"doUpdate\").disabled = true;}";
        message += "</script>";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));
        return ESP_OK;
    }

    message = "<h3>3. Reboot</h3><p>";
    message += "Reboot to proceed the update process.<br>The device is going restart twice ";
    message += "and then connect to configured access point.<br>Please find the IP in your router settings and access it with the new IP address.<p>";
    message += "The first update and initialization process can take up to 3 minutes.<br>Error logs can be found ";
    message += "using UART / serial console.<p>Have fun!<p>";
    message += "<button class=\"button\" type=\"button\" onclick=\"rb()\")>Reboot</button>";
    message += "<script language=\"JavaScript\">async function rb(){";
    message += "api = \"/reboot\";";
    message += "fetch(api);await new Promise(resolve => setTimeout(resolve, 1000));location.reload();}</script>";
    httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


esp_err_t config_handler_AP(httpd_req_t *req)
{
    char query[384];
    char valuechar[64];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "ssid", valuechar, sizeof(valuechar)) == ESP_OK) {
            ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.ssid = urlDecode(std::string(valuechar));
        }

        if (httpd_query_key_value(query, "pwd", valuechar, sizeof(valuechar)) == ESP_OK) {
            ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password = urlDecode(std::string(valuechar));
            ConfigClass::getInstance()->saveMigDataToNVS("wlan_pw", ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan.password);
        }
    }
    ConfigClass::getInstance()->persistConfig();

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "WLAN config set");
    return ESP_OK;
}


esp_err_t upload_handler_AP(httpd_req_t *req)
{
    ESP_LOGI(TAG, "upload_handler_AP");

    makeDir("/sdcard/config");
    makeDir("/sdcard/firmware");
    makeDir("/sdcard/html");

    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;

    const char *filename = getPathFromUri(filepath, "/sdcard", req->uri + sizeof("/upload") - 1, sizeof(filepath));
    if (!filename) {
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Filename too long");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "filepath: %s, filename: %s\n", filepath, filename);

    deleteFile(std::string(filepath));

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file: %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file: %s", filename);

    char buf[1024];
    int received;
    int remaining = req->content_len;
    while (remaining > 0) {
        ESP_LOGI(TAG, "Remaining size: %d", remaining);
        if ((received = httpd_req_recv(req, buf, MIN(remaining, 1024))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }

            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        if (received && (received != fwrite(buf, 1, received, fd))) {
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        remaining -= received;
    }
    fclose(fd);
    SDCardContentExisting = true;

    FILE* pfile = fopen("/sdcard/update.txt", "w");
    std::string zw = "/sdcard" + std::string(filename);
    fwrite(zw.c_str(), strlen(zw.c_str()), 1, pfile);
    fclose(pfile);

    ESP_LOGI(TAG, "File reception complete");

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File reception complete");
    return ESP_OK;
}


esp_err_t reboot_handler_AP(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Trigger reboot due to update.");
    doRebootOTA();
    return ESP_OK;
}


httpd_handle_t start_webserverAP(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);

    httpd_uri_t reboot_handle = {
        .uri       = "/reboot",
        .method    = HTTP_GET,
        .handler   = reboot_handler_AP,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &reboot_handle);

    httpd_uri_t config_handleAP = {
        .uri       = "/config",
        .method    = HTTP_GET,
        .handler   = config_handler_AP,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &config_handleAP);

    httpd_uri_t file_uploadAP = {
        .uri       = "/upload/*", // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = upload_handler_AP,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &file_uploadAP);

    httpd_uri_t main_handlerAP = {
        .uri      = "*",
        .method   = HTTP_GET,
        .handler  = main_handler_AP,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &main_handlerAP);

    return NULL;
}


void checkStartAPMode()
{
    SDCardContentExisting = fileExists("/sdcard/html/index.html");
    if (!SDCardContentExisting)
        ESP_LOGW(TAG, "SD content not found (html/index.html)");

    if (ConfigClass::getInstance()->get()->sectionNetwork.wlan.ssid.empty() || !SDCardContentExisting) {
        ESP_LOGI(TAG, "Starting access point");
        setStatusLed(AP_OR_OTA, 2, true);
        wifiInitAP();
        start_webserverAP();

        while(1) { // wait until reboot
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

#endif //#ifdef ENABLE_SOFTAP
