#ifndef CLASSLOGFILE_H
#define CLASSLOGFILE_H


#include <string>

#include "esp_log.h"


class ClassLogFile
{
    private:
        std::string logFileRootFolder;
        std::string logfile;
        std::string dataFileRootFolder;
        std::string datafile;
        std::string debugFileRootFolder;
        std::string debugfolder;
        int logFileRetentionInDays;
        int dataLogRetentionInDays;
        int debugFilesRetentionInDays;
        bool doDataLogToSD;
        esp_log_level_t loglevel;


    public:
        ClassLogFile(std::string _logFileRootFolder, std::string _logfile, std::string _dataFileRootFolder, std::string _datafile, std::string _debugFileRootFolder, std::string debugfolder);

        void WriteHeapInfo(std::string _id);

        void setLogLevel(esp_log_level_t _logLevel);
        void SetLogFileRetention(int _LogFileRetentionInDays);
        void SetDataLogRetention(int _DataLogRetentionInDays);
        void SetDebugFilesRetention(int _DebugFilesRetentionInDays);
        void SetDataLogToSD(bool _doDataLogToSD);
        bool GetDataLogToSD();

        void WriteToFile(esp_log_level_t level, std::string tag, std::string message, bool _time);
        void WriteToFile(esp_log_level_t level, std::string tag, std::string message);

        void CloseLogFileAppendHandle();

        bool CreateLogDirectories();
        void RemoveOldLogFile();
        void RemoveOldDataLog();
        void RemoveOldDebugFiles();

        void WriteToData(std::string _timestamp, std::string _name, std::string  _sRawValue, std::string  _sValue,
                        std::string  _sFallbackValue, std::string  _sRatePerMin, std::string  _sRatePerInterval,
                        std::string  _sValueStatus, std::string  _digital, std::string  _analog);


        std::string GetCurrentFileName();
        std::string GetCurrentFileNameData();
};

extern ClassLogFile LogFile;

#endif //CLASSLOGFILE_H