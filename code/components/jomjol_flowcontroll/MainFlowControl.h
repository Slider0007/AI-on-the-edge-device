#ifndef MAINFLOWCONTROL_H
#define MAINFLOWCONTROL_H

#include <string>

#include <esp_log.h>
#include <esp_http_server.h>

#include "CImageBasis.h"
#include "ClassFlowControll.h"


extern ClassFlowControll flowctrl;


#ifdef ENABLE_MQTT
esp_err_t triggerFlowStartByMqtt(std::string _topic);
#endif //ENABLE_MQTT
void triggerFlowStartByGpio();

void setTaskAutoFlowState(int _value);
int getTaskAutoFlowState();

std::string getProcessStatus();
int getFlowCycleCounter();
int getFlowProcessingTime();

void CreateMainFlowTask();
void DeleteMainFlowTask();

void register_server_main_flow_task_uri(httpd_handle_t server);

#endif //MAINFLOWCONTROL_H
