#ifndef CLASSFLOW_H
#define CLASSFLOW_H

#include <string>
#include <vector>

#include "ClassFlowDefineTypes.h"
//#include "helper.h"
//#include "CImageBasis.h"


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
		static std::vector<SequenceData *> sequenceData;

    	strFlowState FlowState;

    public:
    	ClassFlow(void);
		ClassFlow(void *);

    	virtual bool loadParameter();
    	virtual bool doFlow(std::string time);
    	virtual void doPostProcessEventHandling();

		void presetFlowStateHandler(bool _init = false, std::string _time = "");
    	void setFlowStateHandlerEvent(int _eventCode = 0);
    	struct strFlowState* getFlowState();

    	virtual std::string name() {return "ClassFlow";};
};

#endif //CLASSFLOW_H