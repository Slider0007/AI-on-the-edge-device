#include "ClassLogFile.h"
#include "time_sntp.h"
#include "esp_log.h"
#include <cstring>
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

ClassLogFile LogFile("/sdcard/log/message", "log_%Y-%m-%d.txt", "/sdcard/log/data", "data_%Y-%m-%d.csv");


void ClassLogFile::WriteHeapInfo(std::string _id)
{
    if (loglevel >= ESP_LOG_DEBUG) {
        std::string _zw =  _id + "\t" + getESPHeapInfo();
        WriteToFile(ESP_LOG_DEBUG, "HEAP", _zw);
    }
}


void ClassLogFile::WriteToData(std::string _timestamp, std::string _name, std::string  _sRawValue, std::string _sValue, 
                               std::string _sFallbackValue, std::string  _sRatePerMin, std::string  _sRatePerProcessing, 
                               std::string _sValueStatus, std::string _digital, std::string _analog)
{
    //ESP_LOGD(TAG, "Start WriteToData");
    time_t rawtime;

    time(&rawtime);
    std::string logpath = dataroot + "/" + ConvertTimeToString(rawtime, datafile.c_str()); 
    
    FILE* pFile;
    std::string zwtime;

    //ESP_LOGD(TAG, "Datalogfile: %s", logpath.c_str());
    pFile = fopen(logpath.c_str(), "a+");

    if (pFile != NULL) {
        /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
        // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
        setvbuf(pFile, NULL, _IOFBF, 512);

        fputs(_timestamp.c_str(), pFile);
        fputs(",", pFile);
        fputs(_name.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sRawValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sFallbackValue.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sRatePerMin.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sRatePerProcessing.c_str(), pFile);
        fputs(",", pFile);
        fputs(_sValueStatus.c_str(), pFile);
        fputs(_digital.c_str(), pFile);
        fputs(_analog.c_str(), pFile);
        fputs("\n", pFile);

        fclose(pFile);    
    } 
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open data file: " + logpath);
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


void ClassLogFile::SetLogFileRetention(unsigned short _LogFileRetentionInDays){
    logFileRetentionInDays = _LogFileRetentionInDays;
}


void ClassLogFile::SetDataLogRetention(unsigned short _DataLogRetentionInDays){
    dataLogRetentionInDays = _DataLogRetentionInDays;
}


void ClassLogFile::SetDataLogToSD(bool _doDataLogToSD){
    doDataLogToSD = _doDataLogToSD;
}


bool ClassLogFile::GetDataLogToSD(){
    return doDataLogToSD;
}


static FILE* logFileAppendHandle = NULL;
std::string fileNameDate;

void ClassLogFile::WriteToFile(esp_log_level_t level, std::string tag, std::string message, bool _time)
{
    std::string fileNameDateNew;
    std::string zwtime;
    std::string ntpTime = "";

    time_t rawtime;

    time(&rawtime);
    fileNameDateNew = ConvertTimeToString(rawtime, logfile.c_str());

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

    if (_time) {
        ntpTime = ConvertTimeToString(rawtime, "%Y-%m-%dT%H:%M:%S");
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

    std::string fullmessage = "[" + getFormatedUptime(true) + "] "  + ntpTime + "\t<" + loglevelString + ">\t" + message + "\n";


    #ifdef KEEP_LOGFILE_OPEN_FOR_APPENDING
        if (fileNameDateNew != fileNameDate) { // Filename changed
            // Make sure each day gets its own logfile
            // Also we need to re-open it in case it needed to get closed for reading
            std::string logpath = logroot + "/" + fileNameDateNew; 

            ESP_LOGI(TAG, "Opening logfile %s for appending", logpath.c_str());
            logFileAppendHandle = fopen(logpath.c_str(), "a+");
            if (logFileAppendHandle==NULL) {
                ESP_LOGE(TAG, "Can't open log file %s", logpath.c_str());
                return;
            }

            fileNameDate = fileNameDateNew;
        }
    #else
        std::string logpath = logroot + "/" + fileNameDateNew; 
        logFileAppendHandle = fopen(logpath.c_str(), "a+");
        if (logFileAppendHandle==NULL) {
            ESP_LOGE(TAG, "Can't open log file %s", logpath.c_str());
            return;
        }
    #endif

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(logFileAppendHandle, NULL, _IOFBF, 512);

    fputs(fullmessage.c_str(), logFileAppendHandle);
    
    #ifdef KEEP_LOGFILE_OPEN_FOR_APPENDING
        fflush(logFileAppendHandle);
        fsync(fileno(logFileAppendHandle));
    #else
        CloseLogFileAppendHandle();
    #endif
}


void ClassLogFile::CloseLogFileAppendHandle() {
    if (logFileAppendHandle != NULL) {
        fclose(logFileAppendHandle);
        logFileAppendHandle = NULL;
        fileNameDate = "";
    }
}


void ClassLogFile::WriteToFile(esp_log_level_t level, std::string tag, std::string message) {
    LogFile.WriteToFile(level, tag, message, true);
}


std::string ClassLogFile::GetCurrentFileNameData()
{
    time_t rawtime;

    time(&rawtime);
    std::string logpath = dataroot + "/" + ConvertTimeToString(rawtime, datafile.c_str());

    return logpath;
}


std::string ClassLogFile::GetCurrentFileName()
{
    time_t rawtime;

    time(&rawtime);
    std::string logpath = logroot + "/" + ConvertTimeToString(rawtime, logfile.c_str()); 

    return logpath;
}


void ClassLogFile::RemoveOldLogFile()
{
    if (logFileRetentionInDays == 0) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Delete log files older than retention setting");

    time_t rawtime;
    struct tm timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -logFileRetentionInDays + 1);   
    localtime_r(&rawtime, &timeinfo);
    //ESP_LOGD(TAG, "logFileRetentionInDays: %d", logFileRetentionInDays);
    strftime(cmpfilename, sizeof(cmpfilename), logfile.c_str(), &timeinfo);
    //ESP_LOGD(TAG, "log file name to compare: %s", cmpfilename);

    DIR *dir = opendir(logroot.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory: " + logroot);
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGD(TAG, "compare log file: %s to %s", entry->d_name, cmpfilename);
            if ((strlen(entry->d_name) == strlen(cmpfilename)) && (strcmp(entry->d_name, cmpfilename) < 0)) {
                //ESP_LOGD(TAG, "delete log file: %s", entry->d_name);
                std::string filepath = logroot + "/" + entry->d_name;
                if ((strcmp(entry->d_name, "log_1970-01-01.txt") == 0) && getTimeWasNotSetAtBoot()) { // keep logfile log_1970-01-01.txt if time was not set at boot (some boot logs are in there)
                    //ESP_LOGD(TAG, "Skip deleting this file: %s", entry->d_name); 
                    notDeleted++;
                }
                else {          
                    if (unlink(filepath.c_str()) == 0) {
                        deleted++;
                    } 
                    else {
                        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete file: " + filepath);
                        notDeleted++;
                    }
                }
            } 
            else {
                notDeleted++;
            }
        }
    }

    closedir(dir);

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Files deleted: " + std::to_string(deleted) + " | Files kept: " + std::to_string(notDeleted));
}


