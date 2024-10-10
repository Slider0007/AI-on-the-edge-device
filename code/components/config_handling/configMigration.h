#ifndef CONFIGMIGRATION_H
#define CONFIGMIGRATION_H

#include <vector>
#include <string>


void migrateConfiguration(void);
void migrateConfigIni(void);
void migrateWlanIni();
std::vector<std::string> splitString(std::string input, std::string delimiter = " = \t");
std::vector<std::string> splitStringWLAN(std::string input, std::string _delimiter = "");
bool replaceString(std::string &s, std::string const &toReplace, std::string const &replaceWith);
bool replaceString(std::string &s, std::string const &toReplace, std::string const &replaceWith, bool logIt);

#endif //CONFIGMIGRATION_H