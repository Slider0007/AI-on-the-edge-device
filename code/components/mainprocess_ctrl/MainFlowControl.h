#ifndef MAINFLOWCONTROL_H
#define MAINFLOWCONTROL_H

#include <string>

#include <esp_log.h>
#include <esp_http_server.h>

#include "CImageBasis.h"
#include "ClassFlowControl.h"


extern ClassFlowControl flowctrl;

esp_err_t triggerReloadConfig(httpd_req_t *req);

#ifdef ENABLE_MQTT
esp_err_t triggerFlowStartByMqtt(std::string _topic);
#endif // ENABLE_MQTT
void triggerFlowStartByGpio();

void setTaskAutoFlowState(int _value);
int getTaskAutoFlowState();

std::string getProcessStatus();
int getFlowCycleCounter();
int getFlowProcessingTime();

void createMainFlowTask();
void deleteMainFlowTask();

void registerMainFlowTaskUri(httpd_handle_t server);

#endif // MAINFLOWCONTROL_H
