#pragma once

#ifndef CLASSFLOWIMAGE_H
#define CLASSFLOWIMAGE_H

#include "ClassFlow.h"
#include "ClassFlowDefineTypes.h"

using namespace std;

class ClassFlowImage : public ClassFlow
{
protected:
	string imagesLocation;
    bool isLogImage;
    unsigned short imagesRetention;
	const char* logTag;

	string CreateLogFolder(string time);
	void LogImage(std::string _logPath, std::string _numbername, t_CNNType _type, int _value, std::string _time, CImageBasis *_img);


public:
	ClassFlowImage(const char* logTag);
	ClassFlowImage(std::vector<ClassFlow*> * lfc, const char* logTag);
	ClassFlowImage(std::vector<ClassFlow*> * lfc, ClassFlow *_prev, const char* logTag);
	virtual ~ClassFlowImage();
	
	void RemoveOldLogs();
};

#endif //CLASSFLOWIMAGE_H