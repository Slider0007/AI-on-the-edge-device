#include "connect_wlan.h"
#include "../../include/defines.h"

#include <netdb.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_wnm.h>
#include <esp_rrm.h>
#include <esp_mbo.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif // ENABLE_MQTT

#include "configClass.h"
#include "time_sntp.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "statusled.h"


static const char *TAG = "WLAN";


static const CfgData::SectionNetwork *cfgDataPtr = NULL;

static struct strWifiState {
    bool initialized = false;
    bool connected = false;
    bool connectionSucessful = false;
    bool connectionSupended = false;
    time_t connectionSuspendBaseTime = 0LL;

    int reconnectCnt = 0;
    bool fallbackApActive = false;

    bool accessPointWithBetterRSSI = false;
} wifiState;


static struct IpCfg {
    std::string ipAddress = "";
    std::string subnetMask = "";
    std::string gatewayAddress = "";
    std::string dnsServer = "";
} ipCfg;


std::string bssidToString(const char *c)
{
    char cBssid[6 * 2 + 5 + 1]; // AA:BB:CC:DD:EE:FF
    sprintf(cBssid, "%02x:%02x:%02x:%02x:%02x:%02x", c[0], c[1], c[2], c[3], c[4], c[5]);
    return std::string(cBssid);
}


std::string macToString(uint8_t mac[6])
{
    char macFormated[6 * 2 + 5 + 1]; // AA:BB:CC:DD:EE:FF
    sprintf(macFormated, MACSTR, MAC2STR(mac));
    return std::string(macFormated);
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifiState.connected = false;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
        if (disconn->reason == WIFI_REASON_ROAMING) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected (" + std::to_string(disconn->reason) + ", Roaming 802.11kv)");
            // --> no reconnect neccessary, it should automatically reconnect to new AP
        }
        else {
            wifiState.connected = false;
            if (disconn->reason == WIFI_REASON_NO_AP_FOUND || disconn->reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD ||
                disconn->reason == WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected (" + std::to_string(disconn->reason) + ", No AP found)");
                setStatusLed(WLAN_CONN, 1, false);
            }
            else if (disconn->reason == WIFI_REASON_AUTH_EXPIRE || disconn->reason == WIFI_REASON_AUTH_FAIL ||
                     disconn->reason == WIFI_REASON_NOT_AUTHED || disconn->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
                     disconn->reason == WIFI_REASON_HANDSHAKE_TIMEOUT || disconn->reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected (" + std::to_string(disconn->reason) + ", Auth fail)");
                setStatusLed(WLAN_CONN, 2, false);
            }
            else if (disconn->reason == WIFI_REASON_BEACON_TIMEOUT) {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected (" + std::to_string(disconn->reason) + ", Timeout)");
                setStatusLed(WLAN_CONN, 3, false);
            }
            else {
                LogFile.writeToFile(ESP_LOG_WARN, TAG, "Disconnected (" + std::to_string(disconn->reason) + ")");
                setStatusLed(WLAN_CONN, 4, false);
            }

            wifiState.reconnectCnt++;
            esp_wifi_connect(); // Try to connect again
        }

        if (wifiState.connectionSucessful) {
            if (wifiState.reconnectCnt >= WLAN_RECONNECT_RETRIES_ERROR_MSG) {
                wifiState.reconnectCnt = 0;
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Multiple reconnect attempts failed. Retry to connect");
            }
        }
        else {
            // Fallback to AP mode if initial connection cannot be established after defined time since boot [seconds]
            if (getUptime() >= WLAN_CONNECT_FALLBACK_AP_DELAY) {
                wifiState.reconnectCnt = 0;
                wifiState.fallbackApActive = true;
                LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to establish connection. Start access point (fallback)");
                deinitWifi();
                initWifiAp(wifiState.fallbackApActive);
            }
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Connected to: " + cfgDataPtr->wlan.ssid + ", RSSI: " + std::to_string(getWifiRssi()));

#ifdef WLAN_USE_MESH_ROAMING
        printRoamingFeatureSupport();

#ifdef WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES
        // wifiRoamingQuery();	// Avoid client triggered query during processing flow (reduce risk of heap shortage). Request will be
        // triggered at the end of every cycle anyway
#endif // WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES
#endif // WLAN_USE_MESH_ROAMING

        wifiState.connectionSuspendBaseTime = getUptime();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifiState.connectionSucessful = true;
        wifiState.connected = true;
        wifiState.reconnectCnt = 0;

        if (cfgDataPtr->wlan.ipv4.networkConfig == NETWORK_WLAN_IP_CONFIG_DHCP) {
            char buf[20];
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

            ipCfg.ipAddress = std::string(esp_ip4addr_ntoa(&event->ip_info.ip, buf, sizeof(buf)));
            ipCfg.subnetMask = std::string(esp_ip4addr_ntoa(&event->ip_info.netmask, buf, sizeof(buf)));
            ipCfg.gatewayAddress = std::string(esp_ip4addr_ntoa(&event->ip_info.gw, buf, sizeof(buf)));

            esp_netif_dns_info_t dnsInfo;
            esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dnsInfo);
            ipCfg.dnsServer = std::string(esp_ip4addr_ntoa((const esp_ip4_addr_t *)&dnsInfo.ip, buf, sizeof(buf)));
        }

        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Assigned IP: " + ipCfg.ipAddress + ", Subnet: " + ipCfg.subnetMask + ", Gateway: " + ipCfg.gatewayAddress +
                                ", DNS: " + ipCfg.dnsServer);

        // Start NTP service
        if (getUseNtp()) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Start NTP service");
            esp_netif_sntp_start();
        }

