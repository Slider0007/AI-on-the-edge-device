#ifndef CLASSFLOWIMAGE_H
#define CLASSFLOWIMAGE_H

#include "ClassFlow.h"
#include "ClassFlowDefineTypes.h"


class ClassFlowImage : public ClassFlow
{
	protected:
		std::string imagesLocation;
		bool isLogImage;
		int imagesRetention;
		const char* logTag;

		std::string CreateLogFolder(std::string time);
		void LogImage(std::string _logPath, std::string _numbername, t_CNNType _type, int _value, std::string _time, CImageBasis *_img);


	public:
		ClassFlowImage(const char* logTag);
		ClassFlowImage(std::vector<ClassFlow*> * lfc, const char* logTag);
		ClassFlowImage(std::vector<ClassFlow*> * lfc, ClassFlow *_prev, const char* logTag);
		virtual ~ClassFlowImage();

		void RemoveOldLogs();
};

#endif //CLASSFLOWIMAGE_H