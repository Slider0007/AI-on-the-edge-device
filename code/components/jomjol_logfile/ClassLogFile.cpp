#include "ClassLogFile.h"
#include "time_sntp.h"
#include "esp_log.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include "Helper.h"
#include "time_sntp.h"
#include "../../include/defines.h"

static const char *TAG = "LOGFILE";

ClassLogFile LogFile("/sdcard/log/message", "log_%Y-%m-%d.txt", 
                     "/sdcard/log/data", "data_%Y-%m-%d.csv",
                     "/sdcard/log/debug", LOGFILE_TIME_FORMAT);



ClassLogFile::ClassLogFile(std::string _logFileRootFolder, std::string _logfile, std::string _dataFileRootFolder,
                           std::string _datafile, std::string _debugFileRootFolder, std::string _debugfolder)
{
    logFileRootFolder = _logFileRootFolder;
    logfile = _logfile;
    dataFileRootFolder = _dataFileRootFolder;
    datafile = _datafile;
    debugFileRootFolder = _debugFileRootFolder;
    debugfolder = _debugfolder;
    logFileRetentionInDays = 5;
    dataLogRetentionInDays = 5;
    debugFilesRetentionInDays = 5;
    doDataLogToSD = true;
    loglevel = ESP_LOG_INFO;
}


void ClassLogFile::WriteHeapInfo(std::string _id)
{
    if (loglevel >= ESP_LOG_DEBUG) {
        std::string _zw =  _id + "\t" + getESPHeapInfo();
        WriteToFile(ESP_LOG_DEBUG, "HEAP", _zw);
    }
}


void ClassLogFile::WriteToData(std::string _timestamp, std::string _name, std::string  _ReturnRawValue, std::string  _ReturnValue, std::string  _ReturnPreValue, std::string  _ReturnRateValue, std::string  _ReturnChangeAbsolute, std::string  _ErrorMessageText, std::string  _digital, std::string  _analog)
{
    ESP_LOGD(TAG, "Start WriteToData");
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[30];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 30, datafile.c_str(), timeinfo);
    std::string logpath = dataFileRootFolder + "/" + buffer; 
    
    FILE* pFile;
    std::string zwtime;

    ESP_LOGD(TAG, "Datalogfile: %s", logpath.c_str());
    pFile = fopen(logpath.c_str(), "a+");

    if (pFile!=NULL) {
        fputs(_timestamp.c_str(), pFile);
        fputs(",", pFile);
        fputs(_name.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ReturnRawValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ReturnValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ReturnPreValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ReturnRateValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ReturnChangeAbsolute.c_str(), pFile);
        fputs(",", pFile);
        fputs(_ErrorMessageText.c_str(), pFile);
        fputs(_digital.c_str(), pFile);
        fputs(_analog.c_str(), pFile);
        fputs("\n", pFile);

        fclose(pFile);    
    } else {
        ESP_LOGE(TAG, "Can't open data file %s", logpath.c_str());
    }

}


void ClassLogFile::setLogLevel(esp_log_level_t _logLevel)
{
    std::string levelText;

    // Print log level to log file
    switch(_logLevel) {            
        case ESP_LOG_WARN:
            levelText = "WARNING";
            break;
            
        case ESP_LOG_INFO:
            levelText = "INFO";
            break;
            
        case ESP_LOG_DEBUG:
            levelText = "DEBUG";
            break;
    
        case ESP_LOG_ERROR:
        default:
            levelText = "ERROR";
            break;
    }
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Set log level to " + levelText);

    // set new log level
    loglevel = _logLevel;

    /*
    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Test");
    LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Test");
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Test");
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Test");
    */
}


void ClassLogFile::SetLogFileRetention(int _LogFileRetentionInDays)
{
    logFileRetentionInDays = _LogFileRetentionInDays;
}


void ClassLogFile::SetDataLogRetention(int _DataLogRetentionInDays)
{
    dataLogRetentionInDays = _DataLogRetentionInDays;
}


void ClassLogFile::SetDebugFilesRetention(int _DebugFilesRetentionInDays)
{
    debugFilesRetentionInDays = _DebugFilesRetentionInDays;
}


void ClassLogFile::SetDataLogToSD(bool _doDataLogToSD)
{
    doDataLogToSD = _doDataLogToSD;
}


bool ClassLogFile::GetDataLogToSD()
{
    return doDataLogToSD;
}


static FILE* logFileAppendHandle = NULL;
std::string fileNameDate;

