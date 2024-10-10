//#pragma warning(disable : 4996)
#include "helper.h"
#include "../../include/defines.h"

#include <sstream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include <esp_log.h>
#include <esp_timer.h>

#include "ClassLogFile.h"


static const char* TAG = "HELPER";

// File related helper
// **********************************************************
bool fileExists(std::string filename)
{
	FILE* fpFile = fopen(filename.c_str(), "rb");
	if (!fpFile) { // File not existing
		return false;
	}
	fclose(fpFile);
	return true;
}


bool copyFile(std::string input, std::string output)
{
	input = formatFileName(input);
	output = formatFileName(output);

	char cTemp;
	FILE* fpSourceFile = fopen(input.c_str(), "rb");
	if (!fpSourceFile) { // File not existing
		ESP_LOGE(TAG, "copyFile: File %s not existing", input.c_str());
		return false;
	}

	/* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(fpSourceFile, NULL, _IOFBF, 512);


	FILE* fpTargetFile = fopen(output.c_str(), "wb");

	/* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(fpTargetFile, NULL, _IOFBF, 512);

	// Code Section

	// Read From The Source File - "Copy"
	while (fread(&cTemp, 1, 1, fpSourceFile) == 1) {
		fwrite(&cTemp, 1, 1, fpTargetFile); // Write To The Target File - "Paste"
	}

	// Close The Files
	fclose(fpSourceFile);
	fclose(fpTargetFile);
	ESP_LOGD(TAG, "File copied: %s to %s", input.c_str(), output.c_str());
	return true;
}


bool renameFile(std::string from, std::string to)
{
	FILE* fpFile = fopen(from.c_str(), "rb");
	if (!fpFile) { // File not existing
		ESP_LOGE(TAG, "renameFile: File %s not existing", from.c_str());
		return false;
	}
	fclose(fpFile);

	return (rename(from.c_str(), to.c_str()) == 0);
}


bool deleteFile(std::string fn)
{
	FILE* fpFile = fopen(fn.c_str(), "rb");
	if (!fpFile) { // File not existing
		return false;
	}
	fclose(fpFile);

	unlink(fn.c_str());
	return true;
}


std::string getFileFullFileName(std::string filename)
{
	size_t lastpos = filename.find_last_of('/');

	if (lastpos == std::string::npos)
		return "";

//	ESP_LOGD(TAG, "Last position: %d", lastpos);

	std::string zw = filename.substr(lastpos + 1, filename.size() - lastpos);

	return zw;
}


std::string getFileType(std::string filename)
{
	size_t lastpos = filename.rfind(".", filename.length());
	size_t neu_pos;
	while ((neu_pos = filename.find(".", lastpos + 1)) > -1) {
		lastpos = neu_pos;
	}

	if (lastpos == std::string::npos)
		return "";

	std::string zw = filename.substr(lastpos + 1, filename.size() - lastpos);
	zw = toUpper(zw);

	return zw;
}


bool getFileIsFiletype(const std::string &filename, const std::string &filetype)
{
    return (filename.substr(filename.find_last_of(".") + 1) == filetype);

    /*std::size_t extPos = filename.rfind(".", filename.length());
    if (extPos == std::string::npos)
        return false;

    ESP_LOGI(TAG, "check: %s, %s", filename.substr(filename.rfind(".", filename.length()) + 1).c_str(), filetype.c_str());

    return (filename.substr(filename.rfind(".", filename.length()) + 1) == filetype);*/
}


std::size_t getFileSize(const std::string &filename)
{
    std::ifstream file(filename.c_str(),std::ios::in | std::ios::binary);
    if (!file)
		return 0;

    file.seekg (0, std::ios::end);
    return static_cast<std::size_t>(file.tellg());
}


// Directory related helper
// **********************************************************
std::string getDirectory(std::string filename)
{
	size_t lastpos = filename.rfind('/');

	if (lastpos == std::string::npos) {
		lastpos = filename.rfind('\\');
		if (lastpos == std::string::npos)
			return "";
	}

	// ESP_LOGD(TAG, "Directory: %d", lastpos);
	return filename.substr(0, lastpos);
}


/**
 * Create a folder and its parent folders as needed
 */
bool makeDir(std::string path)
{
	std::string parent;

	//LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Create folder: " + path);

	bool bSuccess = false;
    int nRC = ::mkdir( path.c_str(), 0775 );
    if( nRC == -1 ) {
        switch( errno ) {
            case ENOENT:
                //parent didn't exist, try to create it
				parent = path.substr(0, path.find_last_of('/'));
        		//LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Create parent folder first: " + parent);
                if(makeDir(parent)) {
                    //Now, try to create again.
                    bSuccess = 0 == ::mkdir( path.c_str(), 0775 );
				}
                else {
        			LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create parent folder: " + parent);
                    bSuccess = false;
				}
                break;

            case EEXIST:
                //Done!
                bSuccess = true;
                break;

            default:
				LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to create folder: " + path + " (errno: " + std::to_string(errno) + ")");
                bSuccess = false;
                break;
        }
    }
    else {
        bSuccess = true;
	}

    return bSuccess;
}


