#include "../../include/defines.h"
#ifdef ENABLE_INFLUXDB

#ifndef INTERFACE_INFLUXDBV2_H
#define INTERFACE_INFLUXDBV2_H

#include <string>

#include "configClass.h"

bool influxDBv2Init(const CfgData::SectionInfluxDBv2 *_cfgDataPtr);
void influxDBv2Publish(const std::string &_measurement, const std::string &_key, const std::string &_content, const std::string &_timestamp);
bool getInfluxDBv2isEncrypted();

#endif //INTERFACE_INFLUXDBV2_H
#endif //ENABLE_INFLUXDB