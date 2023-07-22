#include "ClassFlowImage.h"
#include <string>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include "time_sntp.h"
#include "ClassLogFile.h"
#include "CImageBasis.h"
#include "esp_log.h"
#include "../../include/defines.h"

static const char* TAG = "FLOWIMAGE";


ClassFlowImage::ClassFlowImage(const char* logTag)
{
	this->logTag = logTag;
	this->isLogImage = false;
    this->disabled = false;
    this->imagesLocation = "/log/source";
    this->imagesRetention = 5;
}


ClassFlowImage::ClassFlowImage(std::vector<ClassFlow*> * lfc, const char* logTag) : ClassFlow(lfc)
{
	this->logTag = logTag;
	this->isLogImage = false;
    this->disabled = false;
    this->imagesLocation = "/log/source";
    this->imagesRetention = 5;
}


ClassFlowImage::ClassFlowImage(std::vector<ClassFlow*> * lfc, ClassFlow *_prev, const char* logTag) :  ClassFlow(lfc, _prev)
{
	this->logTag = logTag;
	this->isLogImage = false;
    this->disabled = false;
    this->imagesLocation = "/log/source";
    this->imagesRetention = 5;
}


std::string ClassFlowImage::CreateLogFolder(std::string time) 
{
	if (!isLogImage)
		return "";

	std::string logPath = imagesLocation + "/" + time.LOGFILE_TIME_FORMAT_DATE_EXTR + "/" + time.LOGFILE_TIME_FORMAT_HOUR_EXTR;
    isLogImage = mkdir_r(logPath.c_str(), S_IRWXU) == 0;
    if (!isLogImage) {
        LogFile.WriteToFile(ESP_LOG_ERROR, logTag, "Can't create log folder for analog images. Path " + logPath);
        return "";
    }

	return logPath;
}


void ClassFlowImage::LogImage(std::string logPath, std::string name, float *resultFloat, int *resultInt, std::string time, CImageBasis *_img) 
{
	if (!isLogImage)
		return;

    if (logPath.empty()) {
        LogFile.WriteToFile(ESP_LOG_ERROR, logTag, "LogImage: logPath empty");
        return;
    }
	
	char buf[10];

	if (resultFloat != NULL) {
        if (*resultFloat < 0)
            sprintf(buf, "N.N_");
        else {
            sprintf(buf, "%.1f_", *resultFloat);
            if (strcmp(buf, "10.0_") == 0)
                sprintf(buf, "0.0_");
        }
            
	} 
    else if (resultInt != NULL) {
		sprintf(buf, "%d_", *resultInt);
	} 
    else {
		buf[0] = '\0';
	}

	std::string nm = logPath + "/" + buf + name + "_" + time + ".jpg";
	nm = FormatFileName(nm);
	std::string output = "/sdcard/img_tmp/" + name + ".jpg";
	output = FormatFileName(output);
	ESP_LOGD(logTag, "save to file: %s", nm.c_str());
    if (_img == NULL) {
        LogFile.WriteToFile(ESP_LOG_ERROR, logTag, "LogImage: rawImage not initialized");
        return;
    }
	_img->SaveToFile(nm);
}


void ClassFlowImage::RemoveOldLogs()
{
	if (!isLogImage)
		return;
	
	ESP_LOGD(TAG, "remove old images");
    if (imagesRetention == 0) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, logTag, "RemoveOldLogs: Retention deactivated");
        return;
    }

    time_t rawtime;
    struct tm* timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -1 * imagesRetention + 1);
    timeinfo = localtime(&rawtime);
    //ESP_LOGD(TAG, "ImagefileRetentionInDays: %d", imagesRetention);
    
    strftime(cmpfilename, 30, LOGFILE_TIME_FORMAT, timeinfo);
    //ESP_LOGD(TAG, "file name to compare: %s", cmpfilename);
	std::string folderName = std::string(cmpfilename).LOGFILE_TIME_FORMAT_DATE_EXTR;

    DIR* dir = opendir(imagesLocation.c_str());
    if (!dir) {
        LogFile.WriteToFile(ESP_LOG_ERROR, logTag, "LogImage: Failed to open directory " + imagesLocation);
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        std::string folderPath = imagesLocation + "/" + entry->d_name;
		if (entry->d_type == DT_DIR) {
			//ESP_LOGD(TAG, "Compare %s to %s", entry->d_name, folderName.c_str());	
			if ((strlen(entry->d_name) == folderName.length()) && (strcmp(entry->d_name, folderName.c_str()) < 0)) {
                removeFolder(folderPath.c_str(), logTag);
                deleted++;
			} else {
                notDeleted ++;
            }
		}
    }
    
    LogFile.WriteToFile(ESP_LOG_DEBUG, logTag, "Folder deleted: " + std::to_string(deleted) + ", folder not deleted: " + std::to_string(notDeleted));
    closedir(dir);
}


ClassFlowImage::~ClassFlowImage()
{
    // nothing to do
}
