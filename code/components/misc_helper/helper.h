#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <vector>
#include <ctime>

#include <esp_err.h>

bool fileExists(std::string filename);
bool copyFile(std::string input, std::string output);
bool renameFile(std::string srcFile, std::string destFile);
bool deleteFile(std::string file);
bool isValidFilename(const std::string &filename);
std::string getFileName(const std::string &path);
std::string getFileType(const std::string &filename);
bool getFileIsFiletype(const std::string &filename, const std::string &filetype);
size_t getFileSize(const std::string &filename);
bool readFileToString(const std::string &path, std::string &out);

bool isSafePath(const std::string &path);
bool dirExists(const std::string &directory);
std::string getDirectory(const std::string &filename);
bool makeDir(std::string _what);
int makeDirRecursive(const char *dir, const mode_t mode);
int removeFolder(const char *folderPath, const char *logTag);
bool replaceFolder(const std::string &sourceDir, const std::string &targetDir);
bool mergeFolder(const std::string &sourceDir, const std::string &targetDir);
esp_err_t deleteAllFilesInDirectory(const std::string &directory, bool recursive = false, bool deleteRootFolder = false);

std::string formatFileName(std::string input);
std::string trim(std::string istring, std::string adddelimiter = "");
std::string toLower(std::string in);
std::string toUpper(std::string in);

void replaceAll(std::string &s, const std::string &toReplace, const std::string &replaceWith);
size_t findDelimiterPos(std::string input, std::string delimiter);

std::string to_stringWithPrecision(const double _value, int _decPlace);
std::string intToHexString(int _valueInt);

time_t addDays(time_t startTime, int days);
time_t getUptime(void);
std::string getFormattedUptime(bool compact);

const char *get404(void);
std::string urlDecode(const std::string &value);

#endif // HELPER_H
