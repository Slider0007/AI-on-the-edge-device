#include "ClassLogFile.h"
#include "../../include/defines.h"

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
#include "system.h"
#include "time_sntp.h"


static const char *TAG = "LOGFILE";

ClassLogFile LogFile(LOG_LOGS_ROOT_FOLDER, LOG_FILE_TIME_FORMAT, 
                     LOG_DATA_ROOT_FOLDER, DATA_FILE_TIME_FORMAT,
                     LOG_DEBUG_ROOT_FOLDER, DEBUG_FOLDER_TIME_FORMAT);



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


void ClassLogFile::WriteToData(std::string _timestamp, std::string _name, std::string  _sRawValue, std::string _sValue, 
                               std::string _sFallbackValue, std::string  _sRatePerMin, std::string  _sRatePerProcessing, 
                               std::string _sValueStatus, std::string _digital, std::string _analog)
{
    //ESP_LOGD(TAG, "Start WriteToData");
    time_t rawtime;

    time(&rawtime);
    std::string logpath = dataFileRootFolder + "/" + ConvertTimeToString(rawtime, datafile.c_str()); 
    
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
    if (level > loglevel)// Skip logging if defined message loglevel is more verbose than configured threshold loglevel
        return;

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
            std::string logpath = logFileRootFolder + "/" + fileNameDateNew; 

            ESP_LOGI(TAG, "Opening logfile %s for appending", logpath.c_str());
            logFileAppendHandle = fopen(logpath.c_str(), "a");
            if (logFileAppendHandle==NULL) {
                ESP_LOGE(TAG, "WriteToFile: Failed to open logfile %s", logpath.c_str());
                return;
            }

            fileNameDate = fileNameDateNew;
        }
    #else
        std::string logpath = logFileRootFolder + "/" + fileNameDateNew; 
        logFileAppendHandle = fopen(logpath.c_str(), "a");
        if (logFileAppendHandle == NULL) {
            ESP_LOGE(TAG, "WriteToFile: Failed to open logfile %s", logpath.c_str());
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

    time(&rawtime);
    std::string logpath = dataFileRootFolder + "/" + ConvertTimeToString(rawtime, datafile.c_str());

    return logpath;
}


std::string ClassLogFile::GetCurrentFileName()
{
    time_t rawtime;

    time(&rawtime);
    std::string logpath = logFileRootFolder + "/" + ConvertTimeToString(rawtime, logfile.c_str()); 

    return logpath;
}


void ClassLogFile::RemoveOldLogFile()
{
    if (logFileRetentionInDays == 0) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Delete log files older than retention setting");

    DIR *dir = opendir(logFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + logFileRootFolder);
        return;
    }

    time_t rawtime;
    time(&rawtime);
    rawtime = addDays(rawtime, -logFileRetentionInDays + 1);
    //ESP_LOGI(TAG, "logFileRetentionInDays: %d", logFileRetentionInDays);
    std::string cmpfilename = ConvertTimeToString(rawtime, logfile.c_str());
    //ESP_LOGI(TAG, "log file name to compare: %s", cmpfilename.c_str());

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGI(TAG, "compare log file: %s to %s", entry->d_name, cmpfilename.c_str());
            if ((strlen(entry->d_name) == cmpfilename.length()) && (strcmp(entry->d_name, cmpfilename.c_str()) < 0)) {
                //ESP_LOGI(TAG, "delete log file: %s", entry->d_name);
                std::string filepath = logFileRootFolder + "/" + entry->d_name;
                if ((strcmp(entry->d_name, "log_1970-01-01.txt") == 0) && getTimeWasNotSetAtBoot()) { // keep logfile log_1970-01-01.txt if time was not set at boot (some boot logs are in there)
                    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Time was not set at boot, keep log file \'log_1970-01-01.txt\'");
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

    closedir(dir);

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Files deleted: " + std::to_string(deleted) + " | Files kept: " + std::to_string(notDeleted));
}


void ClassLogFile::RemoveOldDataLog()
{
    if (dataLogRetentionInDays == 0 || !doDataLogToSD) {
        return;
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Delete data files older than retention setting");

    DIR *dir = opendir(dataFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + dataFileRootFolder);
        return;
    }

    time_t rawtime;
    time(&rawtime);
    rawtime = addDays(rawtime, -dataLogRetentionInDays + 1);
    //ESP_LOGI(TAG, "dataLogRetentionInDays: %d", dataLogRetentionInDays);
    std::string cmpfilename = ConvertTimeToString(rawtime, datafile.c_str());
    //ESP_LOGI(TAG, "data file name to compare: %s", cmpfilename.c_str());

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGI(TAG, "Compare data file: %s to %s", entry->d_name, cmpfilename.c_str());
            if ((strlen(entry->d_name) == cmpfilename.length()) && (strcmp(entry->d_name, cmpfilename.c_str()) < 0)) {
                //ESP_LOGI(TAG, "delete data file: %s", entry->d_name);
                std::string filepath = dataFileRootFolder + "/" + entry->d_name; 
                if (unlink(filepath.c_str()) == 0) {
                    deleted ++;
                }
                else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete file " + filepath);
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


void ClassLogFile::RemoveOldDebugFiles()
{
    if (debugFilesRetentionInDays == 0) {
        return;
    }
    
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Delete debug folder older than retention setting");

    DIR *dir = opendir(debugFileRootFolder.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + debugFileRootFolder);
        return;
    }

    time_t rawtime;
    time(&rawtime);
    rawtime = addDays(rawtime, -debugFilesRetentionInDays + 1);
    //ESP_LOGI(TAG, "debugFilesRetentionInDays: %d", debugFilesRetentionInDays);
    std::string cmpfolderame = ConvertTimeToString(rawtime, debugfolder.c_str());
    //ESP_LOGI(TAG, "Delete all folder older than %s", cmpfolderame.c_str());

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            //ESP_LOGI(TAG, "Compare folder %s to %s", entry->d_name, cmpfolderame.c_str());
            if ((strlen(entry->d_name) == cmpfolderame.length()) && (strcmp(entry->d_name, cmpfolderame.c_str()) < 0)) {
                std::string folderpath = debugFileRootFolder + "/" + entry->d_name; 
                //ESP_LOGI(TAG, "Delete folder %s", folderpath.c_str());
                if (removeFolder(folderpath.c_str(), TAG) > 0) {
                    deleted++;
                } 
                else {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to delete folder " + folderpath);
                    notDeleted ++;
                }
            }
            else {
                notDeleted ++;
            }
        }
    }

    closedir(dir);

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Folders deleted: " + std::to_string(deleted) + " | Folders kept: " + std::to_string(notDeleted));
}


bool ClassLogFile::CreateLogDirectories()
{
    bool bRetval = false;
    bRetval = MakeDir(LOG_ROOT_FOLDER);
    bRetval = MakeDir(LOG_IMAGE_RAW_ROOT_FOLDER);
    bRetval = MakeDir(LOG_IMAGE_DIGIT_ROOT_FOLDER);
    bRetval = MakeDir(LOG_IMAGE_ANALOG_ROOT_FOLDER);
    bRetval = MakeDir(LOG_LOGS_ROOT_FOLDER);
    bRetval = MakeDir(LOG_DATA_ROOT_FOLDER);
    bRetval = MakeDir(LOG_DEBUG_ROOT_FOLDER);

    return bRetval;
}