#ifdef ENABLE_MQTT
        // Start MQTT serivce
        if (getMqttIsEnabled()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            startMqttClient();
        }
#endif // ENABLE_MQTT
    }
    else if (event_id == WIFI_EVENT_AP_START) {
        wifiState.connectionSuspendBaseTime = getUptime();
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Access point: Client connected. MAC: " + macToString(event->mac));
        wifiState.connectionSuspendBaseTime = getUptime();
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Access point: Client disconnected. MAC: " + macToString(event->mac) +
                                ", Reason: " + std::to_string(event->reason));
        wifiState.connectionSuspendBaseTime = getUptime();
    }
}


bool suspendWifiConnection(void)
{
    if (wifiState.initialized && !wifiState.connectionSupended &&
        (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF ||
         ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF)) {
        // Set base time to actual time if connection to AP is still established
        if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF && getWifiIsConnected()) {
            wifiState.connectionSuspendBaseTime = getUptime();
            return false;
        }

        if ((int)((getUptime() - wifiState.connectionSuspendBaseTime) / 60) >=
            ConfigClass::getInstance()->get()->sectionNetwork.timedOffDelay) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Suspending WLAN connection by time (parameter: Timed-Off Delay)");

            wifiState.connected = false;
            wifiState.connectionSupended = true;
            setStatusLed(WLAN_CONN, 5, false);

            // Stop MQTT client
            if (getMqttIsEnabled()) {
                deinitMqttClient();
            }

            // Disconnect WLAN
            esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler);
            esp_wifi_stop();

            return true;
        }
    }

    return false;
}


bool resumeWifiConnection(std::string source)
{
    if (wifiState.initialized && wifiState.connectionSupended &&
        (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF ||
         ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF)) {
        wifiState.connectionSuspendBaseTime = getUptime();
        wifiState.connectionSupended = false;
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Resuming WLAN connection | Source: " + source);

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
        esp_wifi_start();

        // Underlaying wifi related services will be restarted within wifi event handler

        return true;
    }

    return false;
}


esp_err_t initWifi(void)
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionNetwork;

    if (cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_CLIENT || cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        return initWifiClient();
    }
    else if (cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_AP || cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF) {
        return initWifiAp();
    }
    else if (cfgDataPtr->opmode == NETWORK_OPMODE_DISABLED) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "WLAN disabled");
        return ESP_OK;
    }

    return ESP_FAIL;
}


esp_err_t initWifiClient(void)
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init client mode");

    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionNetwork;

    esp_err_t retVal = esp_event_loop_create_default();
    if (retVal != ESP_OK && retVal != ESP_ERR_INVALID_STATE) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_loop_create_default: Error: " + intToHexString(retVal));
        return retVal;
    }

    esp_netif_t *wifiStation = esp_netif_create_default_wifi_sta();

    if (cfgDataPtr->wlan.ipv4.networkConfig == NETWORK_WLAN_IP_CONFIG_STATIC) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Use static network config | IP: " + cfgDataPtr->wlan.ipv4.ipAddress +
                                ", Subnet: " + cfgDataPtr->wlan.ipv4.subnetMask + ", Gateway: " + cfgDataPtr->wlan.ipv4.gatewayAddress +
                                ", DNS: " + cfgDataPtr->wlan.ipv4.dnsServer);

        retVal = esp_netif_dhcpc_stop(wifiStation); // Stop DHCP service
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_dhcpc_stop: Error: " + intToHexString(retVal));
            return retVal;
        }

        esp_netif_ip_info_t ipInfo;
        memset(&ipInfo, 0, sizeof(esp_netif_ip_info_t));

        ipCfg.ipAddress = cfgDataPtr->wlan.ipv4.ipAddress;
        ipInfo.ip.addr = esp_ip4addr_aton(ipCfg.ipAddress.c_str());

        ipCfg.subnetMask = cfgDataPtr->wlan.ipv4.subnetMask;
        ipInfo.netmask.addr = esp_ip4addr_aton(ipCfg.subnetMask.c_str());

        ipCfg.gatewayAddress = cfgDataPtr->wlan.ipv4.gatewayAddress;
        ipInfo.gw.addr = esp_ip4addr_aton(ipCfg.gatewayAddress.c_str());

        retVal = esp_netif_set_ip_info(wifiStation, &ipInfo);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_ip_info: Error: " + intToHexString(retVal));
            return retVal;
        }

        if (cfgDataPtr->wlan.ipv4.dnsServer.empty()) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "No DNS address set, use gateway address as DNS");
            ipCfg.dnsServer = cfgDataPtr->wlan.ipv4.gatewayAddress;
        }
        else {
            ipCfg.dnsServer = cfgDataPtr->wlan.ipv4.dnsServer;
        }

        esp_netif_dns_info_t dnsInfo;
        dnsInfo.ip.u_addr.ip4.addr = esp_ip4addr_aton(ipCfg.dnsServer.c_str());

        retVal = esp_netif_set_dns_info(wifiStation, ESP_NETIF_DNS_MAIN, &dnsInfo);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_dns_info: Error: " + intToHexString(retVal));
            return retVal;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Use DHCP provided network config");
    }

    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    retVal = esp_wifi_init(&wifiInitCfg);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_init: Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_handler_register - WIFI_ANY: Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_handler_register - GOT_IP: Error: " + intToHexString(retVal));
        return retVal;
    }