void ClassLogFile::WriteToFile(esp_log_level_t level, std::string tag, std::string message, bool _time)
{
    time_t rawtime;
    struct tm* timeinfo;
    std::string fileNameDateNew;

    std::string zwtime;
    std::string ntpTime = "";


    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char buf[30];
    strftime(buf, sizeof(buf), logfile.c_str(), timeinfo);
    fileNameDateNew = std::string(buf);

    std::replace(message.begin(), message.end(), '\n', ' '); // Replace all newline characters

    if (tag != "") {
        ESP_LOG_LEVEL(level, tag.c_str(), "%s", message.c_str());
        message = "[" + tag + "] " + message;
    }
    else {
        ESP_LOG_LEVEL(level, "", "%s", message.c_str());
    }
    

    if (level > loglevel) {// Only write to file if loglevel is below threshold
        return;
    }


    if (_time)
    {
        char logLineDate[30];
        strftime(logLineDate, sizeof(logLineDate), "%Y-%m-%dT%H:%M:%S", timeinfo);
        ntpTime = std::string(logLineDate);
    }

    std::string loglevelString; 
    switch(level) {
        case  ESP_LOG_ERROR:
            loglevelString = "ERR";
            break;
        case  ESP_LOG_WARN:
            loglevelString = "WRN";
            break;
        case  ESP_LOG_INFO:
            loglevelString = "INF";
            break;
        case  ESP_LOG_DEBUG:
            loglevelString = "DBG";
            break;
        case  ESP_LOG_VERBOSE:
            loglevelString = "VER";
            break;
        case  ESP_LOG_NONE:
        default:
            loglevelString = "NONE";
            break;
    }

    std::string formatedUptime = getFormatedUptime(true);

    std::string fullmessage = "[" + formatedUptime + "] "  + ntpTime + "\t<" + loglevelString + ">\t" + message + "\n";


#ifdef KEEP_LOGFILE_OPEN_FOR_APPENDING
    if (fileNameDateNew != fileNameDate) { // Filename changed
        // Make sure each day gets its own logfile
        // Also we need to re-open it in case it needed to get closed for reading
        std::string logpath = logFileRootFolder + "/" + fileNameDateNew; 

        ESP_LOGI(TAG, "Opening logfile %s for appending", logpath.c_str());
        logFileAppendHandle = fopen(logpath.c_str(), "a+");
        if (logFileAppendHandle==NULL) {
            ESP_LOGE(TAG, "Can't open log file %s", logpath.c_str());
            return;
        }

        fileNameDate = fileNameDateNew;
    }
#else
    std::string logpath = logFileRootFolder + "/" + fileNameDateNew; 
    logFileAppendHandle = fopen(logpath.c_str(), "a+");
    if (logFileAppendHandle==NULL) {
        ESP_LOGE(TAG, "Can't open log file %s", logpath.c_str());
        return;
    }
  #endif

    fputs(fullmessage.c_str(), logFileAppendHandle);
    
#ifdef KEEP_LOGFILE_OPEN_FOR_APPENDING
    fflush(logFileAppendHandle);
    fsync(fileno(logFileAppendHandle));
#else
    CloseLogFileAppendHandle();
#endif
}


void ClassLogFile::CloseLogFileAppendHandle()
{
    if (logFileAppendHandle != NULL) {
        fclose(logFileAppendHandle);
        logFileAppendHandle = NULL;
        fileNameDate = "";
    }
}


void ClassLogFile::WriteToFile(esp_log_level_t level, std::string tag, std::string message)
{
    LogFile.WriteToFile(level, tag, message, true);
}


std::string ClassLogFile::GetCurrentFileNameData()
{
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[60];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 60, datafile.c_str(), timeinfo);
    std::string logpath = dataFileRootFolder + "/" + buffer; 

    return logpath;
}


std::string ClassLogFile::GetCurrentFileName()
{
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[60];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 60, logfile.c_str(), timeinfo);
    std::string logpath = logFileRootFolder + "/" + buffer; 

    return logpath;
}


void ClassLogFile::RemoveOldLogFile()
{
    if (logFileRetentionInDays == 0) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Remove old log files");

    time_t rawtime;
    struct tm* timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -logFileRetentionInDays + 1);
    timeinfo = localtime(&rawtime);
    //ESP_LOGD(TAG, "logFileRetentionInDays: %d", logFileRetentionInDays);


    strftime(cmpfilename, 30, logfile.c_str(), timeinfo);
    //ESP_LOGD(TAG, "log file name to compare: %s", cmpfilename);

    DIR *dir = opendir(logFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + logFileRootFolder);
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    std::string filepath;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGD(TAG, "compare log file: %s to %s", entry->d_name, cmpfilename);
            if ((strlen(entry->d_name) == strlen(cmpfilename)) && (strcmp(entry->d_name, cmpfilename) < 0)) {
                //ESP_LOGD(TAG, "delete log file: %s", entry->d_name);
                filepath = logFileRootFolder + "/" + entry->d_name;
                if ((strcmp(entry->d_name, "log_1970-01-01.txt") == 0) && getTimeWasNotSetAtBoot()) { // keep logfile log_1970-01-01.txt if time was not set at boot (some boot logs are in there)
                    //ESP_LOGD(TAG, "Skip deleting this file: %s", entry->d_name); 
                    notDeleted++;
                }
                else {          
                    if (unlink(filepath.c_str()) == 0) {
                        deleted++;
                    } 
                    else {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete file " + filepath);
                        notDeleted++;
                    }
                }
            } 
            else {
                notDeleted++;
            }
        }
    }
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Log files deleted: " + std::to_string(deleted) + ", Log files not deleted: " + std::to_string(notDeleted));
    closedir(dir);
}


