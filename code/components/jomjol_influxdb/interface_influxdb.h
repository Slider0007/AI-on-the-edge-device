#ifdef ENABLE_INFLUXDB

#pragma once
#ifndef INTERFACE_INFLUXDB_H
#define INTERFACE_INFLUXDB_H

#include <string>

// Interface to InfluxDB v1.x
bool InfluxDBInit(std::string _influxDBURI, std::string _database, std::string _user, std::string _password,
                    bool _TLSEncryption, std::string _TLSCACertFilename, std::string _TLSClientCertFilename, 
                    std::string _TLSClientKeyFilename);
void InfluxDBPublish(std::string _measurement, std::string _key, std::string _content, std::string _timestamp);

// Interface to InfluxDB v2.x
bool InfluxDBv2Init(std::string _uri, std::string _bucket, std::string _org, std::string _token,
                        bool _TLSEncryption, std::string _TLSCACertFilename, std::string _TLSClientCertFilename, 
                        std::string _TLSClientKeyFilename);
void InfluxDBv2Publish(std::string _measurement, std::string _key, std::string _content, std::string _timestamp);

void InfluxDBdestroy();

#endif //INTERFACE_INFLUXDB_H
#endif //ENABLE_INFLUXDB