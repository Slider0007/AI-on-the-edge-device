#ifndef NETWORK_MAIN_H
#define NETWORK_MAIN_H

#include <string>

#include <esp_err.h>

#include "../../include/defines.h"

typedef enum {
    NETWORK_OPMODE_TYPE_UNKNOWN = -1,
    NETWORK_OPMODE_TYPE_DISABLED = 0,
    NETWORK_OPMODE_TYPE_WLAN = 1,
    NETWORK_OPMODE_TYPE_WLAN_AP = 2,
#ifdef BOARD_FEATURE_ETHERNET
    NETWORK_OPMODE_TYPE_ETHERNET = 3,
#endif // BOARD_FEATURE_ETHERNET
    NETWORK_OPMODE_TYPE_MAX,
} NetworkOpModeType;


esp_err_t initNetwork();
void deinitNetwork();

NetworkOpModeType getNetworkOpmodeType(void);
std::string getNetworkOpmode(void);
std::string getHostname(void);
bool getDhcpStatus(void);
std::string getIpAddress(void);
std::string getNetmaskAddress(void);
std::string getGatewayAddress(void);
std::string getDnsAddress(void);
std::string getMac(void);

bool getNetworkConnectionState(bool improvProvisioning = false);

#endif // NETWORK_MAIN_H
