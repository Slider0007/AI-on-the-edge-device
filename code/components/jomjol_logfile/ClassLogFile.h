#pragma once

#ifndef CLASSLOGFILE_H
#define CLASSLOGFILE_H


#include <string>
#include "esp_log.h"


class ClassLogFile
{
private:
    std::string logroot;
    std::string logfile;
    std::string dataroot;
    std::string datafile;
    std::string errorroot;
    std::string errorfolder;
    int logFileRetentionInDays;
    int dataLogRetentionInDays;
    int errorLogRetentionInDays;
    bool doDataLogToSD;
    esp_log_level_t loglevel;
public:
    ClassLogFile(std::string _logroot, std::string _logfile, std::string _logdatapath,std::string _datafile, std::string _errorroot, std::string errorfolder);

    void WriteHeapInfo(std::string _id);

    void setLogLevel(esp_log_level_t _logLevel);
    void SetLogFileRetention(int _LogFileRetentionInDays);
    void SetDataLogRetention(int _DataLogRetentionInDays);
    void SetErrorLogRetention(int _ErrorLogRetentionInDays);
    void SetDataLogToSD(bool _doDataLogToSD);
    bool GetDataLogToSD();

    void WriteToFile(esp_log_level_t level, std::string tag, std::string message, bool _time);
    void WriteToFile(esp_log_level_t level, std::string tag, std::string message);

    void CloseLogFileAppendHandle();

    bool CreateLogDirectories();
    void RemoveOldLogFile();
    void RemoveOldDataLog();
    void RemoveOldErrorLog();

//    void WriteToData(std::string _ReturnRawValue, std::string _ReturnValue, std::string _ReturnPreValue, std::string _ErrorMessageText, std::string _digital, std::string _analog);
    void WriteToData(std::string _timestamp, std::string _name, std::string  _ReturnRawValue, std::string  _ReturnValue, std::string  _ReturnPreValue, std::string  _ReturnRateValue, std::string  _ReturnChangeAbsolute, std::string  _ErrorMessageText, std::string  _digital, std::string  _analog);


    std::string GetCurrentFileName();
    std::string GetCurrentFileNameData();
};

extern ClassLogFile LogFile;

#endif //CLASSLOGFILE_H