#ifndef CLASSLOGFILE_H
#define CLASSLOGFILE_H


#include <string>

#include <esp_log.h>


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
        bool dataLogToSDEnabled;
        esp_log_level_t loglevel;


    public:
        ClassLogFile(std::string _logFileRootFolder, std::string _logfile, std::string _dataFileRootFolder,
                    std::string _datafile, std::string _debugFileRootFolder, std::string debugfolder);

        void writeHeapInfo(std::string _id);

        void setLogLevel(esp_log_level_t _logLevel);
        void setLogFileRetention(int _LogFileRetentionInDays);
        void setDataLogRetention(int _DataLogRetentionInDays);
        void setDebugFilesRetention(int _DebugFilesRetentionInDays);
        void enableDataLogToSD(bool _dataLogToSDEnabled);
        bool getDataLogToSDStatus();

        void writeToFile(esp_log_level_t level, std::string tag, std::string message, bool _time);
        void writeToFile(esp_log_level_t level, std::string tag, std::string message);

        void closeLogFileAppendHandle();

        bool createLogDirectories();
        void removeOldLogFile();
        void removeOldDataLog();
        void removeOldDebugFiles();

        void writeToData(std::string _timestamp, std::string _name, std::string  _sRawValue, std::string  _sValue,
                        std::string  _sFallbackValue, std::string  _sRatePerMin, std::string  _sRatePerInterval,
                        std::string  _sValueStatus, std::string  _digital, std::string  _analog);


        std::string getCurrentFileName();
        std::string getCurrentFileNameData();
};

extern ClassLogFile LogFile;

#endif //CLASSLOGFILE_H