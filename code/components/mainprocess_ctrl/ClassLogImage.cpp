#include "ClassLogImage.h"
#include "../../include/defines.h"

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include <esp_log.h>

#include "ClassLogFile.h"
#include "helper.h"
#include "time_sntp.h"

static const char* TAG = "LOGIMAGE";


ClassLogImage::ClassLogImage(const char *logTag)
{
	this->logTag = logTag;
	this->saveImagesEnabled = false;
    this->imagesLocation = "/sdcard/log/source";
    this->imagesRetention = 5;
}

std::string ClassLogImage::createLogFolder(std::string time)
{
	if (!saveImagesEnabled)
		return "";

	std::string logPath = imagesLocation + "/" + time.DEFAULT_TIME_FORMAT_DATE_EXTR + "/" + time.DEFAULT_TIME_FORMAT_HOUR_EXTR;

    if (makeDirRecursive(logPath.c_str(), S_IRWXU) != 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, logTag, "createLogFolder: failed to create folder. Path: " + logPath);
        return "";
    }

	return logPath;
}


void ClassLogImage::logImage(std::string _logPath, std::string _sequenceName, CNNType _type, int _value, std::string _time, CImageBasis *_img)
{
	if (!saveImagesEnabled)
		return;

    if (_logPath.empty()) {
        LogFile.writeToFile(ESP_LOG_ERROR, logTag, "logImage: logPath empty");
        return;
    }

	char valueBuf[10];

    if (_type == CNNTYPE_NONE) { // log with no label -> raw image
        valueBuf[0] = '\0';
    }
    else if (_type == CNNTYPE_DIGIT_CLASS11) { // dig-class10 (0-9 + NaN)
        sprintf(valueBuf, "%d_", _value);
    }
    else { // ana-class100, dig-class100, dig-cont
        sprintf(valueBuf, "%.1f_", _value/10.0);
    }

	std::string nm = _logPath + "/" + valueBuf + _sequenceName + "_" + _time + ".jpg";
	nm = formatFileName(nm);
	std::string output = "/sdcard/img_tmp/" + _sequenceName + ".jpg";
	output = formatFileName(output);
	ESP_LOGD(logTag, "save to file: %s", nm.c_str());
    if (_img == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, logTag, "logImage: rawImage not initialized");
        return;
    }
	_img->saveToFile(nm);
}


void ClassLogImage::removeOldLogs()
{
	if (!saveImagesEnabled)
		return;

    if (imagesRetention == 0) {
        LogFile.writeToFile(ESP_LOG_DEBUG, logTag, "RemoveOldLogs: Retention deactivated");
        return;
    }

    LogFile.writeToFile(ESP_LOG_DEBUG, logTag, "Delete image folder older than retention setting (This might take a while)");

    DIR* dir = opendir(imagesLocation.c_str());
    if (!dir) {
        LogFile.writeToFile(ESP_LOG_ERROR, logTag, "Failed to open directory " + imagesLocation);
        return;
    }

    time_t rawtime;
    time(&rawtime);
    rawtime = addDays(rawtime, -imagesRetention + 1);
    //ESP_LOGI(TAG, "imagesRetention: %d", imagesRetention);
	std::string cmpfolderame = convertTimeToString(rawtime, DEFAULT_TIME_FORMAT).DEFAULT_TIME_FORMAT_DATE_EXTR;
    //ESP_LOGI(TAG, "Delete all folder older than %s", cmpfolderame.c_str());

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;

    while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR) {
			//ESP_LOGD(TAG, "Compare folder %s to %s", entry->d_name, cmpfolderame.c_str());
			if ((strlen(entry->d_name) == cmpfolderame.length()) && (strcmp(entry->d_name, cmpfolderame.c_str()) < 0)) {
                std::string folderpath = imagesLocation + "/" + entry->d_name;
                //ESP_LOGI(TAG, "Delete folder %s", folderpath.c_str());
                if (removeFolder(folderpath.c_str(), TAG) > 0) {
                    deleted++;
                }
                else {
                    LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to delete folder " + folderpath);
                    notDeleted ++;
                }
			}
            else {
                notDeleted ++;
            }
		}
    }

    closedir(dir);

    LogFile.writeToFile(ESP_LOG_DEBUG, logTag, "Folders deleted: " + std::to_string(deleted) + " | Folders kept: " + std::to_string(notDeleted));
}


ClassLogImage::~ClassLogImage()
{
    // nothing to do
}
