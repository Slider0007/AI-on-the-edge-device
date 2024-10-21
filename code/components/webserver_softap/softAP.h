#include "../../include/defines.h"
#ifdef ENABLE_SOFTAP

#ifndef SOFTAP_H
#define SOFTAP_H

#include <esp_http_server.h>

void checkStartAPMode(void);
void wifiDeinitAP(void);

#endif  //SOFTAP_H
#endif //#ifdef ENABLE_SOFTAP