#include "mdns_service.h"

#include "mdns.h"

#include "ClassLogFile.h"
#include "helper.h"


static const char *TAG = "MDNS";


esp_err_t mDnsInit(std::string hostname)
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init mDNS service");

    esp_err_t retVal = mdns_init();
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to init mDNS service | Error: " + intToHexString(retVal));
        return retVal;
    }

    retVal = mdns_hostname_set(hostname.c_str());
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to set mDNS hostname | Error: " + intToHexString(retVal));
        return retVal;
    }

    return ESP_OK;
}


void mDnsDeinit()
{
    mdns_free();
}
