#ifndef CLASSLOGFILE_H
#define CLASSLOGFILE_H

#include <string>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include "../../include/defines.h"

#include "ClassMeterModel.h"


class ClassLogFile
{
  private:
    SemaphoreHandle_t logfileMutex;
    FILE *logfileHandle;

#ifdef KEEP_LOGFILE_OPEN_FOR_APPENDING
    std::string logfileNameOpen;
#endif // KEEP_LOGFILE_OPEN_FOR_APPENDING

    std::string logFileRootFolder;
    std::string logFileTimeFormat;
    std::string dataFileRootFolder;
    std::string dataFileTimeFormat;
    std::string debugFileRootFolder;
    std::string debugFolderTimeFormat;
    int logFileRetentionInDays;
    int dataLogRetentionInDays;
    int debugFilesRetentionInDays;
    bool dataLogToSDEnabled;
    esp_log_level_t loglevel;

  public:
    ClassLogFile();

    void writeHeapInfo(std::string _id);

    void setLogLevel(esp_log_level_t _logLevel);
    void setLogFileRetention(int _LogFileRetentionInDays);
    void setDataLogRetention(int _DataLogRetentionInDays);
    void setDebugFilesRetention(int _DebugFilesRetentionInDays);
    void enableDataLogToSD(bool _dataLogToSDEnabled);
    bool getDataLogToSDStatus();

    void writeToFile(esp_log_level_t level, std::string tag, std::string message, bool _time);
    void writeToFile(esp_log_level_t level, std::string tag, std::string message);
    bool createLogDirectories();
    void removeOldLogFile();
    void removeOldDataLog();
    void removeOldDebugFiles();

    void writeToData(const SequenceData &sequence, const MeterModel::Result &modelResult);

    std::string getCurrentFileName();
    std::string getCurrentFileNameData();
};

extern ClassLogFile LogFile;

#endif // CLASSLOGFILE_H