#ifdef WLAN_USE_MESH_ROAMING
    retVal = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW, &esp_bss_rssi_low_handler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_handler_register - BSS_RSSI_LOW: Error: " + std::to_string(retVal));
        return retVal;
    }
#endif

    retVal = esp_wifi_set_mode(WIFI_MODE_STA);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_mode: Error: " + intToHexString(retVal));
        return retVal;
    }

    wifi_config_t wifiConfig = {};

    wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;     // Scan all channels instead of stopping after first match
    wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL; // Sort by signal strength and keep up to 4 best APs
    wifiConfig.sta.failure_retry_cnt = 5;                   // Number of connection retries station will do before moving to next AP

#ifdef WLAN_USE_MESH_ROAMING
    wifiConfig.sta.rm_enabled = 1;  // 802.11k (Radio Resource Management)
    wifiConfig.sta.btm_enabled = 1; // 802.11v (BSS Transition Management)
    // wifiConfig.sta.mbo_enabled = 1;	 // Multiband Operation (better use of Wi-Fi network resources in roaming decisions) -> not
    // activated to save heap
    wifiConfig.sta.pmf_cfg.capable = 1; // 802.11w (Protected Management Frame, activated by default if other device also advertizes PMF
                                        // capability)
    // wifiConfig.sta.ft_enabled = 1;	 // 802.11r (BSS Fast Transition) -> Upcoming IDF version 5.0 will support 11r
#endif

    if (cfgDataPtr->wlan.ssid.empty()) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "SSID empty");
    }

    strcpy((char *)wifiConfig.sta.ssid, (const char *)cfgDataPtr->wlan.ssid.c_str());
    strcpy((char *)wifiConfig.sta.password, (const char *)cfgDataPtr->wlan.password.c_str());

    retVal = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    if (retVal != ESP_OK) {
        if (retVal == ESP_ERR_WIFI_PASSWORD) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_config: Password invalid | Error: " + intToHexString(retVal));
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_config: Error: " + intToHexString(retVal));
            return retVal;
        }
    }

    retVal = esp_wifi_start();
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_start: Error: " + intToHexString(retVal));
        return retVal;
    }

    if (!cfgDataPtr->wlan.hostname.empty()) {
        retVal = esp_netif_set_hostname(wifiStation, cfgDataPtr->wlan.hostname.c_str());
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_hostname: Error: " + intToHexString(retVal));
        }
        else {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Assigned hostname: " + cfgDataPtr->wlan.hostname);
        }
    }

    wifiState.initialized = true;

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init client mode successful");
    return ESP_OK;
}


