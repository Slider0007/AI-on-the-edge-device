#include "softAP.h"
#include "../../include/defines.h"

#include <string>
#include <unistd.h>
#include <sys/param.h>

#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>

#include "network_main.h"
#include "connect_wlan.h"
#include "configClass.h"
#include "server_help.h"
#include "helper.h"
#include "statusled.h"
#include "server_ota.h"


static const char *TAG = "WLAN_AP";

static bool credentialsSet = false;
static bool SDCardContentExisting = false;
static bool deviceProvisioningByApStarted = false;


esp_err_t main_handler_AP(httpd_req_t *req)
{
    std::string message = "<h1>AI-on-the-Edge Device [SLFork] | Device Provisioning</h1>";
    message += "<p>Prepare the device and SD card with the required content and configuration.<br><br>";
    message += "The provisioning is completed in 3 steps:<br>1. Configure WLAN network (Not required for ethernet connection)<br>";
    message += "2. Upload the firmware package<br>3. Install the firmware package<br></p>";
    httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));

    if (!credentialsSet) {
        message = "<h3>1. Configure WLAN network</h3>";
        message += "<table>";
        message += "<tr><td>Network Name (SSID)</td><td style=\"padding:10px\"><input style=\"width:200px\" type=\"text\" name=\"ssid\" "
                   "id=\"ssid\"></td>";
        message += "<td>Enter your Wi-Fi network name</td></tr>";
        message += "<tr><td>Network Password</td><td style=\"padding:10px\"><input style=\"width:200px\" type=\"text\" name=\"password\" "
                   "id=\"password\"></td>";
        message += "<td>Enter your Wi-Fi network password (Note: Password is transmitted as plain text)</td><tr>";
        message += "</table><br><br>";
        message += "<button style=\"width:150px; padding:5px\" class=\"button\" type=\"button\" onclick=\"wr()\">Save config</button>";
        message += "<script language=\"JavaScript\">async function wr(){";
        message +=
            "api = \"/config?\"+\"ssid=\"+document.getElementById(\"ssid\").value+\"&pwd=\"+document.getElementById(\"password\").value;";
        message += "fetch(api);await new Promise(resolve => setTimeout(resolve, 1000));location.reload();}</script>";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));
        credentialsSet = true;
        return ESP_OK;
    }

    if (!SDCardContentExisting) {
        message = "<h3>2. Upload the firmware package</h3><p>";
        message += "Upload the firmware package \"AI-on-the-edge-device__{Board Type}__*.zip\" to install the SD card content.<p>";
        message += "<input id=\"newfile\" type=\"file\"><br><br>";
        message += "<button style=\"width:150px; padding:5px\" class=\"button\" type=\"button\" id=\"doUpdate\" "
                   "onclick=\"upload()\">Upload File</button><p>";
        message += "The upload may take up to 60s. After the package is successfully uploaded, the page is automatically reloaded.";
        message += "<script language=\"JavaScript\">";
        message += "function upload() {";
        message += "let xhttp = new XMLHttpRequest();";
        message += "xhttp.onreadystatechange = function() {if (xhttp.readyState == 4) {if (xhttp.status == 200) {location.reload();}}};";
        message += "let filePath = document.getElementById(\"newfile\").value.split(/[\\\\/]/).pop();";
        message += "let file = document.getElementById(\"newfile\").files[0];";
        message += "if (!file.name.includes(\"AI-on-the-edge-device__\")){if (!confirm(\"The zip file name should contain "
                   "'AI-on-the-edge-device__'. ";
        message += "Are you sure that you have chosen the correct file?\"))return;};";
        message += "let upload_path = \"/upload/firmware/\" + filePath; xhttp.open(\"POST\", upload_path, true); xhttp.send(file);";
        message += "document.getElementById(\"doUpdate\").disabled = true;}</script>";
        httpd_resp_send_chunk(req, message.c_str(), strlen(message.c_str()));
        return ESP_OK;
    }

    message = "<h3>3. Install the firmware package</h3><p>";
    message += "The firmware package was successfully uploaded to the device.<br>";
    message += "The device wull now reboot and install the package. This process may take up to 3 minutes.<br>";
    message += "The installation process can be monitored via serial console (e.g. using Web Installer interface).<br>";
    message += "If device is provisioned using web installer, just wait until installation is completed and refresh browser window.<br>";
    message += "Connect to WLAN network and access the device by hostname (default: http://watermeter | mDNS: http://watermeter.local)<br>";
    message += " or IP address automatically assigned by local DHCP server (e.g. FritzBox, check router logs).<br><br>";
    message += "<button style=\"width:150px; padding:5px\" class=\"button\" type=\"button\" id=\"doReboot\" onclick=\"rb()\")>Reboot To "
               "Proceed</button>";
    message += "<script language=\"JavaScript\">async function rb(){";
    message += "api = \"/reboot\";";
    message += "fetch(api);await new Promise(resolve => setTimeout(resolve, 1000));location.reload();";
    message += "document.getElementById(\"doReboot\").disabled = true;}</script>";
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

    FILE *pfile = fopen("/sdcard/update.txt", "w");
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
        .uri = "/reboot",
        .method = HTTP_GET,
        .handler = reboot_handler_AP,
        .user_ctx = NULL //
    };
    httpd_register_uri_handler(server, &reboot_handle);

    httpd_uri_t config_handleAP = {
        .uri = "/config",
        .method = HTTP_GET,
        .handler = config_handler_AP,
        .user_ctx = NULL //
    };
    httpd_register_uri_handler(server, &config_handleAP);

    httpd_uri_t file_uploadAP = {
        .uri = "/upload/*",
        .method = HTTP_POST,
        .handler = upload_handler_AP,
        .user_ctx = NULL //
    };
    httpd_register_uri_handler(server, &file_uploadAP);

    httpd_uri_t main_handlerAP = {
        .uri = "*",
        .method = HTTP_GET,
        .handler = main_handler_AP,
        .user_ctx = NULL //
    };
    httpd_register_uri_handler(server, &main_handlerAP);

    return NULL;
}


