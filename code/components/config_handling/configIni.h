#ifndef CONFIGINI_H
#define CONFIGINI_H

#include <string>
#include <vector>


class ConfigIni
{
    private:
        FILE* pFile;

    public:
        ConfigIni(std::string filePath);
        ~ConfigIni();

        bool isNewParagraph(std::string input);
        bool GetNextParagraph(std::string& aktparamgraph, bool &disabled, bool &eof);
        bool getNextLine(std::string* rt, bool &disabled, bool &eof);
        bool ConfigFileExists() {return pFile;};
};

#endif //CONFIGINI_H