esp_err_t initWifiAp(bool _useDefaultConfig)
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init access point mode");

    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionNetwork;

    esp_err_t retVal = esp_event_loop_create_default();
    if (retVal != ESP_OK && retVal != ESP_ERR_INVALID_STATE) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_loop_create_default: Error: " + intToHexString(retVal));
        return retVal;
    }

    esp_netif_t *wifiAp = esp_netif_create_default_wifi_ap();

    retVal = esp_netif_dhcps_stop(wifiAp); // Stop DHCP server
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_dhcps_stop: Error: " + intToHexString(retVal));
        return retVal;
    }

    esp_netif_ip_info_t ipInfo;
    memset(&ipInfo, 0, sizeof(esp_netif_ip_info_t));

    if (_useDefaultConfig) { // Use default config
        ipCfg.ipAddress = "192.168.4.1";
    }
    else {
        ipCfg.ipAddress = cfgDataPtr->wlanAp.ipv4.ipAddress;
    }
    ipInfo.ip.addr = esp_ip4addr_aton(ipCfg.ipAddress.c_str());

    ipCfg.subnetMask = "255.255.255.0";
    ipInfo.netmask.addr = esp_ip4addr_aton(ipCfg.subnetMask.c_str());

    ipCfg.gatewayAddress = ipCfg.ipAddress;
    ipInfo.gw.addr = esp_ip4addr_aton(ipCfg.gatewayAddress.c_str());

    retVal = esp_netif_set_ip_info(wifiAp, &ipInfo);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_ip_info: Error: " + intToHexString(retVal));
        return retVal;
    }

    esp_netif_dns_info_t dnsInfo;
    memset(&dnsInfo, 0, sizeof(esp_netif_dns_info_t));

    ipCfg.dnsServer = ipCfg.ipAddress;
    dnsInfo.ip.u_addr.ip4.addr = esp_ip4addr_aton(ipCfg.dnsServer.c_str());

    retVal = esp_netif_set_dns_info(wifiAp, ESP_NETIF_DNS_MAIN, &dnsInfo);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_dns_info: Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = esp_netif_dhcps_start(wifiAp); // Restart DHCP server
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_dhcps_start: Error: " + intToHexString(retVal));
        return retVal;
    }

    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    retVal = esp_wifi_init(&wifiInitCfg);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_init: Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_event_handler_register - WIFI_ANY: Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = esp_wifi_set_mode(WIFI_MODE_AP);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_mode: Error: " + intToHexString(retVal));
        return retVal;
    }

    wifi_config_t wifiConfig = {};

    if (_useDefaultConfig) { // Use default config
        strcpy((char *)wifiConfig.ap.ssid, "AI-on-the-Edge Device");
        strcpy((char *)wifiConfig.ap.password, "");
        wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
        wifiConfig.ap.channel = 11;
        wifiConfig.ap.max_connection = 1;
    }
    else {
        if (cfgDataPtr->wlanAp.ssid.empty()) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Access point SSID empty");
        }

        if (cfgDataPtr->wlanAp.password.empty()) {
            wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
        }
        else if (cfgDataPtr->wlanAp.password.length() < 8) {
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Access point password less than 8 character");
            wifiConfig.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
        }
        else {
            wifiConfig.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
        }

        strcpy((char *)wifiConfig.ap.ssid, (const char *)cfgDataPtr->wlanAp.ssid.c_str());
        strcpy((char *)wifiConfig.ap.password, (const char *)cfgDataPtr->wlanAp.password.c_str());

        wifiConfig.ap.channel = cfgDataPtr->wlanAp.channel;
        wifiConfig.ap.max_connection = 3;
    }

    retVal = esp_wifi_set_config(WIFI_IF_AP, &wifiConfig);
    if (retVal != ESP_OK) {
        if (retVal == ESP_ERR_WIFI_PASSWORD) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_config: Password invalid | Error: " + intToHexString(retVal));
        }
        else {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_set_config: Error: " + intToHexString(retVal));
            return retVal;
        }
    }

    retVal = esp_wifi_start();
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_wifi_start: Error: " + intToHexString(retVal));
        return retVal;
    }

    wifiState.initialized = true;

    if (wifiState.fallbackApActive) {
        setStatusLedOff();
        setStatusLed(AP_OR_OTA, 3, true);
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                        "Init access point mode successful | SSID: " + std::string((char *)wifiConfig.ap.ssid) +
                            ", PW: " + std::string((char *)wifiConfig.ap.password) + ", CH: " + std::to_string(wifiConfig.ap.channel) +
                            ", IP: " + ipCfg.ipAddress);
    return ESP_OK;
}


#ifdef WLAN_USE_ROAMING_BY_SCANNING
std::string getAuthModeName(const wifi_auth_mode_t authMode)
{
    std::string authModeNames[] = {"OPEN",     "WEP",           "WPA PSK",  "WPA2 PSK", "WPA WPA2 PSK", "WPA2 ENTERPRISE",
                                   "WPA3 PSK", "WPA2 WPA3 PSK", "WAPI_PSK", "MAX"};
    return authModeNames[authMode];
}


