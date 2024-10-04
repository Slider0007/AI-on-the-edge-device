#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef INTERFACE_INFLUXDBV2_H
#define INTERFACE_INFLUXDBV2_H

#include <string>

#include "configClass.h"

bool influxDBv2Init(const CfgData::SectionInfluxDBv2 *_cfgDataPtr);
esp_err_t influxDBv2Publish(const std::string &_measurement, const std::string &_fieldkey1,
                            const std::string &_fieldvalue1, const std::string &_timestamp);
bool getInfluxDBv2isEncrypted();

#endif //INTERFACE_INFLUXDBV2_H
#endif //ENABLE_INFLUXDB