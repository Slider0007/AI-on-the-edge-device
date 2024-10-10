#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <vector>
#include <ctime>

bool fileExists(std::string filename);
bool copyFile(std::string input, std::string output);
bool renameFile(std::string from, std::string to);
bool deleteFile(std::string fn);

std::string getFileFullFileName(std::string filename);
std::string getFileType(std::string filename);
bool getFileIsFiletype(const std::string &filename, const std::string &filetype);
size_t getFileSize(const std::string &filename);

std::string getDirectory(std::string filename);
bool makeDir(std::string _what);
int makeDirRecursive(const char *dir, const mode_t mode);
int removeFolder(const char* folderPath, const char* logTag);
void deleteAllFilesInDirectory(std::string directory);
void moveAllFilesWithFiletype(std::string sourceDir, std::string destinationDir, std::string filetype);

std::string formatFileName(std::string input);
std::string trim(std::string istring, std::string adddelimiter = "");
std::string toLower(std::string in);
std::string toUpper(std::string in);

void findReplace(std::string &line, std::string &oldString, std::string &newString);
void replaceAll(std::string &s, const std::string &toReplace, const std::string &replaceWith);
bool isInString(std::string &s, std::string const &toFind);
std::vector<std::string> splitStringAtNewline(const std::string &str);
size_t findDelimiterPos(std::string input, std::string delimiter);

std::string to_stringWithPrecision(const double _value, int _decPlace);
std::string intToHexString(int _valueInt);

time_t addDays(time_t startTime, int days);
time_t getUptime(void);
std::string getFormatedUptime(bool compact);

const char* get404(void);
std::string urlDecode(const std::string& value);

#endif //HELPER_H