esp_err_t wifiScan(httpd_req_t *req, bool checkRoaming)
{
    esp_err_t retVal = ESP_OK;
    wifi_scan_config_t wifiScanConfig;
    memset(&wifiScanConfig, 0, sizeof(wifiScanConfig));

    if (checkRoaming) {
        wifiScanConfig.ssid = (uint8_t *)cfgDataPtr->wlan.ssid.c_str(); // Scan only for configured SSID
    }
    wifiScanConfig.show_hidden = true; // Scan also hidden SSIDs
    wifiScanConfig.channel = 0;        // Scan all channels

    esp_wifi_scan_start(&wifiScanConfig, true); // Not using event handler SCAN_DONE by purpose to keep SYS_EVENT heap smaller
                                                // and the calling task task_autodoFlow is after scan is finish in wait state anyway
                                                // Scan duration: ca. (120ms + 30ms) * Number of channels -> ca. 1,5 - 2s

    uint16_t maxNumberOfApFound = 10;              // Max. number of APs, value will be updated by function "esp_wifi_scan_get_ap_num"
    esp_wifi_scan_get_ap_num(&maxNumberOfApFound); // Get actual found APs
    wifi_ap_record_t *wifiApRecords = new wifi_ap_record_t[maxNumberOfApFound]; // Allocate necessary record datasets
    if (wifiApRecords == NULL) {
        esp_wifi_scan_get_ap_records(0, NULL); // Free internal heap
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "wifiScan: Failed to allocate heap for wifiApRecords");
        return ESP_FAIL;
    }
    else {
        if (esp_wifi_scan_get_ap_records(&maxNumberOfApFound, wifiApRecords) != ESP_OK) { // Retrieve results (and free internal heap)
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "wifiScan: esp_wifi_scan_get_ap_records: Error retrieving datasets");
            delete[] wifiApRecords;
            return ESP_FAIL;
        }
    }

    if (checkRoaming) { // Check scanned network against actual access point to check if better AP is available
        wifi_ap_record_t currentAP;
        esp_wifi_sta_get_ap_info(&currentAP);

        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Roaming: Current AP BSSID=" + bssidToString((char *)currentAP.bssid));
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "Roaming: Scan completed, APs found with configured SSID: " + std::to_string(maxNumberOfApFound));
        for (int i = 0; i < maxNumberOfApFound; i++) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                                "Roaming: " + std::to_string(i + 1) + ": SSID=" + std::string((char *)wifiApRecords[i].ssid) +
                                    ", BSSID=" + bssidToString((char *)wifiApRecords[i].bssid) +
                                    ", RSSI=" + std::to_string(wifiApRecords[i].rssi) + ", CH=" + std::to_string(wifiApRecords[i].primary) +
                                    ", AUTH=" + getAuthModeName(wifiApRecords[i].authmode));
            if (wifiApRecords[i].rssi > (currentAP.rssi + 5) && // RSSI is better than actual RSSI + 5 --> Avoid switching to AP with
                                                                // roughly same RSSI
                (strcmp(bssidToString((char *)wifiApRecords[i].bssid).c_str(), bssidToString((char *)currentAP.bssid).c_str()) != 0)) {
                wifiState.accessPointWithBetterRSSI = true;
            }
        }
    }
    else if (req != NULL) { // Provide networks (REST API, JSON notation)
        cJSON *cJSONObject = cJSON_CreateObject();
        if (cJSONObject == NULL) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
            return ESP_FAIL;
        }
        cJSON *networks, *networkEl;
        if (!cJSON_AddItemToObject(cJSONObject, "networks", networks = cJSON_CreateArray())) {
            cJSON_Delete(cJSONObject);
            return ESP_FAIL;
        }

        for (uint16_t i = 0; i < maxNumberOfApFound; i++) {
            cJSON_AddItemToArray(networks, networkEl = cJSON_CreateObject());
            if (cJSON_AddStringToObject(networkEl, "ssid", (char *)wifiApRecords[i].ssid) == NULL) {
                retVal = ESP_FAIL;
            }
            if (cJSON_AddNumberToObject(networkEl, "channel", wifiApRecords[i].primary) == NULL) {
                retVal = ESP_FAIL;
            }
            if (cJSON_AddNumberToObject(networkEl, "rssi", wifiApRecords[i].rssi) == NULL) {
                retVal = ESP_FAIL;
            }
            if (cJSON_AddStringToObject(networkEl, "authmode", getAuthModeName(wifiApRecords[i].authmode).c_str()) == NULL) {
                retVal = ESP_FAIL;
            }
        }

        char *jsonChar = cJSON_Print(cJSONObject);
        cJSON_Delete(cJSONObject);

        if (jsonChar != NULL) {
            httpd_resp_set_type(req, "application/json");
            retVal = httpd_resp_send(req, jsonChar, strlen(jsonChar));
            cJSON_free(jsonChar);
        }
    }

    delete[] wifiApRecords;
    return retVal;
}


void wifiRoamByScanning(void)
{
    if (cfgDataPtr->wlan.wlanRoaming.enabled && getWifiRssi() != -127 && getWifiRssi() < cfgDataPtr->wlan.wlanRoaming.rssiThreshold) {
        LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Roaming: Start scan of all channels for SSID " + cfgDataPtr->wlan.ssid);
        wifiScan(NULL, true);

        if (wifiState.accessPointWithBetterRSSI) {
            wifiState.accessPointWithBetterRSSI = false;
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Roaming: AP with better RSSI in range, disconnect to switch AP");
            esp_wifi_disconnect();
        }
        else {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Roaming: Scan completed, stay on current AP");
        }
    }
}
#endif // WLAN_USE_ROAMING_BY_SCANNING


