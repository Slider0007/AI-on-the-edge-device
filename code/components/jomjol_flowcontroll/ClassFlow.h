#pragma once

#ifndef CLASSFLOW_H
#define CLASSFLOW_H

#include <fstream>
#include <string>
#include <vector>

#include "Helper.h"
#include "CImageBasis.h"

struct HTMLInfo
{
	float val;
	CImageBasis *image = NULL;
	CImageBasis *image_org = NULL;
	std::string filename;
	std::string filename_org;
	std::string name;
	int position;
};

struct strFlowState
{
	std::string ClassName = "";
	bool getCalled = false;
	bool isSuccessful = true;
	int8_t ErrorCode = 0;
};


class ClassFlow
{
protected:
	bool isNewParagraph(std::string input);
	bool GetNextParagraph(FILE* pfile, std::string& aktparamgraph);
	bool getNextLine(FILE* pfile, std::string* rt);

	std::vector<ClassFlow*>* ListFlowControll;
	ClassFlow *previousElement;

	virtual void SetInitialParameter(void);

	std::string GetParameterName(std::string _input);

	bool disabled;

	strFlowState FlowState;

public:
	ClassFlow(void);
	ClassFlow(std::vector<ClassFlow*> * lfc);
	ClassFlow(std::vector<ClassFlow*> * lfc, ClassFlow *_prev);
	
    void PresetFlowStateHandler(bool _init = false);
	void FlowStateHandlerSetError(int8_t _error);
	struct strFlowState* getFlowState();
	virtual void doAutoErrorHandling();
	
	virtual bool ReadParameter(FILE* pfile, std::string &aktparamgraph);
	virtual bool doFlow(std::string time);
	virtual std::string getHTMLSingleStep(std::string host);
	virtual std::string getReadout();

	virtual std::string name() {return "ClassFlow";};

};

#endif //CLASSFLOW_H