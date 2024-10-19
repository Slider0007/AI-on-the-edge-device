#include "../../include/defines.h"
#ifdef ENABLE_WEBHOOK

#ifndef INTERFACE_WEBHOOK_H
#define INTERFACE_WEBHOOK_H

#include <string>

#include "configClass.h"
#include "CImageBasis.h"

bool webhookInit(const CfgData::SectionWebhook *_cfgDataPtr);
esp_err_t webhookPublish(const char* _data, ImageData *_imgData = NULL, time_t _imageTimeProcessed = 0L);
bool getWebhookIsEncrypted();

#endif //INTERFACE_WEBHOOK_H
#endif //ENABLE_WEBHOOK