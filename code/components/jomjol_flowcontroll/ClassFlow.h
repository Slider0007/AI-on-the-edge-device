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
	std::string ExecutionTime = "";
	bool getExecuted = false;
	bool isSuccessful = true;
	std::vector<int> EventCode; // negative event code -> error; positive event code -> warning
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
    	
        void presetFlowStateHandler(bool _init = false, std::string _time = "");
    	void setFlowStateHandlerEvent(int _eventCode = 0);
    	struct strFlowState* getFlowState();
    	virtual void doPostProcessEventHandling();
    	
    	virtual bool ReadParameter(FILE* pfile, std::string &aktparamgraph);
    	virtual bool doFlow(std::string time);
    	virtual std::string getHTMLSingleStep(std::string host);
    	virtual std::string getReadout();
    
    	virtual std::string name() {return "ClassFlow";};
};

#endif //CLASSFLOW_H