/* recursive mkdir */
int makeDirRecursive(const char *dir, const mode_t mode)
{
    char tmp[FILE_PATH_MAX];
    char *p = NULL;
    struct stat sb;
    size_t len;

    /* copy path */
    len = strnlen(dir, FILE_PATH_MAX);
    if (len == 0 || len == FILE_PATH_MAX) {
        return -1;
    }
    memcpy (tmp, dir, len);
    tmp[len] = '\0';

    /* remove trailing slash */
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    /* check if path exists and is a directory */
    if (stat (tmp, &sb) == 0) {
        if (S_ISDIR (sb.st_mode)) {
            return 0;
        }
    }

    /* recursive mkdir */
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            /* test path */
            if (stat(tmp, &sb) != 0) {
                /* path does not exist - create directory */
                if (mkdir(tmp, mode) < 0) {
                    return -1;
                }
            } else if (!S_ISDIR(sb.st_mode)) {
                /* not a directory */
                return -1;
            }
            *p = '/';
        }
    }
    /* test path */
    if (stat(tmp, &sb) != 0) {
        /* path does not exist - create directory */
        if (mkdir(tmp, mode) < 0) {
            return -1;
        }
    } else if (!S_ISDIR(sb.st_mode)) {
        /* not a directory */
        return -1;
    }
    return 0;
}


int removeFolder(const char* folderPath, const char* logTag)
{
	//ESP_LOGD(logTag, "Delete content in path %s", folderPath);

	DIR *dir = opendir(folderPath);
    if (!dir) {
		LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to open directory " + std::string(folderPath));
        return -1;
    }

    struct dirent *entry;
    int deleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        std::string path = std::string(folderPath) + "/" + entry->d_name;
		if (entry->d_type == DT_REG) {
			//ESP_LOGD(logTag, "Delete file %s", path.c_str());
			if (unlink(path.c_str()) == 0) {
				deleted ++;
			} else {
				LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to delete file " + path);
			}
        }
		else if (entry->d_type == DT_DIR) {
			if (removeFolder(path.c_str(), logTag) > 0)
				deleted++;
		}
    }
    closedir(dir);

	if (rmdir(folderPath) != 0) {
		LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to delete folder " + std::string(folderPath));
	}
	//ESP_LOGD(logTag, "%d files in folder %s deleted.", deleted, folderPath);

	return deleted;
}


void deleteAllFilesInDirectory(std::string directory)
{
    struct dirent *entry;
    DIR *dir = opendir(directory.c_str());
    std::string filename;

    if (!dir) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "deleteAllFilesInDirectory: Failed to open directory: " + directory);
        return;
    }

    /* Iterate over all files / folders and fetch their names and sizes */
    while ((entry = readdir(dir)) != NULL) {
        if (!(entry->d_type == DT_DIR)){
            filename = directory + "/" + std::string(entry->d_name);
            LogFile.writeToFile(ESP_LOG_DEBUG, TAG, "Delete file: " + filename);
            unlink(filename.c_str()); // Delete file
        }
    }

    closedir(dir);
}


void moveAllFilesWithFiletype(std::string sourceDir, std::string destinationDir, std::string filetype)
{
    DIR *dir = opendir(sourceDir.c_str());

    if (!dir) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "moveAllFilesWithFiletype: Failed to open directory: " + sourceDir);
        return;
    }

    // Iterate over all files in folder and move if extention is matching
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!(entry->d_type == DT_DIR)){
            if (getFileIsFiletype(std::string(entry->d_name), filetype)) {
                std::string sourceFilename = sourceDir + "/" + std::string(entry->d_name);
                std::string destFilename = destinationDir + "/" + std::string(entry->d_name);
                if (!fileExists(destFilename)) { // Move source file if file not existing at destination
                    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Move file: " + sourceFilename);
                    renameFile(sourceFilename, destinationDir + "/" + std::string(entry->d_name));
                }
                else { // Delete source file if file is already existing at destination
                    deleteFile(sourceFilename);
                }
            }
        }
    }

    closedir(dir);
}


// String manipulation helper
// **********************************************************
std::string formatFileName(std::string input)
{
#ifdef ISWINDOWS_TRUE
    input.erase(0, 1);
    std::string os = "/";
    std::string ns = "\\";
    findReplace(input, os, ns);
#endif
    return input;
}


bool ctype_space(const char c, std::string adddelimiter)
{
	if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 11) {
		return true;
	}

	if (adddelimiter.find(c) != std::string::npos) {
		return true;
	}

	return false;
}