#ifdef WLAN_USE_MESH_ROAMING
/* rrm ctx */
int rrm_ctx = 0;

static inline unsigned int WPA_GET_LE32(const uint8_t *a)
{
    return ((unsigned int)a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}


#ifndef WLAN_EID_MEASURE_REPORT
#define WLAN_EID_MEASURE_REPORT 39
#endif
#ifndef MEASURE_TYPE_LCI
#define MEASURE_TYPE_LCI 9
#endif
#ifndef MEASURE_TYPE_LOCATION_CIVIC
#define MEASURE_TYPE_LOCATION_CIVIC 11
#endif
#ifndef WLAN_EID_NEIGHBOR_REPORT
#define WLAN_EID_NEIGHBOR_REPORT 52
#endif
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif


#define MAX_NEIGHBOR_LEN 512
static char *get_btm_neighbor_list(uint8_t *report, size_t report_len)
{
    size_t len = 0;
    const uint8_t *data;
    int ret = 0;

/*
 * Neighbor Report element (IEEE P802.11-REVmc/D5.0)
 * BSSID[6]
 * BSSID Information[4]
 * Operating Class[1]
 * Channel Number[1]
 * PHY Type[1]
 * Optional Subelements[variable]
 */
#define NR_IE_MIN_LEN (ETH_ALEN + 4 + 1 + 1 + 1)

    if (!report || report_len == 0) {
        ESP_LOGD(TAG, "Roaming: RRM neighbor report is not valid");
        return NULL;
    }

    char *buf = (char *)calloc(1, MAX_NEIGHBOR_LEN);
    data = report;

    while (report_len >= 2 + NR_IE_MIN_LEN) {
        const uint8_t *nr;
        char lci[256 * 2 + 1];
        char civic[256 * 2 + 1];
        uint8_t nr_len = data[1];
        const uint8_t *pos = data, *end;

        if (pos[0] != WLAN_EID_NEIGHBOR_REPORT || nr_len < NR_IE_MIN_LEN) {
            ESP_LOGD(TAG, "Roaming CTRL: Invalid Neighbor Report element: id=%u len=%u", data[0], nr_len);
            ret = -1;
            goto cleanup;
        }

        if (2U + nr_len > report_len) {
            ESP_LOGD(TAG, "Roaming CTRL: Invalid Neighbor Report element: id=%u len=%zu nr_len=%u", data[0], report_len, nr_len);
            ret = -1;
            goto cleanup;
        }
        pos += 2;
        end = pos + nr_len;

        nr = pos;
        pos += NR_IE_MIN_LEN;

        lci[0] = '\0';
        civic[0] = '\0';
        while (end - pos > 2) {
            uint8_t s_id, s_len;

            s_id = *pos++;
            s_len = *pos++;
            if (s_len > end - pos) {
                ret = -1;
                goto cleanup;
            }
            if (s_id == WLAN_EID_MEASURE_REPORT && s_len > 3) {
                /* Measurement Token[1] */
                /* Measurement Report Mode[1] */
                /* Measurement Type[1] */
                /* Measurement Report[variable] */
                switch (pos[2]) {
                    case MEASURE_TYPE_LCI:
                        if (lci[0]) {
                            break;
                        }
                        memcpy(lci, pos, s_len);
                        break;
                    case MEASURE_TYPE_LOCATION_CIVIC:
                        if (civic[0]) {
                            break;
                        }
                        memcpy(civic, pos, s_len);
                        break;
                }
            }

            pos += s_len;
        }

        ESP_LOGI(TAG, "Roaming: RMM neighbor report bssid=" MACSTR " info=0x%x op_class=%u chan=%u phy_type=%u%s%s%s%s", MAC2STR(nr),
                 WPA_GET_LE32(nr + ETH_ALEN), nr[ETH_ALEN + 4], nr[ETH_ALEN + 5], nr[ETH_ALEN + 6], lci[0] ? " lci=" : "", lci,
                 civic[0] ? " civic=" : "", civic);


        LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                            "Roaming: RMM neighbor report BSSID: " + bssidToString((char *)nr) +
                                ", Channel: " + std::to_string(nr[ETH_ALEN + 5]));

        /* neighbor start */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, " neighbor=");
        /* bssid */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, MACSTR, MAC2STR(nr));
        /* , */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, ",");
        /* bssid info */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, "0x%04x", WPA_GET_LE32(nr + ETH_ALEN));
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, ",");
        /* operating class */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, "%u", nr[ETH_ALEN + 4]);
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, ",");
        /* channel number */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, "%u", nr[ETH_ALEN + 5]);
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, ",");
        /* phy type */
        len += snprintf(buf + len, MAX_NEIGHBOR_LEN - len, "%u", nr[ETH_ALEN + 6]);
        /* optional elements, skip */

        data = end;
        report_len -= 2 + nr_len;
    }

