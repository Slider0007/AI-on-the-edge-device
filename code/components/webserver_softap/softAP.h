#include "../../include/defines.h"

#ifndef SOFTAP_H
#define SOFTAP_H

#include <esp_http_server.h>

void startAPForDeviceProvisioning(void);
void stopAPForDeviceProvisioning(void);

#endif // SOFTAP_H