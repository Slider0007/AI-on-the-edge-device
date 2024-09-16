/*#include "read_wlanini.h"
#include "../../include/defines.h"

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <esp_log.h>

#include "ClassLogFile.h"
#include "helper.h"
#include "connect_wlan.h"


static const char *TAG = "WLANINI";


struct wlan_config wlan_config = {};


int loadWlanConfigFromFile(std::string fn)
{
    std::string line = "";
    std::string tmp = "";
    std::vector<std::string> splitted;

    fn = formatFileName(fn);
    FILE* pFile = fopen(fn.c_str(), "r");
    if (pFile == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Unable to open file (read). Device init aborted");
        return -1;
    }

    // Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(pFile, NULL, _IOFBF, 512);

    ESP_LOGD(TAG, "loadWlanConfigFromFile: wlan.ini opened");

    char zw[256];
    if (fgets(zw, sizeof(zw), pFile) == NULL) {
        line = "";
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "file opened, but empty or content not readable. Device init aborted");
        fclose(pFile);
        return -1;
    }
    else {
        line = std::string(zw);
    }

    while ((line.size() > 0) || !(feof(pFile))) {
        //ESP_LOGD(TAG, "line: %s", line.c_str());
        if (line[0] != ';') {   // Skip lines which starts with ';'

            splitted = splitStringWLAN(line, "=");
            splitted[0] = trim(splitted[0], " ");

            if ((splitted.size() > 1) && (toUpper(splitted[0]) == "SSID")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.ssid = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "SSID: " + wlan_config.ssid);
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "PASSWORD")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.password = tmp;
                if (!wlan_config.password.empty()) {
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Password: *****");
                }
                else {
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Password: No password set");
                }
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "HOSTNAME")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.hostname = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "Hostname: " + wlan_config.hostname);
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "IP")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.ipaddress = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "IP-Address: " + wlan_config.ipaddress);
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "GATEWAY")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.gateway = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "Gateway: " + wlan_config.gateway);
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "NETMASK")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.netmask = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "Netmask: " + wlan_config.netmask);
            }

            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "DNS")) {
                tmp = splitted[1];
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.dns = tmp;
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "DNS: " + wlan_config.dns);
            }
            #if (defined WLAN_USE_ROAMING_BY_SCANNING || (defined WLAN_USE_MESH_ROAMING && defined WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES))
            else if ((splitted.size() > 1) && (toUpper(splitted[0]) == "RSSITHRESHOLD")) {
                tmp = trim(splitted[1]);
                if ((tmp[0] == '"') && (tmp[tmp.length()-1] == '"')) {
                    tmp = tmp.substr(1, tmp.length()-2);
                }
                wlan_config.rssi_threshold = atoi(tmp.c_str());
                LogFile.writeToFile(ESP_LOG_INFO, TAG, "RSSIThreshold: " + std::to_string(wlan_config.rssi_threshold));
            }
            #endif
        }

        // read next line
        if (fgets(zw, sizeof(zw), pFile) == NULL) {
            line = "";
        }
        else {
            line = std::string(zw);
        }
    }
    fclose(pFile);

    // Check if SSID is empty (mandatory parameter)
    if (wlan_config.ssid.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "SSID empty. Device init aborted");
        return -2;
    }

    // No check if password is empty --> handle password only as optional parameter: see issue #2393

    return 0;
}
#endif*/