cleanup:
    if (ret < 0) {
        free(buf);
        buf = NULL;
    }
    return buf;
}


void neighbor_report_recv_cb(void *ctx, const uint8_t *report, size_t report_len)
{
    int *val = (int *)ctx;
    uint8_t *pos = (uint8_t *)report;
    int cand_list = 0;
    int ret;

    if (!report) {
        ESP_LOGD(TAG, "Roaming: Neighbor report is null");
        return;
    }
    if (*val != rrm_ctx) {
        ESP_LOGE(TAG, "Roaming: rrm_ctx value didn't match, not initiated by us");
        return;
    }
    /* dump report info */
    ESP_LOGD(TAG, "Roaming: RRM neighbor report len=%d", report_len);
    ESP_LOG_BUFFER_HEXDUMP(TAG, pos, report_len, ESP_LOG_DEBUG);

    /* create neighbor list */
    char *neighbor_list = get_btm_neighbor_list(pos + 1, report_len - 1);

    /* In case neighbor list is not present issue a scan and get the list from that */
    if (!neighbor_list) {
        /* issue scan */
        wifi_scan_config_t params;
        memset(&params, 0, sizeof(wifi_scan_config_t));
        if (esp_wifi_scan_start(&params, true) < 0) {
            goto cleanup;
        }
        /* cleanup from net802.11 */
        uint16_t number = 1;
        wifi_ap_record_t ap_records;
        esp_wifi_scan_get_ap_records(&number, &ap_records);
        cand_list = 1;
    }
    /* send AP btm query requesting to roam depending on candidate list of AP */
    // btm_query_reasons: https://github.com/espressif/esp-idf/blob/release/v4.4/components/wpa_supplicant/esp_supplicant/include/esp_wnm.h
    ret = esp_wnm_send_bss_transition_mgmt_query(REASON_FRAME_LOSS, neighbor_list, cand_list); // query reason 16 -> LOW RSSI -->
                                                                                               // (btm_query_reason)16
    ESP_LOGD(TAG, "neighbor_report_recv_cb retVal - bss_transisition_query: %d", ret);

cleanup:
    if (neighbor_list) {
        free(neighbor_list);
    }
}


static void esp_bss_rssi_low_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    int retVal = -1;
    wifi_event_bss_rssi_low_t *event = (wifi_event_bss_rssi_low_t *)event_data;

    LogFile.writeToFile(ESP_LOG_DEBUG, TAG,
                        "Roaming Event: RSSI " + std::to_string(event->rssi) + " < RSSI_Threshold " +
                            std::to_string(cfgDataPtr->wlan.wlanRoaming.rssiThreshold));

    /* If RRM is supported, call RRM and then send BTM query to AP */
    if (esp_rrm_is_rrm_supported_connection() && esp_wnm_is_btm_supported_connection()) {
        /* Lets check channel conditions */
        rrm_ctx++;

        retVal = esp_rrm_send_neighbor_rep_request(neighbor_report_recv_cb, &rrm_ctx);
        ESP_LOGD(TAG, "esp_rrm_send_neighbor_rep_request retVal: %d", retVal);
        if (retVal == 0) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Roaming: RRM + BTM query sent");
        }
        else {
            ESP_LOGD(TAG, "esp_rrm_send_neighbor_rep_request retVal: %d", retVal);
        }
    }

    /* If RRM is not supported or RRM request failed, send directly BTM query to AP */
    if (retVal < 0 && esp_wnm_is_btm_supported_connection()) {
        // btm_query_reasons:
        // https://github.com/espressif/esp-idf/blob/release/v4.4/components/wpa_supplicant/esp_supplicant/include/esp_wnm.h
        retVal = esp_wnm_send_bss_transition_mgmt_query(REASON_FRAME_LOSS, NULL, 0); // query reason 16 -> LOW RSSI --> (btm_query_reason)16
        ESP_LOGD(TAG, "esp_wnm_send_bss_transition_mgmt_query retVal: %d", retVal);
        if (retVal == 0) {
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Roaming: BTM query sent");
        }
        else {
            ESP_LOGD(TAG, "esp_wnm_send_bss_transition_mgmt_query retVal: %d", retVal);
        }
    }
}


void printRoamingFeatureSupport(void)
{
    if (esp_rrm_is_rrm_supported_connection()) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Roaming: RRM (802.11k) supported by AP");
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Roaming: RRM (802.11k) NOT supported by AP");
    }

    if (esp_wnm_is_btm_supported_connection()) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Roaming: BTM (802.11v) supported by AP");
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Roaming: BTM (802.11v) NOT supported by AP");
    }
}