void ClassLogFile::RemoveOldDataLog()
{
    if (dataLogRetentionInDays == 0 || !doDataLogToSD) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Delete data files older than retention setting");

    time_t rawtime;
    struct tm timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -dataLogRetentionInDays + 1);
    localtime_r(&rawtime, &timeinfo);
    //ESP_LOGD(TAG, "dataLogRetentionInDays: %d", dataLogRetentionInDays);
    strftime(cmpfilename, sizeof(cmpfilename), datafile.c_str(), &timeinfo);
    //ESP_LOGD(TAG, "data file name to compare: %s", cmpfilename);

    DIR *dir = opendir(dataroot.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory: " + dataroot);
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGD(TAG, "Compare data file: %s to %s", entry->d_name, cmpfilename);
            if ((strlen(entry->d_name) == strlen(cmpfilename)) && (strcmp(entry->d_name, cmpfilename) < 0)) {
                //ESP_LOGD(TAG, "delete data file: %s", entry->d_name);
                std::string filepath = dataroot + "/" + entry->d_name; 
                if (unlink(filepath.c_str()) == 0) {
                    deleted ++;
                } 
                else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete file: " + filepath);
                    notDeleted ++;
                }
            } 
            else {
                notDeleted ++;
            }
        }
    }

    closedir(dir);

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Files deleted: " + std::to_string(deleted) + " | Files kept: " + std::to_string(notDeleted));
}


bool ClassLogFile::CreateLogDirectories()
{
    bool bRetval = false;
    bRetval = MakeDir("/sdcard/log");
    bRetval = MakeDir("/sdcard/log/data");
    bRetval = MakeDir("/sdcard/log/analog");
    bRetval = MakeDir("/sdcard/log/digit");
    bRetval = MakeDir("/sdcard/log/message");
    bRetval = MakeDir("/sdcard/log/source");

    return bRetval;
}


ClassLogFile::ClassLogFile(std::string _logroot, std::string _logfile, std::string _logdatapath, std::string _datafile)
{
    logroot = _logroot;
    logfile =  _logfile;
    datafile = _datafile;
    dataroot = _logdatapath;
    logFileRetentionInDays = 3;
    dataLogRetentionInDays = 3;
    doDataLogToSD = true;
    loglevel = ESP_LOG_INFO;
}
