#include "network_main.h"

#ifdef BOARD_FEATURE_ETHERNET
#include "connect_ethernet.h"
#endif // BOARD_FEATURE_ETHERNET

#include "connect_wlan.h"
#include "softAP.h"
#include "configClass.h"
#include "ClassLogFile.h"
#include "statusled.h"


static const char *TAG = "NETWORK";

static const CfgData::SectionNetwork *cfgDataPtr = NULL;
static bool ethernetFallbackActive = false;


esp_err_t initNetwork(void)
{
    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionNetwork;

    if (cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_CLIENT || cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init WLAN connection");
        esp_err_t retVal = initWifiClient();
        if (retVal != ESP_OK) {
            setStatusLed(NETWORK_INIT, 1, true);
        }
        return retVal;
    }
    else if (cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_AP || cfgDataPtr->opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init WLAN connection");
        esp_err_t retVal = initWifiAp();
        if (retVal != ESP_OK) {
            setStatusLed(NETWORK_INIT, 1, true);
        }
        return retVal;
    }

#ifdef BOARD_FEATURE_ETHERNET
    else if (cfgDataPtr->opmode == NETWORK_OPMODE_ETHERNET_ONLY) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init ethernet connection");
        esp_err_t retVal = initEthernetW5500();
        if (retVal != ESP_OK) {
            setStatusLed(NETWORK_INIT, 2, true);
        }
        return retVal;
    }
    else if (cfgDataPtr->opmode == NETWORK_OPMODE_ETHERNET_FALLBACK_WLAN) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init ethernet connection");
        esp_err_t retVal = initEthernetW5500();
        if (retVal != ESP_OK) {
            setStatusLed(NETWORK_INIT, 2, true);
            return retVal;
        }

        if (!getEthernetConnectionState()) {
            deinitEthernet();
            LogFile.writeToFile(ESP_LOG_WARN, TAG, "Fallback: Init WLAN access point");
            ethernetFallbackActive = true;
            esp_err_t retVal = initWifiAp();
            if (retVal != ESP_OK) {
                setStatusLed(NETWORK_INIT, 1, true);
            }
        }

        return retVal;
    }
#endif // BOARD_FEATURE_ETHERNET

    else if (cfgDataPtr->opmode == NETWORK_OPMODE_DISABLED) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "All networks disabled");
        return ESP_OK;
    }

    return ESP_FAIL;
}


void deinitNetwork(void)
{
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN || getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN_AP) {
        deinitWifi();
    }
#ifdef BOARD_FEATURE_ETHERNET
    else if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        deinitEthernet();
    }
#endif // BOARD_FEATURE_ETHERNET
}


NetworkOpModeType getNetworkOpmodeType(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP ||
        ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_AP_TIMED_OFF || getWlanFallbackActive() ||
        ethernetFallbackActive || getDeviceProvisioningByApStarted()) {
        return NETWORK_OPMODE_TYPE_WLAN_AP;
    }
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_WLAN_CLIENT_TIMED_OFF) {
        return NETWORK_OPMODE_TYPE_WLAN;
    }

#ifdef BOARD_FEATURE_ETHERNET
    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_ETHERNET_ONLY ||
             ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_ETHERNET_FALLBACK_WLAN) {
        return NETWORK_OPMODE_TYPE_ETHERNET;
    }
#endif // BOARD_FEATURE_ETHERNET

    else if (ConfigClass::getInstance()->get()->sectionNetwork.opmode == NETWORK_OPMODE_DISABLED) {
        return NETWORK_OPMODE_TYPE_DISABLED;
    }

    return NETWORK_OPMODE_TYPE_UNKNOWN;
}


std::string getNetworkOpmode(void)
{
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN_AP) {
        return "WLAN Access Point";
    }
    else if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN) {
        return "WLAN Client";
    }

#ifdef BOARD_FEATURE_ETHERNET
    else if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return "Ethernet";
    }
#endif // BOARD_FEATURE_ETHERNET

    else if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_DISABLED) {
        return "Disabled";
    }

    return "Unknown";
}


bool getNetworkConnectionState(bool improvProvisioning)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthernetConnectionState();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanConnectionState(improvProvisioning);
}


bool getDhcpStatus(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthDhcpStatus();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanDhcpStatus();
}


std::string getIpAddress(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthIpAddress();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanIpAddress();
}


std::string getNetmaskAddress(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthNetmaskAddress();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanNetmaskAddress();
}


std::string getGatewayAddress(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthGatewayAddress();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanGatewayAddress();
}


std::string getDnsAddress(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthDnsAddress();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanDnsAddress();
}


std::string getMac(void)
{
#ifdef BOARD_FEATURE_ETHERNET
    if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
        return getEthMac();
    }
#endif // BOARD_FEATURE_ETHERNET

    return getWlanMac();
}


std::string getHostname(void)
{
    return ConfigClass::getInstance()->get()->sectionNetwork.hostname;
}