#ifdef WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES
void wifiRoamingQuery(void)
{
    /* Query only if WIFI is connected and feature is supported by AP */
    if (wifiState.connected && (esp_rrm_is_rrm_supported_connection() || esp_wnm_is_btm_supported_connection())) {
        /* Client is allowed to send query to AP for roaming request if RSSI is lower than threshold */
        /* Note 1: Set RSSI threshold funtion needs to be called to trigger WIFI_EVENT_STA_BSS_RSSI_LOW */
        /* Note 2: Additional querys will be sent after flow cycle is finshed --> server_tflite.cpp - function "task_autodoFlow" */
        /* Note 3: RSSI_Threshold = 0 --> Disable client query by application (WebUI parameter) */

        if (cfgDataPtr->wlan.wlanRoaming.rssiThreshold != 0 && getWifiRssi() != -127 &&
            getWifiRssi() < cfgDataPtr->wlan.wlanRoaming.rssiThreshold) {
            esp_wifi_set_rssi_threshold(cfgDataPtr->wlan.wlanRoaming.rssiThreshold);
        }
    }
}
#endif // WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES
#endif // WLAN_USE_MESH_ROAMING


std::string getNetworkOpmode(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        return "WLAN Access Point";
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        return "WLAN Client";
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_DISABLED) {
        return "Disabled";
    }

    return "Unknown";
}


std::string getMac(void)
{
    uint8_t macInt[6];

    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        esp_read_mac(macInt, ESP_MAC_WIFI_SOFTAP);
        return macToString(macInt);
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        esp_read_mac(macInt, ESP_MAC_WIFI_STA);
        return macToString(macInt);
    }

    return "";
}


bool getDhcpStatus(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        return true;
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        if (ConfigClass::getInstance()->get()->sectionNetwork.wlan.ipv4.networkConfig == NETWORK_WLAN_IP_CONFIG_DHCP) {
            return true;
        }
        return false;
    }

    return true;
}


std::string getIpAddress(void)
{
    return ipCfg.ipAddress;
}


std::string getNetmaskAddress(void)
{
    return ipCfg.subnetMask;
}


std::string getGatewayAddress(void)
{
    return ipCfg.gatewayAddress;
}


std::string getDnsAddress(void)
{
    return ipCfg.dnsServer;
}


std::string getWifiSsid(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        return ConfigClass::getInstance()->get()->sectionNetwork.wlanAp.ssid;
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        return ConfigClass::getInstance()->get()->sectionNetwork.wlan.ssid;
    }

    return "";
}


std::string getHostname(void)
{
    return ConfigClass::getInstance()->get()->sectionNetwork.wlan.hostname;
}


int getWifiChannel(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        return ConfigClass::getInstance()->get()->sectionNetwork.wlanAp.channel;
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        wifi_config_t wifiConfig;
        if (esp_wifi_get_config(WIFI_IF_STA, &wifiConfig) == ESP_OK) {
            return (int)wifiConfig.sta.channel;
        }
    }

    return -1;
}


int getWifiRssi(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        wifi_sta_list_t clientList;
        if (esp_wifi_ap_get_sta_list(&clientList) == ESP_OK && clientList.num > 0) {
            return (int)clientList.sta[0].rssi; // Return RSSI of first connected client
        }
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        wifi_ap_record_t apInfo;
        if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
            return apInfo.rssi;
        }
    }

    return -127; // Not connected or no info
}


bool getWifiIsConnected(bool improvProvisioning)
{
    if (improvProvisioning) {
        return wifiState.connected;
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || wifiState.fallbackApActive) {
        wifi_sta_list_t clientList;
        if (esp_wifi_ap_get_sta_list(&clientList) == ESP_OK && clientList.num > 0) {
            return true;
        }
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        return wifiState.connected;
    }

    return false;
}


/** Return Wifi connection status
 * @return
 * - WIFI_CONNECTION_NOT_INITIALIZED wifi not intialized
 * - WIFI_CONNECTION_INITIALIZED wifi initialized
 * - WIFI_CONNECTION_CONNECTED wifi connected
 * - WIFI_CONNECTION_DISCONNECTED wifi disconnected
 * - WIFI_CONNECTION_SUSPENDED wifi connection suspended
 */
wifi_connection_status_t getWifiConnectionStatus(void)
{
    if (wifiState.initialized && wifiState.connectionSupended) {
        return WIFI_CONNECTION_SUSPENDED;
    }
    else if (wifiState.initialized && getWifiIsConnected()) {
        return WIFI_CONNECTION_CONNECTED;
    }
    else if (wifiState.initialized && !getWifiIsConnected()) {
        return WIFI_CONNECTION_DISCONNECTED;
    }
    else if (wifiState.initialized) {
        return WIFI_CONNECTION_INITIALIZED;
    }

    return WIFI_CONNECTION_NOT_INITIALIZED;
}


void deinitWifi(void)
{
    wifiState.initialized = false;

    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler);
#ifdef WLAN_USE_MESH_ROAMING
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW, esp_bss_rssi_low_handler);
#endif

    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}