std::string trim(std::string istring, std::string adddelimiter)
{
	bool trimmed = false;

	if (ctype_space(istring[istring.length() - 1], adddelimiter)) {
		istring.erase(istring.length() - 1);
		trimmed = true;
	}

	if (ctype_space(istring[0], adddelimiter)) {
		istring.erase(0, 1);
		trimmed = true;
	}

	if ((trimmed == false) || (istring.size() == 0)) {
		return istring;
	}
	else {
		return trim(istring, adddelimiter);
	}
}


size_t findDelimiterPos(std::string input, std::string delimiter)
{
	size_t pos = std::string::npos;
	size_t zw;
	std::string akt_del;

	for (int anz = 0; anz < delimiter.length(); ++anz) {
		akt_del = delimiter[anz];
		if ((zw = input.find(akt_del)) != std::string::npos) {
			if (pos != std::string::npos) {
				if (zw < pos)
					pos = zw;
			}
			else {
				pos = zw;
			}
		}
	}
	return pos;
}


std::string toUpper(std::string in)
{
	for (int i = 0; i < in.length(); ++i)
		in[i] = toupper(in[i]);

	return in;
}


std::string toLower(std::string in)
{
	for (int i = 0; i < in.length(); ++i)
		in[i] = tolower(in[i]);

	return in;
}


void findReplace(std::string &line, std::string &oldString, std::string &newString)
{
    const size_t oldSize = oldString.length();

    // do nothing if line is shorter than the string to find
    if (oldSize > line.length())
		return;

    const size_t newSize = newString.length();
    for (size_t pos = 0; ; pos += newSize) {
        // Locate the substring to replace
        pos = line.find(oldString, pos);
        if (pos == std::string::npos) return;
        if (oldSize == newSize) {
            // if they're same size, use std::string::replace
            line.replace(pos, oldSize, newString);
        }
        else {
            // if not same size, replace by erasing and inserting
            line.erase(pos, oldSize);
            line.insert(pos, newString);
        }
    }
}


// from https://stackoverflow.com/a/14678800
void replaceAll(std::string &s, const std::string &toReplace, const std::string &replaceWith)
{
    size_t pos = 0;
    while ((pos = s.find(toReplace, pos)) != std::string::npos) {
         s.replace(pos, toReplace.length(), replaceWith);
         pos += replaceWith.length();
    }
}


bool isInString(std::string &s, std::string const &toFind)
{
    std::size_t pos = s.find(toFind);

    if (pos == std::string::npos) { // Not found
        return false;
    }
    return true;
}


std::vector<std::string> splitStringAtNewline(const std::string &str) {
    std::vector<std::string> tokens;

    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, '\n')) {
        tokens.push_back(token);
    }

    return tokens;
}


std::string to_stringWithPrecision(const double _value, int _decPlace = 6)
{
	std::ostringstream out;

	if (_decPlace < 0)
		_decPlace = 0;

    out.precision(_decPlace);
    out << std::fixed << _value;
    return out.str();
}


std::string intToHexString(int _valueInt)
{
	char valueHex[33];
	sprintf(valueHex,"0x%02x", _valueInt);
	return std::string(valueHex);
}


// Time related helper
// **********************************************************
time_t addDays(time_t startTime, int days) {
	struct tm* tm = localtime(&startTime);
	tm->tm_mday += days;
	return mktime(tm);
}


time_t getUptime(void)
{
    return (uint32_t)(esp_timer_get_time()/1000/1000); // in seconds
}


// Returns the current uptime  formated ad xxf xxh xxm [xxs]
std::string getFormatedUptime(bool compact)
{
	char buf[20];
	#pragma GCC diagnostic ignored "-Wformat-truncation"

    int uptime = getUptime(); // in seconds

    int days = int(floor(uptime / (3600*24)));
    int hours = int(floor((uptime - days * 3600*24) / (3600)));
    int minutes = int(floor((uptime - days * 3600*24 - hours * 3600) / (60)));
    int seconds = uptime - days * 3600*24 - hours * 3600 - minutes * 60;

	if (compact) {
		snprintf(buf, sizeof(buf), "%dd%02dh%02dm%02ds", days, hours, minutes, seconds);
	}
	else {
		snprintf(buf, sizeof(buf), "%3dd %02dh %02dm %02ds", days, hours, minutes, seconds);
	}

	return std::string(buf);
}


// URL related helper
// **********************************************************
const char* get404(void)
{
    return
		"<pre>\n\n\n\n"
		"        _\n"
		"    .__(.)< ( oh oh! This page does not exist! )\n"
		"    \\___)\n"
		"\n\n"
		"                You could try your <a href=index.html target=_parent>luck</a> here!</pre>\n"
		"<script>document.cookie = \"page=overview.html\"</script>"; // Make sure we load the overview page
}


std::string urlDecode(const std::string& value)
{
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        auto ch = value[i];

        if (ch == '%' && (i + 2) < value.size()) {
            auto hex = value.substr(i + 1, 2);
            auto dec = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(dec);
            i += 2;
        }
        else if (ch == '+') {
            result.push_back(' ');
        }
        else {
            result.push_back(ch);
        }
    }

    return result;
}
