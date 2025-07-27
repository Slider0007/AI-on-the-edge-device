#ifndef CONNECT_WLAN_H
#define CONNECT_WLAN_H

#include <string>

#include <esp_err.h>
#include <esp_http_server.h>


esp_err_t initWifi(void);
esp_err_t initWifiClient(void);
esp_err_t initWifiAp(bool _useDefaultConfig = false);

bool suspendWifiConnection(void);
bool resumeWifiConnection(std::string source = "unknown");

#if (defined WLAN_USE_MESH_ROAMING && defined WLAN_USE_MESH_ROAMING_ACTIVATE_CLIENT_TRIGGERED_QUERIES)
void wifiRoamingQuery(void);
#endif // WLAN_USE_MESH_ROAMING

#ifdef WLAN_USE_ROAMING_BY_SCANNING
void wifiRoamByScanning(void);
#endif // WLAN_USE_ROAMING_BY_SCANNING

esp_err_t wifiScan(httpd_req_t *req = NULL, bool checkRoaming = false);

bool getWlanConnectionState(bool improvProvisioning = false);
bool getWlanDhcpStatus(void);
std::string getWlanIpAddress(void);
std::string getWlanNetmaskAddress(void);
std::string getWlanGatewayAddress(void);
std::string getWlanDnsAddress(void);
std::string getWlanMac(void);

std::string getWifiSsid(void);
int getWifiChannel(void);
int getWifiRssi(void);

bool getWlanFallbackActive(void);

void deinitWifi(void);

#endif // CONNECT_WLAN_H