void startAPForDeviceProvisioning()
{
    SDCardContentExisting = fileExists("/sdcard/html/index.html");
    if (!SDCardContentExisting) {
        ESP_LOGW(TAG, "Unable to load HTML content: 'html/index.html' not found on SD card");
    }

    bool wlanSsidEmpty = (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN &&
                          ConfigClass::getInstance()->get()->sectionNetwork.wlan.ssid.empty());
    if (wlanSsidEmpty) {
        ESP_LOGW(TAG, "WLAN SSID empty");
    }

    if (!SDCardContentExisting || wlanSsidEmpty) {
        ESP_LOGI(TAG, "Init device provisioning");
        setStatusLed(AP_OR_OTA, 2, true);

#ifdef BOARD_FEATURE_ETHERNET
        if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
            // Init network and wait for network connection
            initNetwork();
            while (!getNetworkConnectionState()) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            // Skip config of WLAN credentials
            credentialsSet = true;
        }
        else {
            initWifiAp(true);
            deviceProvisioningByApStarted = true;
        }
#else
        initWifiAp(true);
        deviceProvisioningByApStarted = true;
#endif // BOARD_FEATURE_ETHERNET

        start_webserverAP();

        // Idle until reboot
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}


/** This function is only used to switch the Improv provisioning service where
 * step 1, WLAN configuration, is provisioned via serial / USB console,
 * continued with step 2 and step 3 defined in softAP.cpp -> function 'main_handler_AP'
 */
void stopAPForDeviceProvisioning(void)
{
    setStatusLedOff();
    deinitNetwork();
    credentialsSet = true; // Skip step 1
}


bool getDeviceProvisioningByApStarted()
{
    return deviceProvisioningByApStarted;
}
