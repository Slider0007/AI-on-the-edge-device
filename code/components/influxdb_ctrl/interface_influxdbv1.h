#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef INTERFACE_INFLUXDBV1_H
#define INTERFACE_INFLUXDBV1_H

#include <string>

#include "configClass.h"

bool influxDBv1Init(const CfgData::SectionInfluxDBv1 *_cfgDataPtr);
esp_err_t influxDBv1Publish(const std::string &_measurement, const std::string &_fieldkey1,
                            const std::string &_fieldvalue1, const std::string &_timestamp);
bool getInfluxDBv1isEncrypted();

#endif //INTERFACE_INFLUXDBV1_H
#endif //ENABLE_INFLUXDB