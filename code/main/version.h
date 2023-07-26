#ifndef _VERSION_H
#define _VERSION_H

// These variables are autogenerated and compiled
// into the library by the version.cmake script
extern "C"
{
    extern const char* GIT_TAG;
    extern const char* GIT_REV;
    extern const char* GIT_BRANCH;
    extern const char* BUILD_TIME;
}

#include <string>
#include "Helper.h"
#include <fstream>
#include <algorithm>


const char* build_time(void)
{
    return BUILD_TIME;
}

const char* libfive_git_version(void)
{
    return GIT_TAG;
}

const char* libfive_git_revision(void)
{
    return GIT_REV;
}

const char* libfive_git_branch(void)
{
    return GIT_BRANCH;
}


std::string getFwVersion(void) {
    std::string buf;
    if (std::string(GIT_TAG) == "") { // Tag not set, show branch
        buf = "Development-Branch: " + std::string(GIT_BRANCH);
    }
    else { // Tag is set, ignore branch
        buf = "Release: " + std::string(GIT_TAG);
    }
    buf = buf + " (Commit: " + std::string(GIT_REV) + ")";

    return buf;
}

std::string getHTMLversion(void){
    char buf[100]="?\0";
    FILE* pFile;
    std::string fn = FormatFileName("/sdcard/html/version.txt");
    pFile = fopen(fn.c_str(), "r");

    if (pFile == NULL)
        return std::string(buf);

    fgets(buf, sizeof(buf), pFile); // Line 1: Version
    fclose(pFile);

    std::string value = std::string(buf);
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end()); // Remove any newlines

    return value;
}

std::string getHTMLcommit(void){
    char buf[100]="?\0";
    FILE* pFile;
    std::string fn = FormatFileName("/sdcard/html/version.txt");
    pFile = fopen(fn.c_str(), "r");

    if (pFile == NULL)
        return std::string(buf);

    fgets(buf, sizeof(buf), pFile); // Line 1: Version -> ignored
    fgets(buf, sizeof(buf), pFile); // Line 2: Commit
    fclose(pFile);

    std::string value = std::string(buf);
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end()); // Remove any newlines

    return value;
}

#endif // _VERSION_H