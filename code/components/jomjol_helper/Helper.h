#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <fstream>
#include <vector>

std::string FormatFileName(std::string input);
std::size_t file_size(const std::string& file_name);
void FindReplace(std::string& line, std::string& oldString, std::string& newString);

bool CopyFile(std::string input, std::string output);
bool DeleteFile(std::string fn);
bool RenameFile(std::string from, std::string to);
bool MakeDir(std::string _what);
bool FileExists(std::string filename);

std::string to_stringWithPrecision(const double _value, int _decPlace);

size_t findDelimiterPos(std::string input, std::string delimiter);
std::string trim(std::string istring, std::string adddelimiter = "");
bool ctype_space(const char c, std::string adddelimiter);

std::string getFileType(std::string filename);
std::string getFileFullFileName(std::string filename);
std::string getDirectory(std::string filename);
long getFileSize(std::string filename);

int mkdir_r(const char *dir, const mode_t mode);
int removeFolder(const char* folderPath, const char* logTag);
void deleteAllFilesInDirectory(std::string _directory);

std::string toLower(std::string in);
std::string toUpper(std::string in);

time_t addDays(time_t startTime, int days);

void memCopyGen(uint8_t* _source, uint8_t* _target, int _size);

std::vector<std::string> HelperZerlegeZeile(std::string input, std::string _delimiter);
std::vector<std::string> ZerlegeZeile(std::string input, std::string delimiter = " =, \t");

time_t getUptime(void);
std::string getFormatedUptime(bool compact);

const char* get404(void);

std::string UrlDecode(const std::string& value);

bool replaceString(std::string& s, std::string const& toReplace, std::string const& replaceWith);
bool replaceString(std::string& s, std::string const& toReplace, std::string const& replaceWith, bool logIt);
bool isInString(std::string& s, std::string const& toFind);
std::vector<std::string> splitString(const std::string& str);
std::string intToHexString(int _valueInt);

#endif //HELPER_H
