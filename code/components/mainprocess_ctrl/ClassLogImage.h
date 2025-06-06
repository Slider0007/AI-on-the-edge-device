#ifndef CLASSFLOWIMAGE_H
#define CLASSFLOWIMAGE_H

#include <string>

#include "ClassFlow.h"
#include "CImage.h"


class ClassLogImage : public ClassFlow
{
  protected:
    const char *logTag;
    bool saveImagesEnabled;
    std::string imagesLocation;
    int imagesRetention;

    std::string createLogFolder(std::string time);
    void logImage(std::string _logPath, std::string _sequenceName, CNNType _type, int _value, std::string _time, CImage *_img,
                  uint8_t _quality = 90);
    void removeOldLogs();

  public:
    ClassLogImage(const char *logTag);
    virtual ~ClassLogImage();
};

#endif // CLASSFLOWIMAGE_H
