#ifndef CONNECT_WLAN_H
#define CONNECT_WLAN_H

#include <string>

#include <esp_err.h>

typedef enum WIFI_CONNECTION_STATUS {
    WIFI_CONNECTION_NOT_INITIALIZED = 0,
    WIFI_CONNECTION_INITIALIZED = 1,
    WIFI_CONNECTION_CONNECTED = 2,
    WIFI_CONNECTION_DISCONNECTED = 3,
    WIFI_CONNECTION_SUSPENDED = 4
} wifi_connection_status_t;


esp_err_t initWifi(void);
esp_err_t initWifiClient(void);
esp_err_t initWifiAp(bool _useDefaultConfig = false);

bool suspendWifiConnection(void);
bool resumeWifiConnection(std::string source = "unknown");

#if (defined WLAN_USE_MESH_ROAMING && defined WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES)
void wifiRoamingQuery(void);
#endif

#ifdef WLAN_USE_ROAMING_BY_SCANNING
void wifiRoamByScanning(void);
#endif

std::string getNetworkOpmode(void);
std::string getMac(void);
bool getDhcpStatus(void);
std::string getIpAddress(void);
std::string getNetmaskAddress(void);
std::string getGatewayAddress(void);
std::string getDnsAddress(void);
std::string getWifiSsid(void);
std::string getHostname(void);
int getWifiChannel(void);
int getWifiRssi(void);
bool getWifiIsConnected(bool improvProvisioning = false);
wifi_connection_status_t getWifiConnectionStatus(void);

void deinitWifi(void);

#endif //CONNECT_WLAN_H