void ClassLogFile::RemoveOldDataLog()
{
    if (dataLogRetentionInDays == 0 || !doDataLogToSD) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Remove old data files");

    time_t rawtime;
    struct tm* timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -dataLogRetentionInDays + 1);
    timeinfo = localtime(&rawtime);
    //ESP_LOGD(TAG, "dataLogRetentionInDays: %d", dataLogRetentionInDays);

    strftime(cmpfilename, 30, datafile.c_str(), timeinfo);
    //ESP_LOGD(TAG, "data file name to compare: %s", cmpfilename);

    DIR *dir = opendir(dataFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + dataFileRootFolder);
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    std::string filepath;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGD(TAG, "Compare data file: %s to %s", entry->d_name, cmpfilename);
            if ((strlen(entry->d_name) == strlen(cmpfilename)) && (strcmp(entry->d_name, cmpfilename) < 0)) {
                //ESP_LOGD(TAG, "delete data file: %s", entry->d_name);
                filepath = dataFileRootFolder + "/" + entry->d_name; 
                if (unlink(filepath.c_str()) == 0) {
                    deleted ++;
                } else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete file " + filepath);
                    notDeleted ++;
                }
            } else {
                notDeleted ++;
            }
        }
    }
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Data files deleted: " + std::to_string(deleted) + ", Data files not deleted: " + std::to_string(notDeleted));
    closedir(dir);
}


void ClassLogFile::RemoveOldDebugFiles()
{
    if (debugFilesRetentionInDays == 0) {
        return;
    }
    
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Remove old debug files");

    time_t rawtime;
    struct tm* timeinfo;
    char cmpfolderame[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -debugFilesRetentionInDays + 1);
    timeinfo = localtime(&rawtime);
    //ESP_LOGI(TAG, "debugFilesRetentionInDays: %d", debugFilesRetentionInDays);

    strftime(cmpfolderame, 30, debugfolder.c_str(), timeinfo);
    //ESP_LOGI(TAG, "delete folder and older than this: %s", cmpfolderame);

    DIR *dir = opendir(debugFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + debugFileRootFolder);
        return;
    }

    struct dirent *entry;
    struct dirent *subentry;
    DIR *subdir;
    std::string path, subfolder;
    int deleted = 0;
    int notDeleted = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            path = debugFileRootFolder + "/" + entry->d_name;
            //ESP_LOGI(TAG, "filepath (root + entry): %s", path.c_str());
            if (entry->d_type == DT_DIR) {
                subdir = opendir(path.c_str());
                if (!subdir) {
                    closedir(dir);
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + path);
                    return;
                }
                while ((subentry = readdir(subdir)) != NULL) {
                    subfolder = path + "/" + subentry->d_name;
                    //ESP_LOGI(TAG, "Compare subfolder: %s to %s", subentry->d_name, cmpfolderame);

                    if ((strlen(subentry->d_name) == strlen(cmpfolderame)) && (strcmp(subentry->d_name, cmpfolderame) < 0)) {
                        //ESP_LOGI(TAG, "delete subfolder (subdir + entry): %s", subfolder.c_str());
                        if (removeFolder(subfolder.c_str(), TAG) > 0)
                            deleted++;
                    }
                    else {
                        notDeleted++;
                    }
                }
                closedir(subdir);
            }
        }
    }
    closedir(dir);
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Debug files deleted: " + std::to_string(deleted) + ", Debug files not deleted: " + std::to_string(notDeleted));
}


bool ClassLogFile::CreateLogDirectories()
{
    bool bRetval = false;
    bRetval = MakeDir("/sdcard/log");
    bRetval = MakeDir("/sdcard/log/source");
    bRetval = MakeDir("/sdcard/log/digit");
    bRetval = MakeDir("/sdcard/log/analog");
    bRetval = MakeDir("/sdcard/log/message");
    bRetval = MakeDir("/sdcard/log/data");
    bRetval = MakeDir("/sdcard/log/debug");

    return bRetval;
}
