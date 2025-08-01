#include "../../include/defines.h"

#ifdef BOARD_FEATURE_ETHERNET

#ifndef CONNECT_ETHERNET_H
#define CONNECT_ETHERNET_H

#include <string>

#include <esp_err.h>


esp_err_t initEthernetW5500();

bool getEthernetConnectionState();
bool getEthDhcpStatus(void);
std::string getEthIpAddress(void);
std::string getEthNetmaskAddress(void);
std::string getEthGatewayAddress(void);
std::string getEthDnsAddress(void);
std::string getEthMac(void);

void deinitEthernet();

#endif // CONNECT_ETHERNET_H

#endif // BOARD_FEATURE_ETHERNET