#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include <string>

#include <esp_err.h>


esp_err_t mDnsInit(std::string hostname);
void mDnsDeinit(void);

#endif // MDNS_SERVICE_H
