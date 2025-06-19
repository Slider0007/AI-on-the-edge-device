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

    std::string createLogFolder(std::string time, bool createResizedFolder = false);
    void logImage(std::string logPath, std::string name, CNNType cnnType, int value, std::string timestamp, CImage *img,
                  uint8_t quality = 90);
    void removeOldLogs();

  public:
    ClassLogImage(const char *logTag);
    virtual ~ClassLogImage();
};

#endif // CLASSFLOWIMAGE_H
